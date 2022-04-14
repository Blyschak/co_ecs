#include <cobalt/asl/sparse_set.hpp>

#include <numeric>

#include <gtest/gtest.h>


TEST(sparse_set, insert_remove_clear) {
    cobalt::asl::sparse_set<unsigned int> set{};

    EXPECT_TRUE(set.empty());
    EXPECT_FALSE(set.contains(5));
    EXPECT_FALSE(set.contains(6));
    EXPECT_FALSE(set.contains(7));

    set.emplace(5);
    set.insert(6);

    EXPECT_TRUE(set.contains(5));
    EXPECT_TRUE(set.contains(6));
    EXPECT_FALSE(set.contains(7));
    EXPECT_FALSE(set.empty());

    set.erase(5);

    EXPECT_FALSE(set.contains(5));
    EXPECT_TRUE(set.contains(6));
    EXPECT_FALSE(set.contains(7));
    EXPECT_FALSE(set.empty());

    set.clear();

    EXPECT_TRUE(set.empty());
    EXPECT_FALSE(set.contains(5));
    EXPECT_FALSE(set.contains(6));
    EXPECT_FALSE(set.contains(7));
}

TEST(sparse_set, initializer_list_construction) {
    cobalt::asl::sparse_set<unsigned> set = { 5, 6 };

    EXPECT_EQ(set.size(), 2);
    EXPECT_TRUE(set.contains(5));
    EXPECT_TRUE(set.contains(6));
    EXPECT_FALSE(set.contains(7));
}

TEST(sparse_set, iteration) {
    cobalt::asl::sparse_set<unsigned> set = { 1, 2, 3, 4, 5 };
    set.insert(6);
    EXPECT_TRUE(set.contains(6));
    set.erase(6);
    EXPECT_EQ(std::accumulate(set.cbegin(), set.cend(), 0), 15);
}