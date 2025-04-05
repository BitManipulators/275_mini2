#include "bakery_mutex.hpp"

#include <atomic>
#include <format>
#include <iostream>

const std::size_t MAX_INITIALIZING_WAIT_SECONDS = 10;

void BakeryMutex::lock(std::uint64_t process_rank) {
//    std::cout << std::format("{}: Attempting to acquire lock", process_rank)
//              << std::endl;

    process_flags[process_rank] = true;

    std::uint64_t largest_ticket = 0;
    for (int rank = 0; rank < MAX_NUM_PROCESSES; ++rank) {
        std::uint64_t current_ticket = process_tickets[rank].load();

//        std::cout << std::format("{}: Checking rank: {}, ticket is: {}", process_rank, rank, current_ticket)
//                  << std::endl;

        if (current_ticket > largest_ticket) {
            largest_ticket = current_ticket;
        }
    }
    std::uint64_t my_ticket = largest_ticket + 1;
    process_tickets[process_rank] = my_ticket;

//    std::cout << std::format("{}: Setting my_ticket: {}", process_rank, my_ticket)
//              << std::endl;

    for (int rank = 0; rank < MAX_NUM_PROCESSES; ++rank) {
        // Skip self process
        if (rank == process_rank) {
            continue;
        }

        // Skip processes not currently trying to lock
        if (!process_flags[rank].load()) {
            continue;
        }

        while (true) {
            if (!process_flags[rank].load()) {
                break;
            }

            std::uint64_t current_process_ticket = process_tickets[rank].load();
            if (current_process_ticket > my_ticket || (current_process_ticket == my_ticket && rank > process_rank)) {
                break;
            }
        }
    }

//    std::cout << std::format("{}: Acquired lock", process_rank)
//              << std::endl;
}

void BakeryMutex::unlock(std::uint8_t process_rank) {
    process_flags[process_rank] = false;

//    std::cout << std::format("{}: Released lock", process_rank)
//              << std::endl;
}

BakeryMutexGuard::BakeryMutexGuard(BakeryMutex& mutex, std::uint64_t process_rank)
  : mutex_{mutex}
  , process_rank_{process_rank}
{
    mutex_.lock(process_rank_);
}

BakeryMutexGuard::~BakeryMutexGuard() {
    mutex_.unlock(process_rank_);
}
