#pragma once

#include <algorithm>
#include <locale>
#include <string>

namespace cobalt::asl {

/// @brief Calculate the value % b=2^n
///
/// @param value Value
/// @param b Power of 2 value
/// @return decltype(auto) Result
constexpr decltype(auto) mod_2n(auto value, auto b) noexcept {
    return value & (b - 1u);
}

/// @brief Left trim the string
///
/// @param s String to ltrim
/// @return std::string
static inline std::string ltrim(auto&& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

/// @brief Right trim the string
///
/// @param s String to rtrim
/// @return std::string
static inline std::string rtrim(auto&& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

/// @brief Trim the string
///
/// @param s String to trim
/// @return std::string
static inline std::string trim(auto&& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

} // namespace cobalt::asl