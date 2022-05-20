#pragma once

#include <functional>
#include <memory>
#include <string>

namespace cobalt::platform {

enum class vsync_mode {
    disable,
    full,
};

struct window_spec {
    int width{ 0 };
    int height{ 0 };
    std::string title{ "example" };
    vsync_mode vsync{ vsync_mode::full };
};

class window {
public:
    virtual ~window() = default;

    static std::unique_ptr<window> create(const window_spec& spec);

    virtual void set_key_callback(std::function<void(int key, int action)> callback) = 0;

    virtual bool should_close() const = 0;
    virtual void poll_events() const = 0;
    virtual void swap_buffers() = 0;
    virtual void set_vsync(vsync_mode mode) = 0;
};

} // namespace cobalt::platform
