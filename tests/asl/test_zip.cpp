#include <cobalt/asl/zip.hpp>

#include <gtest/gtest.h>

#include <numeric>
#include <ranges>
#include <string>
#include <tuple>
#include <vector>

TEST(zip, zip_vectors_and_string) {
    std::vector<int> vec = { 1, 2, 3, 4 };
    std::string str = "abcdefg";
    std::vector<int> vecsqr = { 1, 4, 9, 16 };

    auto sum = 0;
    for (auto [a, b, c] : cobalt::asl::zip_view(vec, str, vecsqr)) {
        sum += a + c;
    }

    ASSERT_EQ(sum, 40);
}

TEST(zip, zip_joined_vectors) {
    std::vector<int> v1 = { 1, 2 };
    std::vector<int> c1 = { 3, 4 };

    std::vector<std::vector<int>> range1 = { v1, c1 };

    std::vector<int> v2 = { 5, 6 };
    std::vector<int> c2 = { 7, 8 };

    std::vector<std::vector<int>> range2 = { v2, c2 };

    auto joined1 = range1 | std::views::join;
    auto joined2 = range2 | std::views::join;

    auto sum = 0;

    for (auto [a, b] : cobalt::asl::zip_view(joined1, joined2)) {
        sum += a + b;
    }

    ASSERT_EQ(sum, 36);
}