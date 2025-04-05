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
//    void* allocate(std::size_t bytes);

private:
    std::uint64_t rank_;
    int shm_id_;
    void* shm_ptr_;
    std::size_t shm_size_;
//    std::size_t shm_current_size_;
//    std::uint8_t* next_free_bytes_ptr_;
    bool attached_;
};
