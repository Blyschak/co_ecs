#pragma once

#include <concepts>
#include <string>

namespace cobalt::asl {

/// @brief Parse string as T and return the result
///
/// @tparam T Type
/// @param str String to parse
/// @return T Result
template<std::signed_integral T>
T from_string(auto&& str) {
    return static_cast<T>(std::stoll(str));
}

/// @brief Parse string as T and return the result
///
/// @tparam T Type
/// @param str String to parse
/// @return T Result
template<std::unsigned_integral T>
T from_string(auto&& str) {
    return static_cast<T>(std::stoull(str));
}

/// @brief Parse string as T and return the result. Specialization for convertiable types
///
/// @tparam T Type
/// @param str String to parse
/// @return T Result
template<typename T>
T from_string(auto&& str) requires(std::is_convertible_v<decltype(str), T>) {
    return str;
}

} // namespace cobalt::asl