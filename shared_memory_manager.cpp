#include "shared_memory_manager.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <format>
#include <iostream>
#include <thread>
#include <unistd.h>

const std::size_t MAX_INITIALIZING_WAIT_SECONDS = 30;

SharedMemoryManager::SharedMemoryManager(const std::uint64_t rank, const std::size_t block_size)
  : rank_{rank}
  , shared_memory_{rank} {

    // Attach to shared memory and setup global shared data in shared memory space
    initialize(block_size);
}

SharedMemoryManager::~SharedMemoryManager() {
    uninitialize();
}

void SharedMemoryManager::initialize(const std::size_t block_size) {
    std::cout << std::format("{}: Initializing shared memory manager.", rank_) << std::endl;

    // Check if we attached to shared memory
    if (!shared_memory_.is_attached()) {
        exit(1);
    }

    // Convert void* ptr to byte* ptr
    std::span<std::uint8_t> shm_data = {static_cast<std::uint8_t*>(shared_memory_.get_data()), shared_memory_.get_size()};

    std::cout << "Shared memory total size: " << shm_data.size() << std::endl;

    // Cast shared memory ptr to the shared memory flags data structure
    //
    // All programs on the same machine use these flags to ensure only one
    // program will perform the initialization of the global shared data
    shared_memory_control_flags_ = reinterpret_cast<SharedMemoryControlFlags*>(shm_data.data());

    // Move byte* ptr to the next free bytes
    shm_data = shm_data.subspan(sizeof(SharedMemoryControlFlags), shm_data.size() - sizeof(SharedMemoryControlFlags));

    // Initialize global shared data or wait for another process to finish initialization
    bool global_data_initialized = initialize_global_shared_data(shm_data, block_size);
    if (!global_data_initialized) {
        exit(1);
    }

    std::cout << std::format("{}: Initialized shared memory manager.", rank_) << std::endl;
}

bool SharedMemoryManager::initialize_global_shared_data(std::span<uint8_t> shm_data, const std::size_t block_size) {
    if (!shared_memory_control_flags_->shm_is_initialized.load()) {
        if (!shared_memory_control_flags_->shm_is_initializing.exchange(true)) {
            std::cout << std::format("{}: Initializing global shared data.", rank_)
                      << std::endl;

            std::span<uint8_t> free_list_memory_pool = shm_data.subspan(sizeof(SharedMemoryGlobalData), shm_data.size() - sizeof(SharedMemoryGlobalData));

            std::cout << "Free list size: " << free_list_memory_pool.size() << std::endl;

            std::size_t num_blocks = free_list_memory_pool.size() / block_size;

            // Use placement new to create object at specified memory address
            // (as opposed to regular new that creates new location in heap memory)
            SharedMemoryGlobalData* global_shared_data = new (shm_data.data()) SharedMemoryGlobalData{free_list_memory_pool, block_size, num_blocks};
            shared_memory_control_flags_->shm_is_initialized = true;

            std::cout << std::format("{}: Initialized global shared data.", rank_)
                      << std::endl;
        } else {
            std::cout << std::format("{}: Waiting for global shared data initialization.", rank_)
                      << std::endl;

            auto start = std::chrono::steady_clock::now();

            while (true) {
                auto now = std::chrono::steady_clock::now();

                auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - start);
                if (duration.count() >= MAX_INITIALIZING_WAIT_SECONDS) {
                    std::cerr << std::format("{}: Timed out while waiting for global shared data initialization.", rank_)
                              << std::endl;
                    std::cerr << std::format("{}: Start: {}, Now: {}",
                                             rank_,
                                             std::chrono::duration_cast<std::chrono::seconds>(start.time_since_epoch()),
                                             std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()))
                              << std::endl;
                    return false;
                }

                if (shared_memory_control_flags_->shm_is_initialized.load()) {
                    std::cout << std::format("{}: Global shared data has been initialized by another process.", rank_)
                              << std::endl;
                    break;
                }
            }
        }
    } else {
        std::cout << std::format("{}: Global shared data is already initialized.", rank_)
                  << std::endl;
    }

    shared_memory_global_data_ = reinterpret_cast<SharedMemoryGlobalData*>(shm_data.data());

    std::span<uint8_t> free_list_memory_pool = shm_data.subspan(sizeof(SharedMemoryGlobalData), shm_data.size() - sizeof(SharedMemoryGlobalData));
    free_list_memory_pool_ = free_list_memory_pool;

    SharedMemoryLocalData* shared_memory_local_data = &(shared_memory_global_data_->shared_memory_local_data[rank_]);
    if (shared_memory_local_data->initialized) {
        std::cerr << std::format("{}: Shared memory data is somehow already initialized?", rank_) << std::endl;
        return false;
    } else {
        // Use placement new to create object at specified memory address
        // (as opposed to regular new that creates new location in heap memory)
        shared_memory_local_data_ = new (shared_memory_local_data) SharedMemoryLocalData{};
        shared_memory_local_data_->initialized = true;

        std::cout << std::format("{}: Initialized shared memory data.", rank_)
                  << std::endl;
    }

    shared_memory_control_flags_->shm_is_initialized = true;
    return true;
}

