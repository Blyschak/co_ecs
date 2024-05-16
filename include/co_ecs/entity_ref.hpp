#pragma once

#include <co_ecs/base_registry.hpp>

namespace co_ecs {

/// @brief Represents a reference to an entity within a registry.
///
/// This class provides an interface to interact with a specific entity managed by a given registry.
/// It encapsulates the entity and the registry it belongs to, allowing for operations such as querying,
/// modifying, or checking the status of the entity in a safe and convenient manner. This reference
/// facilitates access to various functionalities provided by the registry concerning the specific entity.
class entity_ref {
public:
    /// @brief Constructs an entity reference for a specific entity in the given registry.
    ///
    /// This constructor initializes an `entity_ref` object that links an entity to its managing registry.
    /// It allows for subsequent operations on the entity through methods provided by this class.
    /// @param registry Reference to the `base_registry` that manages the entity.
    /// @param entity The `entity` that this reference will encapsulate.
    constexpr entity_ref(base_registry& registry, entity entity) noexcept : _entity(entity), _registry(registry) {
    }

    /// @brief Retrieves a constant reference to the archetype of the entity.
    ///
    /// This method returns a reference to the archetype associated with the encapsulated entity.
    /// An archetype defines the configuration of components that an entity possesses in the entity-component system.
    /// Accessing the archetype can be useful for inspecting the structure and component makeup of the entity.
    ///
    /// @return const archetype& A reference to the archetype of the entity.
    [[nodiscard]] constexpr auto archetype() const noexcept -> const archetype& {
        return *_registry.get().get_location(_entity).archetype;
    }

    /// @brief Checks if the entity is currently active in the registry.
    ///
    /// This method queries the registry to determine if the entity is still active and has not been destroyed.
    /// It is particularly useful for validity checks before performing operations on the entity.
    ///
    /// @return bool Returns true if the entity is active; otherwise, false.
    [[nodiscard]] constexpr auto alive() const noexcept -> bool {
        return _registry.get().alive(_entity);
    }

    /// @brief Determines if the entity has all specified components.
    ///
    /// This template method checks whether the entity has each of the components listed in the template arguments.
    /// This can be useful for conditional logic where actions depend on the presence of specific components
    /// in an entity. For example, an operation that should only be performed if an entity has both a `position`
    /// and `velocity` component.
    ///
    /// @tparam C The component types to check for.
    /// @return bool Returns true if the entity has all specified components; otherwise, false.
    template<component... C>
    [[nodiscard]] constexpr auto has() const noexcept -> bool {
        return _registry.get().template has<C...>(_entity);
    }

    /// @brief Retrieves a component of type C from the entity.
    ///
    /// This method template is designed to access a specific component from the entity managed by the registry.
    /// It provides direct access to a component, assuming that the entity contains it. If the entity does not have
    /// the requested component, it throws a `component_not_found` exception.
    ///
    /// @tparam C The type of component to retrieve, specified as a non-const reference.
    /// @return C A reference to the requested component of the entity.
    /// @throws component_not_found If the requested component C is not found in the entity.
    ///
    /// @section Example
    /// @code
    /// auto entity = registry.create<position>({});
    /// position& pos = entity.get<position>(); // Accesses position component, succeeds.
    /// // Below line throws component_not_found since the velocity component is not present
    /// velocity& vel = entity.get<velocity>();
    /// @endcode
    template<component C>
    [[nodiscard]] constexpr auto get() -> C& {
        return std::get<0>(base_registry::template get_impl<C>(_registry.get(), _entity));
    }

