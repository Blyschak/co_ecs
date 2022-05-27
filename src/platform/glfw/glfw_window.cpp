#include "glfw_window.hpp"
#include "glfw_monitor.hpp"

#include <cobalt/core/logging.hpp>

#include <cobalt/asl/check.hpp>

namespace cobalt {

key_event_callback glfw_window::_callback = [](auto&&...) {};
mouse_event_callback glfw_window::_mouse_callback = [](auto&&...) {};
mouse_button_event_callback glfw_window::_mouse_button_callback = [](auto&&...) {};
scroll_event_callback glfw_window::_scroll_callback = [](auto&&...) {};

void glfw_window::glfw_error_callback(int error, const char* description) {
    log_err("glfw error ({}): {}", error, description);
}

void glfw_window::glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    glfw_window::_callback(static_cast<key_code>(key), static_cast<key_state>(action));
}

void glfw_window::glfw_mouse_callback(GLFWwindow* window, double x_pos, double y_pos) {
    glfw_window::_mouse_callback(mouse_position{ x_pos, y_pos });
}

void glfw_window::glfw_mouse_button_callback(GLFWwindow* window, int key, int action, int mods) {
    glfw_window::_mouse_button_callback(static_cast<mouse_code>(key), static_cast<key_state>(action));
}

void glfw_window::glfw_scroll_callback(GLFWwindow* window, double x_offset, double y_offset) {
    glfw_window::_scroll_callback(scroll_offset{ x_offset, y_offset });
}

glfw_window::glfw_window(const window_spec& spec, render_api api) : _spec(spec) {
    glfwSetErrorCallback(glfw_error_callback);

    glfwInit();

    switch (api) {
    case render_api::opengl:
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
        break;
    }

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    log_info("creating window {} {}x{}", _spec.title, _spec.width, _spec.height);

    _window = glfwCreateWindow(_spec.width, _spec.height, _spec.title.c_str(), nullptr, nullptr);
    asl::check(_window, "failed to create GLFW window");

    switch (api) {
    case render_api::opengl:
        glfwMakeContextCurrent(_window);
        break;
    case render_api::vulkan:
        query_glfw_required_extensions();
        break;
    }

    set_vsync(spec.vsync);

    glfwSetKeyCallback(_window, glfw_key_callback);
    glfwSetCursorPosCallback(_window, glfw_mouse_callback);
    glfwSetMouseButtonCallback(_window, glfw_mouse_button_callback);
    glfwSetScrollCallback(_window, glfw_scroll_callback);

    auto monitor = get_primary_monitor();
    auto monitor_spec = monitor->get_spec();

    log_info("primary monitor {}", monitor_spec.name);
    log_info("\t width {} mm", monitor_spec.width_mm);
    log_info("\t height {} mm", monitor_spec.height_mm);
    log_info("\t width {}", monitor_spec.width);
    log_info("\t height {}", monitor_spec.height);
    log_info("\t green bits {}", monitor_spec.green_bits);
    log_info("\t blue bits {}", monitor_spec.blue_bits);
    log_info("\t red bits {}", monitor_spec.red_bits);
    log_info("\t refresh rate {}", monitor_spec.refresh_rate);
}

glfw_window::~glfw_window() {
    if (_window) {
        log_info("destroying window");
        glfwDestroyWindow(_window);
    }
    log_info("terminating platform window");
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

std::unique_ptr<monitor> glfw_window::get_primary_monitor() {
    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    asl::check(primary, "failed to get primary monitor");
    return std::make_unique<glfw_monitor>(primary);
}

void glfw_window::set_key_callback(key_event_callback callback) {
    _callback = callback;
}

void glfw_window::set_mouse_callback(mouse_event_callback callback) {
    _mouse_callback = callback;
}

void glfw_window::set_mouse_button_callback(mouse_button_event_callback callback) {
    _mouse_button_callback = callback;
}

void glfw_window::set_scroll_callback(scroll_event_callback callback) {
    _scroll_callback = callback;
}

} // namespace cobalt