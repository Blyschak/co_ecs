#include <cobalt/render/renderer.hpp>

#include "vulkan/vk_renderer.hpp"

namespace cobalt::render {

std::unique_ptr<renderer> renderer::create(platform::window& window) {
    return std::make_unique<vk_renderer>(window);
}

} // namespace cobalt::render