#include <cobalt/ecs/commands.hpp>

#include <gtest/gtest.h>

using namespace cobalt;

struct component_a {
    int a;
};
struct component_b {
    int b;
};

TEST(commands, basic) {
    ecs::registry registry;
    ecs::command_queue commands;

    commands.create(component_a{ 2 }, component_b{ 3 });
    commands.create(component_a{ 6 }, component_b{ 5 });
    commands.create(component_a{ 6 });
    commands.create(component_a{ 10 });
    commands.create(component_b{ 12 });

    commands.execute(registry);

    auto view = registry.view<const ecs::entity&, const component_a&, const component_b&>();

    int sum_a = 0;
    int sum_b = 0;
    ecs::entity ent;
    for (auto&& [e, a, b] : view.each()) {
        ent = e;
        sum_a += a.a;
        sum_b += b.b;
    }

    EXPECT_EQ(sum_a, 8);
    EXPECT_EQ(sum_b, 8);

    commands.destroy(ent);
    commands.execute(registry);

    sum_a = 0;
    sum_b = 0;
    for (auto&& [e, a, b] : view.each()) {
        ent = e;
        sum_a += a.a;
        sum_b += b.b;
    }

    EXPECT_EQ(sum_a, 6);
    EXPECT_EQ(sum_b, 5);

    commands.remove<component_a>(ent);
    commands.execute(registry);

    sum_a = 0;
    sum_b = 0;
    for (auto&& [e, a, b] : view.each()) {
        ent = e;
        sum_a += a.a;
        sum_b += b.b;
    }

    EXPECT_EQ(sum_a, 0);
    EXPECT_EQ(sum_b, 0);

    commands.set<component_a>(ent, 1);
    commands.execute(registry);

    sum_a = 0;
    sum_b = 0;
    for (auto&& [e, a, b] : view.each()) {
        ent = e;
        sum_a += a.a;
        sum_b += b.b;
    }

    EXPECT_EQ(sum_a, 1);
    EXPECT_EQ(sum_b, 5);
}