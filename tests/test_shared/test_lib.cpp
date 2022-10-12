#define CO_ECS_HOST

#include "components.hpp"


CO_ECS_EXPORT std::size_t get_my_1_id_from_shared() {
    return component_id::value<My_1_component>;
}

CO_ECS_EXPORT std::size_t get_my_2_id_from_shared() {
    return component_id::value<My_2_component>;
}

CO_ECS_EXPORT std::size_t get_my_3_id_from_shared() {
    return component_id::value<My_3_component>;
}