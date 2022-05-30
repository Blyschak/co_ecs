#pragma once

#include <cobalt/core/logging.hpp>

#include <stdexcept>

#define co_unreachable(...)                        \
    do {                                           \
        auto msg = fmt::format(__VA_ARGS__);       \
        log_critical("unreachable code: {}", msg); \
        std::abort();                              \
    } while (0)

#define co_assert(expr, ...)                                         \
    do {                                                             \
        if (!(expr)) {                                               \
            auto msg = fmt::format(__VA_ARGS__);                     \
            log_critical("assertion failed \"{}\": {}", #expr, msg); \
            std::abort();                                            \
        }                                                            \
    } while (0)
