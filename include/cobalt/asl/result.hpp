#pragma once

#include <variant>

namespace cobalt::asl {

template<typename T, typename E>
class result {
public:
    constexpr result(T value) : _result(std::move(value)) { // NOLINT(hicpp-explicit-conversions)
    }

    constexpr result(E error) : _result(std::move(error)) { // NOLINT(hicpp-explicit-conversions)
    }

    constexpr ~result() = default;

    constexpr result(const result& rhs) = default;
    constexpr result& operator=(const result& rhs) = default;

    constexpr result(result&& rhs) noexcept = default;
    constexpr result& operator=(result&& rhs) noexcept = default;

    [[nodiscard]] constexpr bool is_ok() const noexcept {
        return std::holds_alternative<T>(_result);
    }

    [[nodiscard]] constexpr bool is_error() const noexcept {
        return std::holds_alternative<E>(_result);
    }

    [[nodiscard]] constexpr T& get() {
        return std::get<T>(_result);
    }

    [[nodiscard]] constexpr const T& get() const {
        return std::get<T>(_result);
    }

    [[nodiscard]] constexpr T& get_default(T& default_value) noexcept {
        if (is_ok()) {
            return std::get<T>(_result);
        }
        return default_value;
    }

    [[nodiscard]] constexpr const T& get_default(const T& default_value) const noexcept {
        if (is_ok()) {
            return std::get<T>(_result);
        }
        return default_value;
    }

    [[nodiscard]] constexpr E& error() {
        return std::get<E>(_result);
    }

    [[nodiscard]] constexpr const E& error() const {
        return std::get<E>(_result);
    }

private:
    std::variant<T, E> _result;
};

} // namespace cobalt::asl
