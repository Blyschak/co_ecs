#include <cobalt/asl/check.hpp>
#include <cobalt/render/renderer.hpp>

#include "opengl/gl_renderer.hpp"
#include "vulkan/vk_renderer.hpp"

namespace cobalt::renderer {

std::unique_ptr<renderer> renderer::create(platform::window& window, render_api api) {
    switch (api) {
    case render_api::opengl:
        return std::make_unique<gl_renderer>(window);
    case render_api::vulkan:
        return std::make_unique<vk_renderer>(window);
    }

    asl::check_failed("unknown render API");
}

} // namespace cobalt::renderer