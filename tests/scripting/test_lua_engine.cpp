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
    lua_engine.expose_component<position, position(), position(int, int)>(
        "position", "x", &position::x, "y", &position::y);

    auto lua_script = R"#(
        pos = position:new(1, 2)
        assert(pos.x == 1)
        assert(pos.y == 2)
    )#";
    lua_engine.script(lua_script);

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
    lua_engine.script(lua_script2);
}

TEST(scripting, lua_engine_throws_unknown_component) {
    scripting::lua_engine lua_engine{};
    auto throwing_lua = R"#(
        reg = registry:new()
        ent = reg:create()
        local some_table = {}
        reg:set(ent, some_table)
    )#";
    EXPECT_THROW(lua_engine.script(throwing_lua), scripting::lua_execution_error);
}

TEST(scripting, lua_external_components) {
    scripting::lua_engine lua_engine{};
    lua_engine.expose_component<position, position(), position(int, int)>(
        "position", "x", &position::x, "y", &position::y);

    lua_engine.script(R"#(
        Rectangle = {}
        MetaRectangle = {}
        MetaRectangle.__index = Rectangle
        function Rectangle:new(length, breadth)
            local instance = setmetatable({}, MetaRectangle)
            instance.length = length
            instance.breadth = breadth
            return self
        end
        Circle = {}
        MetaCircle = {}
        MetaCircle.__index = Circle
        function Circle:new(radius)
            local instance = setmetatable({}, MetaCircle)
            instance.radius = radius
            return self
        end
        
        reg = registry:new()

        register_component(Rectangle)
        register_component(Circle)
        
        r = Rectangle:new(10,20)
        c = Circle:new(10)

        ent = reg:create(position:new(5, 10), r, c)
        cc = reg:get(ent, Circle)
        assert(cc.radius == c.radius)
        rr = reg:get(ent, Rectangle)
        assert(rr.length == r.length)
        assert(rr.breadth == r.breadth)
        pp = reg:get(ent, position)
        assert(pp.x == 5)
        assert(pp.y == 10)
        assert(reg:has(ent, position))

        reg:view(position, Circle, Rectangle):each(
            function(pos, circle, rectangle)
            end
        )
    )#");
}