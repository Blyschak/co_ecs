#include <cobalt/core/assert.hpp>
#include <cobalt/core/logging.hpp>

#include "../../platform/glfw/glfw_window.hpp"
#include "gl_renderer.hpp"

#include <glad/glad.h>

namespace cobalt {

gl_renderer::gl_renderer(window& window) : _window(window) {
    auto* w = dynamic_cast<glfw_window*>(&_window);
    assert_with_message(w, "GLFW window implementation is required");

    int status = gladLoadGLLoader((GLADloadproc)(glfwGetProcAddress));
    assert_with_message(status, "failed to load GL loader");

    log_info("OpenGL info:");
    log_info("\tvendor: {}", glGetString(GL_VENDOR));
    log_info("\trenderer: {}", glGetString(GL_RENDERER));
    log_info("\tversion: {}", glGetString(GL_VERSION));

    assert_with_message(
        GLVersion.major > 3 || (GLVersion.major == 3 && GLVersion.minor >= 3), "OpenGL version >= 4.5 is required!");
}

} // namespace cobalt