#pragma once

#include <chrono>
#include <memory>
#include <source_location>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace cobalt::core {

namespace detail {
    /// @brief Create logger instance
    ///
    /// @return std::shared_ptr<spdlog::logger>
    std::shared_ptr<spdlog::logger> init_logger() {
        auto logger = spdlog::stderr_color_mt("cobalt");
        logger->set_pattern("[%H:%M:%S %z] [%n] [%^%l%$] [%s.%#] %v");
        logger->set_level(spdlog::level::level_enum::trace);
        return logger;
    }
} // namespace detail

/// @brief Global logger instance
inline static std::shared_ptr<spdlog::logger> logger = detail::init_logger();

template<typename... Args>
struct log_critical {
    inline log_critical(fmt::format_string<Args...> fmt,
        Args&&... args,
        const std::source_location& loc = std::source_location::current()) {
        logger->log(spdlog::source_loc{ loc.file_name(), static_cast<int>(loc.line()), loc.function_name() },
            spdlog::level::level_enum::critical,
            std::forward<Args>(args)...);
    }
};

template<typename... Args>
log_critical(fmt::format_string<Args...> fmt, Args&&...) -> log_critical<Args...>;

template<typename... Args>
struct log_err {
    inline log_err(fmt::format_string<Args...> fmt,
        Args&&... args,
        const std::source_location& loc = std::source_location::current()) {
        logger->log(spdlog::source_loc{ loc.file_name(), static_cast<int>(loc.line()), loc.function_name() },
            spdlog::level::level_enum::err,
            std::forward<Args>(args)...);
    }
};

template<typename... Args>
log_err(fmt::format_string<Args...> fmt, Args&&...) -> log_err<Args...>;

template<typename... Args>
struct log_warn {
    inline log_warn(fmt::format_string<Args...> fmt,
        Args&&... args,
        const std::source_location& loc = std::source_location::current()) {
        logger->log(spdlog::source_loc{ loc.file_name(), static_cast<int>(loc.line()), loc.function_name() },
            spdlog::level::level_enum::warn,
            std::forward<Args>(args)...);
    }
};

template<typename... Args>
log_warn(fmt::format_string<Args...> fmt, Args&&...) -> log_warn<Args...>;

template<typename... Args>
struct log_info {
    inline log_info(fmt::format_string<Args...> fmt,
        Args&&... args,
        const std::source_location& loc = std::source_location::current()) {
        logger->log(spdlog::source_loc{ loc.file_name(), static_cast<int>(loc.line()), loc.function_name() },
            spdlog::level::level_enum::info,
            fmt,
            std::forward<Args>(args)...);
    }
};

template<typename... Args>
log_info(fmt::format_string<Args...> fmt, Args&&...) -> log_info<Args...>;

template<typename... Args>
struct log_debug {
    inline log_debug(fmt::format_string<Args...> fmt,
        Args&&... args,
        const std::source_location& loc = std::source_location::current()) {
        logger->log(spdlog::source_loc{ loc.file_name(), static_cast<int>(loc.line()), loc.function_name() },
            spdlog::level::level_enum::debug,
            std::forward<Args>(args)...);
    }
};

template<typename... Args>
log_debug(fmt::format_string<Args...> fmt, Args&&...) -> log_debug<Args...>;

template<typename... Args>
struct log_trace {
    inline log_trace(fmt::format_string<Args...> fmt,
        Args&&... args,
        const std::source_location& loc = std::source_location::current()) {
        logger->log(spdlog::source_loc{ loc.file_name(), static_cast<int>(loc.line()), loc.function_name() },
            spdlog::level::level_enum::trace,
            std::forward<Args>(args)...);
    }
};

template<typename... Args>
log_trace(fmt::format_string<Args...> fmt, Args&&...) -> log_trace<Args...>;

class scoped_log {
public:
    inline scoped_log(std::string_view ident, const std::source_location& loc = std::source_location::current()) :
        _ident(ident), _loc(loc) {
        logger->log(spdlog::source_loc{ _loc.file_name(), static_cast<int>(_loc.line()), _loc.function_name() },
            spdlog::level::level_enum::trace,
            "entered \"{}\"",
            _ident);
    }

    scoped_log(const scoped_log&) = delete;
    scoped_log& operator=(const scoped_log&) = delete;
    scoped_log(scoped_log&&) = delete;
    scoped_log& operator=(scoped_log&&) = delete;

    inline ~scoped_log() {
        logger->log(spdlog::source_loc{ _loc.file_name(), static_cast<int>(_loc.line()), _loc.function_name() },
            spdlog::level::level_enum::trace,
            "exited \"{}\"",
            _ident);
    }

private:
    std::string_view _ident;
    std::source_location _loc;
};

class scoped_timer_log {
public:
    inline scoped_timer_log(std::string_view ident, const std::source_location& loc = std::source_location::current()) :
        _ident(ident), _loc(loc), _start(std::chrono::high_resolution_clock::now()) {
        logger->log(spdlog::source_loc{ _loc.file_name(), static_cast<int>(_loc.line()), _loc.function_name() },
            spdlog::level::level_enum::debug,
            "entered timer \"{}\"",
            _ident);
    }

    scoped_timer_log(const scoped_timer_log&) = delete;
    scoped_timer_log& operator=(const scoped_timer_log&) = delete;
    scoped_timer_log(scoped_timer_log&&) = delete;
    scoped_timer_log& operator=(scoped_timer_log&&) = delete;

    inline ~scoped_timer_log() {
        auto duration = std::chrono::high_resolution_clock::now() - _start;
        logger->log(spdlog::source_loc{ _loc.file_name(), static_cast<int>(_loc.line()), _loc.function_name() },
            spdlog::level::level_enum::debug,
            "exited timer \"{}\", took {} us",
            _ident,
            std::chrono::duration<double, std::micro>(duration).count());
    }

private:
    std::string_view _ident;
    std::source_location _loc;
    std::chrono::time_point<std::chrono::high_resolution_clock> _start;
};

} // namespace cobalt::core
