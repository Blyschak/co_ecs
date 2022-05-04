#pragma once

#include <cassert>

#include <cobalt/asl/sparse_map.hpp>
#include <cobalt/ecs/archetype.hpp>
#include <cobalt/ecs/entity.hpp>
#include <cobalt/ecs/entity_location.hpp>
#include <cobalt/ecs/resource.hpp>

namespace cobalt::ecs {

// forward declaration
class registry;

/// @brief A view lets you get a viewable range over components of Args out of a registry
///
/// A view isn't invalidated when there are changes made to the registry which lets a create one an re-use over time.
///
/// @tparam Args Component references types
template<component_reference... Args>
class view {
public:
    using value_type = std::tuple<Args...>;

    /// @brief Construct a new view object
    ///
    /// @param registry Reference to the registry
    view(registry& registry) noexcept : _registry(registry) {
    }

    /// @brief Returns an iterator that yields a std::tuple<Args...>
    ///
    /// @return decltype(auto) Iterator
    decltype(auto) each();

    /// @brief Run func on every entity that matches the Args requirement
    ///
    /// NOTE: This kind of iteration might be faster and better optimized by the compiler since the func can operate on
    /// a chunk that yields two tuples of pointers to the actual data whereas an each() variant returns an iterator over
    /// iterator over iterator to the actual data which is a challenge for compiler to optimize. Look at the benchmarks
    /// to see the actual difference.
    ///
    /// @param func A callable to run on entity components
    void each(auto&& func);

    /// @brief Get components for a single entity
    ///
    /// @param ent Entity to query
    /// @return value_type Components tuple
    value_type get(entity ent);

private:
    registry& _registry;
};

/// @brief Runtime view, used for scripting
class runtime_view {
public:
    /// @brief Construct a new runtime view object
    ///
    /// @tparam container_t Container type holding component IDs
    /// @param registry Registry reference
    /// @param ids IDs to match archetypes
    template<typename container_t>
    runtime_view(registry& registry, container_t ids) : _registry(registry), _ids(ids.begin(), ids.end()) {
    }

    /// @brief Iterate over entities that match given IDs
    ///
    /// @param func Function to call on every entity
    void each(auto&& func);

private:
    registry& _registry;
    std::vector<component_id> _ids;
};

/// @brief Registry is a container for all our entities and components. Components are stored in contiguosly in memory
/// allowing for very fast iterations, a so called SoA approach. A set of unique components form an archetype, where
/// every entity is mapped to an archetype.
class registry {
public:
    /// @brief Creates a new entity in the world with components Args... attached and returns an ecs::entity that
    /// user can use to operate on the enitty later, example:
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
        auto archetype = _archetypes.ensure_archetype(std::move(components));
        auto location = archetype->template emplace_back<Args...>(entity, std::forward<Args>(args)...);
        set_location(entity.id(), location);
        return entity;
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
    /// @tparam C Compone type
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
            auto components = archetype->components();
            components.insert(component_meta::of<C>());
            auto new_archetype = _archetypes.ensure_archetype(std::move(components));
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
        auto id = ent.id();
        auto& location = get_location(id);
        auto*& archetype = location.archetype;

        if (!archetype->contains<C>()) {
            return;
        }

        auto components = archetype->components();
        components.erase<C>();
        auto new_archetype = _archetypes.ensure_archetype(std::move(components));
        auto [new_location, moved] = archetype->move(location, *new_archetype);
        if (moved) {
            set_location(moved->id(), location);
        }

        archetype = new_archetype;
        set_location(id, new_location);
    }

    /// @brief Check if an entity is alive or not
    ///
    /// @param ent Entity
    /// @return true Alive
    /// @return false Dead
    [[nodiscard]] bool alive(entity ent) const noexcept {
        return _entity_pool.alive(ent);
    }

    /// @brief Get reference to component C
    ///
    /// @tparam C Component C
    /// @param ent Entity to read componet from
    /// @return C& Reference to component C
    template<component C>
    [[nodiscard]] C& get(entity ent) {
        return std::get<0>(get_impl<C&>(*this, ent));
    }

    /// @brief Get const reference to component C
    ///
    /// @tparam C Component C
    /// @param ent Entity to read componet from
    /// @return const C& Const reference to component C
    template<component C>
    [[nodiscard]] const C& get(entity ent) const {
        return std::get<0>(get_impl<const C&>(*this, ent));
    }

    /// @brief Get components for a single entity
    ///
    /// @tparam Args Component reference
    /// @param ent Entity to query
    /// @return value_type Components tuple
    template<component_reference... Args>
    [[nodiscard]] std::tuple<Args...> get(entity ent) {
        return get_impl<Args...>(*this, ent);
    }

