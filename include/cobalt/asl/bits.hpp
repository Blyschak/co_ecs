#pragma once

namespace cobalt::asl {

/// @brief Calculate the value % b=2^n
///
/// @param value Value
/// @param divisor Power of 2 value
/// @return decltype(auto) Result
constexpr decltype(auto) mod_2n(auto value, auto divisor) noexcept {
    return value & (divisor - 1U);
}

} // namespace cobalt::asl