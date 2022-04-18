#include <cobalt/ecs/archetype.hpp>
#include <cobalt/ecs/entity.hpp>

#include <gtest/gtest.h>

using namespace cobalt;

namespace components {

struct a {
    int foo;
    bool bar;

    a() : foo(15), bar(false) {
    }

    a(int foo, bool bar) : foo(foo), bar(bar) {
    }
};
struct b {
    bool c{};
};

} // namespace components

TEST(chunk, basic) {
    auto set = ecs::component_meta_set();
    set.insert<components::a>();
    set.insert<components::b>();
    ecs::chunk chunk(set);
    EXPECT_EQ(chunk.max_size(), 963);
}

TEST(chunk, basic_emplace) {
    auto set = ecs::component_meta_set();
    set.insert<components::a>();
    set.insert<components::b>();
    ecs::chunk chunk(set);

    chunk.emplace_back(components::a(15, false), components::b(false));
    EXPECT_EQ(chunk.at<components::a>(0).foo, 15);
    EXPECT_EQ(chunk.at<components::a>(0).bar, false);
    EXPECT_EQ(chunk.at<components::b>(0).c, false);
}
