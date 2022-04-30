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

    auto& pos = registry.get<position>(entity);
    EXPECT_EQ(pos.x, 1);
    EXPECT_EQ(pos.y, 2);

    registry.set<position>(entity, 5, 10);

    pos = registry.get<position>(entity);
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

    auto pos = registry.get<position>(entity);
    EXPECT_EQ(pos.x, 1);
    EXPECT_EQ(pos.y, 2);

    registry.set<some_component>(entity, 5);

    auto c = registry.get<some_component>(entity);
    EXPECT_EQ(c.m, 5);

    auto& refpos = registry.get<position>(entity);
    refpos.x = 10;

    pos = registry.get<position>(entity);
    EXPECT_EQ(pos.x, 10);
    EXPECT_EQ(pos.y, 2);

    registry.remove<some_component>(entity);

    pos = registry.get<position>(entity);
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
        auto pos = registry.get<position>(entity);
        EXPECT_EQ(pos.x, 1);
        EXPECT_EQ(pos.y, 2);
    }

    for (const auto& entity : entities) {
        registry.set<some_component>(entity, 5);
        auto c = registry.get<some_component>(entity);
    }

    for (const auto& entity : entities) {
        auto c = registry.get<some_component>(entity);
        EXPECT_EQ(c.m, 5);
    }

    for (const auto& entity : entities) {
        registry.destroy(entity);
        EXPECT_FALSE(registry.alive(entity));
    }
}

TEST(registry, view) {
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

    for (auto [vel, pos, rot] : registry.each<velocity&, position&, rotation&>()) {
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

TEST(registry, ref_view) {
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

    for (auto [vel, pos, rot] : registry.each<velocity&, const position&, rotation&>()) {
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

    for (auto [vel, rot] : registry.each<const velocity&, const rotation&>()) {
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

TEST(registry, view_part) {
    ecs::registry registry;
    auto entity1 = registry.create<position>({ 1, 1 });
    auto entity2 = registry.create<position, velocity>({ 2, 2 }, { 22, 22 });

    auto range = registry.each<const position&, const velocity&>();
    auto iter = range.begin();
    EXPECT_EQ(std::get<0>(*iter).x, 2);
    EXPECT_EQ(std::get<0>(*iter).y, 2);
    EXPECT_EQ(std::get<1>(*iter).x, 22);
    EXPECT_EQ(std::get<1>(*iter).y, 22);
}

TEST(registry, view_class) {
    ecs::registry registry;
    auto entity1 = registry.create<position>({ 1, 1 });
    auto entity2 = registry.create<position, velocity>({ 2, 2 }, { 22, 22 });

    auto view = registry.view<const position&, const velocity&>();

    int sum = 0;
    view.each([&sum](const auto& pos, const auto& vel) { sum += pos.x + vel.x; });
    EXPECT_EQ(sum, 24);
}

TEST(registry, view_entity) {
    ecs::registry registry;
    registry.create<position>({ 1, 1 });
    registry.create<position, velocity>({ 2, 2 }, { 22, 22 });

    auto view = registry.view<const ecs::entity&>();
    int sum = 0;
    view.each([&sum](auto& ent) { sum += ent.id(); });
    EXPECT_EQ(sum, 1);
}

TEST(registry, view_get) {
    ecs::registry registry;
    auto ent = registry.create<position, velocity>({ 2, 2 }, { 22, 22 });

    auto view = registry.view<const position&, velocity&>();
    auto components = view.get(ent);
    EXPECT_EQ(std::get<0>(components).x, 2);
    EXPECT_EQ(std::get<0>(components).y, 2);
    EXPECT_EQ(std::get<1>(components).x, 22);
    EXPECT_EQ(std::get<1>(components).y, 22);
}

TEST(registry, resources) {
    struct my_resource {
        std::string name;
    } res, other_res;

    ecs::registry registry;

    registry.register_resource(res);
    auto& pres = registry.get_resource<my_resource>();
    EXPECT_EQ(&pres, &res);
    EXPECT_THROW(registry.register_resource(other_res), std::invalid_argument);
    registry.unregister_resource<my_resource>();
    EXPECT_THROW(registry.get_resource<my_resource>(), std::out_of_range);
}

TEST(registry, exceptions) {
    ecs::registry registry;
    auto ent = registry.create<position>({ 2, 2 });
    EXPECT_THROW(static_cast<void>(registry.get<velocity>(ent)), ecs::component_not_found);
    EXPECT_THROW(
        static_cast<void>(registry.template get<const velocity&, const position&>(ent)), ecs::component_not_found);
    registry.destroy(ent);
    EXPECT_THROW(static_cast<void>(registry.get<velocity>(ent)), ecs::entity_not_found);
    EXPECT_THROW(static_cast<void>(registry.has<velocity>(ent)), ecs::entity_not_found);
    EXPECT_THROW(registry.set<velocity>(ent), ecs::entity_not_found);
    EXPECT_THROW(registry.destroy(ent), ecs::entity_not_found);
}