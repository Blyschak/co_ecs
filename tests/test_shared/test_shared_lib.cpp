#define CO_ECS_CLIENT

#include "components.hpp"

#include <catch2/catch_all.hpp>

CO_ECS_IMPORT std::size_t get_my_1_id_from_shared();
CO_ECS_IMPORT std::size_t get_my_2_id_from_shared();
CO_ECS_IMPORT std::size_t get_my_3_id_from_shared();

struct My_client_component {};

TEST_CASE("ECS across boundaries", "Make sure components get same IDs") {
    // re-arrange done on purpose
    auto my_3_id = component_id::value<My_3_component>;
    auto my_client_id = component_id::value<My_client_component>;
    auto my_1_id = component_id::value<My_1_component>;
    auto my_2_id = component_id::value<My_2_component>;

    REQUIRE(my_1_id == get_my_1_id_from_shared());
    REQUIRE(my_2_id == get_my_2_id_from_shared());
    REQUIRE(my_3_id == get_my_3_id_from_shared());

    REQUIRE(my_client_id != get_my_1_id_from_shared());
    REQUIRE(my_client_id != get_my_2_id_from_shared());
    REQUIRE(my_client_id != get_my_3_id_from_shared());
}