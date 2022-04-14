#pragma once

#include <cstdint>

namespace cobalt::asl {

/// @brief Reference Counting object
class ref_count {
public:
    /// @brief Construct a new ref count object
    constexpr ref_count() = default;

    /// @brief Default copy constructor
    ///
    /// @param rhs Other object
    constexpr ref_count(const ref_count& rhs) = default;

    /// @brief Default copy assignment operator
    ///
    /// @param rhs Other object
    /// @return constexpr ref_count& New object
    constexpr ref_count& operator=(const ref_count& rhs) = default;

    /// @brief Increment reference count
    ///
    /// @return std::size_t Reference count after increment
    constexpr std::size_t increment() {
        return ++_rc;
    }

    /// @brief Decrement reference count
    ///
    /// @return std::size_t Reference count after decrement
    constexpr std::size_t decrement() {
        return --_rc;
    }

    /// @brief Return reference count
    ///
    /// @return std::size_t Reference count
    constexpr std::size_t reference_count() const {
        return _rc;
    }

    /// @brief Check whether reference count is not 0
    ///
    /// @return true If reference count is not 0
    /// @return false If reference count reached 0
    [[nodiscard]] constexpr bool alive() const {
        return _rc != 0;
    }

private:
    std::size_t _rc{};
};

} // namespace cobalt::asl