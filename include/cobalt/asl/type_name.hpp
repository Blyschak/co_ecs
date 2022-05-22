#pragma once

#include <cobalt/asl/demangle.hpp>
#include <cobalt/asl/platform.hpp>

#include <string>
#include <typeinfo>

namespace cobalt::asl {

/// @brief Returns a human readable type name
///
/// @tparam T Type to return name for
/// @return std::string
template<typename T>
std::string type_name() {
    const auto& tinfo = typeid(T);
#ifdef COBALT_COMPILER_GCC
    return asl::demangle(tinfo.name());
#else
    return tinfo.name();
#endif
}

} // namespace cobalt::asl