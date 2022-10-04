#include "components.hpp"

#include <catch2/catch_all.hpp>
#include <co_ecs/co_ecs.hpp>

#include <set>

using namespace co_ecs;
using namespace co_ecs::detail;

TEST_CASE("Family ID", "Family ID generation") {
    REQUIRE(family<>::id<int> != family<>::id<const int>);
    REQUIRE(family<>::id<int> == family<>::id<int>);

    using family_ints = family<int>;

    REQUIRE(family_ints::id<int> == family<>::id<int>);
    REQUIRE(family_ints::id<int> != family_ints::id<const int>);
    REQUIRE(family_ints::id<int> == family_ints::id<int>);

    using family_ints_small = co_ecs::detail::family<int, std::uint8_t>;

    REQUIRE(sizeof(family_ints_small::id<int>) == sizeof(std::uint8_t));
}

TEST_CASE("Components", "Components utilities") {
    component_meta_set meta_set{};

    SECTION("Insertion and erasure") {
        meta_set.insert<foo<0>>();
        meta_set.insert<foo<1>>();

        REQUIRE(meta_set.contains<foo<0>>());
        REQUIRE(meta_set.contains<foo<1>>());
        REQUIRE_FALSE(meta_set.contains<foo<2>>());

        meta_set.insert<foo<2>>();

        REQUIRE(meta_set.contains<foo<0>>());
        REQUIRE(meta_set.contains<foo<1>>());
        REQUIRE(meta_set.contains<foo<2>>());

        meta_set.erase<foo<0>>();

        REQUIRE_FALSE(meta_set.contains<foo<0>>());
        REQUIRE(meta_set.contains<foo<1>>());
        REQUIRE(meta_set.contains<foo<2>>());
    }

    SECTION("Iteration over component set") {
        std::set<component_meta_set::value_type> s;

        meta_set.insert<foo<0>>();
        meta_set.insert<foo<1>>();

        auto b = meta_set.begin();
        auto e = meta_set.end();
        REQUIRE(b != e);
        s.insert(*b++);
        REQUIRE(b != e);
        s.insert(*b++);
        REQUIRE(b == e);

        std::set<component_meta_set::value_type> c;
        c.insert(component_meta::of<foo<0>>());
        c.insert(component_meta::of<foo<1>>());
        REQUIRE(s == c);
    }

    SECTION("Test type metadata") {
        auto meta = component_meta::of<foo<0>>();
        REQUIRE(meta.id == component_family::id<foo<0>>);
        REQUIRE(meta.type->size == sizeof(foo<0>));
        REQUIRE(meta.type->align == alignof(foo<0>));
        REQUIRE(meta.type->destruct);

        meta = component_meta::of<foo<1>>();
        REQUIRE(meta.id == component_family::id<foo<1>>);
        REQUIRE(meta.type->size == sizeof(foo<1>));
        REQUIRE(meta.type->align == alignof(foo<1>));
        REQUIRE(meta.type->destruct);
    }
}

TEST_CASE("Component references", "Component reference types") {
    REQUIRE(mutable_component_reference_v<foo<0>&>);
    REQUIRE_FALSE(mutable_component_reference_v<const foo<0>&>);
    REQUIRE(const_component_reference_v<const foo<1>&>);
    REQUIRE_FALSE(const_component_reference_v<foo<1>&>);
}