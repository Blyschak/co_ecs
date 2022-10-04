#pragma once

#include <ranges>

#include <co_ecs/chunk.hpp>
#include <co_ecs/component.hpp>
#include <co_ecs/detail/hash_map.hpp>
#include <co_ecs/entity.hpp>
#include <co_ecs/entity_location.hpp>


namespace co_ecs {

/// @brief Archetype groups entities that share the same types of components. Archetype has a list of fixed size chunks
/// where entities and their components are stored in a packed arrays, in a so called SoA fashion
class archetype {
public:
    /// @brief Chunks storage type
    using chunks_storage_t = std::vector<chunk>;

    /// @brief Construct a new archetype object without components
    archetype() = default;

    /// @brief Construct a new archetype object
    ///
    /// @param components Components
    explicit archetype(component_meta_set components) noexcept : _components(std::move(components)) {
        _max_size = get_max_size(_components);
        assert((_max_size < chunk::chunk_bytes) && "Total size of components exceeds chunk block size of 16KB");
        init_blocks(_components);
        _chunks.emplace_back(_blocks, _max_size);
    }

    /// @brief Return components set
    ///
    /// @return const component_meta_set&
    [[nodiscard]] const component_meta_set& components() const noexcept {
        return _components;
    }

    /// @brief Return reference to chunks vector
    ///
    /// @return chunks_storage_t&
    [[nodiscard]] chunks_storage_t& chunks() noexcept {
        return _chunks;
    }

    /// @brief Return const reference to chunks vector
    ///
    /// @return const chunks_storage_t&
    [[nodiscard]] const chunks_storage_t& chunks() const noexcept {
        return _chunks;
    }

    /// @brief Emplace new entity and assign given components to it, return entities location
    ///
    /// @tparam Components Components types
    /// @param ent Entity
    /// @param components Components parameter pack
    /// @return entity_location
    template<component... Components>
    entity_location emplace_back(entity ent, Components&&... components) {
        auto& free_chunk = ensure_free_chunk();
        auto entry_index = free_chunk.size();
        auto chunk_index = _chunks.size() - 1;
        free_chunk.emplace_back(ent, std::forward<Components>(components)...);
        return entity_location{
            this,
            chunk_index,
            entry_index,
        };
    }

    /// @brief Swap erase an entity at given location, returns an entity that has been moved as a result of this
    /// operation or std::nullopt if no entities were moved
    ///
    /// @param location Entity location
    /// @return std::optional<entity>
    std::optional<entity> swap_erase(const entity_location& location) noexcept {
        auto& chunk = get_chunk(location);
        auto& last_chunk = _chunks.back();
        auto maybe_ent = chunk.swap_erase(location.entry_index, last_chunk);
        if (last_chunk.empty() && _chunks.size() > 1) {
            _chunks.pop_back();
        }
        return maybe_ent;
    }

    /// @brief Move entity and its components to a different archetype and returns a pair where the first element is
    /// moved entity location in a new archetype and the second is the entity that has been moved in this archetype or
    /// std::nullopt if no entities were moved
    ///
    /// @param location Entity location
    /// @param other Archetype to move entity and its components to
    /// @return std::pair<entity_location, std::optional<entity>>
    std::pair<entity_location, std::optional<entity>> move(const entity_location& location, archetype& other) {
        auto& chunk = get_chunk(location);
        assert((location.entry_index < chunk.size()) && "Entity location index exceeds chunk size");

        auto& free_chunk = other.ensure_free_chunk();
        auto entry_index = chunk.move(location.entry_index, free_chunk);
        auto moved = swap_erase(location);
        auto new_location = entity_location{
            &other,
            other._chunks.size() - 1,
            entry_index,
        };

        return std::make_pair(new_location, moved);
    }

    /// @brief Get component data
    ///
    /// @tparam ComponentRef Component reference type
    /// @param id Component ID
    /// @param location Entity location
    /// @return Component& Component reference
    template<component_reference ComponentRef>
    ComponentRef get(entity_location location) {
        return read_impl<ComponentRef>(*this, location);
    }

