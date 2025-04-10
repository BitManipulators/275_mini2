#pragma once

#include <cstring>
#include <format>
#include <iostream>

template <std::size_t N>
struct FixedString {
    char data[N + 1];  // +1 for null-terminator
    std::size_t length; // Store the actual length of the string

    // Constructor to initialize the string
    explicit FixedString(const char* str) : length(0) {
        std::size_t str_length = std::strlen(str);
        if (str_length > N) {
            std::string error_message = std::format("String in 'constructor char*': '{}' is longer than length: {}", str, N);
            std::cerr << error_message << std::endl;
            throw std::runtime_error(error_message);
        }

        // Initialize string to 0's
        std::memset(data, '\0', sizeof(data));
        // Copy at most N characters from str to data
        std::strncpy(data, str, str_length);
        // Ensure null termination
        data[str_length] = '\0';
        // Calculate and store the actual length of the string
        length = std::strlen(data);
    }

    // Constructor to initialize the string
    explicit FixedString(const std::string_view sv) : length(0) {
        std::size_t str_length = sv.size();
        if (str_length > N) {
            std::string error_message = std::format("String in 'constructor string_view': '{}' is longer than length: {}", std::string(sv), N);
            std::cerr << error_message << std::endl;
            throw std::runtime_error(error_message);
        }

        // Initialize string to 0's
        std::memset(data, '\0', sizeof(data));
        // Copy at most N characters from str to data
        std::strncpy(data, sv.data(), str_length);
        // Ensure null termination
        data[str_length] = '\0';
        // Calculate and store the actual length of the string
        length = std::strlen(data);
    }

    // Constructor to initialize with an empty string
    FixedString() : length(0) {
        data[0] = '\0'; // Ensure null termination
    }

    // Accessor to get the underlying data
    const char* c_str() const {
        return data;
    }

    // Size of the string (excluding null terminator), using stored length
    std::size_t size() const {
        return length;
    }

    FixedString& operator=(const char* other_str) {
        std::size_t str_length = std::strlen(other_str);
        if (str_length > N) {
            std::string error_message = std::format("String in '=': '{}' is longer than length: {}", other_str, N);
            std::cerr << error_message << std::endl;
            throw std::runtime_error(error_message);
        }

        // Initialize string to 0's
        std::memset(data, '\0', sizeof(data));
        // Copy at most N characters from str to data
        std::strncpy(data, other_str, str_length);
        // Ensure null termination
        data[str_length] = '\0';
        // Calculate and store the actual length of the string
        length = std::strlen(data);

        return *this;
    }

    // Method to compare two FixedString instances
    bool operator==(const FixedString& other) const {
        return length == other.length && std::strncmp(data, other.data, length) == 0;
    }

    // Method to compare with string literal char* instances
    bool operator==(const char* other) const {
        return std::strcmp(data, other) == 0;
    }

    // Method to compare with std::string instances
    bool operator==(const std::string& other) const {
        return length == other.size() && std::strncmp(data, other.c_str(), length) == 0;
    }

    // Method to print the string (for convenience)
    void print() const {
        std::cout << data << std::endl;
    }
};
