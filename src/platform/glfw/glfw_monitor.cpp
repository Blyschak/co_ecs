#include "glfw_monitor.hpp"

namespace cobalt {

monitor_spec glfw_monitor::get_spec() const {
    monitor_spec s{};
    const GLFWvidmode* mode = glfwGetVideoMode(_monitor);
    asl::check(mode, "failed to get video mode");

    s.width = mode->width;
    s.height = mode->height;
    s.green_bits = mode->greenBits;
    s.blue_bits = mode->blueBits;
    s.red_bits = mode->redBits;
    s.refresh_rate = mode->refreshRate;

    glfwGetMonitorPhysicalSize(_monitor, &s.width_mm, &s.height_mm);

    const char* name = glfwGetMonitorName(_monitor);
    asl::check(name, "failed to get the monitor name");

    s.name = name;

    return s;
}

} // namespace cobalt