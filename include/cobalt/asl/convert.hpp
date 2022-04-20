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
static inline T from_string(std::string_view str);

/// @brief Parse string as T and return the result
///
/// @tparam T Type
/// @param str String to parse
/// @return T Result
template<std::signed_integral T>
static inline T from_string(std::string str) {
    return static_cast<T>(std::stoll(str));
}

/// @brief Parse string as T and return the result
///
/// @tparam T Type
/// @param str String to parse
/// @return T Result
template<std::unsigned_integral T>
static inline T from_string(std::string str) {
    return static_cast<T>(std::stoull(str));
}

/// @brief Parse string as T and return the result. Specialization for convertiable types
///
/// @tparam T Type
/// @param str String to parse
/// @return T Result
template<>
inline std::string from_string<std::string>(std::string_view str) {
    return std::string(str);
}

} // namespace cobalt::asl