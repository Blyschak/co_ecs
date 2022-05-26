#pragma once

#include <cstdint>

namespace cobalt {

using mouse_code_t = uint16_t;

enum class mouse_code : mouse_code_t {
    // From glfw3.h
    button0 = 0,
    button1 = 1,
    button2 = 2,
    button3 = 3,
    button4 = 4,
    button5 = 5,
    button6 = 6,
    button7 = 7,

    button_last = button7,
    button_left = button0,
    button_right = button1,
    button_middle = button2
};

struct mouse_position {
    double x;
    double y;
};

struct scroll_offset {
    double dx;
    double dy;
};

} // namespace cobalt