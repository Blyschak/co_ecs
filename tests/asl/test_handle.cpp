#include <cobalt/asl/handle.hpp>
#include <cobalt/asl/handle_pool.hpp>

#include <gtest/gtest.h>

using namespace cobalt::asl;

#ifdef HAS_FORMAT
TEST(handle, format) {
    auto h = handle{ 1, 2 };
    EXPECT_EQ(std::format("{}", h), "[1, 2]");
}
#endif

TEST(handle, handle_ordering) {
    auto h1 = handle{ 0, 0 };
    auto h2 = handle{ 0, 0 };

    EXPECT_TRUE(h1 == h2);

    auto h3 = handle{ 5, 6 };
    auto h4 = handle{ 5, 6 };

    EXPECT_TRUE(h3 == h4);

    auto h5 = handle{ 10, 6 };

    EXPECT_TRUE(h1 != h5);

    auto handle1_gen1 = handle{ 0, 1 };
    auto handle1_id1 = handle{ 1, 0 };

    EXPECT_TRUE(h1 < handle1_gen1);
    EXPECT_TRUE(h1 < handle1_id1);
    EXPECT_TRUE(handle1_id1 > handle1_gen1);
}

TEST(handle, handle_invalid) {
    auto h = handle{ 0, 0 };
    auto invalid = handle<>::invalid();

    EXPECT_TRUE(h.valid());
    EXPECT_FALSE(invalid.valid());
}

TEST(handle_pool, basic) {
    handle_pool<handle<>> pool;
    auto h1 = pool.create();
    EXPECT_TRUE(pool.alive(h1));

    auto h2 = pool.create();
    EXPECT_TRUE(pool.alive(h2));

    pool.recycle(h1);
    EXPECT_FALSE(pool.alive(h1));
    auto h3 = pool.create();
    EXPECT_TRUE(pool.alive(h3));
    EXPECT_EQ(h3.id(), h1.id());
}