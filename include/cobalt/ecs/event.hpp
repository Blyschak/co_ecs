#pragma once

#include <type_traits>

#include <cobalt/asl/family.hpp>

namespace cobalt::ecs {

/// @brief Type for event ID
using event_id = std::uint32_t;

/// @brief event concept
///
/// @tparam T event type
template<typename T>
concept event = !std::is_reference_v<T> &&                 //
                !std::is_pointer_v<T> &&                   //
                !std::is_const_v<T> &&                     //
                std::is_class_v<T> &&                      //
                std::is_nothrow_move_constructible_v<T> && //
                std::is_nothrow_move_assignable_v<T>;

/// @brief Type for family used to generated event IDs.
using event_family = cobalt::asl::family<struct _event_family_t, event_id>;

} // namespace cobalt::ecs
