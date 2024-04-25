#include "components.hpp"

#include <catch2/catch_all.hpp>
#include <co_ecs/co_ecs.hpp>

using namespace co_ecs;

TEST_CASE("ECS Registry", "Creation and destruction of entities") {
    registry test_registry;

    SECTION("Test empty entity creation") {
        auto entity = test_registry.create();

        REQUIRE(test_registry.alive(entity));
        test_registry.destroy(entity);
        REQUIRE_FALSE(test_registry.alive(entity));
    }

    SECTION("Test non empty entities creation") {
        std::vector<co_ecs::entity> entities;
        const int number_of_entities = GENERATE(1, 10000);

        for (int i = 0; i < number_of_entities; i++) {
            auto entity = test_registry.create<foo<0>, foo<1>, foo<2>>({}, {}, {});
            REQUIRE(test_registry.alive(entity));
            entities.emplace_back(entity);
        }

        for (auto& entity : entities) {
            test_registry.destroy(entity);
            REQUIRE_FALSE(test_registry.alive(entity));
        }
    }
}

TEST_CASE("ECS Archetypes", "Set and remove components") {
    co_ecs::registry registry;
    std::vector<co_ecs::entity> entities;

    const int number_of_entities = GENERATE(1, 10000);

    for (int i = 0; i < number_of_entities; i++) {
        auto entity = registry.create<foo<0>, foo<1>>({ 1, 2 }, { 3, 0 });
        REQUIRE(registry.alive(entity));
        entities.emplace_back(entity);
    }

    for (auto entity : entities) {
        auto foo_0 = registry.get<foo<0>>(entity);
        REQUIRE(foo_0.a == 1);
        REQUIRE(foo_0.b == 2);

        registry.set<foo<2>>(entity, 4, 5);

        auto foo_2 = registry.get<foo<2>>(entity);
        REQUIRE(foo_2.a == 4);
        REQUIRE(foo_2.b == 5);

        auto& ref_foo_0 = registry.get<foo<0>>(entity);
        ref_foo_0.a = 10;

        foo_0 = registry.get<foo<0>>(entity);
        REQUIRE(foo_0.a == 10);
        REQUIRE(foo_0.b == 2);

        registry.remove<foo<2>>(entity);

        foo_0 = registry.get<foo<0>>(entity);
        REQUIRE(foo_0.a == 10);
        REQUIRE(foo_0.b == 2);
    }

    for (auto entity : entities) {
        registry.destroy(entity);
        REQUIRE_FALSE(registry.alive(entity));
    }
}

