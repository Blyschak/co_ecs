#pragma once

#include <concepts>
#include <string>

namespace cobalt::asl {

/// @brief Parse string into T
///
/// @tparam T Target type
/// @param str String to parse
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