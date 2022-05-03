#pragma once

#include <cstdint>

namespace cobalt::ecs {

// forward declaration
class archetype;

/// @brief Entity location
class entity_location {
public:
    ecs::archetype* archetype{};
    std::size_t chunk_index{};
    std::size_t entry_index{};
};

} // namespace cobalt::ecs