TEST_CASE("ECS Views", "Iteration over registry") {
    registry test_registry;

    const int number_of_entities = GENERATE(2, 10000);

    for (int i = 0; i < number_of_entities / 2; i++) {
        auto entity = test_registry.create<foo<0>, foo<1>, foo<2>>({ 1, 2 }, { 3, 4 }, { 5, 6 });
        REQUIRE(test_registry.alive(entity));
    }

    for (int i = 0; i < number_of_entities / 2; i++) {
        auto entity = test_registry.create<foo<0>, foo<2>>({ 1, 2 }, { 5, 6 });
        REQUIRE(test_registry.alive(entity));
    }

    int sum_0{};
    int sum_1{};
    int sum_2{};

    SECTION("Test non-const view") {
        for (auto [foo_0, foo_1, foo_2] : test_registry.view<foo<0>&, const foo<1>&, foo<2>&>().each()) {
            static_assert(std::is_same_v<decltype(foo_0), foo<0>&>);
            static_assert(std::is_same_v<decltype(foo_1), const foo<1>&>);
            static_assert(std::is_same_v<decltype(foo_2), foo<2>&>);
            sum_0 += foo_0.a;
            sum_1 += foo_1.a;
            sum_2 += foo_2.a;
        }

        REQUIRE(sum_0 == number_of_entities / 2 * 1);
        REQUIRE(sum_1 == number_of_entities / 2 * 3);
        REQUIRE(sum_2 == number_of_entities / 2 * 5);
    }

    SECTION("Test const view") {
        const co_ecs::registry& c_reg = test_registry;
        for (auto [foo_0, foo_2] : c_reg.view<const foo<0>&, const foo<2>&>().each()) {
            static_assert(std::is_same_v<decltype(foo_0), const foo<0>&>);
            static_assert(std::is_same_v<decltype(foo_2), const foo<2>&>);
            sum_0 += foo_0.a;
            sum_2 += foo_2.a;
        }

        REQUIRE(sum_0 == number_of_entities * 1);
        REQUIRE(sum_2 == number_of_entities * 5);
    }

    SECTION("Test non-const each method") {
        test_registry.each([&](foo<0>& foo_0, const foo<1>& foo_1, foo<2>& foo_2) {
            sum_0 += foo_0.a;
            sum_1 += foo_1.a;
            sum_2 += foo_2.a;
        });

        REQUIRE(sum_0 == number_of_entities / 2 * 1);
        REQUIRE(sum_1 == number_of_entities / 2 * 3);
        REQUIRE(sum_2 == number_of_entities / 2 * 5);
    }

    SECTION("Test const each method") {
        const co_ecs::registry& c_reg = test_registry;
        c_reg.each([&](const foo<0>& foo_0, const foo<2>& foo_2) {
            sum_0 += foo_0.a;
            sum_2 += foo_2.a;
        });

        REQUIRE(sum_0 == number_of_entities * 1);
        REQUIRE(sum_2 == number_of_entities * 5);
    }

    SECTION("Single entity") {
        auto maybe_components = test_registry.single<foo<0>&, foo<2>&>();
        REQUIRE(maybe_components.has_value());
        auto [foo_0, foo_2] = *maybe_components;
        REQUIRE(foo_0.a == 1);
        REQUIRE(foo_0.b == 2);
        REQUIRE(foo_2.a == 5);
        REQUIRE(foo_2.b == 6);
    }
}

TEST_CASE("ECS Registry component not found exception", "Catch exceptions raised on invalid component queries") {
    registry test_registry;
    auto ent = test_registry.create<foo<0>>({ 2, 2 });

    REQUIRE_THROWS_AS(test_registry.get<foo<1>>(ent), component_not_found);
    REQUIRE_THROWS_AS((test_registry.template get<const foo<0>&, const foo<1>&>(ent)), component_not_found);
}

TEST_CASE("ECS Registry entity not found exception", "Catch exceptions raised on invalid entity queries") {
    registry test_registry;
    auto ent = test_registry.create<foo<0>>({ 2, 2 });
    test_registry.destroy(ent);

    REQUIRE_THROWS_AS(test_registry.get<const foo<0>&>(ent), entity_not_found);
    REQUIRE_THROWS_AS(test_registry.has<foo<0>>(ent), entity_not_found);
    REQUIRE_THROWS_AS(test_registry.set<foo<0>>(ent, 0, 0), entity_not_found);
    REQUIRE_FALSE(test_registry.destroy(ent));
}

TEST_CASE("ECS Registry insufficient chunk size",
    "Catch exceptions raised when entity components can not fit in a chunk") {
    registry test_registry;

    struct first_big_struct {
        std::array<char, 8192> data{};
    };

    struct second_big_struct {
        std::array<char, 8192> data{};
    };

    // A properly aligned structure containing first_big_strcut, second_big_struct and an entity handle takes more than
    // 16 KB which means a chunk will not be able to store a single entity.

    auto ent = test_registry.create<first_big_struct>({});

    REQUIRE_THROWS_AS((test_registry.create<first_big_struct, second_big_struct>({}, {})), insufficient_chunk_size);
    REQUIRE_THROWS_AS(test_registry.set<second_big_struct>(ent), insufficient_chunk_size);
}

TEST_CASE("ECS Registry constraints", "Non-copiable components") {
    registry reg;

    struct test_struct {
        std::unique_ptr<int> c{};
    };

    // Just make sure it compiles
    reg.create<test_struct>({});
}

