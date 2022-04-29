#include <set>

#include <cobalt/asl/algorithm.hpp>
#include <cobalt/ecs/component.hpp>

#include <gtest/gtest.h>

using namespace cobalt;

struct component_a {};
struct component_b {};
struct component_c {};

TEST(component, basic) {
    auto set = ecs::component_meta_set();
    set.insert<component_a>();
    set.insert<component_b>();

    EXPECT_TRUE(set.contains<component_a>());
    EXPECT_TRUE(set.contains<component_b>());
    EXPECT_FALSE(set.contains<component_c>());

    set.insert<component_c>();

    EXPECT_TRUE(set.contains<component_a>());
    EXPECT_TRUE(set.contains<component_b>());
    EXPECT_TRUE(set.contains<component_c>());

    set.erase<component_a>();
    EXPECT_FALSE(set.contains<component_a>());
    EXPECT_TRUE(set.contains<component_b>());
    EXPECT_TRUE(set.contains<component_c>());
}

TEST(component, iteration) {
    auto set = ecs::component_meta_set();
    set.insert<component_a>();
    set.insert<component_b>();

    std::set<const ecs::component_meta*> s;

    auto b = set.begin();
    auto e = set.end();
    ASSERT_NE(b, e);
    s.insert(*b++);
    ASSERT_NE(b, e);
    s.insert(*b++);
    EXPECT_EQ(b, e);

    std::set<const ecs::component_meta*> c;
    c.insert(ecs::component_traits<component_a>::meta());
    c.insert(ecs::component_traits<component_b>::meta());
    EXPECT_EQ(s, c);
}

TEST(component, references) {
    EXPECT_TRUE(ecs::mutable_component_reference_v<component_a&>);
    EXPECT_FALSE(ecs::mutable_component_reference_v<const component_a&>);
    EXPECT_TRUE(ecs::const_component_reference_v<const component_b&>);
    EXPECT_FALSE(ecs::const_component_reference_v<component_b&>);
}

struct component_d {
    std::string a;
};

TEST(component, meta) {
    auto meta = ecs::component_traits<component_a>::meta();
    EXPECT_EQ(meta->id, ecs::component_traits<component_a>::id());
    EXPECT_EQ(meta->size, sizeof(component_a));
    EXPECT_EQ(meta->align, alignof(component_a));
    EXPECT_TRUE(meta->dtor);

    meta = ecs::component_traits<component_d>::meta();
    EXPECT_EQ(meta->id, ecs::component_traits<component_d>::id());
    EXPECT_EQ(meta->size, sizeof(component_d));
    EXPECT_EQ(meta->align, alignof(component_d));
    EXPECT_TRUE(meta->dtor);
}

TEST(component, sorted) {
    auto set = ecs::component_meta_set();
    set.insert<component_a>();
    set.insert<component_b>();
    set.insert<component_c>();

    auto iter = set.begin();
    auto value = *iter++;

    while (iter != set.end()) {
        EXPECT_LE(value, *iter);
        value = *iter++;
    }
}