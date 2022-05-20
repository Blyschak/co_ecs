#include <cobalt/core/application.hpp>
#include <cobalt/core/logging.hpp>
#include <cobalt/core/pointer.hpp>
#include <fstream>

namespace cobalt::core {

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
    core::log_info("starting...");


    _scheduler
        .add_init_system([](ecs::command_queue& commands) {
            config conf;

            if (std::ifstream config_file(default_config_path); !config_file.is_open()) {
                core::log_info("config file {} does not exists, using defaults", default_config_path);
            } else {
                conf = config::from_stream(config_file);
            }

            conf.set_default("core.log_level", core::log_level::info);
            core::set_log_level(conf.get<core::log_level>("core.log_level"));

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
            platform::window_spec spec{
                conf.get<int>("window.width"),
                conf.get<int>("window.height"),
                conf.get("window.title"),
            };

            auto window = platform::window::create(spec);

            window->set_key_callback(
                [&key_events](key_code key, key_state action) { key_events.publish(key, action); });

            window->set_mouse_button_callback(
                [&mouse_events](mouse_code key, key_state action) { mouse_events.publish(key, action); });

            window->set_mouse_callback(
                [&mouse_pos_events](mouse_position position) { mouse_pos_events.publish(position); });

            window->set_scroll_callback(
                [&scroll_offset_events](scroll_offset offset) { scroll_offset_events.publish(offset); });

            commands.set_resource<owned<platform::window>>(std::move(window));
        })
        .add_init_system([](const owned<platform::window>& window, ecs::command_queue& commands) {
            auto renderer = renderer::renderer::create(*window);
            commands.set_resource<owned<renderer::renderer>>(std::move(renderer));
        })
        .add_system([](const owned<platform::window>& window, ecs::scheduler_control& scheduler_control) {
            window->poll_events();
            window->swap_buffers();
            if (window->should_close()) {
                core::log_info("window should close");
                scheduler_control.should_exit = true;
            }
        })
        .add_system([](const ecs::event_reader<key_event>& key_events,
                        const ecs::event_reader<mouse_event>& mouse_events,
                        const ecs::event_reader<mouse_position>& mouse_pos_events,
                        const ecs::event_reader<scroll_offset>& scroll_offset_events,
                        ecs::scheduler_control& scheduler_control) {
            for (const auto& key_event : key_events) {
                core::log_trace("key {} action {}", key_event.key, key_event.state);
            }

            for (const auto& mouse_event : mouse_events) {
                core::log_trace("mouse button {} action {}", mouse_event.button, mouse_event.state);
            }

            for (const auto& pos : mouse_pos_events) {
                core::log_trace("mouse position {} {}", pos.x, pos.y);
            }

            for (const auto& offset : scroll_offset_events) {
                core::log_trace("scroll offset {} {}", offset.dx, offset.dy);
            }
        });
}

void application::run() {
    _scheduler.run();
}

} // namespace cobalt::core