    /// @brief Retrieves a const-qualified component of type C from the entity in a read-only manner.
    ///
    /// This const method template is designed for read-only access to a specific component from the entity
    /// managed by the registry. It is used when the entity is expected to contain the component and modifications
    /// are not intended. If the requested component is not present, it throws a `component_not_found` exception.
    ///
    /// @tparam C The type of the component to retrieve, specified as a const-qualified reference.
    /// @return C A const reference to the requested component of the entity.
    /// @throws component_not_found If the requested component C is not found in the entity.
    ///
    /// @section Example
    /// @code
    /// const auto entity = registry.create<position>({});
    /// const position pos = entity.get<position>(); // Accesses position component, succeeds.
    /// // Below line throws component_not_found since velocity component is not present
    /// const velocity vel = entity.get<velocity>();
    /// @endcode
    template<component C>
    [[nodiscard]] constexpr auto get() const -> const C& {
        return std::get<0>(base_registry::template get_impl<C>(_registry.get(), _entity));
    }

    /// @brief Retrieves multiple components from the underlying registry for the current entity.
    ///
    /// This method allows accessing multiple components associated with the current entity from the underlying
    /// registry.
    ///
    /// @tparam C The first component type to retrieve.
    /// @tparam O The second component type to retrieve.
    /// @tparam Args Additional component types to retrieve.
    /// @return A tuple containing the requested components.
    ///
    /// @section Example
    /// @code
    /// const auto entity = registry.create<position, velocity>({});
    /// auto [pos, vel] = entity.get<position, velocity>();
    /// @endcode
    template<component C, component O, component... Args>
    [[nodiscard]] constexpr auto get() -> decltype(auto) {
        return base_registry::template get_impl<C, O, Args...>(_registry.get(), _entity);
    }

    /// @brief Retrieves multiple components from the underlying registry for the current entity (const
    /// version).
    ///
    /// This method allows accessing multiple components associated with the current entity from the underlying registry
    /// in a const context.
    ///
    /// @tparam C The first component type to retrieve.
    /// @tparam O The second component type to retrieve.
    /// @tparam Args Additional component types to retrieve.
    /// @return A tuple containing the requested components.
    template<component C, component O, component... Args>
    [[nodiscard]] constexpr auto get() const -> decltype(auto) {
        return base_registry::template get_impl<C, O, Args...>(_registry.get(), _entity);
    }

    /// @brief Get reference to component C, or insert it with provided arguments if not present.
    ///
    /// This function tries to access a component of type C associated with the specified entity.
    /// If the component does not exist, it is inserted by forwarding the provided arguments to the constructor of C.
    /// This ensures that the component is initialized according to the arguments passed. After insertion,
    /// the entity's archetype is updated to include this new component type, and all related entity locations are
    /// adjusted accordingly to reflect changes in the archetype structure.
    ///
    /// @tparam C Component type which must be constructible with the provided arguments.
    /// @param args Arguments to forward to the constructor of C if component C needs to be created.
    /// @return C& Reference to the component C.
    template<component C>
    [[nodiscard]] constexpr auto get_or_insert(auto&&... args) -> C& {
        auto [inserted, ptr] = _registry.get().set_impl<C>(_entity);
        if (inserted) {
            std::construct_at(ptr, std::forward<decltype(args)>(args)...);
        }
        return *ptr;
    }

    /// @brief Set component to an entity. It can either override a component value that is already assigned to an
    /// entity or it may construct a new once and assign to it.
    ///
    /// @note Such operation involves an archetype change which is a costly operation.
    ///
    /// @section Example
    /// @code {.cpp}
    /// auto entity = registry.create<position>({ 1, 2 });
    ///
    /// entity
    ///     .set<position>(10, 20)
    ///     .set<velocity>(3, 4);
    /// @endcode
    ///
    /// @tparam C Component type
    /// @tparam Args Parameter pack, argument types to construct C from
    /// @param args Arguments to construct C from
    /// @return entity_ref Returns this entity to allow method chaining.
    template<component C, typename... Args>
    constexpr auto set(Args&&... args) -> entity_ref {
        auto [inserted, ptr] = _registry.get().set_impl<C>(_entity);
        if (inserted) {
            std::construct_at(ptr, std::forward<Args>(args)...);
        } else {
            *ptr = C{ std::forward<Args>(args)... };
        }
        return *this;
    }

