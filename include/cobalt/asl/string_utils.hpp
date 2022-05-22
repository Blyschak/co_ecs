#pragma once

#include <string>

namespace cobalt::asl {

/// @brief Left trim the string
///
/// @param str String to ltrim
/// @return std::string
static inline std::string ltrim(std::string str) {
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](auto ch) { return !std::isspace(ch); }));
    return str;
}

/// @brief Right trim the string
///
/// @param str String to rtrim
/// @return std::string
static inline std::string rtrim(std::string str) {
    str.erase(std::find_if(str.rbegin(), str.rend(), [](auto ch) { return !std::isspace(ch); }).base(), str.end());
    return str;
}

/// @brief Trim the string
///
/// @param str String to trim
/// @return std::string
static inline std::string trim(std::string str) {
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](auto ch) { return !std::isspace(ch); }));
    str.erase(std::find_if(str.rbegin(), str.rend(), [](auto ch) { return !std::isspace(ch); }).base(), str.end());
    return str;
}

} // namespace cobalt::asl