#pragma once

#include <co_ecs/detail/allocator/stack_allocator.hpp>

namespace co_ecs::detail {

constexpr auto global_stack_allocator_size = 16ull * 1024 * 1024; // 16 MB

static thread_local inline std::unique_ptr<uint8_t[]> global_stack_allocator_buffer(
    new uint8_t[global_stack_allocator_size]);

/// @brief Global stack allocator
static thread_local inline stack_allocator global_stack_allocator{ global_stack_allocator_buffer.get(),
    global_stack_allocator_size };

/// @brief Frame allocator, use for temporary allocations
/// @tparam T Type
template<typename T>
class temp_allocator {
public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using propagate_on_container_move_assignment = std::true_type;

    /// @brief Constructor
    temp_allocator() = default;

    /// @brief Copy constructor
    /// @tparam U
    /// @param other allocator instance
    template<class U>
    constexpr temp_allocator(const temp_allocator<U>& other) noexcept {
    }

    /// @brief Allocate n elements
    /// @param n Elements to allocate
    /// @return Pointer to allocated memory
    [[nodiscard]] constexpr T* allocate(std::size_t n) {
        auto* ptr = static_cast<T*>(global_stack_allocator.allocate(n * sizeof(T), alignof(T)));
        if (!ptr) {
            throw std::bad_alloc{};
        }
        return ptr;
    }

    /// @brief Deallocate storage referenced by p
    /// @param p Pointer to memory to deallocate
    /// @param n Number of elements to allocate (must be the same as passed to the corresponding allocate() p is
    /// obtained from)
    constexpr void deallocate(T* p, [[maybe_unused]] std::size_t n) {
        return global_stack_allocator.deallocate(p);
    }
};

} // namespace co_ecs::detail


template<class T, class U>
constexpr bool operator==(const co_ecs::detail::temp_allocator<T>&, const co_ecs::detail::temp_allocator<U>&) noexcept {
    return true;
}

template<class T, class U>
constexpr bool operator!=(const co_ecs::detail::temp_allocator<T>&, const co_ecs::detail::temp_allocator<U>&) noexcept {
    return false;
}