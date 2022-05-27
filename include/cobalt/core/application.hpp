#pragma once

#include <cobalt/core/config.hpp>
#include <cobalt/ecs/registry.hpp>
#include <cobalt/ecs/scheduler.hpp>
#include <cobalt/platform/window.hpp>
#include <cobalt/render/renderer.hpp>

namespace cobalt {

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
    ecs::registry _registry;
    ecs::simple_scheduler _scheduler{ _registry };
};

} // namespace cobalt