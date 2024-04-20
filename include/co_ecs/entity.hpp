#pragma once

#include <atomic>
#include <cstdint>
#include <numeric>
#include <vector>

#include <co_ecs/detail/handle.hpp>

namespace co_ecs {

/// @brief Represents an entity, consisting of an ID and generation.
using entity = detail::handle<struct entity_tag_t>;

/// @brief Pool of entities that generates and recycles entity IDs.
using entity_pool = detail::handle_pool<entity>;

} // namespace co_ecs
