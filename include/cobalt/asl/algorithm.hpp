#pragma once

#include <algorithm>

namespace cobalt::asl {

/// @brief Calculate the value % b=2^n
///
/// @param value Value
/// @param b Power of 2 value
/// @return decltype(auto) Result
constexpr decltype(auto) mod_2n(auto value, auto b) noexcept {
    return value & (b - 1u);
}

} // namespace cobalt::asl