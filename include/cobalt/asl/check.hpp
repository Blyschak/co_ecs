#pragma once

#include <stdexcept>

namespace cobalt::asl {

static inline void check_failed [[noreturn]] (auto&& msg) {
    throw std::runtime_error(msg);
}

static inline void check(bool result, auto&& msg) {
    if (!result) {
        check_failed(std::forward<decltype(msg)>(msg));
    }
}

} // namespace cobalt::asl
