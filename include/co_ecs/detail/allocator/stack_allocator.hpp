#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>

namespace co_ecs::detail {

/// @brief Stack allocator
class stack_allocator {
private:
    struct alloc_header {
        uint8_t padding;
    };

public:
    /// @brief Construct
    /// @param size Size of the underlaying buffer
    stack_allocator(std::size_t size) : _start(new char[size]), _size(size) {
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

    /// @brief Destructor
    ~stack_allocator() {
        delete[] _start;
    }

    /// @brief Allocate bytes given the alignemnt
    /// @param bytes Number of bytes to allocate
    /// @param alignment Alignment
    /// @return Pointer to allocated chunk
    void* allocate(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t)) noexcept {
        assert((alignment % 2 == 0) && "Alignment must be a power of two");
        assert(
            (alignment <= std::numeric_limits<decltype(alloc_header::padding)>::max()) && "Alignment is out of range");

        auto padding = get_padding(bytes, alignment);

        if (_offset + padding + sizeof(alloc_header) + bytes > _size) {
            return nullptr;
        }

        _offset += padding;
        auto* hdr_ptr = _start + _offset;
        _offset += sizeof(alloc_header);
        new (hdr_ptr) alloc_header{ padding };
        auto* ptr = _start + _offset;
        _offset += bytes;
        return ptr;
    }

    /// @brief Deallocate by pointer (should be top of the stack)
    /// @param ptr Pointer to deallocate
    void deallocate(void* ptr) noexcept {
        assert((ptr < _start + _size) && (ptr >= _start) && "Outside allocation range");

        auto* hdr = reinterpret_cast<alloc_header*>(static_cast<char*>(ptr) - sizeof(alloc_header));
        _offset = static_cast<char*>(ptr) - hdr->padding - _start;
        hdr->~alloc_header(); // no-op
    }

    void free_all() noexcept {
        _offset = 0;
    }

private:
    [[nodiscard]]
    uint8_t get_padding(std::size_t size, std::size_t alignment) const noexcept {
        void* ptr = _start + _offset + sizeof(alloc_header);
        std::size_t space = (static_cast<char*>(_start) + _size) - static_cast<char*>(ptr);
        void* aligned_ptr = std::align(alignment, size, ptr, space);
        assert(aligned_ptr && "Failed to align");
        return static_cast<uint8_t>(
            static_cast<char*>(aligned_ptr) - (static_cast<char*>(_start) + _offset) - sizeof(alloc_header));
    }

private:
    char* _start;
    std::size_t _offset{};
    std::size_t _size;
};

} // namespace co_ecs::detail