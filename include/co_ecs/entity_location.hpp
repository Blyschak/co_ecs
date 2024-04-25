#pragma once

#include <cstddef>
#include <cstdint>

namespace co_ecs {

// forward declaration
class archetype;

/// @brief Entity location
class entity_location {
public:
    // Non-owning pointer to an archetype this entity belongs to
    co_ecs::archetype* archetype{};

    // Chunk index
    std::size_t chunk_index{};

    // Entry index in the chunk
    std::size_t entry_index{};
};

} // namespace co_ecs