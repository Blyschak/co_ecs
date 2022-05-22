#pragma once

#include <cobalt/asl/demangle.hpp>
#include <cobalt/asl/platform.hpp>

#include <typeinfo>

namespace cobalt::asl {

inline std::string type_name(const std::type_info& tinfo) {
#ifdef COBALT_COMPILER_GCC
    return asl::demangle(tinfo.name());
#else
    return tinfo.name();
#endif
}