    /// @brief Get component data
    ///
    /// @tparam ComponentRef Component reference type
    /// @param id Component ID
    /// @param location Entity location
    /// @return ComponentRef Component reference
    template<component_reference ComponentRef>
    ComponentRef get(entity_location location) const {
        static_assert(
            const_component_reference_v<ComponentRef>, "Can only get a non-const reference on const archetype");
        return read_impl<ComponentRef>(*this, location);
    }

    /// @brief Check if archetype has component C
    ///
    /// @tparam C Component type
    /// @return true If this archetype has component C
    /// @return false If this archetype does not have component C
    template<component C>
    [[nodiscard]] bool contains() const noexcept {
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
    [[nodiscard]] bool contains(component_id component_id) const noexcept {
        if (component_id == component_family::id<entity>) {
            return true;
        } else {
            return components().contains(component_id);
        }
    }

private:
    void init_blocks(const component_meta_set& components_meta) {
        // make space for entity
        auto offset = add_block(0, component_meta::of<entity>());

        // space for all components
        for (const auto& meta : components_meta) {
            offset = add_block(offset, meta);
        }
    }

    std::size_t add_block(std::size_t offset, const component_meta& meta) {
        const std::size_t size_in_bytes = _max_size * meta.type->size;
        const std::size_t align = meta.type->align;

        _blocks.emplace(meta.id, offset, meta);

        offset += detail::mod_2n(offset, align) + size_in_bytes;

        return offset;
    }

    // Calculates the maximum size of individual components this chunk buffer can hold
    static std::size_t get_max_size(auto&& components_meta) {
        // Calculate size of the following structure:
        //
        // struct {
        //     component_a_t a;
        //     component_b_t b;
        //     component_c_t c;
        //     ...
        // };
        //
        auto aligned_size = aligned_components_size(components_meta);

        // Remaining size for packed components
        auto remaining_space = chunk::chunk_bytes - aligned_size;

        // Calculate how much components we can pack into remaining space
        auto remaining_elements_count = remaining_space / packed_components_size(components_meta);

        // The maximum amount of entities we can hold is grater by 1 for which we calculated aligned_size
        return remaining_elements_count + 1;
    }

    // Calculate size of packed structure of components
    static std::size_t packed_components_size(auto&& components_meta) noexcept {
        return std::accumulate(components_meta.begin(),
            components_meta.end(),
            component_meta::of<entity>().type->size,
            [](const auto& res, const auto& meta) { return res + meta.type->size; });
    }

    // Calculate size of properly aligned structure of components
    static std::size_t aligned_components_size(auto&& components_meta) noexcept {
        auto begin = chunk::alloc_alignment;
        auto end = begin;

        // Add single component element size accounting for its alignment
        auto add_elements = [&end](const component_meta& meta) {
            end += detail::mod_2n(std::bit_cast<std::size_t>(end), meta.type->align);
            end += meta.type->size;
        };

        add_elements(component_meta::of<entity>());
        for (const auto& meta : components_meta) {
            add_elements(meta);
        }

        return end - begin;
    }

    template<component_reference ComponentRef>
    inline static ComponentRef read_impl(auto&& self, entity_location location) {
        auto& chunk = self.get_chunk(location);
        assert((location.entry_index < chunk.size()) && "Entity location index exceeds chunk size");
        return *component_fetch::fetch_pointer<ComponentRef>(chunk, location.entry_index);
    }

    inline chunk& get_chunk(entity_location location) noexcept {
        assert((location.archetype == this) && "Location archetype pointer does not point to this archetype");
        assert((location.chunk_index < _chunks.size()) && "Location chunk index exceeds the chunks vector size");
        auto& chunk = _chunks[location.chunk_index];
        return chunk;
    }

    chunk& ensure_free_chunk() {
        auto& chunk = _chunks.back();
        if (!chunk.full()) {
            return chunk;
        }
        _chunks.emplace_back(_blocks, _max_size);
        return _chunks.back();
    }

    std::size_t _max_size{};
    blocks_type _blocks{};
    component_meta_set _components{};
    chunks_storage_t _chunks{};
};

/// @brief Container for archetypes, holds a map from component set to archetype
class archetypes {
public:
    /// @brief Underlying container storage type
    using storage_type = detail::hash_map<component_set, std::unique_ptr<archetype>, component_set_hasher>;
    using iterator = storage_type::iterator;
    using const_iterator = storage_type::const_iterator;
    using value_type = storage_type::value_type;
    using key_type = storage_type::key_type;
    using mapped_type = storage_type::mapped_type;

