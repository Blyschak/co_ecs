#include <cobalt/asl/family.hpp>

#include <gtest/gtest.h>

using namespace cobalt::asl;

TEST(family, family_id_generation) {
    EXPECT_NE(family<>::id<int>, family<>::id<const int>);
    EXPECT_EQ(family<>::id<int>, family<>::id<int>);
}

TEST(family, family_type_id_generation) {
    using family_ints = family<int>;

    EXPECT_EQ(family_ints::id<int>, family<>::id<int>);
    EXPECT_NE(family_ints::id<int>, family_ints::id<const int>);
    EXPECT_EQ(family_ints::id<int>, family_ints::id<int>);
}

TEST(family, family_type_id_generation_small) {
    using family_ints_small = family<int, std::uint8_t>;

    EXPECT_EQ(sizeof(family_ints_small::id<int>), sizeof(std::uint8_t));
}