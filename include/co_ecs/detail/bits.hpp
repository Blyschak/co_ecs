#pragma once

namespace co_ecs::detail {

/// @brief Calculate the value % b=2^n
///
/// @param value Value
/// @param divisor Power of 2 divisor
/// @return decltype(auto) Result
constexpr decltype(auto) mod_2n(auto value, auto divisor) noexcept {
    return value & (divisor - 1U);
}

/// @brief Divide value by divisor, the divisor has to be a power of two
///
/// @param value Value
/// @param divisor Power of 2 divisor
/// @return constexpr decltype(auto) Result
constexpr decltype(auto) div_2n(auto value, auto divisor) noexcept {
    return value >> (divisor - 1U);
}

} // namespace cobalt::asl