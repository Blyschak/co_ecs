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
        reg:set(ent, position:new(1, 2))

        local viewed_ent = nil
        local viewed_pos = nil
        reg:view(position):each(
            function(ent)
                viewed_ent = ent
                viewed_pos = reg:get(ent, position)
            end
        )
        assert(viewed_ent:id() == ent:id())
        assert(viewed_pos.x == 1)
        assert(viewed_pos.y == 2)
    )#");

    lua_engine.script(R"#(
        ent = reg:create()
        s = script_component:new('Hello')
        reg:set(ent, s)

        ent2 = reg:create()
        reg:set(ent2, script_component:new({other_ent=ent, some_more_data=15}))

        reg:view(script_component):each(
            function(ent)
                data = reg:get(ent, script_component)
                assert(data)
            end
        )
    )#");
}