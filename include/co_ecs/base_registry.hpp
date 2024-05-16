#pragma once

#include <co_ecs/archetype.hpp>
#include <co_ecs/component.hpp>
#include <co_ecs/detail/sparse_map.hpp>
#include <co_ecs/detail/type_traits.hpp>
#include <co_ecs/entity.hpp>
#include <co_ecs/entity_location.hpp>

namespace co_ecs {

/// @brief Placeholder (reserved) entity
class placeholder_entity {
public:
    /// @brief Get underlaying entity handle
    /// @return Entity handle
    [[nodiscard]] constexpr entity get_entity() const noexcept {
        return _ent;
    }

private:
    friend class base_registry;

    constexpr placeholder_entity(entity ent) : _ent(ent) {
    }

    [[nodiscard]] constexpr operator entity() const noexcept {
        return _ent;
    }

private:
    entity _ent;
};

class base_registry {
public:
    /// @brief Checks if the specified entity is currently active within the registry.
    ///
    /// This method determines whether an entity is still active and not yet destroyed in the registry.
    /// This is useful for validating entities before performing operations on them, helping to avoid
    /// errors related to using invalid or destroyed entity references.
    ///
    /// @param ent The entity to check for activity.
    /// @return bool Returns true if the entity is active, false otherwise.
    [[nodiscard]] constexpr auto alive(entity ent) const noexcept -> bool {
        return _entity_pool.alive(ent);
    }

    /// @brief Destroys the given entity.
    ///
    /// @code
    /// registry.destroy(entity);
    /// @endcode
    /// @param ent Entity to destroy
    void destroy(entity ent) {
        auto location = get_location(ent);

        // returns the entity that has been moved to a new location
        auto moved = location.archetype->swap_erase(location);
        remove_location(ent.id());

        if (moved) {
            set_location(moved->id(), location);
        }

        _entity_pool.recycle(ent);
    }

    /// @brief Provides access to the modifiable list of archetypes in the registry.
    ///
    /// This method returns a reference to the internal container of archetypes, allowing for modifications
    /// to the archetypes. It can be used to add, modify, or inspect the archetypes directly, which represent
    /// different entity configurations within the entity-component system.
    ///
    /// @return archetypes& A reference to the modifiable archetypes container.
    [[nodiscard]] constexpr auto archetypes() noexcept -> class archetypes& {
        return _archetypes;
    }

    /// @brief Provides access to the immutable list of archetypes in the registry.
    ///
    /// This constant method returns a reference to the internal container of archetypes without allowing
    /// modifications. It is intended for inspecting or querying the archetypes, suitable for operations
    /// that require reading archetype information without altering it.
    ///
    /// @return const archetypes& A reference to the immutable archetypes container.
    [[nodiscard]] constexpr auto archetypes() const noexcept -> const class archetypes& {
        return _archetypes;
    }

    /// @brief Reserves an entity in a thread-safe manner.
    ///
    /// This method reserves an entity ID concurrently and guarantees thread safety. The reserved entity
    /// is not fully initialized or published until the \ref sync() method is called. This mechanism allows
    /// multiple threads to reserve entities concurrently without interference.
    ///
    /// @return Returns a reserved entity that will need to be published by calling the `sync()` method.
    [[nodiscard]] constexpr auto reserve() -> placeholder_entity {
        return placeholder_entity{ _entity_pool.reserve() };
    }

    /// @brief Synchronizes concurently reserved entities.
    /// @post Once called, all the entities returned by \ref reserve() accessible in the registry.
    constexpr void sync() {
        _entity_pool.flush();
    }

    /// @brief Checks if the specified entity has all the given components.
    /// @tparam C Variadic template arguments representing the component types to check.
    /// @param ent The entity to check for component existence.
    /// @return True if the entity has all specified components, false otherwise.
    template<component... C>
    [[nodiscard]] constexpr auto has(entity ent) const -> bool {
        const auto& location = get_location(ent);
        return (location.archetype->template contains<C>() && ...);
    }

    /// @brief Visit all components of an entity.
    /// @param ent Entity to visit.
    /// @param func Function, a visitor, to apply components to.
    constexpr void visit(entity ent, auto&& func) {
        const auto& location = get_location(ent);
        auto& archetype = *location.archetype;
        location.archetype->visit(location, std::forward<decltype(func)>(func));
    }

    /// @brief Visit all components of an entity (const variant).
    /// @param ent Entity to visit.
    /// @param func Function, a visitor, to apply components to.
    constexpr void visit(entity ent, auto&& func) const {
        const auto& location = get_location(ent);
        const auto& archetype = *location.archetype;
        archetype.visit(location, std::forward<decltype(func)>(func));
    }

protected:
    friend class entity_ref;
    friend class const_entity_ref;

    template<component... Components>
    constexpr auto create_impl(Components&&... args) -> entity {
        // compile-time check to make sure all component types in parameter pack are unique
        [[maybe_unused]] detail::unique_types<Components...> uniqueness_check;

        auto entity = allocate();
        auto archetype = _archetypes.ensure_archetype<Components...>();
        auto location = archetype->template emplace<Components...>(entity, std::forward<Components>(args)...);
        set_location(entity.id(), location);
        return entity;
    }

