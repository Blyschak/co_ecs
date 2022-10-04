#include "components.hpp"

#include <co_ecs/co_ecs.hpp>
#include <catch2/catch_all.hpp>

#include <iostream>

using namespace co_ecs;

TEST_CASE("ECS Registry", "Creation of entities") {
    registry test_registry;

    BENCHMARK("Entity creation") {
        test_registry.create<foo<0>, foo<1>, foo<2>>({0, 1}, {2, 3}, {4, 5});
    };

    auto entity = test_registry.create<foo<0>, foo<1>>({0, 1}, {2, 3});

    BENCHMARK("Get component attached to an entity") {
        return test_registry.get<foo<0>>(entity);
    };

    BENCHMARK("Attach component to an entity") {
        return test_registry.set<foo<2>>(entity, 100, 200);
    };

    BENCHMARK("Change entity's archetype by removing and assigning a component") {
        test_registry.remove<foo<2>>(entity);
        return test_registry.set<foo<2>>(entity, 100, 200);
    };
}

TEST_CASE("ECS Views", "Iteration over registry") {
    registry test_registry;

    auto number_of_entities_to_iterate = GENERATE(100, 1000000);

    for (auto i = 0; i < number_of_entities_to_iterate; i++) {
        test_registry.create<foo<3>, foo<4>>({0, 1}, {2, 3});
    }

    INFO("Iterate over " << number_of_entities_to_iterate << " entities");

    BENCHMARK("Iterate components with view") {
        auto view = co_ecs::view<const foo<3>&, const foo<4>&>(test_registry);
        int sum_of_foo_a = 0;
        view.each([&] (const auto& foo_3, const auto& foo_4) {
            sum_of_foo_a += foo_3.a + foo_4.a;
        });
        return sum_of_foo_a;
    };
}

TEST_CASE("ECS Registry component not found exception", "Catch exceptions raised on invalid component queries") {
    registry test_registry;
    auto ent = test_registry.create<foo<0>>({2, 2});

    REQUIRE_THROWS_AS(test_registry.get<foo<1>>(ent), component_not_found);
    REQUIRE_THROWS_AS((test_registry.template get<const foo<0>&, const foo<1>&>(ent)), component_not_found);
}

TEST_CASE("ECS Registry entity not found exception", "Catch exceptions raised on invalid entity queries") {
    registry test_registry;
    auto ent = test_registry.create<foo<0>>({2, 2});
    test_registry.destroy(ent);

    REQUIRE_THROWS_AS(test_registry.get<const foo<0>&>(ent), entity_not_found);
    REQUIRE_THROWS_AS(test_registry.has<foo<0>>(ent), entity_not_found);
    REQUIRE_THROWS_AS(test_registry.set<foo<0>>(ent, 0, 0), entity_not_found);
    REQUIRE_THROWS_AS(test_registry.destroy(ent), entity_not_found);
}