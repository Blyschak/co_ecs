#include <cobalt/asl/hash_map.hpp>
#include <numeric>
#include <string>

#include <gtest/gtest.h>

using namespace cobalt;

TEST(has_map, empty) {
    asl::hash_map<std::string, std::string> ht;
}

TEST(hash_map, basic_insert) {
    asl::hash_map<std::string, std::string> ht;
    ht.insert({ "hello", "world" });
    EXPECT_EQ(ht["hello"], "world");
    ht.insert({ "hey", "there" });
    EXPECT_EQ(ht["hey"], "there");
}

TEST(hash_map, basic_emplace) {
    asl::hash_map<std::string, std::string> ht;
    ht.emplace("hello", "world");
    EXPECT_EQ(ht["hello"], "world");
    ht.emplace(std::piecewise_construct, std::forward_as_tuple("hey"), std::forward_as_tuple(5, 'c'));
    EXPECT_EQ(ht["hey"], "ccccc");
}

TEST(hash_map, basic_with_alloc) {
    asl::hash_map<std::string, std::string> ht{ std::allocator<std::pair<std::string, std::string>>() };
    ht.insert({ "hello", "world" });
    EXPECT_EQ(ht["hello"], "world");
    ht.insert({ "hey", "there" });
    EXPECT_EQ(ht["hey"], "there");
}

TEST(hash_map, construct_from_initializer_list) {
    asl::hash_map<std::string, std::string> ht = { { "hello", "world" }, { "hey", "there" } };
    EXPECT_EQ(ht["hey"], "there");
    EXPECT_EQ(ht["hello"], "world");
}

TEST(hash_map, construct_from_iterators) {
    std::vector<std::pair<std::string, std::string>> vec = { { "hello", "world" }, { "hey", "there" } };
    asl::hash_map<std::string, std::string> ht{ vec.begin(), vec.end() };
    EXPECT_EQ(ht["hey"], "there");
    EXPECT_EQ(ht["hello"], "world");
}

TEST(hash_map, copy_ctor) {
    asl::hash_map<std::string, std::vector<std::string>> ht = { { "hello", { "world", "!" } },
        { "hey", { "there", "!" } } };
    decltype(ht) copy_ht(ht);
    EXPECT_EQ(ht["hey"][0], "there");
    EXPECT_EQ(ht["hello"][0], "world");
    EXPECT_EQ(copy_ht["hey"][0], "there");
    EXPECT_EQ(copy_ht["hello"][0], "world");
}

TEST(hash_map, copy_assign) {
    asl::hash_map<std::string, std::vector<std::string>> ht = { { "hello", { "world", "!" } },
        { "hey", { "there", "!" } } };
    auto copy_ht = ht;
    EXPECT_EQ(ht["hey"][0], "there");
    EXPECT_EQ(ht["hello"][0], "world");
    EXPECT_EQ(copy_ht["hey"][0], "there");
    EXPECT_EQ(copy_ht["hello"][0], "world");
}

TEST(hash_map, move_ctor) {
    asl::hash_map<std::string, std::vector<std::string>> ht = { { "hello", { "world", "!" } },
        { "hey", { "there", "!" } } };
    decltype(ht) moved_ht(std::move(ht));
    EXPECT_EQ(moved_ht["hey"][0], "there");
    EXPECT_EQ(moved_ht["hello"][0], "world");
}

TEST(hash_map, move_assign) {
    asl::hash_map<std::string, std::vector<std::string>> ht = { { "hello", { "world", "!" } },
        { "hey", { "there", "!" } } };
    auto moved_ht = std::move(ht);
    EXPECT_EQ(moved_ht["hey"][0], "there");
    EXPECT_EQ(moved_ht["hello"][0], "world");
}

TEST(hash_map, move_ctor_unique_ptr) {
    asl::hash_map<std::string, std::unique_ptr<std::string>> ht;
    ht.emplace("hey", std::make_unique<std::string>("there"));
    ht.emplace("hello", std::make_unique<std::string>("world"));
    decltype(ht) moved_ht(std::move(ht));
    EXPECT_EQ(*moved_ht["hey"], "there");
    EXPECT_EQ(*moved_ht["hello"], "world");
}

TEST(hash_map, value_default_ctor) {
    asl::hash_map<std::string, std::string> ht;
    ht.emplace(std::piecewise_construct, std::forward_as_tuple("hello"), std::forward_as_tuple());
    EXPECT_EQ(ht.size(), 1);
    EXPECT_EQ(ht["hello"], "");
}

TEST(hash_map, find) {
    asl::hash_map<int, int> ht;
    ht.emplace(1, 1);
    ht.emplace(2, 4);
    ht.emplace(4, 16);

    EXPECT_NE(ht.find(1), ht.end());
    EXPECT_EQ(ht.find(1)->second, 1);
    EXPECT_NE(ht.find(2), ht.end());
    EXPECT_EQ(ht.find(2)->second, 4);
    EXPECT_NE(ht.find(4), ht.end());
    EXPECT_EQ(ht.find(4)->second, 16);

    EXPECT_EQ(ht.find(3), ht.end());
}

TEST(hash_map, iteration_empty) {
    asl::hash_map<int, int> ht{};
    auto res =
        std::accumulate(ht.begin(), ht.end(), 0, [](const auto res, const auto& entry) { return entry.second + res; });
    EXPECT_EQ(res, 0);
}

TEST(hash_map, iteration) {
    asl::hash_map<int, int> ht{
        { 1, 1 },
        { 2, 4 },
        { 3, 9 },
        { 4, 16 },
        { 5, 25 },
    };
    auto res =
        std::accumulate(ht.begin(), ht.end(), 0, [](const auto res, const auto& entry) { return entry.second + res; });
    EXPECT_EQ(res, 55);
}

TEST(hash_map, insert_and_erase) {
    asl::hash_map<std::string, int> ht{
        { "1", 1 },
        { "2", 2 },
        { "3", 3 },
        { "4", 4 },
        { "5", 5 },
    };

    auto res =
        std::accumulate(ht.begin(), ht.end(), 0, [](const auto res, const auto& entry) { return entry.second + res; });
    EXPECT_EQ(res, 15);

    for (int i = 6; i < 100; i++) {
        ht.emplace(std::to_string(i), i);
    }

    res =
        std::accumulate(ht.begin(), ht.end(), 0, [](const auto res, const auto& entry) { return entry.second + res; });
    EXPECT_EQ(res, 4950);

    EXPECT_NE(ht.find("4"), ht.end());
    EXPECT_EQ(ht.find("4")->second, 4);

    ht.erase("4");
    EXPECT_EQ(ht.find("4"), ht.end());

    EXPECT_NE(ht.find("5"), ht.end());
    EXPECT_EQ(ht.find("5")->second, 5);

    ht.erase(ht.find("5"));
    EXPECT_EQ(ht.find("5"), ht.end());

    EXPECT_NE(ht.find("90"), ht.end());

    ht.erase(ht.find("90"));
    EXPECT_EQ(ht.find("90"), ht.end());

    ht.erase(ht.find("1"));
    EXPECT_EQ(ht.find("1"), ht.end());

    for (int i = 0; i < 100; i++) {
        ht.erase(std::to_string(i));
    }
    EXPECT_TRUE(ht.empty());
    EXPECT_EQ(ht.size(), 0);

    res =
        std::accumulate(ht.begin(), ht.end(), 0, [](const auto res, const auto& entry) { return entry.second + res; });
    EXPECT_EQ(res, 0);
}