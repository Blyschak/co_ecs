#pragma once

#include <stdexcept>

namespace cobalt::asl {

static inline void check(bool result, auto&& msg) {
    if (!result) {
        throw std::runtime_error(msg);
    }
}

} // namespace cobalt::asl
