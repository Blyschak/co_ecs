#include "bench.hpp"

#include <co_ecs/co_ecs.hpp>

#include <benchmark/benchmark.h>

#include <array>

// This part contains components structs that are going to be used for tests.
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

std::uint64_t sum{ 0 };

// Creates an entity with the given number of components
template<std::size_t N, std::size_t S = 0>
static void entity_creation_with(benchmark::State& state) {
    using components_tuple = typename components_generator<foo_creator<S>, N>::type;

    auto registry = co_ecs::registry();

    auto bench_func = [&]() {
        std::apply([&]<typename... Args>(Args&&... args) { registry.create<Args...>(std::forward<Args>(args)...); },
            components_tuple{});
    };

    // first cold start
    bench_func();

    for (auto _ : state) {
        bench_func();
    }

    const std::size_t size = std::is_empty_v<components_tuple> ? 0 : sizeof(components_tuple);
    state.SetBytesProcessed(int64_t(state.iterations()) * size);
}

// Iterate N entities with M number of components, S bytes each
template<std::size_t N, std::size_t M, std::size_t S>
static void iterate_entities(benchmark::State& state) {
    const auto entities_count = N;
    using components_tuple = typename components_generator<foo_creator<S>, M>::type;

    auto registry = co_ecs::registry();

    auto create_entity = [&]() {
        std::apply([&]<typename... Args>(Args&&... args) { registry.create<Args...>(std::forward<Args>(args)...); },
            components_tuple{});
    };

    for (auto _ : std::ranges::iota_view{ 0ull, entities_count }) {
        create_entity();
    }

    auto bench_func = [&]() {
        std::apply(
            [&]<typename... Args>(Args&&... args) {
                registry.view<const Args&...>().each(
                    [](auto&&... args) { sum += (static_cast<std::uint8_t>(args.data[0]) + ...); });
            },
            components_tuple{});
    };

    benchmark::DoNotOptimize(sum);

    for (auto _ : state) {
        bench_func();
    }

    const std::size_t size = std::is_empty_v<components_tuple> ? 0 : sizeof(components_tuple);
    state.SetBytesProcessed(int64_t(state.iterations()) * size * entities_count);
}

// Iterate 1M entity with given number of components
template<std::size_t N, std::size_t M, std::size_t S>
static void iterate_entities_with_system(benchmark::State& state) {
    const auto entities_count = N;
    using components_tuple = typename components_generator<foo_creator<S>, M>::type;

    auto registry = co_ecs::registry();

    auto create_entity = [&]() {
        std::apply([&]<typename... Args>(Args&&... args) { registry.create<Args...>(std::forward<Args>(args)...); },
            components_tuple{});
    };

    for (auto _ : std::ranges::iota_view{ 0ull, entities_count }) {
        create_entity();
    }

    auto system = std::apply(
        [&]<typename... Args>(Args&&... args) {
            auto func = [](co_ecs::view<const Args&...> v) {
                v.each([](auto&&... args) { sum += (static_cast<std::uint8_t>(args.data[0]) + ...); });
            };

            return std::make_unique<co_ecs::system<decltype(func)>>(std::forward<decltype(func)>(func));
        },
        components_tuple{});

    auto system_executor = system->create_executor(registry, nullptr);

    benchmark::DoNotOptimize(sum);

    for (auto _ : state) {
        system_executor->run();
    }

    const std::size_t size = std::is_empty_v<components_tuple> ? 0 : sizeof(components_tuple);
    state.SetBytesProcessed(int64_t(state.iterations()) * size * entities_count);
}

BENCHMARK(entity_creation_with<0_components>);
BENCHMARK(entity_creation_with<1_components, 64_bytes_each>);
BENCHMARK(entity_creation_with<2_components, 64_bytes_each>);
BENCHMARK(entity_creation_with<4_components, 64_bytes_each>);
BENCHMARK(entity_creation_with<8_components, 64_bytes_each>);

BENCHMARK(iterate_entities<10_entities, 1_components, 64_bytes_each>);
BENCHMARK(iterate_entities<10_entities, 2_components, 64_bytes_each>);
BENCHMARK(iterate_entities<10_entities, 4_components, 64_bytes_each>);
BENCHMARK(iterate_entities<10_entities, 8_components, 64_bytes_each>);

BENCHMARK(iterate_entities<1000000_entities, 1_components, 64_bytes_each>)->Unit(benchmark::kMillisecond);
BENCHMARK(iterate_entities<1000000_entities, 2_components, 64_bytes_each>)->Unit(benchmark::kMillisecond);
BENCHMARK(iterate_entities<1000000_entities, 4_components, 64_bytes_each>)->Unit(benchmark::kMillisecond);
BENCHMARK(iterate_entities<1000000_entities, 8_components, 64_bytes_each>)->Unit(benchmark::kMillisecond);

BENCHMARK(iterate_entities_with_system<10_entities, 1_components, 64_bytes_each>);
BENCHMARK(iterate_entities_with_system<10_entities, 2_components, 64_bytes_each>);
BENCHMARK(iterate_entities_with_system<10_entities, 4_components, 64_bytes_each>);
BENCHMARK(iterate_entities_with_system<10_entities, 8_components, 64_bytes_each>);

BENCHMARK(iterate_entities_with_system<1000000_entities, 1_components, 64_bytes_each>)->Unit(benchmark::kMillisecond);
BENCHMARK(iterate_entities_with_system<1000000_entities, 2_components, 64_bytes_each>)->Unit(benchmark::kMillisecond);
BENCHMARK(iterate_entities_with_system<1000000_entities, 4_components, 64_bytes_each>)->Unit(benchmark::kMillisecond);
BENCHMARK(iterate_entities_with_system<1000000_entities, 8_components, 64_bytes_each>)->Unit(benchmark::kMillisecond);
