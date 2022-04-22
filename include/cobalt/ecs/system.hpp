#pragma once

#include <cobalt/asl/type_traits.hpp>
#include <cobalt/ecs/registry.hpp>

#include <tuple>

namespace cobalt::ecs {

/// @brief System interface, a system is a type that implements run() method
class system_interface {
public:
    /// @brief Execute system logic
    virtual void run() = 0;
};

/// @brief System view state, pre-creates a view object needed for components iteration
///
/// @tparam view_t View type
template<typename view_t>
class system_view_state {
public:
    /// @brief Construct a new system view state object
    ///
    /// @param registry Registry reference
    explicit system_view_state(registry& registry) noexcept : _view(registry) {
    }

    /// @brief Returns the actual state inside to pass to the system
    ///
    /// @return view_t&
    [[nodiscard]] view_t& get() noexcept {
        return _view;
    }

private:
    view_t _view;
};

/// @brief System resource state, pre-fetches resource to be passed to the system
///
/// @tparam T Resource type
template<typename T>
class system_resource_state {
public:
    /// @brief Get the actual resource type
    using resource_type = std::remove_cvref_t<T>;

    /// @brief Construct a new system resource state object
    ///
    /// @param registry Registry reference
    explicit system_resource_state(registry& registry) noexcept : _resource(registry.get_resource<resource_type>()) {
    }

    /// @brief Returns the actual state inside to pass to the system
    ///
    /// @return resource_type&
    [[nodiscard]] resource_type& get() noexcept {
        return _resource;
    }

private:
    resource_type& _resource;
};

/// @brief System argument state trait primary template trait. Helps to get corresponding state class type based on the
/// input type
///
/// @tparam T Input type
template<typename T>
class system_argument_state_trait;

/// @brief Specialization for resource reference types
///
/// @tparam T Resource reference type
template<typename T>
class system_argument_state_trait {
public:
    /// @brief Actual state type for T
    using state_type = system_resource_state<T>;
};

/// @brief Specialization for ecs::view<Args...>
///
/// @tparam Args View argument types
template<component_reference... Args>
class system_argument_state_trait<view<Args...>> {
public:
    /// @brief Actual state type for T
    using state_type = system_view_state<view<Args...>>;
};

/// @brief System state for Args...
///
/// @tparam Args System arguments
template<typename... Args>
class system_state {
public:
    /// @brief A tuple of system states per argument
    using system_state_tuple = std::tuple<typename system_argument_state_trait<Args>::state_type...>;

    /// @brief Construct a new system state object
    ///
    /// @param registry Registry reference
    explicit system_state(registry& registry) :
        _state(typename system_argument_state_trait<Args>::state_type(registry)...){};

    /// @brief Default copy constructor
    ///
    /// @param rhs Right hand side system_state
    system_state(const system_state& rhs) = default;

    /// @brief Default copy assignment operator
    ///
    /// @param rhs Right hand side system_state
    system_state& operator=(const system_state& rhs) = default;

    /// @brief Default move constructor
    ///
    /// @param rhs Right hand side system_state
    system_state(system_state&& rhs) = default;

    /// @brief Default move assignment operator
    ///
    /// @param rhs Right hand side system_state
    system_state& operator=(system_state&& rhs) = default;

    /// @brief Get the tuple of states per argument
    ///
    /// @return system_state_tuple&
    [[nodiscard]] system_state_tuple& get() noexcept {
        return _state;
    }

private:
    system_state_tuple _state;
};

/// @brief System state trait helper class, helps to get system_state implementation based on T
///
/// @tparam T Type
template<typename T>
struct system_state_trait;

/// @brief System state trait helper class, helps to get system_state implementation based on std::tuple<Ts...>
///
/// @tparam Ts Types
template<typename... Ts>
struct system_state_trait<std::tuple<Ts...>> {
    using type = system_state<Ts...>;
};

/// @brief System implementation class for generic F function-like type
///
/// @tparam F Callable type
template<typename F>
class system : public system_interface {
public:
    /// @brief System function arguments
    using system_arguments = typename asl::function_traits<F>::arguments_tuple_type;

    /// @brief Construct a new system object
    ///
    /// @param registry Registry reference
    /// @param func Function object
    explicit system(registry& registry, F func) : _registry(registry), _func(std::move(func)), _state(_registry) {
    }

    /// @brief Default copy constructor
    ///
    /// @param rhs Right hand side system_state
    system(const system& rhs) = default;

    /// @brief Default copy assignment operator
    ///
    /// @param rhs Right hand side system_state
    system& operator=(const system& rhs) = default;

    /// @brief Default move constructor
    ///
    /// @param rhs Right hand side system_state
    system(system&& rhs) = default;

    /// @brief Default move assignment operator
    ///
    /// @param rhs Right hand side system_state
    system& operator=(system&& rhs) = default;

    /// @brief Execute system
    void run() override {
        std::apply([this](auto&&... args) { _func(args.get()...); }, _state.get());
    }

private:
    registry& _registry;
    F _func;
    system_state_trait<system_arguments>::type _state;
};

} // namespace cobalt::ecs