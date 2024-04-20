#include <catch2/catch_all.hpp>
#include <co_ecs/co_ecs.hpp>

using namespace co_ecs;

TEST_CASE("Entity ordering", "Test ordering of entities") {
    auto e1 = entity{ 0, 0 };
    auto e2 = entity{ 0, 0 };

    REQUIRE(e1 == e2);

    auto e3 = entity{ 5, 6 };
    auto e4 = entity{ 5, 6 };

    REQUIRE(e3 == e4);

    auto e5 = entity{ 10, 6 };

    REQUIRE(e1 != e5);

    auto entity1_gen1 = entity{ 0, 1 };
    auto entity1_id1 = entity{ 1, 0 };

    REQUIRE(e1 < entity1_gen1);
    REQUIRE(e1 < entity1_id1);
    REQUIRE(entity1_id1 > entity1_gen1);
}

TEST_CASE("Entity validness", "Test validness of entities") {
    auto e = entity{ 0, 0 };
    auto invalid = entity::invalid();

    REQUIRE(e.valid());
    REQUIRE_FALSE(invalid.valid());
}

TEST_CASE("Entity pool", "Test entity pool functionality - allocation and recycling") {
    entity_pool pool;
    auto e1 = pool.create();
    REQUIRE(pool.alive(e1));

    auto e2 = pool.create();
    REQUIRE(pool.alive(e2));

    pool.recycle(e1);
    REQUIRE_FALSE(pool.alive(e1));
    auto e3 = pool.create();
    REQUIRE(pool.alive(e3));
    REQUIRE(e3.id() == e1.id());

    auto e4 = pool.reserve();
    auto e5 = pool.reserve();
    auto e6 = pool.reserve();

    pool.flush();
    REQUIRE(pool.alive(e4));
    REQUIRE(pool.alive(e5));
    REQUIRE(pool.alive(e6));
}