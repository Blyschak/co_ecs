#include <co_ecs/registry.hpp>
#include <co_ecs/view.hpp>

#include <benchmark/benchmark.h>

#include <array>

struct position {
    int x{};
    int y{};

    position() = default;
    position(int x, int y) noexcept : x(x), y(y) {
    }
};

struct rotation {
    int x{};
    int y{};

    rotation() = default;
    rotation(int x, int y) noexcept : x(x), y(y) {
    }
};

struct velocity {
    int x{};
    int y{};

    velocity() = default;
    velocity(int x, int y) noexcept : x(x), y(y) {
    }
};

// This header contains components structs that are going to be used for tests.
// Different components are simply generated through meta-programming with use of component_generator class.

template<std::size_t I, std::size_t S = 64>
struct foo {
    std::array<char, S> data{};

    foo() = default;
};

template<std::size_t I>
struct bar {
    int a{};
    int b{};

    bar() = default;
    bar(int a, int b) noexcept : a(a), b(b) {
    }
};

template<std::size_t S>
struct foo_creator {
    template<std::size_t I>
    using type = foo<I, S>;
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


static void setup_registry_for_iteration(co_ecs::registry& registry, std::size_t count) {
    for (int i = 0; i < count; i++) {
        registry.create<position, rotation>({ i, i * 2 }, { i * 3, i });
    }
}

// Creates an entity with the given number of components
template<std::size_t N, std::size_t S>
static void bm_entity_creation_number_of_components(benchmark::State& state) {
    using components_tuple = typename components_generator<foo_creator<S>, N>::type;

    auto registry = co_ecs::registry();

    for (auto _ : state) {
        std::apply([&]<typename... Args>(Args&&... args) { registry.create<Args...>(std::forward<Args>(args)...); },
            components_tuple{});
    }

    const std::size_t size = std::is_empty_v<components_tuple> ? 0 : sizeof(components_tuple);
    state.SetBytesProcessed(int64_t(state.iterations()) * size);
}

// Set component to an entity created with N components
template<std::size_t N>
static void bm_entity_set_component_number_of_components(benchmark::State& state) {
    using components_tuple = typename components_generator<foo_creator, N>::type;

    auto registry = co_ecs::registry();

    auto entity = std::apply(
        [&]<typename... Args>(Args&&... args) { return registry.create<Args...>(std::forward<Args>(args)...); },
        components_tuple{});

    for (auto _ : state) {
        registry.set<foo<N>>(entity);
    }

    state.SetBytesProcessed(int64_t(state.iterations()) * sizeof(foo<N>));
}

// Set component assigned to an entity created with N components
template<std::size_t N>
static void bm_entity_get_component_number_of_components(benchmark::State& state) {
    using components_tuple = typename components_generator<foo_creator, N>::type;

    auto registry = co_ecs::registry();

    auto entity = std::apply(
        [&]<typename... Args>(Args&&... args) { return registry.create<Args...>(std::forward<Args>(args)...); },
        components_tuple{});

    for (auto _ : state) {
        benchmark::DoNotOptimize(registry.get<foo<N - 1>>(entity));
    }

    state.SetBytesProcessed(int64_t(state.iterations()) * sizeof(foo<N - 1>));
}

template<std::size_t N>
static void bm_entity_archetype_change(benchmark::State& state) {
    auto registry = co_ecs::registry();
    auto entity = co_ecs::entity::invalid;
    for (auto i = 0; i < 1000000; i++) {
        entity = registry.create<position, rotation>({}, {});
    }
    for (auto _ : state) {
        for (auto i = 0; i < N; i++) {
            registry.set<velocity>(entity);
            registry.remove<velocity>(entity);
        }
    }
}

template<std::size_t N>
static void bm_entity_iterate_component(benchmark::State& state) {
    auto registry = co_ecs::registry();
    setup_registry_for_iteration(registry, N);

    int sum = 0;
    for (auto _ : state) {
        for (auto [pos, rot] : co_ecs::view<const position&, const rotation&>(registry).each()) {
            sum += pos.x;
        }
    }

    benchmark::DoNotOptimize(sum);
}

template<std::size_t N>
static void bm_entity_iterate_component_with_view(benchmark::State& state) {
    auto registry = co_ecs::registry();
    setup_registry_for_iteration(registry, N);

    int sum = 0;
    for (auto _ : state) {
        auto view = co_ecs::view<const position&, const rotation&>(registry);
        view.each([&sum](const auto& pos, const auto& vel) { sum += pos.x; });
    }

    benchmark::DoNotOptimize(sum);
}

// BENCHMARK(bm_entity_creation_number_of_components<0, 16>);
// BENCHMARK(bm_entity_creation_number_of_components<1, 16>);
// BENCHMARK(bm_entity_creation_number_of_components<2, 16>);
// BENCHMARK(bm_entity_creation_number_of_components<4, 16>);
// BENCHMARK(bm_entity_creation_number_of_components<8, 16>);
// BENCHMARK(bm_entity_creation_number_of_components<16, 16>);
// BENCHMARK(bm_entity_creation_number_of_components<32, 16>);
// BENCHMARK(bm_entity_creation_number_of_components<64, 16>);
// BENCHMARK(bm_entity_creation_number_of_components<128, 16>);

// BENCHMARK(bm_entity_creation_number_of_components<0, 32>);
// BENCHMARK(bm_entity_creation_number_of_components<1, 32>);
// BENCHMARK(bm_entity_creation_number_of_components<2, 32>);
// BENCHMARK(bm_entity_creation_number_of_components<4, 32>);
// BENCHMARK(bm_entity_creation_number_of_components<8, 32>);
// BENCHMARK(bm_entity_creation_number_of_components<16, 32>);
// BENCHMARK(bm_entity_creation_number_of_components<32, 32>);
// BENCHMARK(bm_entity_creation_number_of_components<64, 32>);
// BENCHMARK(bm_entity_creation_number_of_components<128, 32>);

// BENCHMARK(bm_entity_creation_number_of_components<0, 64>);
// BENCHMARK(bm_entity_creation_number_of_components<1, 64>);
// BENCHMARK(bm_entity_creation_number_of_components<2, 64>);
// BENCHMARK(bm_entity_creation_number_of_components<4, 64>);
// BENCHMARK(bm_entity_creation_number_of_components<8, 64>);
// BENCHMARK(bm_entity_creation_number_of_components<16, 64>);
// BENCHMARK(bm_entity_creation_number_of_components<32, 64>);
// BENCHMARK(bm_entity_creation_number_of_components<64, 64>);
// BENCHMARK(bm_entity_creation_number_of_components<128, 64>);

// BENCHMARK(bm_entity_creation_number_of_components<0, 128>);
// BENCHMARK(bm_entity_creation_number_of_components<1, 128>);
// BENCHMARK(bm_entity_creation_number_of_components<2, 128>);
// BENCHMARK(bm_entity_creation_number_of_components<4, 128>);
// BENCHMARK(bm_entity_creation_number_of_components<8, 128>);
// BENCHMARK(bm_entity_creation_number_of_components<16, 128>);
// BENCHMARK(bm_entity_creation_number_of_components<32, 128>);
// BENCHMARK(bm_entity_creation_number_of_components<64, 128>);
BENCHMARK(bm_entity_creation_number_of_components<128, 128>);

// BENCHMARK(bm_entity_set_component_number_of_components<0>);
// BENCHMARK(bm_entity_set_component_number_of_components<1>);
// BENCHMARK(bm_entity_set_component_number_of_components<2>);
// BENCHMARK(bm_entity_set_component_number_of_components<4>);
// BENCHMARK(bm_entity_set_component_number_of_components<8>);
// BENCHMARK(bm_entity_set_component_number_of_components<16>);
// BENCHMARK(bm_entity_set_component_number_of_components<32>);
// BENCHMARK(bm_entity_set_component_number_of_components<64>);
// BENCHMARK(bm_entity_set_component_number_of_components<128>);

// BENCHMARK(bm_entity_get_component_number_of_components<0>);
// BENCHMARK(bm_entity_get_component_number_of_components<1>);
// BENCHMARK(bm_entity_get_component_number_of_components<2>);
// BENCHMARK(bm_entity_get_component_number_of_components<4>);
// BENCHMARK(bm_entity_get_component_number_of_components<8>);
// BENCHMARK(bm_entity_get_component_number_of_components<16>);
// BENCHMARK(bm_entity_get_component_number_of_components<32>);
// BENCHMARK(bm_entity_get_component_number_of_components<64>);
// BENCHMARK(bm_entity_get_component_number_of_components<128>);

// BENCHMARK(bm_entity_archetype_change<1>);
// BENCHMARK(bm_entity_archetype_change<1000000>)->Unit(benchmark::kSecond);
// BENCHMARK(bm_entity_iterate_component<20>);
// BENCHMARK(bm_entity_iterate_component_with_view<20>);
// BENCHMARK(bm_entity_iterate_component<1000000>)->Unit(benchmark::kMicrosecond);
// BENCHMARK(bm_entity_iterate_component_with_view<1000000>)->Unit(benchmark::kMicrosecond);