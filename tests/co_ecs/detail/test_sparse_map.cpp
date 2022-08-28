#include <co_ecs/detail/sparse_map.hpp>

#include <numeric>

#include <gtest/gtest.h>


TEST(sparse_map, insert_remove_clear) {
    auto map = co_ecs::detail::sparse_map<unsigned int, std::tuple<int, bool>>{};

    EXPECT_TRUE(map.empty());
    EXPECT_FALSE(map.contains(5));
    EXPECT_FALSE(map.contains(6));
    EXPECT_FALSE(map.contains(7));

    map.emplace(5, 25, false);
    map.emplace(6, 36, true);

    EXPECT_TRUE(map.contains(5));
    EXPECT_TRUE(map.contains(6));
    EXPECT_FALSE(map.contains(7));
    EXPECT_FALSE(map.empty());

    map.erase(5);

    EXPECT_FALSE(map.contains(5));
    EXPECT_TRUE(map.contains(6));
    EXPECT_FALSE(map.contains(7));
    EXPECT_FALSE(map.empty());

    map.clear();

    EXPECT_TRUE(map.empty());
    EXPECT_FALSE(map.contains(5));
    EXPECT_FALSE(map.contains(6));
    EXPECT_FALSE(map.contains(7));
}

TEST(sparse_map, iteration) {
    co_ecs::detail::sparse_map<unsigned int, unsigned int> map = { { 1, 1 }, { 2, 4 }, { 3, 9 }, { 4, 16 }, { 5, 25 } };
    map.emplace(6, 36);
    map.erase(6);
    EXPECT_EQ(
        std::accumulate(map.cbegin(), map.cend(), 0, [](auto res, const auto& kv) { return res + kv.first; }), 15);
    EXPECT_EQ(
        std::accumulate(map.cbegin(), map.cend(), 0, [](auto res, const auto& kv) { return res + kv.second; }), 55);
}

TEST(sparse_map, insert_non_copiable) {
    co_ecs::detail::sparse_map<unsigned int, std::unique_ptr<int>> map{};

    map.emplace(5, std::make_unique<int>(5));
    EXPECT_TRUE(map.contains(5));
}