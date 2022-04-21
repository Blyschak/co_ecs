#pragma once

#include <iostream>

#include <cobalt/asl/vector.hpp>
#include <cobalt/ecs/chunk.hpp>
#include <cobalt/ecs/component.hpp>
#include <cobalt/ecs/entity.hpp>
#include <cobalt/ecs/entity_location.hpp>

namespace cobalt::ecs {

/// @brief Archetype is a storage for component data for entities that share the same set of components. Archetype is
/// derived from node_base since we are going to store archetypes in a graph for fast search, insertion and erasure
/// operations.
class archetype {
public:
    using chunks_storage_type = asl::vector<chunk>;

    /// @brief Construct a new archetype object
    archetype() = default;

    //// @brief Construct a new archetype object
    ///
    /// @param components Components
    archetype(const component_meta_set& components) : _components(components) {
    }

    /// @brief Construct a new archetype object
    ///
    /// @param components Components
    archetype(component_meta_set&& components) : _components(std::move(components)) {
    }

    /// @brief Return components set
    ///
    /// @return const component_meta_set& Component set
    [[nodiscard]] constexpr const component_meta_set& components() const noexcept {
        return _components;
    }

    /// @brief Allocate memory for a new entity in the storage and return its location
    ///
    /// @tparam Args Components
    /// @param ent Entity
    /// @return entity_location Entity location
    template<component... Args>
    entity_location allocate(entity ent, Args&&... args) {
        if (_chunks.empty()) {
            _chunks.emplace_back(components());
        }
        auto* last_chunk = &_chunks.back();
        if (last_chunk->full()) {
            _chunks.emplace_back(components());
            last_chunk = &_chunks.back();
        }
        auto entry_index = last_chunk->size();
        auto chunk_index = _chunks.size() - 1;
        last_chunk->emplace_back(std::forward<entity>(ent), std::forward<Args>(args)...);
        return entity_location{ this, chunk_index, entry_index };
    }

    /// @brief Write component data
    ///
    /// @tparam Component Component type
    /// @tparam Args Parameter pack to construct component from
    /// @param location Entity location
    /// @param args Arguments to construct component from
    template<component Component, typename... Args>
    void write(entity_location location, Args&&... args) {
        assert(location.arch == this);
        assert(location.chunk_index < _chunks.size());
        auto& chunk = _chunks[location.chunk_index];
        assert(location.entry_index < chunk.size());
        chunk.at<Component>(location.entry_index) = Component{ std::forward<Args>(args)... };
    }

    /// @brief Construct Component from Args in place
    ///
    /// @tparam Component Component to construct
    /// @tparam Args Parameter pack to construct Component
    /// @param location Location where to construct
    /// @param args Arguments to pass to Component constructor
    template<component Component, typename... Args>
    void construct(entity_location location, Args&&... args) {
        assert(location.arch == this);
        assert(location.chunk_index < _chunks.size());
        auto& chunk = _chunks[location.chunk_index];
        assert(location.entry_index < chunk.size());
        std::construct_at(chunk.ptr<Component>(location.entry_index), std::forward<Args>(args)...);
    }

    /// @brief Read component data
    ///
    /// @tparam Component Component type
    /// @param location Entity location
    /// @return const Component& Component reference
    template<component Component>
    const Component& read(entity_location location) const {
        assert(location.arch == this);
        assert(location.chunk_index < _chunks.size());
        auto& chunk = _chunks[location.chunk_index];
        assert(location.entry_index < chunk.size());
        return chunk.at<Component>(location.entry_index);
    }

    /// @brief Read component data
    ///
    /// @tparam Component Component type
    /// @param location Entity location
    /// @return Component& Component reference
    template<component Component>
    Component& read(entity_location location) {
        assert(location.arch == this);
        assert(location.chunk_index < _chunks.size());
        auto& chunk = _chunks[location.chunk_index];
        assert(location.entry_index < chunk.size());
        return chunk.at<Component>(location.entry_index);
    }

    /// @brief Check if archetype has component C
    ///
    /// @tparam C Component type
    /// @return true If this archetype has component C
    /// @return false If this archetype does not have component C
    template<component C>
    bool contains() const noexcept {
        return components().contains<C>();
    }

    /// @brief Deallocate memory for entity located at location. This method uses swap remove approach for faster
    /// erasure, so it returns an ID of a entity that has been moved to this location or invalid entity ID is returned
    /// when archetype has only a single entity.
    ///
    /// @param location Entity location
    /// @return entity Moved entity
    entity deallocate(entity_location location) noexcept {
        assert(location.arch == this);
        assert(location.chunk_index < _chunks.size());
        auto& chunk = _chunks[location.chunk_index];
        assert(location.entry_index < chunk.size());
        auto& last_chunk = _chunks.back();
        entity ent = chunk.swap_end(location.entry_index, last_chunk);
        if (last_chunk.empty()) {
            _chunks.pop_back();
        }
        return ent;
    }

    /// @brief Move entity to a different archetype togather with its components.
    ///
    /// @param location Entity location
    /// @param archetype Archetype to move to
    /// @return entity_location Moved entity location in new archetype
    std::pair<entity_location, entity> move(entity_location location, archetype& archetype) {
        assert(location.arch == this);
        assert(location.chunk_index < _chunks.size());
        auto& chunk = _chunks[location.chunk_index];
        assert(location.entry_index < chunk.size());

        if (archetype._chunks.empty()) {
            archetype._chunks.emplace_back(archetype.components());
        }

        auto* last_chunk = &archetype._chunks.back();
        if (last_chunk->full()) {
            archetype._chunks.emplace_back(archetype.components());
            last_chunk = &archetype._chunks.back();
        }

        auto entry_index = chunk.move(location.entry_index, *last_chunk);
        auto moved_id = deallocate(location);
        auto new_location = entity_location{ &archetype, archetype._chunks.size() - 1, entry_index };
        return std::make_pair(new_location, moved_id);
    }

    /// @brief Return reference to chunks vector
    ///
    /// @return chunks_storage_type& Reference to vector of chunks
    [[nodiscard]] chunks_storage_type& chunks() noexcept {
        return _chunks;
    }

    /// @brief Return const reference to chunks vector
    ///
    /// @return const chunks_storage_type& Const reference to vector of chunks
    [[nodiscard]] const chunks_storage_type& chunks() const noexcept {
        return _chunks;
    }

private:
    component_meta_set _components;
    chunks_storage_type _chunks;
};

} // namespace cobalt::ecs