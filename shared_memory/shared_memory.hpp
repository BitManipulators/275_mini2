#pragma once

#include <cstdint>
#include <unistd.h>

class SharedMemory {
public:
    SharedMemory(const std::uint64_t rank);
    ~SharedMemory();

    bool is_attached() const;
    void* get_data();
    std::size_t get_size();

private:
    std::uint64_t rank_;
    int shm_id_;
    void* shm_ptr_;
    std::size_t shm_size_;
    bool attached_;
};
