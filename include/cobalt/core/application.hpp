#pragma once

#include <cobalt/core/config.hpp>

namespace cobalt::core {

/// @brief Main entry point
class application {
public:
    /// @brief Construct a new application object
    ///
    /// @param argc Number of command line arguments
    /// @param argv Command line arguments
    application(int argc, char** argv);

    /// @brief Deleted copy constructor
    ///
    /// @param rhs Other application
    application(const application& rhs) = delete;

    /// @brief Deleted copy assignment operator
    ///
    /// @param rhs Other application
    /// @return application&
    application& operator=(const application& rhs) = delete;

    /// @brief Deleted move constructor
    ///
    /// @param rhs Other application
    application(application&& rhs) = delete;

    /// @brief Deleted move assignment operator
    ///
    /// @param rhs Other application
    /// @return application&
    application& operator=(application&& rhs) = delete;

    /// @brief Run application main loop
    void run();

    /// @brief Returns whether application should close
    ///
    /// @return true If it should
    /// @return false If it should not
    [[nodiscard]] bool should_close() const noexcept {
        return _should_close;
    }

private:
    config _config;
    bool _should_close{ false };
};

} // namespace cobalt::core