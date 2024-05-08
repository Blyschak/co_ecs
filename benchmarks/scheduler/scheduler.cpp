#include <co_ecs/co_ecs.hpp>

#include <benchmark/benchmark.h>

using namespace co_ecs;
using namespace co_ecs::experimental;

using namespace std::chrono_literals;

constexpr std::size_t operator"" _workers(unsigned long long int number) {
    return number;
}

struct read_component_a {
    float value;
};

struct read_component_b {
    float value;
};

struct write_component_a {
    float value;
};

struct write_component_b {
    float value;
};

struct parallel_iter {};
struct serial_iter {};

template<std::size_t N, typename P>
static void bm_schedule(benchmark::State& state) {
    thread_pool tp{ N };
    registry reg;

    for (auto i = 0ull; i < 1E6; i++) {
        reg.create<read_component_a, read_component_b, write_component_a, write_component_b>(
            { float(i) }, { float(i) }, { 0.0 }, { 0.0 });
    }

    schedule schedule;
    schedule.add_system([](co_ecs::view<const read_component_a&, write_component_a&> v) {
        auto func = [](const auto& r, auto& w) { w.value += std::sin(r.value); };

        if constexpr (std::is_same_v<P, parallel_iter>) {
            v.par_each(func);
        } else {
            v.each(func);
        }
    });
    schedule.add_system([](co_ecs::view<const read_component_b&, write_component_b&> v) {
        auto func = [](const auto& r, auto& w) { w.value += std::sin(r.value); };

        if constexpr (std::is_same_v<P, parallel_iter>) {
            v.par_each(func);
        } else {
            v.each(func);
        }
    });

    executor exec{ schedule, reg };

    for (auto _ : state) {
        exec.run_once();
    }
}


BENCHMARK(bm_schedule<1_workers, serial_iter>)->Unit(benchmark::kMillisecond);
BENCHMARK(bm_schedule<2_workers, serial_iter>)->Unit(benchmark::kMillisecond);
BENCHMARK(bm_schedule<4_workers, serial_iter>)->Unit(benchmark::kMillisecond);
BENCHMARK(bm_schedule<8_workers, serial_iter>)->Unit(benchmark::kMillisecond);

BENCHMARK(bm_schedule<1_workers, parallel_iter>)->Unit(benchmark::kMillisecond);
BENCHMARK(bm_schedule<2_workers, parallel_iter>)->Unit(benchmark::kMillisecond);
BENCHMARK(bm_schedule<4_workers, parallel_iter>)->Unit(benchmark::kMillisecond);
BENCHMARK(bm_schedule<8_workers, parallel_iter>)->Unit(benchmark::kMillisecond);
