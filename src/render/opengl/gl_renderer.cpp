#include <cobalt/asl/check.hpp>
#include <cobalt/core/logging.hpp>

#include "../../platform/glfw/glfw_window.hpp"
#include "gl_renderer.hpp"

#include <glad/glad.h>

namespace cobalt {

gl_renderer::gl_renderer(window& window) : _window(window) {
    auto* w = dynamic_cast<glfw_window*>(&_window);
    asl::check(w, "GLFW window implementation is required");

    int status = gladLoadGLLoader((GLADloadproc)(glfwGetProcAddress));
    asl::check(status, "failed to load GL loader");

    log_info("OpenGL info:");
    log_info("\tvendor: {}", glGetString(GL_VENDOR));
    log_info("\trenderer: {}", glGetString(GL_RENDERER));
    log_info("\tversion: {}", glGetString(GL_VERSION));

    asl::check(
        GLVersion.major > 3 || (GLVersion.major == 3 && GLVersion.minor >= 3), "OpenGL version >= 4.5 is required!");
}

} // namespace cobalt