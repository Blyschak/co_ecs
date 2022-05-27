#pragma once

#include <stdexcept>

namespace cobalt::asl {

namespace {

void check_failed [[noreturn]] (auto&& msg) {
    throw std::runtime_error(msg);
}

void check(bool result, auto&& msg) {
    if (!result) {
        check_failed(std::forward<decltype(msg)>(msg));
    }
}

} // namespace

} // namespace cobalt::asl
