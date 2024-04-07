#pragma once

#include <co_ecs/archetype.hpp>
#include <co_ecs/detail/sparse_map.hpp>
#include <co_ecs/detail/type_traits.hpp>
#include <co_ecs/entity.hpp>
#include <co_ecs/entity_location.hpp>
#include <co_ecs/view_arguments.hpp>


namespace co_ecs {

/// @brief Registry is a container for all our entities and components. Components are stored in continuously in memory
/// allowing for very fast iterations, a so called SoA approach. A set of unique components form an archetype, where
/// every entity is mapped to an archetype.
class registry {
public:
    /// @brief Creates a new entity in the world with components Args... attached and returns an ecs::entity that
    /// user can use to operate on the entity later, example:
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
    auto create(Args&&... args) -> entity {
        // compile-time check to make sure all component types in parameter pack are unique
        [[maybe_unused]] detail::unique_types<Args...> uniqueness_check;

        auto entity = _entity_pool.create();
        create_with_entity(entity, std::forward<Args>(args)...);
        return entity;
    }

    /// @brief Creates a new entity in the world with components Args... attached.
    /// Thsi call must be used in conjunction with reserve() and flush_reserved().
    ///
    /// @tparam Args Component types
    /// @param args components
    template<component... Args>
    auto create_with_entity(entity ent, Args&&... args) {
        // compile-time check to make sure all component types in parameter pack are unique
        [[maybe_unused]] detail::unique_types<Args...> uniqueness_check;

        auto archetype = _archetypes.ensure_archetype<Args...>();
        auto location = archetype->template emplace_back<Args...>(ent, std::forward<Args>(args)...);
        set_location(ent.id(), location);
    }

    /// @brief Reserve an entity handle.
    /// This API is thread safe.
    /// @return Entity handle.
    auto reserve() const -> entity {
        return _entity_pool.reserve();
    }

    /// @brief Flushes reserved entities.
    void flush_reserved() {
        _entity_pool.flush();
    }

    /// @brief Destroy an entity
    ///
    /// @param ent Entity to destroy
    void destroy(entity ent) {
        ensure_alive(ent);
        auto location = get_location(ent.id());

        // returns the entity that has been moved to a new location
        auto moved = location.archetype->swap_erase(location);
        remove_location(ent.id());

        if (moved) {
            set_location(moved->id(), location);
        }

        _entity_pool.recycle(ent);
    }

    /// @brief Set component to an entity. It can either override a component value that is already assigned to an
    /// entity or it may construct a new once and assign to it. Note, such operation involves an archetype change
    /// which is a costly operation.
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
    /// @tparam C Component type
    /// @tparam Args Parameter pack, argument types to construct C from
    /// @param ent Entity to assign component to
    /// @param args Arguments to construct C from
    template<component C, typename... Args>
    void set(entity ent, Args&&... args) {
        ensure_alive(ent);
        auto& location = get_location(ent.id());
        auto*& archetype = location.archetype;

        if (archetype->contains<C>()) {
            archetype->template get<C&>(location) = C{ std::forward<Args>(args)... };
        } else {
            auto new_archetype = _archetypes.ensure_archetype_added<C>(archetype);
            auto [new_location, moved] = archetype->move(location, *new_archetype);

            auto ptr = std::addressof(new_archetype->template get<C&>(new_location));
            std::construct_at(ptr, std::forward<Args>(args)...);

            if (moved) {
                set_location(moved->id(), location);
            }

            archetype = new_archetype;
            set_location(ent.id(), new_location);
        }
    }

    /// @brief Remove component C from an entity. In case entity does not have component attached nothing is done
    /// and this method returns. In case component is removed it requires archetype change which is a costly
    /// operation.
    ///
    /// @tparam C Component type
    /// @param ent Entity to remove component from
    template<component C>
    void remove(entity ent) {
        ensure_alive(ent);
        auto entity_id = ent.id();
        auto& location = get_location(entity_id);
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
        set_location(entity_id, new_location);
    }

    /// @brief Check if an entity is alive or not
    ///
    /// @param ent Entity
    /// @return true Alive
    /// @return false Dead
    [[nodiscard]] auto alive(entity ent) const noexcept -> bool {
        return _entity_pool.alive(ent);
    }

    /// @brief Get reference to component C
    ///
    /// @tparam C Component C
    /// @param ent Entity to read component from
    /// @return C& Reference to component C
    template<component C>
    [[nodiscard]] auto get(entity ent) -> C& {
        return std::get<0>(get_impl<C&>(*this, ent));
    }

    /// @brief Get const reference to component C
    ///
    /// @tparam C Component C
    /// @param ent Entity to read component from
    /// @return const C& Const reference to component C
    template<component C>
    [[nodiscard]] auto get(entity ent) const -> const C& {
        return std::get<0>(get_impl<const C&>(*this, ent));
    }

