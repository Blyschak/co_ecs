#include <cobalt/core/logging.hpp>
#include <cobalt/render/renderer.hpp>

namespace cobalt::renderer {

class gl_renderer : public renderer {
public:
    gl_renderer(platform::window& window);

    ~gl_renderer() override {
        core::log_info("destroying OpenGL renderer");
    }

    gl_renderer(const gl_renderer& rhs) = delete;
    gl_renderer& operator=(const gl_renderer& rhs) = delete;
    gl_renderer(gl_renderer&& rhs) = delete;
    gl_renderer& operator=(gl_renderer&& rhs) = delete;

private:
    platform::window& _window;
};

} // namespace cobalt::renderer
