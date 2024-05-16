#include <catch2/catch_all.hpp>
#include <co_ecs/co_ecs.hpp>

#include "components.hpp"

using namespace co_ecs;

TEST_CASE("Command Buffer") {
    co_ecs::registry registry;
    command_writer commands{ registry };

    SECTION("Test entity creation") {
        auto recorded_entity = commands.create<foo<0>, foo<1>>({ 1, 2 }, { 3, 4 }).set<foo<2>>(5, 6);

        command_buffer::flush(registry);

        auto entity = registry.get_entity(recorded_entity);

        REQUIRE(entity.get<foo<0>>() == foo<0>{ 1, 2 });
        REQUIRE(entity.get<foo<1>>() == foo<1>{ 3, 4 });
        REQUIRE(entity.get<foo<2>>() == foo<2>{ 5, 6 });
    }

    SECTION("Test clone") {
        auto recorded_entity = commands.create<foo<0>, foo<1>>({ 1, 2 }, { 3, 4 }).set<foo<2>>(5, 6);

        auto cloned_recorded = recorded_entity.clone();

        command_buffer::flush(registry);

        auto entity = registry.get_entity(recorded_entity);

        REQUIRE(entity.get<foo<0>>() == foo<0>{ 1, 2 });
        REQUIRE(entity.get<foo<1>>() == foo<1>{ 3, 4 });
        REQUIRE(entity.get<foo<2>>() == foo<2>{ 5, 6 });

        auto cloned_entity = registry.get_entity(cloned_recorded);

        REQUIRE(cloned_entity.get<foo<0>>() == foo<0>{ 1, 2 });
        REQUIRE(cloned_entity.get<foo<1>>() == foo<1>{ 3, 4 });
        REQUIRE(cloned_entity.get<foo<2>>() == foo<2>{ 5, 6 });
    }

    SECTION("Test clone and set") {
        auto recorded_entity = commands.create<foo<0>, foo<1>>({ 1, 2 }, { 3, 4 });

        auto cloned_recorded = recorded_entity.clone().set<foo<2>>(5, 6);

        command_buffer::flush(registry);

        auto entity = registry.get_entity(recorded_entity);

        REQUIRE(entity.get<foo<0>>() == foo<0>{ 1, 2 });
        REQUIRE(entity.get<foo<1>>() == foo<1>{ 3, 4 });
        REQUIRE_FALSE(entity.has<foo<2>>());

        auto cloned_entity = registry.get_entity(cloned_recorded);

        REQUIRE(cloned_entity.get<foo<0>>() == foo<0>{ 1, 2 });
        REQUIRE(cloned_entity.get<foo<1>>() == foo<1>{ 3, 4 });
        REQUIRE(cloned_entity.get<foo<2>>() == foo<2>{ 5, 6 });
    }

    SECTION("Test create and delete") {
        auto recorded_entity = commands.create<foo<0>, foo<1>>({ 1, 2 }, { 3, 4 }).set<foo<2>>(5, 6);

        recorded_entity.destroy();

        command_buffer::flush(registry);

        REQUIRE_FALSE(registry.alive(recorded_entity));
    }
}