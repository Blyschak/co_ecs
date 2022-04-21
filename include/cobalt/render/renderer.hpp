#pragma once

#include <memory>

#include <cobalt/platform/window.hpp>

namespace cobalt::render {

class renderer {
public:
    virtual ~renderer() = default;

    static std::unique_ptr<renderer> create(platform::window& window);
};

} // namespace cobalt::render