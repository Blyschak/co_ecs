#pragma once

#include <cstdint>

#include <cobalt/asl/handle.hpp>
#include <cobalt/asl/handle_pool.hpp>

#include <iostream>

namespace cobalt::ecs {

/// @brief Entity ID type, 32 bit value should be sufficient for all use cases
using entity_id = std::uint32_t;

/// @brief Generation ID type
using generation_id = std::uint32_t;

using entity = cobalt::asl::handle<entity_id, generation_id>;

/// @brief Pool of entities, generates entity ids, recycles ids.
using entity_pool = cobalt::asl::handle_pool<entity>;

/// @brief Serialize an entity to a stream
///
/// @param output Stream to write to
/// @param ent Entity to serialize
/// @return std::ostream&
inline std::ostream& operator<<(std::ostream& output, const entity& ent) {
    return output << "[id=" << ent.id() << ", generation=" << ent.generation() << "]";
}

} // namespace cobalt::ecs