TEST_CASE("ECS Registry move entities") {
    registry reg1;
    registry reg2;

    const int number_of_entities = GENERATE(2, 100000);

    std::vector<entity> entities;
    std::vector<entity> moved_entities;

    for (auto i : std::views::iota(0, number_of_entities)) {
        auto ent = reg1.create<foo<0>, foo<1>>({ 1 * i, 2 * i }, { 3 * i, 4 * i });
        entities.push_back(ent);
    }

    REQUIRE(reg1.size() == number_of_entities);
    REQUIRE(reg2.size() == 0);

    for (const auto& ent : entities) {
        REQUIRE(reg1.alive(ent));
    }

    for (const auto& ent : entities) {
        auto moved_ent = reg1.move(ent, reg2);
        moved_entities.push_back(moved_ent);
    }

    REQUIRE(reg1.size() == 0);
    REQUIRE(reg2.size() == number_of_entities);

    for (const auto& ent : entities) {
        REQUIRE_FALSE(reg1.alive(ent));
    }

    for (auto i = 0; i < moved_entities.size(); i++) {
        REQUIRE(reg2.alive(moved_entities[i]));

        REQUIRE(reg2.get<foo<0>>(moved_entities[i]).a == 1 * i);
        REQUIRE(reg2.get<foo<0>>(moved_entities[i]).b == 2 * i);
        REQUIRE(reg2.get<foo<1>>(moved_entities[i]).a == 3 * i);
        REQUIRE(reg2.get<foo<1>>(moved_entities[i]).b == 4 * i);
    }
}

TEST_CASE("ECS Registry copy entities") {
    registry reg1;
    registry reg2;

    const int number_of_entities = GENERATE(2, 100000);

    std::vector<entity> entities;
    std::vector<entity> copied_entities;

    for (auto i : std::views::iota(0, number_of_entities)) {
        auto ent = reg1.create<foo<0>, foo<1>>({ 1 * i, 2 * i }, { 3 * i, 4 * i });
        entities.push_back(ent);
    }

    REQUIRE(reg1.size() == number_of_entities);
    REQUIRE(reg2.size() == 0);

    for (const auto& ent : entities) {
        REQUIRE(reg1.alive(ent));
    }

    for (const auto& ent : entities) {
        auto moved_ent = reg1.copy(ent, reg2);
        copied_entities.push_back(moved_ent);
    }

    REQUIRE(reg1.size() == number_of_entities);
    REQUIRE(reg2.size() == number_of_entities);

    for (const auto i : std::views::iota(0, number_of_entities)) {
        REQUIRE(reg1.alive(entities[i]));
        REQUIRE(reg2.alive(copied_entities[i]));

        REQUIRE(reg1.get<foo<0>>(entities[i]).a == 1 * i);
        REQUIRE(reg1.get<foo<0>>(entities[i]).b == 2 * i);
        REQUIRE(reg1.get<foo<1>>(entities[i]).a == 3 * i);
        REQUIRE(reg1.get<foo<1>>(entities[i]).b == 4 * i);

        REQUIRE(reg2.get<foo<0>>(copied_entities[i]).a == 1 * i);
        REQUIRE(reg2.get<foo<0>>(copied_entities[i]).b == 2 * i);
        REQUIRE(reg2.get<foo<1>>(copied_entities[i]).a == 3 * i);
        REQUIRE(reg2.get<foo<1>>(copied_entities[i]).b == 4 * i);
    }
}

TEST_CASE("ECS Registry clone") {
    registry reg;

    auto e1 = reg.create<foo<0>, foo<1>>({ 1, 2 }, { 3, 4 });
    auto e2 = reg.clone(e1);

    REQUIRE(reg.get<foo<0>>(e1).a == 1);
    REQUIRE(reg.get<foo<0>>(e1).b == 2);
    REQUIRE(reg.get<foo<1>>(e1).a == 3);
    REQUIRE(reg.get<foo<1>>(e1).b == 4);

    REQUIRE(reg.get<foo<0>>(e2).a == 1);
    REQUIRE(reg.get<foo<0>>(e2).b == 2);
    REQUIRE(reg.get<foo<1>>(e2).a == 3);
    REQUIRE(reg.get<foo<1>>(e2).b == 4);
}
