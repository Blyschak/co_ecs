#include <cobalt/core/application.hpp>
#include <cobalt/core/logging.hpp>
#include <fstream>

namespace cobalt::core {

constexpr auto default_config_path = "config.ini";

application::application(int argc, char** argv) {
    core::log_info("starting...");

    std::ifstream config_file(default_config_path);
    if (!config_file.is_open()) {
        core::log_warn("config file {} does not exists, using defaults", default_config_path);
    }

    _config.set_default("core.log_level", "info");

    core::set_log_level(_config.get<core::log_level>("core.log_level"));
}

void application::run() {
    while (!should_close()) {
    }
}

} // namespace cobalt::core