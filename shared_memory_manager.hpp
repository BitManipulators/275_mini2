#pragma once

#include "bakery_mutex.hpp"
#include "collision_proto_converter.hpp"
#include "collision_query_service_impl.hpp"
#include "free_list.hpp"
#include "ring_buffer.hpp"
#include "shared_memory.hpp"

#include <atomic>
#include <array>
#include <chrono>
#include <csignal>
#include <format>
#include <iostream>
#include <unistd.h>


const std::size_t MAX_NUM_RANKS_ON_SAME_MACHINE = 10;
const std::size_t MAX_REQUESTED_BY_DEPTH = 10;

struct SharedMemoryQueryResponse {
    std::size_t id;
    std::array<std::uint32_t, MAX_REQUESTED_BY_DEPTH> requested_by;
    std::size_t requested_by_size;
    std::uint32_t results_from;
    std::size_t data_offset;
    std::size_t data_size;
};

struct SharedMemoryControlFlags {
    std::atomic<bool> shm_is_initializing;
    std::atomic<bool> shm_is_initialized;
    std::atomic<bool> stop_requested;
};

struct SharedMemoryLocalData {
    BakeryMutex mutex;
    Ringbuffer<SharedMemoryQueryResponse, MAX_CONCURRENT_REQUESTS * MAX_NUM_RANKS_ON_SAME_MACHINE> results;
    bool initialized;
};

struct SharedMemoryGlobalData {
    std::array<SharedMemoryLocalData, MAX_NUM_RANKS_ON_SAME_MACHINE> shared_memory_local_data;
    BakeryMutex free_list_global_mutex;
    Freelist memory_blocks_free_list;

    SharedMemoryGlobalData(std::span<uint8_t> shm_data, const std::size_t block_size, const size_t total_blocks)
      : shared_memory_local_data{}
      , free_list_global_mutex{}
      , memory_blocks_free_list{shm_data, block_size, total_blocks} {}
};

class SharedMemoryManager {
public:
    SharedMemoryManager(const std::uint64_t rank, const std::size_t block_size);
    ~SharedMemoryManager();

    bool has_results();
    SharedMemoryQueryResponse&& pop_result();
    BakeryMutex& get_lock(std::uint32_t rank);
    QueryResponse deserialize(SharedMemoryQueryResponse& shared_memory_query_response);
    void send_results(const std::size_t parent_rank, SharedMemoryQueryResponse& shared_memory_query_response);
    void send_results(const std::size_t parent_rank, const QueryResponse& query_response);

private:
    void initialize(const std::size_t block_size);
    bool initialize_global_shared_data(std::span<uint8_t> shm_data, const std::size_t block_size);
    void uninitialize();

    std::uint64_t rank_;
    SharedMemory shared_memory_;
    SharedMemoryControlFlags* shared_memory_control_flags_;
    SharedMemoryGlobalData* shared_memory_global_data_;
    SharedMemoryLocalData* shared_memory_local_data_;
    std::span<uint8_t> free_list_memory_pool_;
};
