#pragma once

#include <sol/sol.hpp>

namespace cobalt::scripting {

/// @brief Lua scripting engine
class lua_engine {
public:
    /// @brief Construct a new lua engine object
    lua_engine();

private:
    sol::state lua;
};

} // namespace cobalt::scripting