#include <cobalt/asl/hash_map.hpp>
#include <cobalt/asl/hash_set.hpp>
#include <cobalt/asl/toposort.hpp>

#include <gtest/gtest.h>

#include <map>
#include <string>
#include <vector>

using namespace cobalt;

template<typename M, typename S = M::mapped_type>
static void test_toposort_generic() {
    M graph;
    std::vector<S> result;
    graph["A"] = { "B", "C", "D" };
    graph["B"] = { "C" };

    auto has_cycle = !asl::topological_sort(graph, std::back_inserter(result));
    EXPECT_FALSE(has_cycle);
    EXPECT_EQ(result.size(), 3);

    EXPECT_EQ(result[0].size(), 2);
    EXPECT_TRUE(std::ranges::find(result[0], "C") != result[0].end());
    EXPECT_TRUE(std::ranges::find(result[0], "D") != result[0].end());

    EXPECT_EQ(result[1].size(), 1);
    EXPECT_TRUE(std::ranges::find(result[1], "B") != result[1].end());

    EXPECT_EQ(result[2].size(), 1);
    EXPECT_TRUE(std::ranges::find(result[2], "A") != result[2].end());
}

template<typename M, typename S = M::mapped_type>
static void test_toposort_generic_cycle() {
    M graph;
    std::vector<S> result;
    graph["A"] = { "B", "C", "D" };
    graph["B"] = { "C" };
    graph["C"] = { "A" };

    auto has_cycle = !asl::topological_sort(graph, std::back_inserter(result));
    EXPECT_TRUE(has_cycle);
}

TEST(toposort, hash_map_set) {
    test_toposort_generic<asl::hash_map<std::string, std::set<std::string>>>();
}

TEST(toposort, map_hash_set) {
    test_toposort_generic<std::map<std::string, std::set<std::string>>>();
}

TEST(toposort, cycle_hash_map_set) {
    test_toposort_generic_cycle<asl::hash_map<std::string, std::set<std::string>>>();
}

TEST(toposort, cycle_map_hash_set) {
    test_toposort_generic_cycle<std::map<std::string, std::set<std::string>>>();
}