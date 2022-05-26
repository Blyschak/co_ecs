#include <cobalt/ecs/registry.hpp>
#include <cobalt/ecs/system.hpp>

#include <benchmark/benchmark.h>

struct position {
    int x{};
    int y{};
};

struct rotation {
    int x{};
    int y{};
};

struct velocity {
    int x{};
    int y{};
};

static void setup_registry_for_iteration(cobalt::ecs::registry& registry, std::size_t count) {
    for (int i = 0; i < count; i++) {
        registry.create<position, rotation>({ i, i * 2 }, { i * 3, i });
    }
}

template<std::size_t N>
static void bm_entity_creation(benchmark::State& state) {
    auto registry = cobalt::ecs::registry();
    for (auto _ : state) {
        for (auto i = 0; i < N; i++) {
            registry.create<position, rotation, velocity>({}, {}, {});
        }
    }
}

template<std::size_t N>
static void bm_entity_archetype_change(benchmark::State& state) {
    auto registry = cobalt::ecs::registry();
    auto entity = cobalt::ecs::entity::invalid();
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

static void bm_entity_set_component(benchmark::State& state) {
    auto registry = cobalt::ecs::registry();
    auto entity = registry.create<position, rotation>({}, {});
    for (auto _ : state) {
        registry.set<velocity>(entity, 1, 2);
    }
}

static void bm_entity_get_component(benchmark::State& state) {
    auto registry = cobalt::ecs::registry();
    auto entity = registry.create<position, rotation>({}, {});
    for (auto _ : state) {
        benchmark::DoNotOptimize(registry.get<position>(entity));
    }
}

template<std::size_t N>
static void bm_entity_iterate_component(benchmark::State& state) {
    auto registry = cobalt::ecs::registry();
    setup_registry_for_iteration(registry, N);

    int sum = 0;
    for (auto _ : state) {
        for (auto [pos, rot] : registry.each<const position&, const rotation&>()) {
            sum += pos.x;
        }
    }

    benchmark::DoNotOptimize(sum);
}

template<std::size_t N>
static void bm_entity_iterate_component_with_view(benchmark::State& state) {
    auto registry = cobalt::ecs::registry();
    setup_registry_for_iteration(registry, N);

    int sum = 0;
    for (auto _ : state) {
        auto view = cobalt::ecs::view<const position&, const rotation&>(registry);
        view.each([&sum](const auto& pos, const auto& vel) { sum += pos.x; });
    }

    benchmark::DoNotOptimize(sum);
}

template<std::size_t N>
static void bm_entity_iterate_component_with_system(benchmark::State& state) {
    auto registry = cobalt::ecs::registry();
    setup_registry_for_iteration(registry, N);

    int sum = 0;
    cobalt::ecs::system system([&sum](cobalt::ecs::view<const position&, const rotation&> view) {
        view.each([&sum](const auto& pos, const auto& rot) { sum += pos.x; });
    });
    auto executor = system.create_executor(registry);
    for (auto _ : state) {
        executor->run();
    }

    benchmark::DoNotOptimize(sum);
}


BENCHMARK(bm_entity_creation<1>);
BENCHMARK(bm_entity_creation<1000000>)->Unit(benchmark::kSecond);
BENCHMARK(bm_entity_set_component);
BENCHMARK(bm_entity_get_component);
BENCHMARK(bm_entity_archetype_change<1>);
BENCHMARK(bm_entity_archetype_change<1000000>)->Unit(benchmark::kSecond);
BENCHMARK(bm_entity_iterate_component<20>);
BENCHMARK(bm_entity_iterate_component_with_view<20>);
BENCHMARK(bm_entity_iterate_component_with_system<20>);
BENCHMARK(bm_entity_iterate_component<1000000>)->Unit(benchmark::kMicrosecond);
BENCHMARK(bm_entity_iterate_component_with_view<1000000>)->Unit(benchmark::kMicrosecond);
BENCHMARK(bm_entity_iterate_component_with_system<1000000>)->Unit(benchmark::kMicrosecond);