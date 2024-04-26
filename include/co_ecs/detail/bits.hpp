#pragma once

namespace co_ecs::detail {

/// @brief Calculate the value % b=2^n
///
/// @param value Value
/// @param divisor Power of 2 divisor
/// @return decltype(auto) Result
constexpr auto mod_2n(auto value, auto divisor) noexcept -> decltype(auto) {
    return value & (divisor - 1U);
}

/// @brief Check if value is a power of 2
/// @param value Power of two value
/// @return True if power of two, false otherwise
constexpr auto is_power_of_2(auto value) -> bool {
    return ((value & (value - 1U)) == 0);
}

} // namespace co_ecs::detail