    /// @brief Get components for a single entity
    ///
    /// @tparam Args Component reference
    /// @param ent Entity to query
    /// @return value_type Components tuple
    template<component_reference... Args>
    [[nodiscard]] auto get(entity ent) -> std::tuple<Args...>
        requires(const_component_references_v<Args...>)
    {
        return get_impl<Args...>(*this, ent);
    }

    /// @brief Get components for a single entity
    ///
    /// @tparam Args Component references
    /// @param ent Entity to query
    /// @return value_type Components tuple
    template<component_reference... Args>
    [[nodiscard]] auto get(entity ent) const -> std::tuple<Args...>
        requires(!const_component_references_v<Args...>)
    {
        return get_impl<Args...>(*this, ent);
    }

    /// @brief Check if entity has component attached or not
    ///
    /// @tparam C Component type
    /// @param ent Entity to check
    /// @return true If entity has component C attached
    /// @return false Otherwise
    template<component C>
    [[nodiscard]] auto has(entity ent) const -> bool {
        ensure_alive(ent);
        auto entity_id = ent.id();
        const auto& location = get_location(entity_id);
        return location.archetype->template contains<C>();
    }

    /// @brief Create a non-const view based on component query in parameter pack
    ///
    /// @tparam Args Component references
    /// @return view<Args...> A view
    template<component_reference... Args>
    auto view() -> co_ecs::view<Args...>
        requires(!const_component_references_v<Args...>);

    /// @brief Create a const view based on component query in parameter pack
    ///
    /// @tparam Args Component references
    /// @return view<Args...> A view
    template<component_reference... Args>
    auto view() const -> co_ecs::view<Args...>
        requires const_component_references_v<Args...>;

    /// @brief Run func on every entity that matches the Args requirement
    ///
    /// NOTE: This kind of iteration might be faster and better optimized by the compiler since the func can operate on
    /// a chunk that yields two tuples of pointers to the actual data whereas an each() variant returns an iterator over
    /// iterator over iterator to the actual data which is a challenge for compiler to optimize. Look at the benchmarks
    /// to see the actual difference.
    ///
    /// @param func A callable to run on entity components
    template<typename F>
    void each(F&& func)
        requires(!detail::func_decomposer<F>::is_const);

    /// @brief Run func on every entity that matches the Args requirement. Constant version
    ///
    /// NOTE: See the note on non-const each()
    ///
    /// @param func A callable to run on entity components
    template<typename F>
    void each(F&& func) const
        requires(detail::func_decomposer<F>::is_const);

    /// @brief Returns the number of enitites in the registry
    [[nodiscard]] std::size_t size() const noexcept {
        return _entity_archetype_map.size();
    }

private:
    [[nodiscard]] auto get_archetypes() noexcept -> archetypes& {
        return _archetypes;
    }

    [[nodiscard]] auto get_archetypes() const noexcept -> const archetypes& {
        return _archetypes;
    }

    template<component_reference... Args>
    static auto get_impl(auto&& self, entity ent) -> std::tuple<Args...> {
        self.ensure_alive(ent);
        auto& location = self.get_location(ent.id());
        auto* archetype = location.archetype;
        return std::tuple<Args...>(std::ref(archetype->template get<Args>(location))...);
    }

    inline void ensure_alive(const entity& ent) const {
        if (!alive(ent)) {
            throw entity_not_found{ ent };
        }
    }

    [[nodiscard]] auto get_location(entity_id_t entity_id) const -> const entity_location& {
        return _entity_archetype_map.at(entity_id);
    }

    [[nodiscard]] auto get_location(entity_id_t id) -> entity_location& {
        return _entity_archetype_map.at(id);
    }

    void set_location(entity_id_t entity_id, const entity_location& location) {
        _entity_archetype_map[entity_id] = location;
    }

    void remove_location(entity_id_t entity_id) {
        _entity_archetype_map.erase(entity_id);
    }

    // Let view access registry private members
    template<component_reference... Args>
    friend class view;

private:
    mutable entity_pool _entity_pool;
    archetypes _archetypes;
    detail::sparse_map<entity_id_t, entity_location> _entity_archetype_map;
};

/// @brief Registry system state
///
class system_registry_state {
public:
    /// @brief Construct a new system registry state object
    ///
    /// @param registry Registry reference
    /// @param user_context User provided context to fetch data from and provide to the system
    explicit system_registry_state(registry& registry, void* user_context) noexcept : _registry(registry) {
    }

    /// @brief Returns the actual state inside to pass to the system
    ///
    /// @return Registry
    [[nodiscard]] registry& get() noexcept {
        return _registry;
    }

    /// @brief Run deferred logic
    void run_deferred() const noexcept {
    }

private:
    registry& _registry;
};

} // namespace co_ecs