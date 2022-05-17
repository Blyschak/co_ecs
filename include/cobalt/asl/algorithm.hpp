#pragma once

#include <algorithm>
#include <locale>
#include <string>

namespace cobalt::asl {

/// @brief Calculate the value % b=2^n
///
/// @param value Value
/// @param divisor Power of 2 value
/// @return decltype(auto) Result
constexpr decltype(auto) mod_2n(auto value, auto divisor) noexcept {
    return value & (divisor - 1U);
}

/// @brief Left trim the string
///
/// @param str String to ltrim
/// @return std::string
static inline std::string ltrim(auto&& str) {
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return str;
}

/// @brief Right trim the string
///
/// @param str String to rtrim
/// @return std::string
static inline std::string rtrim(auto&& str) {
    str.erase(
        std::find_if(str.rbegin(), str.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), str.end());
    return str;
}

/// @brief Trim the string
///
/// @param str String to trim
/// @return std::string
static inline std::string trim(auto&& str) {
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    str.erase(
        std::find_if(str.rbegin(), str.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), str.end());
    return str;
}

} // namespace cobalt::asl