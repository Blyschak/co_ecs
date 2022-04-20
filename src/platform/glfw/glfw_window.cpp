#include "glfw_window.hpp"

namespace cobalt::platform {

glfw_window::glfw_window(const window_spec& spec) : _spec(spec) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    _window = glfwCreateWindow(_spec.width, _spec.height, _spec.title.c_str(), nullptr, nullptr);
}

glfw_window::~glfw_window() {
    if (_window) {
        glfwDestroyWindow(_window);
    }
    glfwTerminate();
}

bool glfw_window::should_close() const {
    return glfwWindowShouldClose(_window);
}

void glfw_window::poll_events() const {
    glfwPollEvents();
}

} // namespace cobalt::platform