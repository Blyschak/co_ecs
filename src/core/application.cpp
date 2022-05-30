#include <cobalt/core/application.hpp>
#include <cobalt/core/logging.hpp>
#include <cobalt/core/pointer.hpp>
#include <fstream>

namespace cobalt {

constexpr auto default_config_path = "config.ini";

struct key_event {
    key_code key;
    key_state state;
};

struct mouse_event {
    mouse_code button;
    key_state state;
};

application::application(int argc, char** argv) {
    log_info("starting...");


    _scheduler
        .add_init_system([](ecs::command_queue& commands) {
            config conf;

            if (std::ifstream config_file(default_config_path); !config_file.is_open()) {
                log_info("config file {} does not exists, using defaults", default_config_path);
            } else {
                conf = config::from_stream(config_file);
            }

            set_log_level(log_level::trace);

            conf.set_default("window.width", 1920);
            conf.set_default("window.height", 1080);
            conf.set_default("window.title", "engine");

            commands.set_resource<config>(std::move(conf));
        })
        .add_init_system([](const config& conf,
                             ecs::command_queue& commands,
                             ecs::event_publisher<key_event>& key_events,
                             ecs::event_publisher<mouse_event>& mouse_events,
                             ecs::event_publisher<mouse_position>& mouse_pos_events,
                             ecs::event_publisher<scroll_offset>& scroll_offset_events) {
            window_spec spec{
                stoi(conf.get("window.width")),
                stoi(conf.get("window.height")),
                conf.get("window.title"),
            };

            auto w = window::create(spec);

            w->set_key_callback([&key_events](key_code key, key_state action) { key_events.publish(key, action); });

            w->set_mouse_button_callback(
                [&mouse_events](mouse_code key, key_state action) { mouse_events.publish(key, action); });

            w->set_mouse_callback([&mouse_pos_events](mouse_position position) { mouse_pos_events.publish(position); });

            w->set_scroll_callback(
                [&scroll_offset_events](scroll_offset offset) { scroll_offset_events.publish(offset); });

            commands.set_resource<owned<window>>(std::move(w));
        })
        .add_init_system([](const owned<window>& window, ecs::command_queue& commands) {
            auto r = renderer::renderer::create(*window);
            commands.set_resource<owned<renderer>>(std::move(r));
        })
        .add_system([](const owned<window>& window, ecs::scheduler_control& scheduler_control) {
            window->poll_events();
            window->swap_buffers();
            if (window->should_close()) {
                log_info("window should close");
                scheduler_control.should_exit = true;
            }
        })
        .add_system([](const ecs::event_reader<key_event>& key_events,
                        const ecs::event_reader<mouse_event>& mouse_events,
                        const ecs::event_reader<mouse_position>& mouse_pos_events,
                        const ecs::event_reader<scroll_offset>& scroll_offset_events,
                        ecs::scheduler_control& scheduler_control) {
            for (const auto& key_event : key_events) {
                log_trace("key {} action {}", key_event.key, key_event.state);
            }

            for (const auto& mouse_event : mouse_events) {
                log_trace("mouse button {} action {}", mouse_event.button, mouse_event.state);
            }

            for (const auto& pos : mouse_pos_events) {
                log_trace("mouse position {} {}", pos.x, pos.y);
            }

            for (const auto& offset : scroll_offset_events) {
                log_trace("scroll offset {} {}", offset.dx, offset.dy);
            }
        });
}

void application::run() {
    _scheduler.run();
}

} // namespace cobalt