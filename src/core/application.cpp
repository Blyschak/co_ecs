#include <cobalt/core/application.hpp>
#include <cobalt/core/logging.hpp>
#include <cobalt/core/pointer.hpp>
#include <fstream>

namespace cobalt::core {

constexpr auto default_config_path = "config.ini";

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
        .add_init_system([](const config& conf, ecs::command_queue& commands) {
            platform::window_spec spec{
                conf.get<int>("window.width"),
                conf.get<int>("window.height"),
                conf.get("window.title"),
            };

            auto window = platform::window::create(spec);

            commands.set_resource<owned<platform::window>>(std::move(window));
        })
        .add_init_system([](const owned<platform::window>& window, ecs::command_queue& commands) {
            auto renderer = render::renderer::create(*window);
            commands.set_resource<owned<render::renderer>>(std::move(renderer));
        })
        .init();
}

void application::run() {
}

} // namespace cobalt::core