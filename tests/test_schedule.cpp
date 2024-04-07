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