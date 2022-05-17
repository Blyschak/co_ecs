#pragma once

#include <string>

/// @brief This module implements converting functions, siblings to std::to_string. This functions are simple utilities
/// and aren't designed to fulfill all the needs of complex configuration for parsing, like parsing integers in hex
/// format, etc.

namespace cobalt::asl {

/// @brief Parse string into T
///
/// @tparam T Target type
/// @param from String to parse
/// @return T Result
template<typename T>
static inline T from_string(std::string_view from);

template<>
inline int from_string(std::string_view from) {
    return std::stoi(std::string(from));
}

template<>
inline long from_string(std::string_view from) {
    return std::stol(std::string(from));
}

template<>
inline long long from_string(std::string_view from) {
    return std::stoll(std::string(from));
}

template<>
inline unsigned from_string(std::string_view from) {
    return std::stoul(std::string(from));
}

template<>
inline unsigned long from_string(std::string_view from) {
    return std::stoul(std::string(from));
}

template<>
inline unsigned long long from_string(std::string_view from) {
    return std::stoull(std::string(from));
}


} // namespace cobalt::asl