void SharedMemoryManager::uninitialize() {
    shared_memory_control_flags_->stop_requested = true;
}

bool SharedMemoryManager::has_results() {
    return !shared_memory_global_data_->shared_memory_local_data[rank_].results.is_empty();
}

SharedMemoryQueryResponse&& SharedMemoryManager::pop_result() {
    return shared_memory_global_data_->shared_memory_local_data[rank_].results.pop();
}

QueryResponse SharedMemoryManager::deserialize(SharedMemoryQueryResponse& shared_memory_query_response) {
    // Copy shared_memory_query_response into a query_response
    std::size_t num_collisions = shared_memory_query_response.data_size / sizeof(Collision);
    std::vector<Collision> collisions(num_collisions); // pre-allocate enough space
    std::memcpy(collisions.data(),
                free_list_memory_pool_.data() + shared_memory_query_response.data_offset,
                shared_memory_query_response.data_size);

    std::vector<std::uint32_t> requested_by(shared_memory_query_response.requested_by_size); // pre-allocate enough space
    std::copy_n(shared_memory_query_response.requested_by.begin(),
                shared_memory_query_response.requested_by_size,
                requested_by.begin());

    // Deallocate the shared_memory_query_response
    {
        BakeryMutexGuard guard(shared_memory_global_data_->free_list_global_mutex, rank_);
        shared_memory_global_data_->memory_blocks_free_list.deallocate(free_list_memory_pool_.data() + shared_memory_query_response.data_offset,
                                                                       free_list_memory_pool_);
        std::cout << std::format("{}: Deallocated block back to free list.", rank_)
                  << std::endl;
    }

    // Destroy shared_memory_query_response so these aren't accidentally re-accessed
    shared_memory_query_response.data_offset = -1;
    shared_memory_query_response.data_size = 0;

    return {
        .id = shared_memory_query_response.id,
        .requested_by = requested_by,
        .results_from = shared_memory_query_response.results_from,
        .collisions = collisions,
    };
}

void SharedMemoryManager::send_results(const std::size_t parent_rank, const QueryResponse& query_response) {
    // Check that data will fit in allocated free list blocks
    std::size_t data_size = sizeof(Collision) * query_response.collisions.size();

    if (shared_memory_global_data_->memory_blocks_free_list.block_size_ < data_size) {
        std::cerr << std::format("{}: Block size {} too small to store data of size {} for request of id {}!",
                                 rank_, shared_memory_global_data_->memory_blocks_free_list.block_size_, data_size, query_response.id)
                  << std::endl;
        shared_memory_control_flags_->stop_requested = true;
        exit(1);
    }

    // Check that the requested_by array will fit
    if (query_response.requested_by.size() > MAX_REQUESTED_BY_DEPTH) {
        std::cerr << std::format("{}: Requested_by size too large!", rank_) << std::endl;
        shared_memory_control_flags_->stop_requested = true;
        exit(1);
    }

    // Wait until able to allocate a free list block
    uint8_t* result_data_ptr = nullptr;
    {
        BakeryMutexGuard guard(shared_memory_global_data_->free_list_global_mutex, rank_);
        result_data_ptr = shared_memory_global_data_->memory_blocks_free_list.allocate(free_list_memory_pool_);
    }

    if (result_data_ptr == nullptr) {
        std::cerr << std::format("{}: Could not allocate block from free list for result!", rank_)
                  << std::endl;
        shared_memory_control_flags_->stop_requested = true;
        exit(1);
    } else {
        std::cout << std::format("{}: Allocated block from free list.", rank_)
                  << std::endl;
    }

    // Copy data to allocated block
    std::size_t data_offset = result_data_ptr - free_list_memory_pool_.data();
    std::memcpy(result_data_ptr, query_response.collisions.data(), data_size);

    // Copy requested_by vector to array
    std::array<std::uint32_t, MAX_REQUESTED_BY_DEPTH> requested_by{};
    std::copy_n(query_response.requested_by.begin(), query_response.requested_by.size(), requested_by.begin());

    // Create the response struct and add it to the parent's ringbuffer
    SharedMemoryQueryResponse response {
        .id = query_response.id,
        .requested_by = requested_by,
        .requested_by_size = query_response.requested_by.size(),
        .results_from = query_response.results_from,
        .data_offset = data_offset,
        .data_size = data_size,
    };

    send_results(parent_rank, response);
}

void SharedMemoryManager::send_results(const std::size_t parent_rank, SharedMemoryQueryResponse& query_response) {
    {
        BakeryMutexGuard guard(shared_memory_global_data_->shared_memory_local_data[parent_rank].mutex, rank_);
        query_response.requested_by_size--; // pop self rank from end of requested_by list
        shared_memory_global_data_->shared_memory_local_data[parent_rank].results.push(query_response);
    }

    std::cout << std::format("{}: Added response to parent rank: {} shared memory ringbuffer.", rank_, parent_rank) << std::endl;
}