    /// @brief Remove component C from an entity. In case entity does not have component attached nothing is done
    /// and this method returns.
    ///
    /// @note Such operation involves an archetype change which is a costly operation.
    ///
    /// @section Example
    /// @code {.cpp}
    /// auto entity = registry.create<position>({ 1, 2 });
    ///
    /// entity.remove<position>(10, 20);
    /// @endcode
    ///
    /// @tparam C Component type
    /// @return entity_ref Returns this entity to allow method chaining.
    template<component C>
    constexpr auto remove() -> entity_ref {
        _registry.get().remove<C>(_entity);
        return *this;
    }

    /// @brief Destroys the current entity.
    ///
    /// This method is responsible for removing the entity from the registry.
    ///
    /// @code
    /// e.destroy();
    /// e.alive() // returns false
    /// @endcode
    void destroy() {
        _registry.get().destroy(_entity);
    }

    /// @brief Visit all components of an entity and apply a function to them.
    ///
    /// This function visits all components associated with an entity in the registry
    /// and applies the provided function to each component.
    ///
    /// @param func A callable object (function, lambda, etc.) that will be applied to each component.
    /// @section Example
    /// @code
    /// entity.visit([](const component_meta& meta, void* ptr) { std::cout << meta.type->name << '\n'; });
    /// @endcode
    constexpr auto visit(auto&& func) {
        _registry.get().visit(_entity, std::forward<decltype(func)>(func));
    }

    /// @brief Visit all components of an entity and apply a function to them (const version).
    ///
    /// This function visits all components associated with an entity in the registry
    /// and applies the provided function to each component. This version is const and
    /// can be used in const contexts.
    ///
    /// @param func A callable object (function, lambda, etc.) that will be applied to each component.
    /// @section Example
    /// @code
    /// entity.visit([](const component_meta& meta, const void* ptr) { std::cout << meta.type->name << '\n'; });
    /// @endcode
    constexpr auto visit(auto&& func) const {
        static_cast<const base_registry&>(_registry.get()).visit(_entity, std::forward<decltype(func)>(func));
    }

    /// @brief Creates a duplicate of the current entity.
    ///
    /// This method is used to clone the current entity, creating an exact copy with the same components and states,
    /// but under a new entity identifier.
    ///
    /// @note Components are copied using their copy constructor.
    /// @throws std::invalid_argument when copy constructor does not exist for one of components. Entity is left in
    /// an undefined state if this occurs.
    ///
    /// @return entity_ref A reference to the new entity, which is a duplicate of the original.
    ///
    /// @section Example
    /// @code
    /// // Assuming 'entity' is an existing entity_ref object
    /// auto cloned_entity = entity.clone();
    /// // 'cloned_entity' is now a new entity with the same components as 'entity'
    /// @endcode
    auto clone() const -> entity_ref {
        return entity_ref{ _registry, _registry.get().clone(_entity) };
    }

    /// @brief Clones the current entity to a placeholder entity.
    ///
    /// This method clones the current entity, creating a copy in a new placeholder entity within the same registry.
    ///
    /// @param placeholder The placeholder entity to clone into.
    /// @return The cloned entity reference.
    auto clone(placeholder_entity placeholder) {
        return entity_ref{ _registry, _registry.get().clone(_entity, placeholder) };
    }

    /// @brief Copies the current entity to another registry.
    ///
    /// This method copies the current entity to a specified destination registry, creating a new entity in the
    /// destination registry.
    ///
    /// @param destination The destination registry where the entity will be copied.
    /// @return The copied entity reference in the destination registry.
    auto copy(base_registry& destination) const -> entity_ref {
        return entity_ref{ destination, _registry.get().copy(_entity, destination) };
    }

    /// @brief Copies the current entity to another registry into a placeholder entity.
    ///
    /// This method copies the current entity to a specified destination registry, creating a new entity in the
    /// destination registry and associating it with a placeholder entity.
    ///
    /// @param destination The destination registry where the entity will be copied.
    /// @param placeholder The placeholder entity in the destination registry.
    /// @return The copied entity reference in the destination registry.
    auto copy(base_registry& destination, placeholder_entity placeholder) const {
        return entity_ref{ destination, _registry.get().copy(_entity, destination, placeholder) };
    }

