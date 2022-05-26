#pragma once

namespace cobalt::platform {

struct monitor_spec {
    std::string name;

    int width_mm{};
    int height_mm{};

    int width{};
    int height{};
    int green_bits{};
    int blue_bits{};
    int red_bits{};
    int refresh_rate{};
};

class monitor {
public:
    virtual ~monitor() = default;

    virtual monitor_spec get_spec() const = 0;
};


} // namespace cobalt::platform