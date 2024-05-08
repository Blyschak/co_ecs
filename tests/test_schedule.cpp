#include "components.hpp"

#include <catch2/catch_all.hpp>
#include <co_ecs/co_ecs.hpp>

using namespace co_ecs;
using namespace co_ecs::experimental;

TEST_CASE("Schedule", "Basic schedule operations") {
    registry reg;
    schedule schedule;

    REQUIRE(reg.size() == 0);

    schedule.add_system([](command_writer cmd) { cmd.create(foo<0>{ 1, 2 }, foo<1>{ 3, 4 }); })
        .add_system([](command_writer cmd) { cmd.create(foo<0>{ 5, 6 }, foo<1>{ 7, 8 }); })
        .add_system([](command_writer cmd) { cmd.create(foo<0>{ 9, 10 }, foo<1>{ 11, 12 }); });

    executor exec{ schedule, reg };

    exec.run_once();
    REQUIRE(reg.size() == 3);

    exec.run_once();
    REQUIRE(reg.size() == 6);
}

TEST_CASE("Schedule stress: Entity commands scale") {
    registry reg;
    schedule schedule;

    struct singleton {
        std::size_t entity_count{};
    };

    auto e = reg.create<singleton>({});

    REQUIRE(reg.size() == 1);

    const int number_of_entities = GENERATE(1, 10, 100, 1000);
    const int number_of_iterations = GENERATE(1, 10, 100);

    schedule
        .add_system([number_of_entities](command_writer cmd) {
            for (auto i : detail::views::iota(0, number_of_entities)) {
                cmd.create<foo<0>, foo<1>>({}, {});
            }
        })
        .barrier()
        .add_system([](command_writer cmd,
                        view<const entity&, const foo<0>&, const foo<1>&> v,
                        view<singleton&> singleton_view) {
            v.each([&](const auto& entity, const auto& foo_0, const auto& foo_1) {
                auto [singleton] = *singleton_view.single();
                singleton.entity_count++;
                cmd.destroy(entity);
            });
        });

    executor exec{ schedule, reg };

    for (auto i : detail::views::iota(0, number_of_iterations)) {
        exec.run_once();
    }

    REQUIRE(reg.get<singleton>(e).entity_count == number_of_entities * (number_of_iterations - 1));
}

TEST_CASE("Parallel for") {
    std::vector<std::uint64_t> vec;

    const std::uint64_t number_of_elements = GENERATE(10, 100, 1000, 1000000);

    std::atomic<uint64_t> sum{ 0 };

    for (int i = 0; i < number_of_elements; i++) {
        vec.push_back(i);
    }

    parallel_for(vec, [&sum](auto elem) { sum.fetch_add(elem); });

    REQUIRE(sum.load() == (number_of_elements) * (number_of_elements - 1) / 2);
}