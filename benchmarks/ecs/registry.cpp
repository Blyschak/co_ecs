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

static void bm_entity_creation(benchmark::State& state) {
    auto registry = cobalt::ecs::registry();
    for (auto _ : state) {
        registry.create<position, rotation, velocity>({}, {}, {});
    }
}

static void bm_entity_archetype_change(benchmark::State& state) {
    auto registry = cobalt::ecs::registry();
    auto entity = registry.create<position, rotation>({}, {});
    for (auto _ : state) {
        registry.set<velocity>(entity);
        registry.remove<velocity>(entity);
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

static void bm_entity_iterate_component(benchmark::State& state) {
    auto registry = cobalt::ecs::registry();
    for (int i = 0; i < 20; i++) {
        registry.create<position, rotation>({ i, i * 2 }, { i * 3, i });
    }

    int sum = 0;
    for (auto _ : state) {
        for (auto [pos, rot] : registry.each<const position&, const rotation&>()) {
            sum += pos.x;
        }
    }

    benchmark::DoNotOptimize(sum);
}

static void bm_entity_iterate_component_with_view(benchmark::State& state) {
    auto registry = cobalt::ecs::registry();
    for (int i = 0; i < 20; i++) {
        registry.create<position, rotation>({ i, i * 2 }, { i * 3, i });
    }

    int sum = 0;
    for (auto _ : state) {
        auto view = registry.view<const position&, const rotation&>();
        view.each([&sum](const auto& pos, const auto& vel) { sum += pos.x; });
    }

    benchmark::DoNotOptimize(sum);
}

static void bm_entity_iterate_component_with_system(benchmark::State& state) {
    auto registry = cobalt::ecs::registry();
    for (int i = 0; i < 20; i++) {
        registry.create<position, rotation>({ i, i * 2 }, { i * 3, i });
    }

    int sum = 0;
    cobalt::ecs::system system(registry, [&sum](cobalt::ecs::view<const position&, const rotation&> view) {
        view.each([&sum](const auto& pos, const auto& rot) { sum += pos.x; });
    });

    for (auto _ : state) {
        system.run();
    }

    benchmark::DoNotOptimize(sum);
}

BENCHMARK(bm_entity_creation);
BENCHMARK(bm_entity_set_component);
BENCHMARK(bm_entity_get_component);
BENCHMARK(bm_entity_archetype_change);
BENCHMARK(bm_entity_iterate_component);
BENCHMARK(bm_entity_iterate_component_with_view);
BENCHMARK(bm_entity_iterate_component_with_system);