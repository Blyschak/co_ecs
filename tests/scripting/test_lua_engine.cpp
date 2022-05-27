#include <cobalt/ecs/registry.hpp>
#include <cobalt/scripting/lua_engine.hpp>

#include <gtest/gtest.h>

using namespace cobalt;

struct position {
    int x;
    int y;
};

TEST(scripting, lua_engine_basic_ecs) {
    lua_engine engine{};
    engine.expose_component<position, position(), position(int, int)>("position", "x", &position::x, "y", &position::y);

    auto lua_script = R"#(
        pos = position:new(1, 2)
        assert(pos.x == 1)
        assert(pos.y == 2)
    )#";
    engine.script(lua_script);

    auto lua_script2 = R"#(
        reg = registry:new()
        ent = reg:create()
        reg:set(ent, position:new(1, 2))

        local viewed_ent = nil
        local viewed_pos = nil
        reg:view(position):each(
            function(pos)
                viewed_pos = pos
            end
        )
        assert(viewed_pos.x == 1)
        assert(viewed_pos.y == 2)
    )#";
    engine.script(lua_script2);
}

TEST(scripting, lua_engine_throws_unknown_component) {
    lua_engine engine{};
    auto throwing_lua = R"#(
        reg = registry:new()
        ent = reg:create()
        local some_table = {}
        reg:set(ent, some_table)
    )#";
    EXPECT_THROW(engine.script(throwing_lua), lua_execution_error);
}
