#include <cobalt/ecs/system.hpp>

#include <gtest/gtest.h>

using namespace cobalt;

struct my_resource {
    std::string name;
};

struct counter {
    int x, y;
};

struct pos {
    int x, y;
};

TEST(system, system_state) {
    my_resource resource{ "resource" };
    counter count{ 0, 0 };

    ecs::registry registry;
    registry.register_resource(resource);
    registry.register_resource(count);

    registry.create<pos>({ 1, 2 });
    registry.create<pos>({ 2, 5 });

    ecs::system system(registry, [](ecs::view<pos&> view, const my_resource& resource, counter& count) {
        view.iter([&](auto& chunk) {
            for (auto [pos] : chunk) {
                count.x += pos.x;
            }
        });
    });

    system.run();

    EXPECT_EQ(count.x, 3);
}