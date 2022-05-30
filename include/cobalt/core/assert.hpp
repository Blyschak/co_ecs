#pragma once

#include <cobalt/core/logging.hpp>

#include <stdexcept>

namespace cobalt {

namespace {

template<typename... Args>
void fail_with_message [[noreturn]] (Args&&... args) {
    // log_err(std::forward<Args>(args)...);
    throw 1;
}

template<typename... Args>
void assert_with_message(bool result, Args&&... args) {
    if (!result) {
        fail_with_message(std::forward<Args>(args)...);
    }
}

} // namespace

} // namespace cobalt
