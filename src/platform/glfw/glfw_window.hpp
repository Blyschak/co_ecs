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

    void set_key_callback(std::function<void(int key, int action)> callback) override;

private:
    void query_glfw_required_extensions();

    std::vector<const char*> _extensions;

    window_spec _spec{};
    GLFWwindow* _window{};

    static void glfw_error_callback(int error, const char* description);

    static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

    static std::function<void(int key, int action)> _callback;
};

} // namespace cobalt::platform
