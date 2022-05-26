#pragma once

#include <concepts>
#include <functional>

namespace cobalt::asl {

/// @brief Defers the execution of callable until the end of the scope using
/// RAII
///
/// @tparam invocable_t Invocable type
template<std::invocable invocable_t>
class defer {
public:
    /// @brief Create defer by copying callable
    ///
    /// @param callable Callable to create defer from
    explicit constexpr defer(invocable_t& callable) requires(std::is_copy_constructible_v<invocable_t>) : _f(callable) {
    }

    /// @brief Create defer by moving callable
    ///
    /// @param callable Callable to create defer from
    explicit constexpr defer(invocable_t&& callable) requires(std::is_move_constructible_v<invocable_t>) :
        _f(std::move(callable)) {
    }

    /// @brief Deleted copy constructor
    ///
    /// @param callable Callable to create defer from
    constexpr defer(const defer& rhs) = delete;

    /// @brief Move constructor
    ///
    /// @param callable Callable to create defer from
    constexpr defer(defer&& rhs) noexcept requires(std::is_nothrow_move_constructible_v<invocable_t>) :
        _active(std::move(rhs._active)), _f(std::move(rhs._f)) {
        rhs.dismiss();
    }

    /// @brief Deleted copy assignment operator
    ///
    /// @param rhs Callable to create defer from
    /// @return Defer object
    constexpr defer& operator=(const defer& rhs) = delete;

    /// @brief Move assignment operator
    ///
    /// @param rhs Callable to create defer from
    /// @return Defer object
    constexpr defer& operator=(defer&& rhs) noexcept requires(std::is_nothrow_move_constructible_v<invocable_t>) {
        _active = std::move(rhs._active);
        _f = std::move(rhs._f);
        rhs.dismiss();
        return *this;
    }

    /// @brief Dismiss the execution
    constexpr void dismiss() noexcept {
        _active = false;
    }

    /// @brief Invokes the callable if not dismissed
    constexpr ~defer() {
        if (_active) {
            std::invoke(_f);
        };
    }

private:
    bool _active{ true };
    invocable_t _f;
};

/// @brief Deduction guide for function type which guides to defer<fn_t*> so
/// that defer stores a pointer
///        to the function since it is not valid to have defer<fn_t> when
///        std::is_function_v<fn_t>> == true
///
/// @tparam fn_t Function type
template<std::invocable fn_t>
requires(std::is_function_v<fn_t>) defer(fn_t&)
->defer<fn_t*>;

} // namespace cobalt::asl
