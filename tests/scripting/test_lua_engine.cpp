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

        reg:register_component(Rectangle)
        reg:register_component(Circle)

        r = Rectangle:new(10,20)
        c = Circle:new(10)

        print('position id', position:id())
        print('Circle id', c:id())
        print('Rectangle id', r:id())
        
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
    )#");
}