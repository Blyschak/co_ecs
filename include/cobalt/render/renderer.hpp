#pragma once

#include <memory>

#include <cobalt/platform/window.hpp>
#include <cobalt/render/render_api.hpp>

namespace cobalt {

class renderer {
public:
    virtual ~renderer() = default;

    static std::unique_ptr<renderer> create(window& window, render_api api = render_api::opengl);
};

} // namespace cobalt