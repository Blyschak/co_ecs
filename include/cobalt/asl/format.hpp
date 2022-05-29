#pragma once

#ifdef __cpp_lib_format
#include <format>
#else
#include <fmt/core.h>
#endif

namespace cobalt::asl {

#ifdef __cpp_lib_format
template<typename T>
using formatter = std::formatter;
#else
template<typename T>
using formatter = fmt::formatter<T>;
#endif

decltype(auto) format(auto&&... args) {
#ifdef __cpp_lib_format
    return std::format(std::forward<decltype(args)>(args)...);
#else
    return fmt::format(std::forward<decltype(args)>(args)...);
#endif
}

} // namespace cobalt::asl
