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

TEST(simple_scheduler, commands) {
    ecs::registry registry;
    ecs::simple_scheduler scheduler{ registry };

    pos viewed_pos{ 0, 0 };

    scheduler //
        .add_init_system([](ecs::command_queue& commands) {
            commands.create<pos>({ 1, 2 });
        })
        .add_system([&](ecs::view<pos&> view) { view.each([&](const auto& p) { viewed_pos = p; }); });

    // first init frame buffers the command
    scheduler.init();

    // second time the regular system will see the buffered command result
    scheduler.init();

    EXPECT_EQ(viewed_pos.x, 1);
    EXPECT_EQ(viewed_pos.y, 2);
}