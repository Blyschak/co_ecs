#pragma once

#include <cstddef>
#include <memory>

namespace co_ecs::detail {

/// @class Linear Allocator
/// @brief A simple linear allocator class that allocates memory sequentially.
///
/// This allocator manages a fixed-size memory region and allocates memory sequentially from it.
class linear_allocator {
public:
    /// @brief Constructs a linear allocator with a given memory region.
    /// @param ptr Pointer to the start of the memory region.
    /// @param size Size of the memory region in bytes.
    explicit constexpr linear_allocator(void* ptr, std::size_t size) noexcept : _ptr(ptr), _head(ptr), _size(size) {
    }

    /// @brief Deleted copy constructor to prevent copying of allocator objects.
    linear_allocator(const linear_allocator&) = delete;

    /// @brief Deleted copy assignment operator to prevent assignment of allocator objects.
    linear_allocator& operator=(const linear_allocator&) = delete;

    /// @brief Defaulted move constructor.
    linear_allocator(linear_allocator&&) = default;

    /// @brief Defaulted move assignment operator.
    linear_allocator& operator=(linear_allocator&&) = default;

    /// @brief Allocates memory with a specified size and alignment.
    /// @param size Size of memory to allocate in bytes.
    /// @param alignment Alignment requirement for the allocation.
    /// @return Pointer to the allocated memory block, or nullptr if allocation fails.
    auto allocate(std::size_t size, std::size_t alignment) noexcept -> void* {
        auto space_left = _size - (static_cast<char*>(_head) - static_cast<char*>(_ptr));
        auto ptr = std::align(alignment, size, _head, space_left);
        if (!ptr) {
            return nullptr;
        }
        _head = static_cast<char*>(ptr) + size;
        return ptr;
    }

    /// @brief Resets the allocator's allocation position back to the start.
    constexpr void reset() noexcept {
        _head = _ptr;
    }

private:
    void* _head;
    void* _ptr;
    std::size_t _size;
};


} // namespace co_ecs::detail
