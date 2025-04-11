#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>

// AI Assistance Was Used

class Freelist {
private:
    size_t free_list_head_offset_;  // Offset to the head of the freelist

public:
    // Constructor: Initializes the freelist with a memory area and block size
    Freelist(std::span<uint8_t> memory_pool, size_t block_size, size_t num_blocks)
    : block_size_{block_size}
    , num_blocks_{num_blocks} {
        std::cout << "Block size: " << block_size << " and num_blocks: " << num_blocks << std::endl;

        // Initialize the freelist with offsets (not pointers)
        free_list_head_offset_ = 0;  // The first block starts at offset 0

        uint8_t* block = memory_pool.data();
        for (size_t i = 0; i < num_blocks - 1; ++i) {
            // Store the offset to the next block
            size_t* next_block_offset = reinterpret_cast<size_t*>(block);
            *next_block_offset = (block + block_size) - memory_pool.data(); // Offset to next block
            block += block_size;
        }

        // The last block's offset points to -1 (indicating the end of the freelist)
        size_t* last_block_offset = reinterpret_cast<size_t*>(block);
        *last_block_offset = static_cast<size_t>(-1);  // End of freelist marker
    }

    // Allocate a block
    uint8_t* allocate(std::span<uint8_t> memory_pool) {
        if (free_list_head_offset_ == static_cast<size_t>(-1)) {
            return nullptr;  // No free blocks left
        }

        // Get the base address of the block using the offset and the base address
        uint8_t* allocated_block = memory_pool.data() + free_list_head_offset_;

        // Get the next block's offset (stored at the beginning of the block)
        size_t* next_block_offset = reinterpret_cast<size_t*>(allocated_block);
        free_list_head_offset_ = *next_block_offset;  // Update the freelist head to the next block's offset

        // Return the usable part of the block (skip the offset field)
        return allocated_block + sizeof(size_t);
    }

    // Deallocate a block
    void deallocate(uint8_t* ptr, std::span<uint8_t> memory_pool) {
        // Adjust the pointer back to the beginning of the block (to where the offset is stored)
        uint8_t* block = ptr - sizeof(size_t);

        // Get the offset to the next block (from the pointer to the next block)
        size_t* next_block_offset = reinterpret_cast<size_t*>(block);
        *next_block_offset = free_list_head_offset_;  // Link the block back to the freelist

        // Update the freelist head to this block's offset
        free_list_head_offset_ = block - memory_pool.data();
    }

    // Print the current freelist (for debugging)
    void print_freelist(std::span<uint8_t> memory_pool) {
        size_t current_offset = free_list_head_offset_;
        while (current_offset != static_cast<size_t>(-1)) {
            uint8_t* current = memory_pool.data() + current_offset;
            std::cout << "Block at offset: " << current_offset << std::endl;
            size_t* next_block_offset = reinterpret_cast<size_t*>(current);
            current_offset = *next_block_offset;
        }
        std::cout << "End of freelist (offset = -1)" << std::endl;
    }

    size_t block_size_;
    size_t num_blocks_;
};
