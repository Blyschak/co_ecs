#include <co_ecs/registry.hpp>
#include <co_ecs/view.hpp>

#include <gtest/gtest.h>

template<std::size_t I>
struct foo {
    int a{};
    int b{};

    foo() = default;
    foo(int a, int b) noexcept : a(a), b(b) {
    }
};

template<std::size_t I>
struct bar {
    int a{};
    int b{};

    bar() = default;
    bar(int a, int b) noexcept : a(a), b(b) {
    }
};

struct foo_creator {
    template<std::size_t I>
    using type = foo<I>;
};

struct bar_creator {
    template<std::size_t I>
    using type = foo<I>;
};

template<typename T, std::size_t N>
class components_generator {
    template<typename = std::make_index_sequence<N>>
    struct impl;

    template<std::size_t... Is>
    struct impl<std::index_sequence<Is...>> {
        template<std::size_t II>
        using wrap = typename T::template type<II>;

        using type = std::tuple<wrap<Is>...>;
    };

public:
    using type = typename impl<>::type;
};

template<std::size_t N, std::size_t M>
static void setup_archetype(co_ecs::registry& registry) {
    // Setup N entities with M foo<i> components
    for (int i = 0; i < N; i++) {
        std::apply([&](auto&&... args) { registry.create(std::forward<decltype(args)>(args)...); },
            typename components_generator<bar_creator, M>::type{});
    }
}

static void setup_archetypes(co_ecs::registry& registry) {
    setup_archetype<1000, 0>(registry);
    setup_archetype<1000, 1>(registry);
    setup_archetype<1000, 2>(registry);
    setup_archetype<1000, 4>(registry);
    setup_archetype<1000, 8>(registry);
    setup_archetype<1000, 16>(registry);
    setup_archetype<1000, 32>(registry);
    setup_archetype<1000, 64>(registry);

    EXPECT_EQ(registry.get_archetypes().size(), 8);
}

template<std::size_t N, bool setup = false>
static void entity_creation_N() {
    co_ecs::registry registry;
    std::vector<co_ecs::entity> entities;

    if constexpr (setup) {
        setup_archetypes(registry);
    }

    for (int i = 0; i < N; i++) {
        auto entity = registry.create<foo<0>, foo<1>, foo<2>>({}, {}, {});
        EXPECT_TRUE(registry.alive(entity));
        entities.emplace_back(entity);
    }

    for (auto& entity : entities) {
        registry.destroy(entity);
        EXPECT_FALSE(registry.alive(entity));
    }
}

