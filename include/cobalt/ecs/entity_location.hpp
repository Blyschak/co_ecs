#pragma once

#include <cstdint>

namespace cobalt::ecs {

// forward declaration
class archetype;

/// @brief Entity location
class entity_location {
public:
    // Non-owning pointer to an archetype this entity belongs to
    ecs::archetype* archetype{};

    // Chunk index
    std::size_t chunk_index{};

    // Entry index in the chunk
    std::size_t entry_index{};
};

} // namespace cobalt::ecs