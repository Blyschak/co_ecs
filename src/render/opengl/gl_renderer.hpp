#include <cobalt/core/logging.hpp>
#include <cobalt/render/renderer.hpp>

namespace cobalt {

class gl_renderer : public renderer {
public:
    gl_renderer(window& window);

    ~gl_renderer() override {
        log_info("destroying OpenGL renderer");
    }

    gl_renderer(const gl_renderer& rhs) = delete;
    gl_renderer& operator=(const gl_renderer& rhs) = delete;
    gl_renderer(gl_renderer&& rhs) = delete;
    gl_renderer& operator=(gl_renderer&& rhs) = delete;

private:
    window& _window;
};

} // namespace cobalt
