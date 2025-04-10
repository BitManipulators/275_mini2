#pragma once

#include <array>
#include <cstdint>
#include <iostream>

template<class T, std::size_t capacity>
struct Ringbuffer {
    std::array<T, capacity> buffer;
    std::size_t head;
    std::size_t tail;
    std::size_t size;

    bool is_empty() {
        return size == 0;
    }

    bool is_full() {
        return size == capacity;
    }

    void push(const T& data_in) {
        if (is_full()) {
            throw std::runtime_error("Ringbuffer is full!");
        }

        buffer[tail] = data_in;
        tail = (tail + 1) % capacity;
        asm volatile("" ::: "memory");
        size++;
    }

    T&& pop() {
        if (is_empty()) {
            throw std::runtime_error("Ringbuffer is empty!");
        }

        T& t = buffer[head];
        head = (head + 1) % capacity;
        asm volatile("" ::: "memory");
        size--;
        return std::move(t);
    }

    [[nodiscard]] bool peek(T* data_out) {
        if (is_empty()) {
            return false;
        }

        *data_out = buffer[head];
        return true;
    }

    class Iterator {
    private:
        Ringbuffer* rb;
        size_t index;
        size_t count;

    public:
        Iterator(Ringbuffer* buffer, size_t idx, size_t cnt) : rb(buffer), index(idx), count(cnt) {}

        // Dereference operator
        T& operator*() {
            return rb->buffer[index];
        }

        // Increment operator (wraps around if needed)
        Iterator& operator++() {
            index = (index + 1) % capacity;
            ++count;
            return *this;
        }

        // Equality operator
        bool operator!=(const Iterator& other) const {
            return index != other.index || count != other.count;
        }

        // Equality operator (for end comparison)
        bool operator==(const Iterator& other) const {
            return index == other.index && count == other.count;
        }
    };

    // Begin iterator
    Iterator begin() {
        return Iterator(this, head, 0);
    }

    // End iterator (points to one past the last element)
    Iterator end() {
        return Iterator(this, tail, size);
    }
};
