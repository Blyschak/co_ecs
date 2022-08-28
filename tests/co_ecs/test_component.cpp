#include <set>

#include <co_ecs/detail/bits.hpp>
#include <co_ecs/component.hpp>
#include <co_ecs/detail/hash_map.hpp>

#include <gtest/gtest.h>

TEST(family, family_id_generation) {
    EXPECT_NE(co_ecs::detail::family<>::id<int>, co_ecs::detail::family<>::id<const int>);
    EXPECT_EQ(co_ecs::detail::family<>::id<int>, co_ecs::detail::family<>::id<int>);
}

TEST(family, family_type_id_generation) {
    using family_ints = co_ecs::detail::family<int>;

    EXPECT_EQ(family_ints::id<int>, co_ecs::detail::family<>::id<int>);
    EXPECT_NE(family_ints::id<int>, family_ints::id<const int>);
    EXPECT_EQ(family_ints::id<int>, family_ints::id<int>);
}

TEST(family, family_type_id_generation_small) {
    using family_ints_small = co_ecs::detail::family<int, std::uint8_t>;

    EXPECT_EQ(sizeof(family_ints_small::id<int>), sizeof(std::uint8_t));
}

struct component_a {};
struct component_b {};
struct component_c {};

TEST(component, basic) {
    auto set = co_ecs::component_meta_set();
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
    auto set = co_ecs::component_meta_set();
    set.insert<component_a>();
    set.insert<component_b>();

    std::set<decltype(set)::value_type> s;

    auto b = set.begin();
    auto e = set.end();
    ASSERT_NE(b, e);
    s.insert(*b++);
    ASSERT_NE(b, e);
    s.insert(*b++);
    EXPECT_EQ(b, e);

    std::set<decltype(set)::value_type> c;
    c.insert(co_ecs::component_meta::of<component_a>());
    c.insert(co_ecs::component_meta::of<component_b>());
    EXPECT_EQ(s, c);
}

TEST(component, references) {
    EXPECT_TRUE(co_ecs::mutable_component_reference_v<component_a&>);
    EXPECT_FALSE(co_ecs::mutable_component_reference_v<const component_a&>);
    EXPECT_TRUE(co_ecs::const_component_reference_v<const component_b&>);
    EXPECT_FALSE(co_ecs::const_component_reference_v<component_b&>);
}

struct component_d {
    std::string a;
};

TEST(component, meta) {
    auto meta = co_ecs::component_meta::of<component_a>();
    EXPECT_EQ(meta.id, co_ecs::component_family::id<component_a>);
    EXPECT_EQ(meta.type->size, sizeof(component_a));
    EXPECT_EQ(meta.type->align, alignof(component_a));
    EXPECT_TRUE(meta.type->destruct);

    meta = co_ecs::component_meta::of<component_d>();
    EXPECT_EQ(meta.id, co_ecs::component_family::id<component_d>);
    EXPECT_EQ(meta.type->size, sizeof(component_d));
    EXPECT_EQ(meta.type->align, alignof(component_d));
    EXPECT_TRUE(meta.type->destruct);
}
