#pragma once

#include <cstdlib>
#include <cxxabi.h>


namespace cobalt::asl {

/// @brief Demangles the name using CXX ABI
///
/// @param mangled_name Mangled name string
/// @return std::string
inline std::string demangle(const std::string& mangled_name) {
    int status;
    std::string name;

    auto c_name = abi::__cxa_demangle(mangled_name.c_str(), 0, 0, &status);
    name = c_name;
    std::free(c_name);

    return name;
}

} // namespace cobalt::asl