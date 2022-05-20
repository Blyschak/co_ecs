#include "glfw_window.hpp"

#include <cobalt/core/logging.hpp>

#include <cobalt/asl/check.hpp>

namespace cobalt::platform {

std::function<void(int key, int action)> glfw_window::_callback = nullptr;

void glfw_window::glfw_error_callback(int error, const char* description) {
    core::log_err("glfw error ({}): {}", error, description);
}

void glfw_window::glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    glfw_window::_callback(key, action);
}

glfw_window::glfw_window(const window_spec& spec, renderer::render_api api) : _spec(spec) {
    glfwSetErrorCallback(glfw_error_callback);

    glfwInit();

    switch (api) {
    case renderer::render_api::opengl:
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
        break;
    }

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    core::log_info("creating window {} {}x{}", _spec.title, _spec.width, _spec.height);

    _window = glfwCreateWindow(_spec.width, _spec.height, _spec.title.c_str(), nullptr, nullptr);
    asl::check(_window, "failed to create GLFW window");

    switch (api) {
    case renderer::render_api::opengl:
        glfwMakeContextCurrent(_window);
        break;
    case renderer::render_api::vulkan:
        query_glfw_required_extensions();
        break;
    }

    set_vsync(spec.vsync);

    glfwSetKeyCallback(_window, glfw_key_callback);
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

void glfw_window::swap_buffers() {
    glfwSwapBuffers(_window);
}

void glfw_window::set_vsync(vsync_mode mode) {
    glfwSwapInterval(mode == vsync_mode::full ? 1 : 0);
}

void glfw_window::set_key_callback(std::function<void(int key, int action)> callback) {
    _callback = callback;
}

} // namespace cobalt::platform