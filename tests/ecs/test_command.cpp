#include <cobalt/ecs/commands.hpp>

#include <gtest/gtest.h>

using namespace cobalt;

struct sa {
    int a;
};
struct sb {
    int b;
};

TEST(commands, basic) {
    ecs::registry registry;
    ecs::command_queue commands;

    commands.create(sa{ 2 }, sb{ 3 });
    commands.create(sa{ 6 }, sb{ 5 });
    commands.create(sa{ 6 });
    commands.create(sa{ 10 });
    commands.create(sb{ 12 });

    commands.execute(registry);

    auto view = registry.view<const ecs::entity&, const sa&, const sb&>();

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

    commands.remove<sa>(ent);
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

    commands.set<sa>(ent, 1);
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