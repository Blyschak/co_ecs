#include <cobalt/ecs/registry.hpp>
#include <cobalt/ecs/view.hpp>

#include <gtest/gtest.h>

using namespace cobalt;

struct position {
    int x{};
    int y{};
};

struct rotation {
    int x{};
    int y{};
};

struct velocity {
    int x{};
    int y{};
};

TEST(view, view) {
    ecs::registry registry;
    std::vector<ecs::entity> entities;

    for (int i = 0; i < 10000; i++) {
        auto entity = registry.create<position, velocity, rotation>({ 1, 2 }, { 3, 0 }, { 5, 6 });
        EXPECT_TRUE(registry.alive(entity));
        entities.emplace_back(entity);
    }

    int sum_vel{};
    int sum_pos{};
    int sum_rot{};

    auto set = ecs::component_set::create<velocity, position, rotation>();

    for (auto [vel, pos, rot] : registry.view<velocity, position, rotation>().each()) {
        static_assert(std::is_same_v<decltype(vel), velocity&>);
        static_assert(std::is_same_v<decltype(pos), position&>);
        static_assert(std::is_same_v<decltype(rot), rotation&>);
        sum_vel += vel.x;
        sum_pos += pos.x;
        sum_rot += rot.x;
    }

    EXPECT_EQ(sum_vel, 30000);
    EXPECT_EQ(sum_pos, 10000);
    EXPECT_EQ(sum_rot, 50000);

    for (const auto& entity : entities) {
        registry.destroy(entity);
        EXPECT_FALSE(registry.alive(entity));
    }
}

TEST(view, ref_view) {
    ecs::registry registry;
    std::vector<ecs::entity> entities;

    for (int i = 0; i < 10000; i++) {
        ecs::entity entity = ecs::entity::invalid();
        if (i % 2 == 0) {
            entity = registry.create<position, velocity, rotation>({ 1, 2 }, { 3, 0 }, { 5, 6 });
        } else {
            entity = registry.create<velocity, rotation>({ 3, 0 }, { 5, 6 });
        }
        EXPECT_TRUE(registry.alive(entity));
        entities.emplace_back(entity);
    }

    int sum_vel{};
    int sum_pos{};
    int sum_rot{};

    for (auto [vel, pos, rot] : registry.view<velocity&, const position&, rotation&>().each()) {
        static_assert(std::is_same_v<decltype(vel), velocity&>);
        static_assert(std::is_same_v<decltype(pos), const position&>);
        static_assert(std::is_same_v<decltype(rot), rotation&>);
        sum_vel += vel.x;
        sum_pos += pos.x;
        sum_rot += rot.x;
    }

    EXPECT_EQ(sum_vel, 15000);
    EXPECT_EQ(sum_pos, 5000);
    EXPECT_EQ(sum_rot, 25000);

    sum_vel = 0;
    sum_rot = 0;

    for (auto [vel, rot] : registry.view<const velocity&, const rotation&>().each()) {
        static_assert(std::is_same_v<decltype(vel), const velocity&>);
        static_assert(std::is_same_v<decltype(rot), const rotation&>);
        sum_vel += vel.x;
        sum_rot += rot.x;
    }

    EXPECT_EQ(sum_vel, 30000);
    EXPECT_EQ(sum_rot, 50000);

    for (const auto& entity : entities) {
        registry.destroy(entity);
        EXPECT_FALSE(registry.alive(entity));
    }
}