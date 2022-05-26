#include <cobalt/asl/check.hpp>
#include <cobalt/core/logging.hpp>

#include "../../platform/glfw/glfw_window.hpp"
#include "gl_renderer.hpp"

#include <glad/glad.h>

namespace cobalt::renderer {

gl_renderer::gl_renderer(platform::window& window) : _window(window) {
    auto* glfw_window = dynamic_cast<platform::glfw_window*>(&_window);
    asl::check(glfw_window, "GLFW window implementation is required");

    int status = gladLoadGLLoader((GLADloadproc)(glfwGetProcAddress));
    asl::check(status, "failed to load GL loader");

    core::log_info("OpenGL info:");
    core::log_info("\tvendor: {}", glGetString(GL_VENDOR));
    core::log_info("\trenderer: {}", glGetString(GL_RENDERER));
    core::log_info("\tversion: {}", glGetString(GL_VERSION));

    asl::check(
        GLVersion.major > 3 || (GLVersion.major == 3 && GLVersion.minor >= 3), "OpenGL version >= 4.5 is required!");
}

} // namespace cobalt::renderer