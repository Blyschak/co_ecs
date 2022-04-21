#pragma once

#include <cassert>
#include <ranges>

#include <cobalt/asl/hash_map.hpp>
#include <cobalt/asl/sparse_map.hpp>
#include <cobalt/asl/zip.hpp>
#include <cobalt/ecs/archetype.hpp>
#include <cobalt/ecs/entity.hpp>
#include <cobalt/ecs/entity_location.hpp>

namespace cobalt::ecs {

class registry {
public:
    /// @brief Spawns a new entity in the world with components Args... attached and returns an ecs::entity that user
    /// can use to operate on the enitty later, example:
    ///
    /// @code {.cpp}
    /// struct position {
    ///    int x;
    ///    int y;
    /// };
    ///
    /// int main() {
    ///     ecs::registry registry;
    ///     auto entity = registry.create<position>({ 1, 2 });
    ///     return 0;
    /// }
    /// @endcode
    ///
    /// @tparam Args Component types
    /// @param args components
    /// @return entity Entity to construct
    template<component... Args>
    entity create(Args&&... args) {
        auto entity = _entity_pool.create();
        auto components = component_meta_set::create<Args...>();
        auto& archetype = _archetypes[components.bitset()];
        if (!archetype) {
            archetype = std::make_unique<ecs::archetype>(std::move(components));
        }
        auto location = archetype->template allocate<Args...>(entity, std::forward<Args>(args)...);
        set_location(entity.id(), location);
        return entity;
    }

    /// @brief Destroy an entity
    ///
    /// @param ent Entity to destroy
    void destroy(entity ent) {
        assert(alive(ent));
        auto location = get_location(ent.id());
        // returns the entity ID that has been moved to a new location
        auto moved_id = location.arch->deallocate(location).id();
        remove_location(ent.id());
        if (moved_id != entity::invalid_id) {
            set_location(moved_id, location);
        }
        _entity_pool.recycle(ent);
    }

    /// @brief Set component to an entity. It can either override a component value that is already assigned to an
    /// entity or it may construct a new once and assign to it. Note, such operation involves an archetype change which
    /// is a costly operation.
    ///
    /// @code {.cpp}
    /// struct position {
    ///    int x;
    ///    int y;
    /// };
    ///
    /// struct velocity {
    ///    int x;
    ///    int y;
    /// };
    ///
    /// int main() {
    ///     ecs::registry registry;
    ///     auto entity = registry.create<position>({ 1, 2 });
    ///     registry.set<position>(entity, 1, 2 );
    ///     return 0;
    /// }
    /// @endcode
    ///
    /// @tparam C Compone type
    /// @tparam Args Parameter pack, argument types to construct C from
    /// @param ent Entity to assign component to
    /// @param args Arguments to construct C from
    template<component C, typename... Args>
    void set(entity ent, Args&&... args) {
        assert(alive(ent));
        auto id = ent.id();
        auto& location = get_location(id);
        auto*& archetype = location.arch;

        if (archetype->template contains<C>()) {
            archetype->template write<C>(location, std::forward<Args>(args)...);
        } else {
            auto components = archetype->components();
            components.insert<C>();
            auto& new_archetype = _archetypes[components.bitset()];
            if (!new_archetype) {
                new_archetype = std::make_unique<ecs::archetype>(components);
            }
            auto [new_location, moved] = archetype->move(location, *new_archetype);
            if (moved.id() != entity::invalid_id) {
                set_location(moved.id(), location);
            }

            new_archetype->template construct<C>(new_location, std::forward<Args>(args)...);

            archetype = new_archetype.get();
            set_location(id, new_location);
        }
    }

    /// @brief Remove component C from an entity. In case entity does not have component attached nothing is done and
    /// this method returns. In case component is removed it requires archetype change which is a costly operation.
    ///
    /// @tparam C Component type
    /// @param ent entity to remove component from
    template<component C>
    void remove(entity ent) {
        assert(alive(ent));
        auto id = ent.id();
        auto& location = get_location(id);
        auto*& archetype = location.arch;

        if (!archetype->template contains<C>()) {
            return;
        }

        auto components = archetype->components();
        components.erase<C>();
        auto& new_archetype = _archetypes[components.bitset()];
        if (!new_archetype) {
            new_archetype = std::make_unique<ecs::archetype>(components);
        }
        auto [new_location, moved] = archetype->move(location, *new_archetype);
        if (moved.id() != entity::invalid_id) {
            set_location(moved.id(), location);
        }

        set_location(id, new_location);
    }

    /// @brief Check if an entity is alive or not
    ///
    /// @param ent Entity
    /// @return true Alive
    /// @return false Dead
    bool alive(entity ent) const noexcept {
        return _entity_pool.alive(ent);
    }

    /// @brief Get reference to component C
    ///
    /// @tparam C Component C
    /// @param ent Entity to read componet from
    /// @return C& Reference to component C
    template<component C>
    C& get(entity ent) {
        assert(alive(ent));
        auto id = ent.id();
        auto& location = get_location(id);
        return location.arch->template read<C>(location);
    }

    /// @brief Get const reference to component C
    ///
    /// @tparam C Component C
    /// @param ent Entity to read componet from
    /// @return const C& Const reference to component C
    template<component C>
    const C& get(entity ent) const {
        assert(alive(ent));
        auto id = ent.id();
        auto& location = get_location(id);
        return location.arch->template read<C>(location);
    }

    /// @brief Check if entity has component attached or not
    ///
    /// @tparam C Compone ttype
    /// @param ent Entity to check
    /// @return true If entity has component C attached
    /// @return false Otherwise
    template<component C>
    bool has(entity ent) const {
        assert(alive(ent));
        auto id = ent.id();
        auto& location = get_location(id);
        return location.arch->template contains<C>();
    }

    /// @brief Iterate every entity that has <Args...> components attached
    ///
    /// @tparam Args Components types pack
    /// @return decltype(auto) Range-like type that yields references to requested components
    template<component_reference... Args>
    decltype(auto) each() {
        auto filter_archetype_predicate = [](auto& archetype) {
            return (... && archetype->template contains<decay_component_t<Args>>());
        };
        auto into_chunks = [](auto& archetype) -> decltype(auto) { return archetype->chunks(); };
        auto as_typed_chunk = [](auto& chunk) -> decltype(auto) { return chunk.template cast_to<Args...>(); };

        return _archetypes | std::views::values | std::views::filter(filter_archetype_predicate)
               | std::views::transform(into_chunks) | std::views::join | std::views::transform(as_typed_chunk)
               | std::views::join;
    }

private:
    [[nodiscard]] const entity_location& get_location(entity_id id) const {
        return _entity_archetype_map.at(id);
    }

    [[nodiscard]] entity_location& get_location(entity_id id) {
        return _entity_archetype_map.at(id);
    }

    void set_location(entity_id id, const entity_location& location) {
        _entity_archetype_map[id] = location;
    }

    void remove_location(entity_id id) {
        _entity_archetype_map.erase(id);
    }

    entity_pool _entity_pool;
    asl::sparse_map<entity_id, entity_location> _entity_archetype_map;
    asl::hash_map<component_set, std::unique_ptr<archetype>, component_set_hasher> _archetypes;
};

} // namespace cobalt::ecs