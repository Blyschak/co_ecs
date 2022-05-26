#include <cobalt/render/renderer.hpp>

#include "vk_device.hpp"

namespace cobalt::renderer {

class vk_renderer : public renderer {
public:
    vk_renderer(platform::window& window) : _device(window) {
    }

    vk_renderer(const vk_renderer& rhs) = delete;
    vk_renderer& operator=(const vk_renderer& rhs) = delete;
    vk_renderer(vk_renderer&& rhs) = delete;
    vk_renderer& operator=(vk_renderer&& rhs) = delete;

    ~vk_renderer() override = default;

private:
    vk_device _device;
};

} // namespace cobalt::renderer
