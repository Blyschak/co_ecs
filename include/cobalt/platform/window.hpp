#pragma once

#include <memory>
#include <string>

namespace cobalt::platform {

enum class vsync_mode {
    disable,
    full,
};

struct window_spec {
    int width;
    int height;
    std::string title;
    vsync_mode vsync;
};

class window {
public:
    virtual ~window() = default;

    static std::unique_ptr<window> create(const window_spec& spec);

    virtual bool should_close() const = 0;
    virtual void poll_events() const = 0;
};

} // namespace cobalt::platform
