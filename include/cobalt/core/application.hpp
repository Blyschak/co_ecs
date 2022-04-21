#pragma once

#include <cobalt/core/config.hpp>
#include <cobalt/platform/window.hpp>
#include <cobalt/render/renderer.hpp>

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

private:
    config _config;
    std::unique_ptr<platform::window> _window{};
    std::unique_ptr<render::renderer> _renderer{};
};

} // namespace cobalt::core