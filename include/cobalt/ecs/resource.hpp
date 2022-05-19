#pragma once

#include <type_traits>

#include <cobalt/asl/family.hpp>

namespace cobalt::ecs {

/// @brief Type for resource ID
using resource_id = std::uint32_t;

/// @brief Resource concept
///
/// @tparam T Resource type
template<typename T>
concept resource = !std::is_reference_v<T> &&                 //
                   !std::is_pointer_v<T> &&                   //
                   !std::is_const_v<T> &&                     //
                   std::is_nothrow_move_constructible_v<T> && //
                   std::is_nothrow_move_assignable_v<T>;

/// @brief Type for family used to generated resource IDs.
using resource_family = cobalt::asl::family<struct _resource_family_t, resource_id>;

} // namespace cobalt::ecs
