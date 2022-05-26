#include <cobalt/asl/string_utils.hpp>

#include <gtest/gtest.h>

using namespace cobalt;

TEST(string_utils, trim) {
    EXPECT_EQ(asl::trim("abc"), "abc");
    EXPECT_EQ(asl::trim("  abc"), "abc");
    EXPECT_EQ(asl::trim("abc  "), "abc");
    EXPECT_EQ(asl::trim("   abc  "), "abc");
}

TEST(string_utils, ltrim) {
    EXPECT_EQ(asl::ltrim("abc"), "abc");
    EXPECT_EQ(asl::ltrim("  abc"), "abc");
    EXPECT_EQ(asl::ltrim("abc  "), "abc  ");
    EXPECT_EQ(asl::ltrim("   abc  "), "abc  ");
}

TEST(string_utils, rtrim) {
    EXPECT_EQ(asl::rtrim("abc"), "abc");
    EXPECT_EQ(asl::rtrim("  abc"), "  abc");
    EXPECT_EQ(asl::rtrim("abc  "), "abc");
    EXPECT_EQ(asl::rtrim("   abc  "), "   abc");
}