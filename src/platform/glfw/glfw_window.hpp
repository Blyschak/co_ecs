#pragma once

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cobalt/asl/vector.hpp>
#include <cobalt/platform/window.hpp>

namespace cobalt::platform {
class glfw_window : public window {
public:
    glfw_window(const window_spec&);

    glfw_window(const glfw_window& rhs) = delete;
    glfw_window& operator=(const glfw_window& rhs) = delete;

    glfw_window(glfw_window&& rhs) = delete;
    glfw_window& operator=(glfw_window&& rhs) = delete;

    ~glfw_window() override;

    bool should_close() const override;
    void poll_events() const override;

    [[nodiscard]] const asl::vector<const char*>& get_glfw_required_extensions() const noexcept {
        return _extensions;
    }

    void create_surface(VkInstance instance, VkSurfaceKHR* surface);

private:
    void query_glfw_required_extensions();

    asl::vector<const char*> _extensions;

    window_spec _spec{};
    GLFWwindow* _window{};
};

} // namespace cobalt::platform
