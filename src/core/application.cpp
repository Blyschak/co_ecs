#include <cobalt/core/application.hpp>
#include <cobalt/core/logging.hpp>
#include <fstream>

namespace cobalt::core {

constexpr auto default_config_path = "config.ini";

application::application(int argc, char** argv) {
    core::log_info("starting...");

    std::ifstream config_file(default_config_path);
    if (!config_file.is_open()) {
        core::log_info("config file {} does not exists, using defaults", default_config_path);
    }

    _config.set_default("core.log_level", "info");
    core::set_log_level(_config.get<core::log_level>("core.log_level"));

    _config.set_default("window.width", "1920");
    _config.set_default("window.height", "1080");
    _config.set_default("window.title", "engine");

    platform::window_spec spec{
        _config.get<int>("window.width"), _config.get<int>("window.height"), _config.get("window.title")
    };
    _window = platform::window::create(spec);
}

void application::run() {
    while (!_window->should_close()) {
        _window->poll_events();
    }
}

} // namespace cobalt::core