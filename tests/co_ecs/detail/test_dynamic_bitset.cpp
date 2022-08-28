#include <co_ecs/detail/dynamic_bitset.hpp>

#include <gtest/gtest.h>

TEST(dynamic_bitset, basic) {
    co_ecs::detail::dynamic_bitset bitset;
    EXPECT_FALSE(bitset.test(5));
    EXPECT_FALSE(bitset.test(3));
    EXPECT_FALSE(bitset.test(100));
    EXPECT_FALSE(bitset.test(50));

    bitset.set(5).set(3).set(100);
    EXPECT_TRUE(bitset.test(5));
    EXPECT_TRUE(bitset.test(3));
    EXPECT_TRUE(bitset.test(100));
    EXPECT_FALSE(bitset.test(50));

    bitset.set(5, false);
    EXPECT_FALSE(bitset.test(5));
    EXPECT_TRUE(bitset.test(3));
    EXPECT_TRUE(bitset.test(100));
    EXPECT_FALSE(bitset.test(50));
}

TEST(dynamic_bitset, hash) {
    co_ecs::detail::dynamic_bitset bitset;
    bitset.set(5).set(3).set(100);

    std::hash<co_ecs::detail::dynamic_bitset<>> hasher{};
    hasher(bitset);
}