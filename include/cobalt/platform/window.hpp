#pragma once

#include <functional>
#include <memory>
#include <string>

#include <cobalt/core/key.hpp>
#include <cobalt/core/mouse.hpp>
#include <cobalt/platform/monitor.hpp>

namespace cobalt::platform {

using key_event_callback = std::function<void(key_code, key_state)>;
using mouse_event_callback = std::function<void(mouse_position)>;
using mouse_button_event_callback = std::function<void(mouse_code, key_state)>;
using scroll_event_callback = std::function<void(scroll_offset)>;

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

    virtual void set_key_callback(key_event_callback callback) = 0;
    virtual void set_mouse_callback(mouse_event_callback callback) = 0;
    virtual void set_mouse_button_callback(mouse_button_event_callback callback) = 0;
    virtual void set_scroll_callback(scroll_event_callback callback) = 0;

    virtual std::unique_ptr<monitor> get_primary_monitor() = 0;

    virtual bool should_close() const = 0;
    virtual void poll_events() const = 0;
    virtual void swap_buffers() = 0;
    virtual void set_vsync(vsync_mode mode) = 0;
};

} // namespace cobalt::platform
