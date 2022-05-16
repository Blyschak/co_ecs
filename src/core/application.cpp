#include <cobalt/core/application.hpp>
#include <cobalt/core/logging.hpp>
#include <fstream>

namespace cobalt::core {

constexpr auto default_config_path = "config.ini";

application::application(int argc, char** argv) {
    core::log_info("starting...");


    _scheduler
        .add_init_system([](ecs::command_queue& commands) {
            config conf;
            std::ifstream config_file(default_config_path);
            if (!config_file.is_open()) {
                core::log_info("config file {} does not exists, using defaults", default_config_path);
            } else {
                conf = config::from_stream(config_file);
            }

            conf.set_default("core.log_level", "info");
            core::set_log_level(conf.get<core::log_level>("core.log_level"));

            conf.set_default("window.width", "1920");
            conf.set_default("window.height", "1080");
            conf.set_default("window.title", "engine");

            commands.set_resource<config>(std::move(conf));
        })
        .add_init_system([](const config& conf, ecs::command_queue& commands) {
            platform::window_spec spec{
                conf.get<int>("window.width"),
                conf.get<int>("window.height"),
                conf.get("window.title"),
            };

            auto window = platform::window::create(spec);

            commands.set_resource<std::unique_ptr<platform::window>>(std::move(window));
        })
        .add_init_system([](const std::unique_ptr<platform::window>& window, ecs::command_queue& commands) {
            auto renderer = render::renderer::create(*window);
            commands.set_resource<std::unique_ptr<render::renderer>>(std::move(renderer));
        })
        .init();
}

void application::run() {
}

} // namespace cobalt::core