#include <co_ecs/entity.hpp>

#include <gtest/gtest.h>

TEST(entity, entity_ordering) {
    auto h1 = co_ecs::entity{ 0, 0 };
    auto h2 = co_ecs::entity{ 0, 0 };

    EXPECT_TRUE(h1 == h2);

    auto h3 = co_ecs::entity{ 5, 6 };
    auto h4 = co_ecs::entity{ 5, 6 };

    EXPECT_TRUE(h3 == h4);

    auto h5 = co_ecs::entity{ 10, 6 };

    EXPECT_TRUE(h1 != h5);

    auto entity1_gen1 = co_ecs::entity{ 0, 1 };
    auto entity1_id1 = co_ecs::entity{ 1, 0 };

    EXPECT_TRUE(h1 < entity1_gen1);
    EXPECT_TRUE(h1 < entity1_id1);
    EXPECT_TRUE(entity1_id1 > entity1_gen1);
}

TEST(entity, entity_invalid) {
    auto h = co_ecs::entity{ 0, 0 };
    auto invalid = co_ecs::entity::invalid;

    EXPECT_TRUE(h.valid());
    EXPECT_FALSE(invalid.valid());
}

TEST(entity_pool, basic) {
    co_ecs::entity_pool pool;
    auto h1 = pool.create();
    EXPECT_TRUE(pool.alive(h1));

    auto h2 = pool.create();
    EXPECT_TRUE(pool.alive(h2));

    pool.recycle(h1);
    EXPECT_FALSE(pool.alive(h1));
    auto h3 = pool.create();
    EXPECT_TRUE(pool.alive(h3));
    EXPECT_EQ(h3.id(), h1.id());
}