    /// @brief Get or create an archetype matching the passed Components types
    ///
    /// @tparam Components Component types
    /// @return archetype*
    template<component... Components>
    archetype* ensure_archetype() {
        _search_component_set.clear();
        (..., _search_component_set.insert<Components>());

        auto& archetype = _archetypes[_search_component_set];
        if (!archetype) {
            archetype = create_archetype(component_meta_set::create<Components...>());
        }
        return archetype.get();
    }

    /// @brief Get or create an archetype by adding new Components to an anchor archetype
    ///
    /// @tparam Components Components to add
    /// @param anchor_archetype Anchor archetype
    /// @return archetype*
    template<component... Components>
    archetype* ensure_archetype_added(const archetype* anchor_archetype) {
        _search_component_set = anchor_archetype->components().ids();
        (..., _search_component_set.insert<Components>());

        auto& archetype = _archetypes[_search_component_set];
        if (!archetype) {
            archetype = create_archetype_added<Components...>(anchor_archetype);
        }
        return archetype.get();
    }

    /// @brief Get or create an archetype by removing Components from an anchor archetype
    ///
    /// @tparam Components Components to remove
    /// @param anchor_archetype Anchor archetype
    /// @return archetype*
    template<component... Components>
    archetype* ensure_archetype_removed(const archetype* anchor_archetype) {
        _search_component_set = anchor_archetype->components().ids();
        (..., _search_component_set.erase<Components>());

        auto& archetype = _archetypes[_search_component_set];
        if (!archetype) {
            archetype = create_archetype_removed<Components...>(anchor_archetype);
        }
        return archetype.get();
    }

    /// @brief Returns iterator to the beginning of archetypes container
    ///
    /// @return decltype(auto)
    iterator begin() noexcept {
        return _archetypes.begin();
    }

    /// @brief Returns an iterator to the end of archetypes container
    ///
    /// @return decltype(auto)
    iterator end() noexcept {
        return _archetypes.end();
    }

    /// @brief Returns iterator to the beginning of archetypes container
    ///
    /// @return decltype(auto)
    [[nodiscard]] const_iterator begin() const noexcept {
        return _archetypes.begin();
    }

    /// @brief Returns an iterator to the end of archetypes container
    ///
    /// @return decltype(auto)
    [[nodiscard]] const_iterator end() const noexcept {
        return _archetypes.end();
    }

    /// @brief Returns the number of archetypes
    ///
    /// @return std::size_t
    [[nodiscard]] std::size_t size() const noexcept {
        return _archetypes.size();
    }

private:
    static decltype(auto) create_archetype(auto&& components_meta) {
        return std::make_unique<co_ecs::archetype>(std::forward<decltype(components_meta)>(components_meta));
    }

    template<component... Components>
    static decltype(auto) create_archetype_added(const archetype* anchor_archetype) {
        auto components_meta = anchor_archetype->components();
        (..., components_meta.insert<Components>());
        return std::make_unique<co_ecs::archetype>(std::move(components_meta));
    }

    template<component... Components>
    static decltype(auto) create_archetype_removed(const archetype* anchor_archetype) {
        auto components_meta = anchor_archetype->components();
        (..., components_meta.erase<Components>());
        return std::make_unique<co_ecs::archetype>(std::move(components_meta));
    }

    // Member component set is here to speed up archetype lookup.
    // If we create component_set every time we need to search for an archetype
    // that requires memory allocation for underlying data structure of the component_set.
    // The downside is that we need to NOT FORGET to clean the _search_component_set before use.
    // The other solution may require a custom allocator but create way more problems when I tried it.
    // In-ability to simply find() in the hash map due to a type mismatch is one of them.
    component_set _search_component_set{};

    storage_type _archetypes{};
};

} // namespace co_ecs