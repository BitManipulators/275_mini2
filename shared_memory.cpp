#include "shared_memory.hpp"

#include <format>
#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {

#define SHM_KEY 1234  // Key for shared memory segment
#define SHM_SIZE (4L * 1024 * 1024 * 1024)  // 4 GB (64-bit literal)

const std::size_t SHM_MAX_RETRIES = 3;

int get_or_create_shared_memory(const off_t shm_size, const std::uint64_t rank) {
    // Get the shared memory segment ID
    int shm_id = shmget(SHM_KEY, 0, 0);

    // If not -1 that means a segment with this key already exists
    if (shm_id != -1) {
        struct shmid_ds shm_ds;
        if (shmctl(shm_id, IPC_STAT, &shm_ds) == -1) {
            std::cerr << std::format("{}: Failed to ctl IPC_STAT shared memory.", rank)
                      << std::endl;
            return -1;
        }

        // Print info about existing shared memory
        std::cout << std::format("{}: Found existing shared memory id: {}, size: {}, nattach: {}",
                                 rank, shm_id, shm_ds.shm_segsz, shm_ds.shm_nattch)
                  << std::endl;

        // If this segment is not the correct size, attempt to remove it
        if (shm_ds.shm_segsz != shm_size) {
            std::cerr << std::format("{}: Existing shared memory of size: {} does not match desired size: {}",
                                     rank, shm_ds.shm_segsz, shm_size)
                      << std::endl;

            // Removal cannot happen unless no processes are attached
            if (shm_ds.shm_nattch != 0) {
                std::cerr << std::format("{}: Existing shared memory has other processes still attached, cannot remove it!",
                                         rank, shm_ds.shm_segsz, shm_size)
                          << std::endl;
                return -1;
            }

            // Remove the shared memory
            if (shmctl(shm_id, IPC_RMID, nullptr) == -1) {
                std::cerr << std::format("{}: Failed to ctl IPC_RMID shared memory.", rank)
                          << std::endl;
                return -1;
            } else {
                std::cout << std::format("{}: Flagged shared memory of id {} for removal.", rank, shm_id)
                          << std::endl;

                // Get the shm_id again and check if it was removed
                int new_shm_id = shmget(SHM_KEY, 0, 0);

                if (new_shm_id == -1) {
                    std::cout << std::format("{}: Successfully removed shared memory.", rank)
                              << std::endl;
                } else if (new_shm_id == shm_id) {
                    std::cerr << std::format("{}: Failed to remove shared memory.", rank, shm_id)
                              << std::endl;
                    return -1;
                } else {
                    std::cout << std::format("{}: Successfully removed shared memory of id {}, new id is: {}",
                                             rank, shm_id, new_shm_id)
                              << std::endl;
                }

                shm_id = new_shm_id;
            }
        }
    }

    // If shmget is -1, try to create the shared memory
    if (shm_id == -1) {
        std::cerr << std::format("{}: Shared memory does not exist. Creating a new one.", rank)
                  << std::endl;

        // Create the shared memory
        shm_id = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT | 0666);
        if (shm_id == -1) {
            std::cerr << std::format("{}: Failed to create shared memory.", rank)
                      << std::endl;
            return -1;
        }
    }

    return shm_id;
}

void* attach_shared_memory(const int shm_id, const std::uint64_t rank) {
    // Attach to the shared memory segment
    void* shm_ptr = shmat(shm_id, nullptr, 0);
    if (shm_ptr == (void*)-1) {
        std::cerr << std::format("{}: Failed to attach to shared memory", rank) << std::endl;
        return nullptr;
    }

    return shm_ptr;
}

}  // namespace

SharedMemory::SharedMemory(const std::size_t rank)
  : rank_{rank}
  , shm_id_{-1}
  , shm_ptr_{nullptr}
  , attached_{false}
{
    int shm_id = -1;
    void* shm_ptr = nullptr;

    for (std::size_t retry_count = 0; retry_count < SHM_MAX_RETRIES; ++retry_count) {
        shm_id = get_or_create_shared_memory(SHM_SIZE, rank);
        if (shm_id == -1) {
            continue;
        }

        shm_ptr = attach_shared_memory(shm_id, rank);
        if (shm_ptr != nullptr) {
            break;
        }
    }

    if (shm_ptr != nullptr) {
        std::cout << std::format("{}: Process has attached to shared memory.", rank) << std::endl;

        shm_id_ = shm_id;
        shm_ptr_ = shm_ptr;
        shm_size_ = SHM_SIZE;
        attached_ = true;
    } else {
        std::cerr << std::format("{}: Process failed to attach to shared memory.", rank) << std::endl;
    }
}

SharedMemory::~SharedMemory() {
    if (attached_) {
        // Detach from the shared memory
        if (shmdt(shm_ptr_) == -1) {
            std::cerr << std::format("{}: Failed to detach from shared memory", rank_)
                      << std::endl;
            return;
        }

        if (shmctl(shm_id_, IPC_RMID, nullptr) == -1) {
            std::cerr << std::format("{}: Failed to ctl IPC_RMID shared memory.", rank_)
                      << std::endl;
            return;
        }

        std::cout << std::format("{}: Process has detached from shared memory.", rank_)
                  << std::endl;
    }
}

bool SharedMemory::is_attached() const {
    return attached_;
}

void* SharedMemory::get_data() {
    return shm_ptr_;
}

std::size_t SharedMemory::get_size() {
    return shm_size_;
}

/*
void* SharedMemory::allocate(std::size_t bytes) {
    std::uint8_t ptr = next_free_bytes_ptr_;

    if (ptr + bytes > shm_ptr_ + shm_size_) {
        return nullptr;
    }

    next_free_bytes_ptr_ += bytes;

    return ptr;
}
*/
