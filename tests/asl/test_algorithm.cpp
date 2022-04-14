#include <cobalt/asl/algorithm.hpp>

#include <set>

#include <gtest/gtest.h>

using namespace cobalt;

TEST(set_difference, basic) {
    std::set<int> s1 = { 1, 2, 3 };
    std::set<int> s2 = { 2, 4 };
    auto res = asl::set_difference(s1, s2);
    EXPECT_EQ(res, std::set<int>({ 1, 3 }));
}

TEST(set_union, basic) {
    std::set<int> s1 = { 1, 2, 3 };
    std::set<int> s2 = { 2, 4 };
    auto res = asl::set_union(s1, s2);
    EXPECT_EQ(res, std::set<int>({ 1, 2, 3, 4 }));
}