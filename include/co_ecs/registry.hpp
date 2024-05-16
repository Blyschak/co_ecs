#pragma once

#include <co_ecs/base_registry.hpp>
#include <co_ecs/entity_ref.hpp>
#include <co_ecs/view_arguments.hpp>


namespace co_ecs {

/// @brief Registry is a container for all our entities and components. Components are stored in continuously in memory
/// allowing for very fast iterations, a so called SoA approach. A set of unique components form an archetype, where
/// every entity is mapped to an archetype.
class registry : public base_registry {
public:
    /// @brief Creates a new entity and attaches the specified components to it.
    ///
    /// This method instantiates a new entity and assigns it the components provided as template arguments.
    /// It ensures all component types are unique, sets the components to the entity, and returns a reference
    /// to the newly created entity.
    ///
    /// Example usage:
    /// @code
    /// struct position {
    ///     int x;
    ///     int y;
    /// };
    ///
    /// int main() {
    ///     ecs::registry registry;
    ///     auto entity = registry.create<position>({1, 2});
    ///     return 0;
    /// }
    /// @endcode
    ///
    /// @tparam Component Component types to attach to the new entity.
    /// @param args Instances of components to attach.
    /// @return entity_ref A reference to the newly created entity, allowing further operations.
    template<component... Components>
    constexpr auto create(Components&&... args) -> entity_ref {
        return get_entity(create_impl(std::forward<Components>(args)...));
    }

    /// @brief Retrieves a mutable reference to an entity.
    ///
    /// This method returns a mutable reference to the specified entity from the registry.
    ///
    /// @param ent The entity to retrieve.
    /// @return A mutable reference to the specified entity.
    /// @note This method is marked as `[[nodiscard]]` and `constexpr`.
    [[nodiscard]]
    constexpr auto get_entity(entity ent) noexcept -> entity_ref {
        return entity_ref{ *this, ent };
    }

    /// @brief Retrieves a constant reference to an entity.
    ///
    /// This method returns a constant reference to the specified entity from the registry.
    ///
    /// @param ent The entity to retrieve.
    /// @return A constant reference to the specified entity.
    /// @note This method is marked as `[[nodiscard]]` and `constexpr`.
    [[nodiscard]]
    constexpr auto get_entity(entity ent) const noexcept -> const_entity_ref {
        return const_entity_ref{ *this, ent };
    }

    /// @brief Retrieves a constant reference to an entity (alias).
    ///
    /// This method is an alias for `get_entity` to return a constant reference to the specified entity from the
    /// registry.
    ///
    /// @param ent The entity to retrieve.
    /// @return A constant reference to the specified entity.
    /// @note This method is marked as `[[nodiscard]]` and `constexpr`.
    [[nodiscard]]
    constexpr auto get_entity_const(entity ent) const noexcept -> const_entity_ref {
        return get_entity(ent);
    }

    /// @brief Create a non-const view based on component query in parameter pack
    ///
    /// @tparam Args Component references
    /// @return view<Args...> A view
    template<component_reference... Args>
    constexpr auto view() -> co_ecs::view<Args...>
        requires(!const_component_references_v<Args...>)
    {

        return co_ecs::view<Args...>{ *this };
    }

    /// @brief Create a const view based on component query in parameter pack
    ///
    /// @tparam Args Component references
    /// @return view<Args...> A view
    template<component_reference... Args>
    constexpr auto view() const -> co_ecs::view<Args...>
        requires const_component_references_v<Args...>
    {
        return co_ecs::view<Args...>{ *this };
    }

    /// @brief Returns a single tuple of components matching Args, if available in the view.
    ///
    /// @return Optional tuple of components if found, otherwise empty optional
    template<component_reference... Args>
    constexpr auto single() -> std::optional<std::tuple<Args...>>
        requires(!const_component_references_v<Args...>)
    {
        return view<Args...>().single();
    }

    /// @brief Returns a single tuple of components matching Args, if available in the view.
    ///
    /// This method is available in const registry and allows accessing a single tuple of components matching Args.
    /// It returns an optional tuple, which is empty if no entities in the view match the component requirements.
    ///
    /// @return Optional tuple of components if found, otherwise empty optional
    template<component_reference... Args>
    constexpr auto single() const -> std::optional<std::tuple<Args...>>
        requires const_component_references_v<Args...>
    {
        return view<Args...>().single();
    }

    /// @brief Executes a given function on each entity that matches the specified component requirements.
    ///
    /// This method iterates over each entity that has the required components and applies the function `func`.
    ///
    /// @tparam F The type of the callable function to run on the entity components.
    /// @param func A callable to run on entity components.
    ///
    /// @section Example
    /// @code
    /// struct position { double x, y; };
    /// struct velocity { double vx, vy; };
    ///
    /// void update_position(position& p, const velocity& v) {
    ///     p.x += v.vx;
    ///     p.y += v.vy;
    /// }
    ///
    /// ecs::registry registry;
    /// registry.each(update_position);
    /// @endcode
    template<typename F>
    constexpr void each(F&& func)
        requires(!detail::func_decomposer<F>::is_const)
    {
        using view_t = typename detail::func_decomposer<F>::view_t;
        view_t{ *this }.each(std::forward<F>(func));
    }

    /// @brief Executes a given function on each entity that matches the specified component requirements, without
    /// modifying the entities.
    ///
    /// This constant version of `each` allows for iteration over entities in a read-only fashion. It is suitable
    /// for operations that only need to read component data without modifying it, ensuring that the entity data
    /// remains unchanged. This method shares the same optimization benefits as its non-constant counterpart.
    ///
    /// @tparam F The type of the callable function to run on the entity components.
    /// @param func A callable to run on entity components, must not modify the entities.
    template<typename F>
    constexpr void each(F&& func) const
        requires(detail::func_decomposer<F>::is_const)
    {
        using view_t = typename detail::func_decomposer<F>::view_t;
        view_t{ *this }.each(std::forward<F>(func));
    }

    /// @brief Returns the number of enitites in the registry
    /// @return Number of entities present in the registry
    [[nodiscard]] constexpr std::size_t size() const noexcept {
        return _entity_archetype_map.size();
    }

    /// @brief Checks if the registry currently manages any entities.
    /// @return bool Returns true if the registry has no entities, false otherwise.
    [[nodiscard]] constexpr bool empty() const noexcept {
        return _entity_archetype_map.empty();
    }
};

} // namespace co_ecs