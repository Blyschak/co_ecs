#include "bench.hpp"

#include <co_ecs/co_ecs.hpp>

#include <benchmark/benchmark.h>

using namespace co_ecs;

using namespace std::chrono_literals;

bench_suffix(workers);

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
static void schedule_execution(benchmark::State& state) {
    thread_pool tp{ N };
    registry reg;

    for (auto i = 0ull; i < 1E6; i++) {
        reg.create<read_component_a, read_component_b, write_component_a, write_component_b>(
            { float(i) }, { float(i) }, { 0.0 }, { 0.0 });
    }

    auto exec = schedule()
                    .begin_stage()
                    .add_system([](co_ecs::view<const read_component_a&, write_component_a&> v) {
                        auto func = [](const auto& r, auto& w) { w.value += std::sin(r.value); };

                        if constexpr (std::is_same_v<P, parallel_iter>) {
                            v.par_each(func);
                        } else {
                            v.each(func);
                        }
                    })
                    .add_system([](co_ecs::view<const read_component_b&, write_component_b&> v) {
                        auto func = [](const auto& r, auto& w) { w.value += std::sin(r.value); };

                        if constexpr (std::is_same_v<P, parallel_iter>) {
                            v.par_each(func);
                        } else {
                            v.each(func);
                        }
                    })
                    .end_stage()
                    .create_executor(reg);

    for (auto _ : state) {
        exec->run_once();
    }
}


BENCHMARK(schedule_execution<1_workers, serial_iter>)->Unit(benchmark::kMillisecond);
BENCHMARK(schedule_execution<2_workers, serial_iter>)->Unit(benchmark::kMillisecond);
BENCHMARK(schedule_execution<4_workers, serial_iter>)->Unit(benchmark::kMillisecond);
BENCHMARK(schedule_execution<8_workers, serial_iter>)->Unit(benchmark::kMillisecond);

BENCHMARK(schedule_execution<1_workers, parallel_iter>)->Unit(benchmark::kMillisecond);
BENCHMARK(schedule_execution<2_workers, parallel_iter>)->Unit(benchmark::kMillisecond);
BENCHMARK(schedule_execution<4_workers, parallel_iter>)->Unit(benchmark::kMillisecond);
BENCHMARK(schedule_execution<8_workers, parallel_iter>)->Unit(benchmark::kMillisecond);
