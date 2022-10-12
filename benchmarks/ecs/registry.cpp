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

constexpr std::size_t operator"" _components(unsigned long long int number) {
    return number;
}

constexpr std::size_t operator"" _bytes_each(unsigned long long int number) {
    return number;
}

// Creates an entity with the given number of components
template<std::size_t N, std::size_t S = 0>
static void bm_entity_creation_with(benchmark::State& state) {
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

BENCHMARK(bm_entity_creation_with<0_components>);
BENCHMARK(bm_entity_creation_with<1_components, 64_bytes_each>);
BENCHMARK(bm_entity_creation_with<2_components, 64_bytes_each>);
BENCHMARK(bm_entity_creation_with<4_components, 64_bytes_each>);
BENCHMARK(bm_entity_creation_with<8_components, 64_bytes_each>);
