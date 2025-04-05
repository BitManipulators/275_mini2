#pragma once

#include <atomic>
#include <cstdint>

const std::size_t MAX_NUM_PROCESSES = 16;

struct BakeryMutex {
    std::atomic<std::uint64_t> process_flags[MAX_NUM_PROCESSES];
    std::atomic<std::uint64_t> process_tickets[MAX_NUM_PROCESSES];
    std::atomic<std::uint64_t> current_ticket;

    void lock(std::uint64_t process_rank);
    void unlock(std::uint8_t process_rank);
};

class BakeryMutexGuard {
public:
    BakeryMutexGuard(BakeryMutex& mutex, std::uint64_t process_rank);
    ~BakeryMutexGuard();

private:
    BakeryMutex& mutex_;
    std::uint64_t process_rank_;
};