    /// @brief Moves the current entity to another registry.
    ///
    /// This method moves the current entity to a specified destination registry, transferring ownership and creating a
    /// new entity in the destination registry.
    ///
    /// @param destination The destination registry where the entity will be moved.
    /// @return The moved entity reference in the destination registry.
    auto move(base_registry& destination) -> entity_ref {
        return entity_ref{ destination, _registry.get().move(_entity, destination) };
    }

    /// @brief Moves the current entity to another registry into a placeholder entity.
    ///
    /// This method moves the current entity to a specified destination registry, transferring ownership and associating
    /// it with a placeholder entity in the destination registry.
    ///
    /// @param destination The destination registry where the entity will be moved.
    /// @param placeholder The placeholder entity in the destination registry.
    /// @return The moved entity reference in the destination registry.
    auto move(base_registry& destination, placeholder_entity placeholder) {
        return entity_ref{ destination, _registry.get().move(_entity, destination, placeholder) };
    }

    /// @brief Converts this `entity_ref` to its underlying `entity`.
    ///
    /// This conversion operator allows `entity_ref` to be treated as its underlying `entity` type directly.
    /// Useful in contexts where the entity's identifier is needed without explicit method calls.
    ///
    /// @return entity The underlying entity associated with this `entity_ref`.
    [[nodiscard]] constexpr operator entity() const noexcept {
        return _entity;
    }

    /// @brief Checks if the underlying entity is valid.
    ///
    /// This conversion operator allows the `entity_ref` to be used in boolean contexts to check if the
    /// underlying entity is valid (i.e., has valid (id, generation) but may not be alive).
    ///
    /// @return bool True if the underlying entity is valid, false otherwise.
    [[nodiscard]] constexpr operator bool() const noexcept {
        return _entity.valid();
    }

private:
    std::reference_wrapper<base_registry> _registry;
    entity _entity;
};


/// @brief Represents a reference to an entity within a registry.
///
///
/// This class provides read only access to entity.
class const_entity_ref {
public:
    /// @brief Constructs a const entity reference for a specific entity in the given registry.
    ///
    /// @param registry Reference to the `base_registry` that manages the entity.
    /// @param entity The `entity` that this reference will encapsulate.
    constexpr const_entity_ref(const base_registry& registry, entity entity) : _entity(entity), _registry(registry) {
    }

    /// @brief Retrieves a constant reference to the archetype of the entity.
    ///
    /// This method returns a reference to the archetype associated with the encapsulated entity.
    /// An archetype defines the configuration of components that an entity possesses in the entity-component system.
    /// Accessing the archetype can be useful for inspecting the structure and component makeup of the entity.
    ///
    /// @return const archetype& A reference to the archetype of the entity.
    [[nodiscard]] constexpr auto archetype() const noexcept -> const archetype& {
        return *_registry.get().get_location(_entity).archetype;
    }

    /// @brief Checks if the entity is currently active in the registry.
    ///
    /// This method queries the registry to determine if the entity is still active and has not been destroyed.
    /// It is particularly useful for validity checks before performing operations on the entity.
    ///
    /// @return bool Returns true if the entity is active; otherwise, false.
    [[nodiscard]] constexpr auto alive() const noexcept -> bool {
        return _registry.get().alive(_entity);
    }

    /// @brief Determines if the entity has all specified components.
    ///
    /// This template method checks whether the entity has each of the components listed in the template arguments.
    /// This can be useful for conditional logic where actions depend on the presence of specific components
    /// in an entity. For example, an operation that should only be performed if an entity has both a `position`
    /// and `velocity` component.
    ///
    /// @tparam C The component types to check for.
    /// @return bool Returns true if the entity has all specified components; otherwise, false.
    template<component... C>
    [[nodiscard]] constexpr auto has() const noexcept -> bool {
        return _registry.get().template has<C...>(_entity);
    }

