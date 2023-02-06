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

} // namespace co_ecs::detail