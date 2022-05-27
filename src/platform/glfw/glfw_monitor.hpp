#pragma once

#include <cobalt/asl/check.hpp>
#include <cobalt/platform/monitor.hpp>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace cobalt {

class glfw_monitor : public monitor {
public:
    glfw_monitor(GLFWmonitor* monitor) : _monitor(monitor) {
        asl::check(_monitor, "null monitor handle");
    }

    ~glfw_monitor() override = default;

    monitor_spec get_spec() const override;

private:
    GLFWmonitor* _monitor;
};

} // namespace cobalt