template<std::size_t N, bool setup = false>
static void emplace_and_set_remove_N() {
    co_ecs::registry registry;
    std::vector<co_ecs::entity> entities;

    if constexpr (setup) {
        setup_archetypes(registry);
    }

    for (int i = 0; i < N; i++) {
        auto entity = registry.create<foo<0>, foo<1>>({ 1, 2 }, { 3, 0 });
        EXPECT_TRUE(registry.alive(entity));
        entities.emplace_back(entity);
    }

    for (auto entity : entities) {
        auto foo_0 = registry.get<foo<0>>(entity);
        EXPECT_EQ(foo_0.a, 1);
        EXPECT_EQ(foo_0.b, 2);

        registry.set<foo<2>>(entity, 4, 5);

        auto foo_2 = registry.get<foo<2>>(entity);
        EXPECT_EQ(foo_2.a, 4);
        EXPECT_EQ(foo_2.b, 5);

        auto& ref_foo_0 = registry.get<foo<0>>(entity);
        ref_foo_0.a = 10;

        foo_0 = registry.get<foo<0>>(entity);
        EXPECT_EQ(foo_0.a, 10);
        EXPECT_EQ(foo_0.b, 2);

        registry.remove<foo<2>>(entity);

        foo_0 = registry.get<foo<0>>(entity);
        EXPECT_EQ(foo_0.a, 10);
        EXPECT_EQ(foo_0.b, 2);
    }

    for (auto entity : entities) {
        registry.destroy(entity);
        EXPECT_FALSE(registry.alive(entity));
    }
}
template<std::size_t N, bool setup = false>
static void view_N() {
    co_ecs::registry registry;

    if constexpr (setup) {
        setup_archetypes(registry);
    }

    for (int i = 0; i < N / 2; i++) {
        auto entity = registry.create<foo<0>, foo<1>, foo<2>>({ 1, 2 }, { 3, 0 }, { 5, 6 });
        EXPECT_TRUE(registry.alive(entity));
    }

    for (int i = 0; i < N / 2; i++) {
        auto entity = registry.create<foo<0>, foo<2>>({ 1, 2 }, { 5, 6 });
        EXPECT_TRUE(registry.alive(entity));
    }

    int sum_0{};
    int sum_1{};
    int sum_2{};

    for (auto [foo_0, foo_1, foo_2] : co_ecs::view<foo<0>&, const foo<1>&, foo<2>&>(registry).each()) {
        static_assert(std::is_same_v<decltype(foo_0), foo<0>&>);
        static_assert(std::is_same_v<decltype(foo_1), const foo<1>&>);
        static_assert(std::is_same_v<decltype(foo_2), foo<2>&>);
        sum_0 += foo_0.a;
        sum_1 += foo_1.a;
        sum_2 += foo_2.a;
    }

    EXPECT_EQ(sum_0, N / 2 * 1);
    EXPECT_EQ(sum_1, N / 2 * 3);
    EXPECT_EQ(sum_2, N / 2 * 5);

    sum_0 = 0;
    sum_2 = 0;

    const co_ecs::registry& c_reg = registry;
    for (auto [foo_0, foo_2] : co_ecs::view<const foo<0>&, const foo<2>&>(c_reg).each()) {
        static_assert(std::is_same_v<decltype(foo_0), const foo<0>&>);
        static_assert(std::is_same_v<decltype(foo_2), const foo<2>&>);
        sum_0 += foo_0.a;
        sum_2 += foo_2.a;
    }

    EXPECT_EQ(sum_0, N * 1);
    EXPECT_EQ(sum_2, N * 5);
}

TEST(registry, entity_creation_empty) {
    co_ecs::registry registry;

    auto entity = registry.create();

    EXPECT_TRUE(registry.alive(entity));
    registry.destroy(entity);
    EXPECT_FALSE(registry.alive(entity));
}

TEST(registry, entity_creation) {
    entity_creation_N<10000>();
}

TEST(registry, entity_creation_10k) {
    entity_creation_N<10000>();
}

TEST(registry, emplace_and_set_remove) {
    emplace_and_set_remove_N<1>();
}

TEST(registry, emplace_and_set_remove_10k) {
    emplace_and_set_remove_N<10000>();
}

TEST(registry, entity_creation_8_archetypes) {
    entity_creation_N<10000, true>();
}

TEST(registry, entity_creation_10k_8_archetypes) {
    entity_creation_N<10000, true>();
}

TEST(registry, emplace_and_set_remove_8_archetypes) {
    emplace_and_set_remove_N<1, true>();
}

TEST(registry, emplace_and_set_remove_10k_8_archetypes) {
    emplace_and_set_remove_N<10000, true>();
}

TEST(registry, view_20) {
    view_N<20, false>();
}

TEST(registry, view_1M) {
    view_N<1000000, false>();
}

TEST(registry, view_20_8_archetypes) {
    view_N<20, true>();
}

TEST(registry, view_1M_8_archetypes) {
    view_N<1000000, true>();
}

TEST(registry, exceptions) {
    co_ecs::registry registry;
    auto ent = registry.create<foo<0>>({ 2, 2 });
    EXPECT_THROW(static_cast<void>(registry.get<foo<1>>(ent)), co_ecs::component_not_found);
    EXPECT_THROW(
        static_cast<void>(registry.template get<const foo<0>&, const foo<1>&>(ent)), co_ecs::component_not_found);
    registry.destroy(ent);
    EXPECT_THROW(static_cast<void>(registry.get<foo<0>>(ent)), co_ecs::entity_not_found);
    EXPECT_THROW(static_cast<void>(registry.has<foo<0>>(ent)), co_ecs::entity_not_found);
    EXPECT_THROW(registry.set<foo<0>>(ent), co_ecs::entity_not_found);
    EXPECT_THROW(registry.destroy(ent), co_ecs::entity_not_found);
}