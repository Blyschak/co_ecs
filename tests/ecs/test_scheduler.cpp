#include <cobalt/ecs/registry.hpp>
#include <cobalt/ecs/scheduler.hpp>

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

TEST(simple_scheduler, basic) {
    my_resource resource{ "resource" };
    counter count{ 0, 0 };

    ecs::registry registry;
    registry.register_resource(resource);
    registry.register_resource(count);

    registry.create<pos>({ 1, 2 });
    registry.create<pos>({ 2, 5 });

    ecs::simple_scheduler scheduler{ registry };

    bool run = false;

    scheduler.add_init_system([&]() { run = true; })
        .add_system([](ecs::view<pos&> view, const my_resource& resource, counter& count) {
            view.each([&](const auto& p) { count.x += p.x; });
        });

    scheduler.init();

    EXPECT_EQ(count.x, 3);
    EXPECT_TRUE(run);
}