    /// @brief Retrieves a const-qualified component of type C from the entity in a read-only manner.
    ///
    /// This const method template is designed for read-only access to a specific component from the entity
    /// managed by the registry. It is used when the entity is expected to contain the component and modifications
    /// are not intended. If the requested component is not present, it throws a `component_not_found` exception.
    ///
    /// @tparam C The type of the component to retrieve, specified as a const-qualified reference.
    /// @return C A const reference to the requested component of the entity.
    /// @throws component_not_found If the requested component C is not found in the entity.
    template<component C>
    [[nodiscard]] constexpr auto get() const -> const C& {
        return std::get<0>(base_registry::template get_impl<C>(_registry.get(), _entity));
    }

    /// @brief Retrieves multiple components from the underlying registry for the current entity (const
    /// version).
    ///
    /// This method allows accessing multiple components associated with the current entity from the underlying registry
    /// in a const context.
    ///
    /// @tparam C The first component type to retrieve.
    /// @tparam O The second component type to retrieve.
    /// @tparam Args Additional component types to retrieve.
    /// @return A tuple containing the requested components.
    template<component C, component O, component... Args>
    [[nodiscard]] constexpr auto get() const -> decltype(auto) {
        return base_registry::template get_impl<C, O, Args...>(_registry.get(), _entity);
    }

    /// @brief Visit all components of an entity and apply a function to them (const version).
    ///
    /// This function visits all components associated with an entity in the registry
    /// and applies the provided function to each component. This version is const and
    /// can be used in const contexts.
    ///
    /// @param func A callable object (function, lambda, etc.) that will be applied to each component.
    /// @section Example
    /// @code
    /// entity.visit([](const component_meta& meta, const void* ptr) { std::cout << meta.type->name << '\n'; });
    /// @endcode
    constexpr auto visit(auto&& func) const {
        static_cast<const base_registry&>(_registry.get()).visit(_entity, std::forward<decltype(func)>(func));
    }

    /// @brief Copies the current entity to another registry.
    ///
    /// This method copies the current entity to a specified destination registry, creating a new entity in the
    /// destination registry.
    ///
    /// @param destination The destination registry where the entity will be copied.
    /// @return The copied entity reference in the destination registry.
    auto copy(base_registry& destination) const -> entity_ref {
        return entity_ref{ destination, _registry.get().copy(_entity, destination) };
    }

    /// @brief Copies the current entity to another registry into a placeholder entity.
    ///
    /// This method copies the current entity to a specified destination registry, creating a new entity in the
    /// destination registry and associating it with a placeholder entity.
    ///
    /// @param destination The destination registry where the entity will be copied.
    /// @param placeholder The placeholder entity in the destination registry.
    /// @return The copied entity reference in the destination registry.
    auto copy(base_registry& destination, placeholder_entity placeholder) const {
        return entity_ref{ destination, _registry.get().copy(_entity, destination, placeholder) };
    }

    /// @brief Converts this `const_entity_ref` to its underlying `entity`.
    ///
    /// This conversion operator allows `const_entity_ref` to be treated as its underlying `entity` type directly.
    /// Useful in contexts where the entity's identifier is needed without explicit method calls.
    ///
    /// @return entity The underlying entity associated with this `const_entity_ref`.
    [[nodiscard]] constexpr operator entity() const noexcept {
        return _entity;
    }

    /// @brief Checks if the underlying entity is valid.
    ///
    /// This conversion operator allows the `entity_ref` to be used in boolean contexts to check if the
    /// underlying entity is valid (i.e., has valid (id, generation) but may not be alive).
    ///
    /// @return bool True if the underlying entity is valid, false otherwise.
    [[nodiscard]] constexpr operator bool() const noexcept {
        return _entity.valid();
    }

private:
    std::reference_wrapper<const base_registry> _registry;
    entity _entity;
};

} // namespace co_ecs
