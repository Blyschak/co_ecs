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

struct event {
    int data;
};

TEST(system, system_state) {
    my_resource resource{ "resource" };
    counter count{ 0, 0 };

    ecs::registry registry;
    registry.set_resource<my_resource>(resource);
    registry.set_resource<counter>(count);

    registry.create<pos>({ 1, 2 });
    registry.create<pos>({ 2, 5 });

    ecs::system_executor system(registry, [](ecs::view<pos&> view, const my_resource& resource, counter& count) {
        view.each([&](const auto& p) { count.x += p.x; });
    });

    system.run();

    EXPECT_EQ(registry.get_resource<counter>().x, 3);
}

TEST(system, system_events) {
    int sum{};
    ecs::registry registry;

    registry.create<pos>({ 1, 2 });
    registry.create<pos>({ 2, 5 });

    ecs::system_executor(registry, [](ecs::view<pos&> view, ecs::event_publisher<event>& publisher) {
        publisher.publish(1);
    }).run();

    ecs::system_executor(registry, [](ecs::view<pos&> view, ecs::event_publisher<event>& publisher) {
        publisher.publish(5);
    }).run();

    ecs::system_executor(registry, [&sum](ecs::view<pos&> view, const ecs::event_reader<event>& reader) {
        for (auto event : reader) {
            sum += event.data;
        }
    }).run();

    EXPECT_EQ(sum, 6);
}