    /// @brief Check if entity has component attached or not
    ///
    /// @tparam C Compone ttype
    /// @param ent Entity to check
    /// @return true If entity has component C attached
    /// @return false Otherwise
    template<component C>
    [[nodiscard]] bool has(entity ent) const {
        ensure_alive(ent);
        auto id = ent.id();
        auto& location = get_location(id);
        return location.archetype->template contains<C>();
    }

    /// @brief Register resource
    ///
    /// @tparam R Resource type
    /// @param resource Reference to the resource
    template<resource R>
    void register_resource(R& resource) {
        if (_resource_map.contains(resource_family::id<R>)) {
            throw std::invalid_argument{ "resource already registered" };
        }
        _resource_map.emplace(resource_family::id<R>, &resource);
    }

    /// @brief Unregister resource
    ///
    /// @tparam R Resource type
    template<resource R>
    void unregister_resource() {
        _resource_map.erase(resource_family::id<R>);
    }

    /// @brief Get the resource object
    ///
    /// @tparam R Resource type
    /// @return R& Reference to the resource
    template<resource R>
    R& get_resource() {
        return *reinterpret_cast<R*>(_resource_map.at(resource_family::id<R>));
    }

    /// @brief Get the resource object
    ///
    /// @tparam R Resource type
    /// @return R& Reference to the resource
    template<resource R>
    const R& get_resource() const {
        return *reinterpret_cast<const R*>(_resource_map.at(resource_family::id<R>));
    }

    /// @brief Iterate every entity that has <Args...> components attached
    ///
    /// @tparam Args Components types pack
    /// @return decltype(auto) Range-like type that yields references to requested components
    template<component_reference... Args>
    decltype(auto) each() {
        return _archetypes.chunks<Args...>() | std::views::join; // join all chunks togather
    }

    /// @brief Iterate every entity that matches given IDs
    ///
    /// @tparam container_t Container type holding IDs
    /// @param ids IDs
    /// @param func Function to call on every entity
    /// @return decltype(auto)
    template<typename container_t>
    void runtime_each(const container_t& ids, auto&& func) {
        for (auto chunk : _archetypes.chunks(ids.begin(), ids.end())) {
            for (auto [entity] : chunk) {
                func(entity);
            }
        }
    }

    /// @brief Iterate every entity that has <Args...> components attached and run a func
    ///
    /// @tparam Args Components types pack
    /// @param func A callable to run on components
    template<component_reference... Args>
    void each(auto&& func) {
        for (auto chunk : _archetypes.chunks<Args...>()) {
            for (auto entry : chunk) {
                std::apply([func](auto&&... args) { func(std::forward<decltype(args)>(args)...); }, entry);
            }
        }
    }

    /// @brief Construct a view out of the registry
    ///
    /// @tparam Args Components types pack
    /// @return ecs::view<Args...>
    template<component_reference... Args>
    ecs::view<Args...> view() {
        return ecs::view<Args...>(*this);
    }

    /// @brief Construct a runtime view to match IDs
    ///
    /// @tparam container_t Container type holding IDs
    /// @param ids IDs to match archetype
    /// @return ecs::runtime_view
    template<typename container_t>
    ecs::runtime_view runtime_view(const container_t& ids) {
        return ecs::runtime_view(*this, ids);
    }

private:
    template<component_reference... Args>
    friend class view;

    template<component_reference... Args>
    static std::tuple<Args...> get_impl(auto&& self, entity ent) {
        self.ensure_alive(ent);
        auto id = ent.id();
        auto& location = self.get_location(id);
        auto* archetype = location.archetype;
        return std::tuple<Args...>(std::ref(archetype->template get<Args>(location))...);
    }

    inline void ensure_alive(const entity& ent) const {
        if (!alive(ent)) {
            throw entity_not_found{ ent };
        }
    }

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
    archetypes _archetypes;
    asl::sparse_map<entity_id, entity_location> _entity_archetype_map;
    asl::sparse_map<resource_id, void*> _resource_map;
};


template<component_reference... Args>
decltype(auto) view<Args...>::each() {
    return _registry.each<Args...>();
}

template<component_reference... Args>
void view<Args...>::each(auto&& func) {
    _registry.each<Args...>(std::move(func));
}


template<component_reference... Args>
std::tuple<Args...> view<Args...>::get(entity ent) {
    return _registry.get<Args...>(ent);
}

void runtime_view::each(auto&& func) {
    _registry.runtime_each(_ids, std::move(func));
}

} // namespace cobalt::ecs