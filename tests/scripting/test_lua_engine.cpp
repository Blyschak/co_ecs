#include <cobalt/ecs/registry.hpp>
#include <cobalt/scripting/lua_engine.hpp>

#include <gtest/gtest.h>

using namespace cobalt;

struct position {
    int x;
    int y;
};

TEST(scripting, lua_engine_basic_ecs) {
    scripting::lua_engine lua_engine{};
    lua_engine.register_component<position, position(), position(int, int)>(
        "position", "x", &position::x, "y", &position::y);

    lua_engine.script(R"#(
        pos = position:new(1, 2)
        assert(pos.x == 1)
        assert(pos.y == 2)
    )#");

    lua_engine.script(R"#(
        reg = registry:new()
        ent = reg:create()

        local viewed_ent = nil
        reg:view():each(
            function(ent)
                viewed_ent = ent
            end
        )
        assert(viewed_ent:id() == ent:id())
    )#");
}