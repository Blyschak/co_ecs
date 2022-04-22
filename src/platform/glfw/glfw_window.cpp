#include "glfw_window.hpp"

#include <cobalt/asl/check.hpp>

namespace cobalt::platform {

glfw_window::glfw_window(const window_spec& spec) : _spec(spec) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    _window = glfwCreateWindow(_spec.width, _spec.height, _spec.title.c_str(), nullptr, nullptr);
    asl::check(_window, "failed to create GLFW window");
    query_glfw_required_extensions();
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

void glfw_window::query_glfw_required_extensions() {
    uint32_t count = 0;
    auto extensions = glfwGetRequiredInstanceExtensions(&count);
    _extensions.assign(extensions, extensions + count);
}

void glfw_window::create_surface(VkInstance instance, VkSurfaceKHR* surface) {
    asl::check(glfwCreateWindowSurface(instance, _window, nullptr, surface) == VK_SUCCESS,
        "failed to create GLFW window surface");
}

} // namespace cobalt::platform