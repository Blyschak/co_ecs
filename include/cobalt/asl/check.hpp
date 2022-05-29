#pragma once

#include <cobalt/asl/format.hpp>

#include <stdexcept>

namespace cobalt::asl {

namespace {

void check_failed [[noreturn]] (auto&& fmt, auto&&... args) {
    throw std::runtime_error(fmt);
}

void check(bool result, auto&& msg, auto&&... args) {
    if (!result) {
        check_failed(std::forward<decltype(msg)>(msg));
    }
}

} // namespace

} // namespace cobalt::asl
