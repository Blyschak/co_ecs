#include <cobalt/core/config.hpp>
#include <sstream>

#include <gtest/gtest.h>

using namespace cobalt;

TEST(config, read_config) {
    std::stringstream ss;
    ss << "a.b.c =  23\n  hey=world\n";

    auto config = config::from_stream(ss);

    EXPECT_EQ(config.get("a.b.c"), "23");
    EXPECT_EQ(config.get("hey"), "world");

    config.set_default("a.b.c", "25");

    EXPECT_EQ(config.get("a.b.c"), "23");

    config.set_default("myval", "1");

    EXPECT_EQ(config.get<int>("myval"), 1);

    EXPECT_THROW(config.get<int>("hey"), std::invalid_argument);
}