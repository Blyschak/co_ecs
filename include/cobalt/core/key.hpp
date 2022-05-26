#pragma once

#include <cstdint>

namespace cobalt {

using key_code_t = std::uint16_t;

enum class key_code : key_code_t {
    // From glfw3.h
    space = 32,
    apostrophe = 39, /* ' */
    comma = 44,      /* , */
    minus = 45,      /* - */
    period = 46,     /* . */
    slash = 47,      /* / */

    digit_0 = 48, /* 0 */
    digit_1 = 49, /* 1 */
    digit_2 = 50, /* 2 */
    digit_3 = 51, /* 3 */
    digit_4 = 52, /* 4 */
    digit_5 = 53, /* 5 */
    digit_6 = 54, /* 6 */
    digit_7 = 55, /* 7 */
    digit_8 = 56, /* 8 */
    digit_9 = 57, /* 9 */

    semicolon = 59, /* ; */
    equal = 61,     /* = */

    a = 65,
    b = 66,
    c = 67,
    d = 68,
    e = 69,
    f = 70,
    g = 71,
    h = 72,
    i = 73,
    j = 74,
    k = 75,
    l = 76,
    m = 77,
    n = 78,
    o = 79,
    p = 80,
    q = 81,
    r = 82,
    s = 83,
    t = 84,
    u = 85,
    v = 86,
    w = 87,
    x = 88,
    y = 89,
    z = 90,

    left_bracket = 91,  /* [ */
    back_slash = 92,    /* \ */
    right_bracket = 93, /* ] */
    grave_accent = 96,  /* ` */

    world1 = 161, /* non-US #1 */
    world2 = 162, /* non-US #2 */

    /* Function keys */
    escape = 256,
    enter = 257,
    tab = 258,
    backspace = 259,
    insert = 260,
    del = 261,
    right = 262,
    left = 263,
    down = 264,
    up = 265,
    page_up = 266,
    page_down = 267,
    home = 268,
    end = 269,
    caps_lock = 280,
    scroll_lock = 281,
    num_lock = 282,
    print_screen = 283,
    pause = 284,
    f1 = 290,
    f2 = 291,
    f3 = 292,
    f4 = 293,
    f5 = 294,
    f6 = 295,
    f7 = 296,
    f8 = 297,
    f9 = 298,
    f10 = 299,
    f11 = 300,
    f12 = 301,
    f13 = 302,
    f14 = 303,
    f15 = 304,
    f16 = 305,
    f17 = 306,
    f18 = 307,
    f19 = 308,
    f20 = 309,
    f21 = 310,
    f22 = 311,
    f23 = 312,
    f24 = 313,
    f25 = 314,

    /* keypad */
    kp0 = 320,
    kp1 = 321,
    kp2 = 322,
    kp3 = 323,
    kp4 = 324,
    kp5 = 325,
    kp6 = 326,
    kp7 = 327,
    kp8 = 328,
    kp9 = 329,
    kp_decimal = 330,
    kp_divide = 331,
    kp_multiply = 332,
    kp_subtract = 333,
    kp_add = 334,
    kp_enter = 335,
    kp_equal = 336,

    left_shift = 340,
    left_control = 341,
    left_alt = 342,
    left_super = 343,
    right_shift = 344,
    right_control = 345,
    right_alt = 346,
    right_super = 347,
    menu = 348
};

using key_state_t = uint8_t;

enum class key_state : key_state_t {
    released = 0,
    pressed = 1,
    repeated = 2,
};

} // namespace cobalt