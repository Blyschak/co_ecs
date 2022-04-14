#include <cobalt/ecs/registry.hpp>

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

struct some_component {
    int m{};
};

TEST(registry, basic) {
    ecs::registry registry;
    auto entity = registry.create<position, velocity>({ 1, 2 }, { 3, 0 });
    EXPECT_TRUE(registry.alive(entity));

    auto& pos = registry.get<position>(entity.id());
    EXPECT_EQ(pos.x, 1);
    EXPECT_EQ(pos.y, 2);

    registry.set<position>(entity.id(), 5, 10);

    pos = registry.get<position>(entity.id());
    EXPECT_EQ(pos.x, 5);
    EXPECT_EQ(pos.y, 10);

    registry.destroy(entity);
    EXPECT_FALSE(registry.alive(entity));
}

TEST(registry, emplace_10k) {
    ecs::registry registry;
    std::vector<ecs::entity> entities;
    for (int i = 0; i < 10000; i++) {
        auto entity = registry.create<position, rotation, velocity>({ 0, 0 }, { 0, 0 }, { 0, 0 });
        EXPECT_TRUE(registry.alive(entity));
        entities.emplace_back(entity);
    }

    for (auto& entity : entities) {
        registry.destroy(entity);
        EXPECT_FALSE(registry.alive(entity));
    }
}

TEST(registry, emplace_and_set_remove) {
    ecs::registry registry;
    auto entity = registry.create<position, velocity>({ 1, 2 }, { 3, 0 });
    EXPECT_TRUE(registry.alive(entity));

    auto pos = registry.get<position>(entity.id());
    EXPECT_EQ(pos.x, 1);
    EXPECT_EQ(pos.y, 2);

    registry.set<some_component>(entity.id(), 5);

    auto c = registry.get<some_component>(entity.id());
    EXPECT_EQ(c.m, 5);

    auto& refpos = registry.get<position>(entity.id());
    refpos.x = 10;

    pos = registry.get<position>(entity.id());
    EXPECT_EQ(pos.x, 10);
    EXPECT_EQ(pos.y, 2);

    registry.remove<some_component>(entity.id());

    pos = registry.get<position>(entity.id());
    EXPECT_EQ(pos.x, 10);
    EXPECT_EQ(pos.y, 2);
}

TEST(registry, emplace_and_set_remove_10k) {
    ecs::registry registry;
    std::vector<ecs::entity> entities;

    for (int i = 0; i < 10000; i++) {
        auto entity = registry.create<position, velocity>({ 1, 2 }, { 3, 0 });
        EXPECT_TRUE(registry.alive(entity));
        entities.emplace_back(entity);
    }

    for (const auto& entity : entities) {
        auto pos = registry.get<position>(entity.id());
        EXPECT_EQ(pos.x, 1);
        EXPECT_EQ(pos.y, 2);
    }

    for (const auto& entity : entities) {
        registry.set<some_component>(entity.id(), 5);
        auto c = registry.get<some_component>(entity.id());
    }

    for (const auto& entity : entities) {
        auto c = registry.get<some_component>(entity.id());
        EXPECT_EQ(c.m, 5);
    }

    for (const auto& entity : entities) {
        registry.destroy(entity);
        EXPECT_FALSE(registry.alive(entity));
    }
}