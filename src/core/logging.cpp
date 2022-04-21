#include <cobalt/core/logging.hpp>

namespace cobalt::core {

static std::shared_ptr<spdlog::logger> logger = detail::init_logger();

std::shared_ptr<spdlog::logger> get_logger() {
    return logger;
}

} // namespace cobalt::core