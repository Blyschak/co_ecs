#include <cobalt/platform/window.hpp>

#include "glfw/glfw_window.hpp"

namespace cobalt::platform {

std::unique_ptr<window> window::create(const window_spec& spec) {
    return std::make_unique<glfw_window>(spec);
};

} // namespace cobalt::platform
