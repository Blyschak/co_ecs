#include <cobalt/asl/hash_set.hpp>
#include <numeric>
#include <string>

#include <unordered_set>

#include <gtest/gtest.h>

using namespace cobalt;

TEST(has_set, empty) {
    asl::hash_set<std::string> ht;
}

TEST(hash_set, basic_insert) {
    asl::hash_set<std::string> ht;
    ht.insert("hello");
    EXPECT_TRUE(ht.contains("hello"));
    ht.insert("hey");
    EXPECT_TRUE(ht.contains("hey"));
    EXPECT_FALSE(ht.contains("world"));
}

TEST(hash_set, basic_emplace) {
    asl::hash_set<std::string> ht;
    ht.emplace("hello");
    EXPECT_TRUE(ht.contains("hello"));
    ht.emplace(3, 'c');
    EXPECT_TRUE(ht.contains("ccc"));
    EXPECT_FALSE(ht.contains("world"));
}

TEST(hash_set, basic_with_alloc) {
    asl::hash_set<std::string> ht{ std::allocator<std::string>() };
    ht.insert("world");
    EXPECT_TRUE(ht.contains("world"));
    ht.insert("hey");
    EXPECT_TRUE(ht.contains("hey"));
}

TEST(hash_set, construct_from_initializer_list) {
    asl::hash_set<std::string> ht = { "hello", "world" };
    EXPECT_TRUE(ht.contains("hello"));
    EXPECT_TRUE(ht.contains("world"));
}

TEST(hash_set, construct_from_iterators) {
    std::vector<std::string> vec = { "hello", "world" };
    asl::hash_set<std::string> ht{ vec.begin(), vec.end() };
    EXPECT_TRUE(ht.contains("hello"));
    EXPECT_TRUE(ht.contains("world"));
}

TEST(hash_set, copy_ctor) {
    asl::hash_set<std::string> ht = { "hello", "world" };
    decltype(ht) copy_ht(ht);
    EXPECT_TRUE(ht.contains("hello"));
    EXPECT_TRUE(ht.contains("world"));
    EXPECT_TRUE(copy_ht.contains("hello"));
    EXPECT_TRUE(copy_ht.contains("world"));
}

TEST(hash_set, copy_assign) {
    asl::hash_set<std::string> ht = { "hello", "world" };
    auto copy_ht = ht;
    EXPECT_TRUE(ht.contains("hello"));
    EXPECT_TRUE(ht.contains("world"));
    EXPECT_TRUE(copy_ht.contains("hello"));
    EXPECT_TRUE(copy_ht.contains("world"));
}

TEST(hash_set, move_ctor) {
    asl::hash_set<std::string> ht = { "hello", "world" };
    decltype(ht) moved_ht(std::move(ht));
    EXPECT_TRUE(moved_ht.contains("hello"));
    EXPECT_TRUE(moved_ht.contains("world"));
}

TEST(hash_set, move_assign) {
    asl::hash_set<std::string> ht = { "hello", "world" };
    auto moved_ht = std::move(ht);
    EXPECT_TRUE(moved_ht.contains("hello"));
    EXPECT_TRUE(moved_ht.contains("world"));
}

TEST(hash_set, value_default_ctor) {
    asl::hash_set<std::string> ht;
    ht.emplace();
    EXPECT_EQ(ht.size(), 1);
    EXPECT_TRUE(ht.contains(""));
}

TEST(hash_set, find) {
    asl::hash_set<int> ht;
    ht.emplace(1);
    ht.emplace(2);
    ht.emplace(4);

    EXPECT_NE(ht.find(1), ht.end());
    EXPECT_EQ(*ht.find(1), 1);
    EXPECT_NE(ht.find(2), ht.end());
    EXPECT_EQ(*ht.find(2), 2);
    EXPECT_NE(ht.find(4), ht.end());
    EXPECT_EQ(*ht.find(4), 4);

    EXPECT_EQ(ht.find(3), ht.end());
}

TEST(hash_set, iteration_empty) {
    asl::hash_set<int> ht{};
    auto res = std::accumulate(ht.begin(), ht.end(), 0, [](const auto res, const auto& entry) { return entry + res; });
    EXPECT_EQ(res, 0);
}

TEST(hash_set, iteration) {
    asl::hash_set<int> ht{ 1, 2, 3, 4, 5 };
    auto res = std::accumulate(ht.begin(), ht.end(), 0, [](const auto res, const auto& entry) { return entry + res; });
    EXPECT_EQ(res, 15);
}

TEST(hash_set, insert_and_erase) {
    asl::hash_set<int> ht{ 1, 2, 3, 4, 5 };

    auto res = std::accumulate(ht.begin(), ht.end(), 0, [](const auto res, const auto& entry) { return entry + res; });
    EXPECT_EQ(res, 15);

    for (int i = 6; i < 100; i++) {
        ht.emplace(i);
    }

    res = std::accumulate(ht.begin(), ht.end(), 0, [](const auto res, const auto& entry) { return entry + res; });
    EXPECT_EQ(res, 4950);

    EXPECT_NE(ht.find(4), ht.end());
    EXPECT_EQ(*ht.find(4), 4);

    ht.erase(4);
    EXPECT_EQ(ht.find(4), ht.end());

    EXPECT_NE(ht.find(5), ht.end());
    EXPECT_EQ(*ht.find(5), 5);

    ht.erase(ht.find(5));
    EXPECT_EQ(ht.find(5), ht.end());

    EXPECT_NE(ht.find(90), ht.end());

    ht.erase(ht.find(90));
    EXPECT_EQ(ht.find(90), ht.end());

    ht.erase(ht.find(1));
    EXPECT_EQ(ht.find(1), ht.end());

    for (int i = 0; i < 100; i++) {
        ht.erase(i);
    }
    EXPECT_TRUE(ht.empty());
    EXPECT_EQ(ht.size(), 0);

    res = std::accumulate(ht.begin(), ht.end(), 0, [](const auto res, const auto& entry) { return entry + res; });
    EXPECT_EQ(res, 0);
}