#pragma once

#include <cstdlib>
#include <cxxabi.h>
#include <stdexcept>
#include <string>


namespace cobalt::asl {

/// @brief Demangles the name using CXX ABI
///
/// @param mangled_name Mangled name string
/// @return std::string
inline std::string demangle(const std::string& mangled_name) {
    // errors codes returned by __cxa_demangle
    constexpr auto allocation_error = -1;
    constexpr auto not_a_valid_name_error = -2;

    int status{};
    std::string name;

    auto* c_name = abi::__cxa_demangle(mangled_name.c_str(), nullptr, nullptr, &status);

    // handle failure
    if (status < 0) {
        if (status == allocation_error) {
            throw std::bad_alloc{};
        } else if (status == not_a_valid_name_error) {
            throw std::invalid_argument{ "invalid mangled name passed" };
        } else {
            throw std::logic_error{ "unexpected status code returned by __cxa_demangle" };
        }
    }

    // use try..catch to free in case of string assignment failure
    try {
        name = c_name;
    } catch (...) {
        // we are interoping with raw API, thus ignore linter warnings
        std::free(c_name); // NOLINT(cppcoreguidelines-no-malloc,hicpp-no-malloc,cppcoreguidelines-owning-memory)
        throw;
    }

    std::free(c_name); // NOLINT(cppcoreguidelines-no-malloc,hicpp-no-malloc,cppcoreguidelines-owning-memory)

    return name;
}

} // namespace cobalt::asl