#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>

#include <co_ecs/detail/bits.hpp>

namespace co_ecs::detail {

/// @brief Stack allocator
class stack_allocator {
private:
    using padding_t = uint8_t;

    /// @brief Allocation header
    struct alloc_header {
        padding_t padding;
    };

public:
    /// @brief Construct stack allocator
    /// @param
    /// @param size Size of the underlaying buffer
    stack_allocator(void* ptr, std::size_t size) : _start(static_cast<char*>(ptr)), _size(size) {
    }

    /// @brief Construct
    /// @param size Size of the underlaying buffer
    stack_allocator(std::size_t size) : stack_allocator(new char[size], size) {
    }

    stack_allocator(const stack_allocator& rhs) = delete;
    stack_allocator& operator=(const stack_allocator& rhs) = delete;

    /// @brief Move constructor
    /// @param rhs Other allocator
    stack_allocator(stack_allocator&& rhs) noexcept :
        _start(std::exchange(rhs._start, nullptr)), _size(std::exchange(rhs._size, 0)) {
    }

    /// @brief Move assignment
    /// @param rhs Other allocator
    /// @return Allocator
    stack_allocator& operator=(stack_allocator&& rhs) noexcept {
        _start = std::exchange(rhs._start, nullptr);
        _size = std::exchange(rhs._size, 0);
        return *this;
    }

    /// @brief Allocate bytes given the alignemnt
    /// @param bytes Number of bytes to allocate
    /// @param alignment Alignment
    /// @return Pointer to allocated chunk
    auto allocate(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t)) noexcept -> void* {
        assert(is_power_of_2(alignment) && "Alignment must be a power of two");
        assert((alignment <= std::numeric_limits<padding_t>::max()) && "Alignment is out of range");

        // Get the top of the stack
        char* top = _start + _offset;
        // Get the pointer to the top + 1 allocation header
        void* ptr = top + sizeof(alloc_header);
        // Calculate remaining space
        auto remaining_space = _size - _offset - sizeof(alloc_header);
        // Calculate aligned pointer
        void* aligned_ptr = std::align(alignment, bytes, ptr, remaining_space);
        if (!aligned_ptr) {
            return nullptr;
        }

        // Get the required padding from the top of the stack to the allocation
        auto padding = static_cast<padding_t>(static_cast<char*>(aligned_ptr) - top);
        // Increment offset by padding and allocation size
        _offset += padding + bytes;

        // Write header
        new (static_cast<char*>(aligned_ptr) - sizeof(alloc_header)) alloc_header{ padding };

        return aligned_ptr;
    }

    /// @brief Deallocate by pointer (should be top of the stack)
    /// @param ptr Pointer to deallocate
    void deallocate(void* ptr) noexcept {
        assert((ptr < _start + _size) && (ptr >= _start) && "Outside allocation range");

        // Fetch header
        auto* hdr = reinterpret_cast<alloc_header*>(static_cast<char*>(ptr) - sizeof(alloc_header));
        // Decrement offset
        _offset = static_cast<char*>(ptr) - hdr->padding - _start;

        hdr->~alloc_header(); // no-op
    }

    /// @brief Get remaining bytes left
    /// @return Bytes left
    [[nodiscard]] auto remaining() const noexcept -> std::size_t {
        return (_size - _offset);
    }

    /// @brief Reset allocator
    void reset() noexcept {
        _offset = 0;
    }

private:
    char* _start;
    std::size_t _offset{};
    std::size_t _size;
};

} // namespace co_ecs::detail