    template<component C>
    constexpr auto set_impl(entity ent) -> std::pair<bool, C*> {
        auto& location = get_location(ent);
        auto*& archetype = location.archetype;

        if (archetype->contains<C>()) {
            return { false, std::addressof(archetype->template get<C>(location)) };
        } else {
            auto new_archetype = _archetypes.ensure_archetype_added<C>(archetype);
            auto [new_location, moved] = archetype->move(location, *new_archetype);

            auto ptr = std::addressof(new_archetype->template get<C>(new_location));

            if (moved) {
                set_location(moved->id(), location);
            }

            archetype = new_archetype;
            set_location(ent.id(), new_location);

            return { true, ptr };
        }
    }

    template<component C>
    constexpr void remove(entity ent) {
        auto& location = get_location(ent);
        auto*& archetype = location.archetype;

        if (!archetype->contains<C>()) {
            return;
        }
        auto new_archetype = _archetypes.ensure_archetype_removed<C>(archetype);
        auto [new_location, moved] = archetype->move(location, *new_archetype);
        if (moved) {
            set_location(moved->id(), location);
        }

        archetype = new_archetype;
        set_location(ent.id(), new_location);
    }

    template<component... Args>
    constexpr static auto get_impl(auto&& self, entity ent) -> decltype(auto) {
        auto& location = self.get_location(ent);

        constexpr auto is_const = std::is_const_v<std::remove_reference_t<decltype(self)>>;
        using archetype_t = std::conditional_t<is_const, const archetype, archetype>;

        archetype_t* archetype = location.archetype;
        return std::forward_as_tuple(archetype->template get<Args>(location)...);
    }

    [[nodiscard]] constexpr auto allocate() -> entity {
        return _entity_pool.create();
    }

    entity move(entity ent, base_registry& dest) {
        assert((&dest != this) && "Move to the same registry does not make sense");
        return move(ent, dest, dest.allocate());
    }

    entity copy(entity ent, base_registry& dest) const {
        return copy(ent, dest, dest.allocate());
    }

    entity move(entity ent, base_registry& dest, placeholder_entity placeholder) {
        auto location = get_location(ent);
        auto* src_archetype = location.archetype;
        auto* dst_archetype = dest._archetypes.ensure_archetype(src_archetype->components());

        auto [new_location, moved] = src_archetype->move(location, *dst_archetype);

        remove_location(ent.id());
        if (moved && moved != ent) { // moved entity has been transfered to a different Registry
            set_location(moved->id(), location);
        }
        _entity_pool.recycle(ent);

        // TODO: handle inside Archetype
        *dst_archetype
             ->chunks()                                                      // in dst Archetype chunks
                 [new_location.chunk_index]                                  // find the Chunk the entity was moved to
             .ptr_unchecked<entity>(new_location.entry_index) = placeholder; // update entity value in the Chunk

        dest.set_location(placeholder.get_entity().id(), new_location);

        return placeholder;
    }

    entity copy(entity ent, base_registry& dest, placeholder_entity placeholder) const {
        auto location = get_location(ent);
        auto* src_archetype = location.archetype;
        auto* dst_archetype = dest._archetypes.ensure_archetype(src_archetype->components());
        auto new_location = src_archetype->copy(location, *dst_archetype);

        // TODO: handle inside Archetype
        *dst_archetype
             ->chunks()                                                      // in dst Archetype chunks
                 [new_location.chunk_index]                                  // find the Chunk the EntityId was moved to
             .ptr_unchecked<entity>(new_location.entry_index) = placeholder; // update EntityId value in the Chunk

        dest.set_location(placeholder.get_entity().id(), new_location);

        return placeholder;
    }

    entity clone(entity ent) {
        return copy(ent, *this, allocate());
    }

    entity clone(entity ent, placeholder_entity placeholder) {
        return copy(ent, *this, placeholder);
    }

private:
    constexpr void ensure_alive(const entity& ent) const {
        if (!alive(ent)) {
            throw entity_not_found{ ent };
        }
    }

    [[nodiscard]] constexpr auto get_location(entity ent) -> entity_location& {
        ensure_alive(ent);
        return _entity_archetype_map.at(ent.id());
    }

    [[nodiscard]] constexpr auto get_location(entity ent) const -> const entity_location& {
        ensure_alive(ent);
        return _entity_archetype_map.at(ent.id());
    }

    constexpr void set_location(typename entity::id_t entity_id, const entity_location& location) {
        _entity_archetype_map[entity_id] = location;
    }

    constexpr void remove_location(typename entity::id_t entity_id) {
        _entity_archetype_map.erase(entity_id);
    }

protected:
    entity_pool _entity_pool;
    class archetypes _archetypes;
    detail::sparse_map<typename entity::id_t, entity_location> _entity_archetype_map;
};


} // namespace co_ecs
