#include <cobalt/asl/convert.hpp>

#include <gtest/gtest.h>

using namespace cobalt;

TEST(convert, basic) {
    EXPECT_EQ(asl::from_string<std::uint32_t>("158"), 158);
    EXPECT_THROW(asl::from_string<std::uint32_t>("invalid"), std::invalid_argument);
}