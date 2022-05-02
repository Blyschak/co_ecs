#pragma once

#include <iostream>
#include <ranges>

#include <cobalt/asl/hash_map.hpp>
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
    void write(component_id id, entity_location location, Args&&... args) {
        read_impl<Component&>(*this, id, location) = Component{ std::forward<Args>(args)... };
    }

    /// @brief Construct Component from Args in place
    ///
    /// @tparam Component Component to construct
    /// @tparam Args Parameter pack to construct Component
    /// @param location Location where to construct
    /// @param args Arguments to pass to Component constructor
    template<component Component, typename... Args>
    void construct(component_id id, entity_location location, Args&&... args) {
        Component& p = read_impl<Component&>(*this, id, location);
        std::construct_at(std::addressof(p), std::forward<Args>(args)...);
    }

    /// @brief Read component data
    ///
    /// @tparam Component Component type
    /// @param location Entity location
    /// @return Component& Component reference
    template<component_reference Component>
    Component read(component_id id, entity_location location) {
        return read_impl<Component>(*this, id, location);
    }

    /// @brief Read component data
    ///
    /// @tparam Component Component type
    /// @param location Entity location
    /// @return const Component& Component reference
    template<component_reference Component>
    Component read(component_id id, entity_location location) const {
        return read_impl<Component>(*this, id, location);
    }

    /// @brief Check if archetype has component C
    ///
    /// @tparam C Component type
    /// @return true If this archetype has component C
    /// @return false If this archetype does not have component C
    template<component C>
    bool contains() const noexcept {
        if constexpr (std::is_same_v<C, entity>) {
            return true;
        } else {
            return components().contains<C>();
        }
    }

    /// @brief Check if archetype has component ID
    ///
    /// @param id Component ID
    /// @return true If this archetype has component C
    /// @return false If this archetype does not have component C
    bool contains(component_id id) const noexcept {
        if (id == component_family::id<entity>) {
            return true;
        } else {
            return components().contains(id);
        }
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
    template<component_reference Component>
    inline static Component read_impl(auto&& self, component_id id, entity_location location) {
        auto& chunk = self.get_chunk(id, location);
        assert(location.entry_index < chunk.size());
        return *chunk.template ptr<Component>(id, location.entry_index);
    }

    inline chunk& get_chunk(component_id id, entity_location location) {
        assert(location.arch == this);
        assert(location.chunk_index < _chunks.size());
        auto& chunk = _chunks[location.chunk_index];
        return chunk;
    }

    component_meta_set _components;
    chunks_storage_type _chunks;
};

/// @brief Container for archetypes, holds a map from component set to archetype
class archetypes {
public:
    /// @brief Underlaying container storage type
    using storage_type = asl::hash_map<component_set, std::unique_ptr<archetype>, component_set_hasher>;

    /// @brief Get or create and return an archetype for given component set
    ///
    /// @param components Component set
    /// @return archetype* Archetype pointer
    archetype* ensure_archetype(component_meta_set components) {
        auto& archetype = _archetypes[components.bitset()];
        if (!archetype) {
            archetype = std::make_unique<ecs::archetype>(std::move(components));
        }
        return archetype.get();
    }

    /// @brief Returns iterator to the beginning of archetypes container
    ///
    /// @return decltype(auto)
    decltype(auto) begin() noexcept {
        return _archetypes.begin();
    }

    /// @brief Returns an iterator to the end of archetypes container
    ///
    /// @return decltype(auto)
    decltype(auto) end() noexcept {
        return _archetypes.end();
    }

    /// @brief Returns iterator to the beginning of archetypes container
    ///
    /// @return decltype(auto)
    decltype(auto) begin() const noexcept {
        return _archetypes.begin();
    }

    /// @brief Returns an iterator to the end of archetypes container
    ///
    /// @return decltype(auto)
    decltype(auto) end() const noexcept {
        return _archetypes.end();
    }

    /// @brief Return a range of chunks that match given component set in Args
    ///
    /// @tparam Args
    /// @return decltype(auto)
    template<component_reference... Args>
    decltype(auto) chunks() {
        auto filter_archetypes = [](auto& archetype) {
            return (... && archetype->template contains<decay_component_t<Args>>());
        };
        auto into_chunks = [](auto& archetype) -> decltype(auto) { return archetype->chunks(); };
        auto as_typed_chunk = [](auto& chunk) -> decltype(auto) { return chunk_view<Args...>(chunk); };

        return *this                                    // for each archetype entry in archetype map
               | std::views::values                     // for each value, a pointer to archetype
               | std::views::filter(filter_archetypes)  // filter archetype by requested components
               | std::views::transform(into_chunks)     // fetch chunks vector
               | std::views::join                       // join chunks togather
               | std::views::transform(as_typed_chunk); // each chunk casted to a typed chunk view range-like type
    }

    /// @brief Return a range of chunks that match given ids in [first, last]
    ///
    /// @tparam iter_t Iterator to component ID
    /// @param first First iterator
    /// @param last Last iterator
    /// @return decltype(auto)
    template<std::forward_iterator iter_t>
    decltype(auto) chunks(iter_t first, iter_t last) {
        auto filter_archetypes = [first, last](auto& archetype) mutable {
            for (; first != last; first++) {
                if (!archetype->template contains(*first)) {
                    return false;
                }
            }
            return true;
        };
        auto into_chunks = [](auto& archetype) -> decltype(auto) { return archetype->chunks(); };
        auto as_typed_chunk = [](auto& chunk) -> decltype(auto) { return chunk_view<const entity&>(chunk); };

        return *this                                    // for each archetype entry in archetype map
               | std::views::values                     // for each value, a pointer to archetype
               | std::views::filter(filter_archetypes)  // filter archetype by requested components
               | std::views::transform(into_chunks)     // fetch chunks vector
               | std::views::join                       // join chunks togather
               | std::views::transform(as_typed_chunk); // each chunk casted to a typed chunk view range-like type
    }

private:
    storage_type _archetypes;
};

} // namespace cobalt::ecs