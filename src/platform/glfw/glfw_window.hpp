#pragma once

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cobalt/platform/window.hpp>
#include <cobalt/render/render_api.hpp>

#include <vector>

namespace cobalt::platform {

class glfw_window : public window {
public:
    glfw_window(const window_spec&, renderer::render_api api = renderer::render_api::opengl);

    glfw_window(const glfw_window& rhs) = delete;
    glfw_window& operator=(const glfw_window& rhs) = delete;

    glfw_window(glfw_window&& rhs) = delete;
    glfw_window& operator=(glfw_window&& rhs) = delete;

    ~glfw_window() override;

    bool should_close() const override;
    void poll_events() const override;

    [[nodiscard]] const std::vector<const char*>& get_glfw_required_extensions() const noexcept {
        return _extensions;
    }

    void create_surface(VkInstance instance, VkSurfaceKHR* surface);

    [[nodiscard]] GLFWwindow* get_window_handle() const noexcept {
        return _window;
    }

    void swap_buffers() override;

    void set_vsync(vsync_mode mode) override;

    std::unique_ptr<monitor> get_primary_monitor() override;

    void set_key_callback(key_event_callback callback) override;
    void set_mouse_callback(mouse_event_callback callback) override;
    void set_mouse_button_callback(mouse_button_event_callback callback) override;
    void set_scroll_callback(scroll_event_callback callback) override;

private:
    void query_glfw_required_extensions();

    std::vector<const char*> _extensions;

    window_spec _spec{};
    GLFWwindow* _window{};

    static void glfw_error_callback(int error, const char* description);
    static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void glfw_mouse_callback(GLFWwindow* window, double x_pos, double y_pos);
    static void glfw_mouse_button_callback(GLFWwindow* window, int key, int action, int mods);
    static void glfw_scroll_callback(GLFWwindow* window, double x_offset, double y_offset);

    static key_event_callback _callback;
    static mouse_event_callback _mouse_callback;
    static mouse_button_event_callback _mouse_button_callback;
    static scroll_event_callback _scroll_callback;
};

} // namespace cobalt::platform
