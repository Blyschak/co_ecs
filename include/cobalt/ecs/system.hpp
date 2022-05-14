#pragma once

#include <cobalt/asl/type_traits.hpp>
#include <cobalt/ecs/commands.hpp>
#include <cobalt/ecs/registry.hpp>

#include <tuple>

namespace cobalt::ecs {

/// @brief System executor interface, a system is a type that implements run() method
class system_executor_interface {
public:
    /// @brief Execute system logic
    virtual void run() = 0;

    /// @brief Run deferred system logic
    virtual void run_deferred() = 0;
};

/// @brief System interface
class system_interface {
public:
    /// @brief Create a system executor object
    ///
    /// @param registry Registry
    /// @return std::unique_ptr<system_executor_interface>
    virtual std::unique_ptr<system_executor_interface> create_executor(registry& registry) = 0;
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

    /// @brief Run deferred logic
    void run_deferred() const noexcept {
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

    /// @brief Run deferred logic
    void run_deferred() const noexcept {
    }

private:
    resource_type& _resource;
};

/// @brief System commands state, creates a command queue to be passed to the system
class system_commands_state {
public:
    /// @brief Construct a new system commands state object
    ///
    /// @param registry Registry reference
    system_commands_state(registry& registry) : _registry(registry) {
    }

    /// @brief Returns the command queue
    ///
    /// @return commands&
    [[nodiscard]] command_queue& get() noexcept {
        return _commands;
    }

    /// @brief Run deferred logic, flushes commands from command queue
    void run_deferred() {
        _commands.execute(_registry);
    }

private:
    registry& _registry;
    command_queue _commands;
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

/// @brief Specialization for command_queue&
template<>
class system_argument_state_trait<command_queue&> {
public:
    /// @brief Actual state type for T
    using state_type = system_commands_state;
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

/// @brief System executor implementation class for generic F function-like type
///
/// @tparam F Callable type
template<typename F>
class system_executor : public system_executor_interface {
public:
    /// @brief System function arguments
    using system_arguments = typename asl::function_traits<F>::arguments_tuple_type;

    /// @brief Construct a new system object
    ///
    /// @param registry Registry reference
    /// @param func Function object
    explicit system_executor(registry& registry, F func) :
        _registry(registry), _func(std::move(func)), _state(_registry) {
    }

    /// @brief Execute system
    void run() override {
        std::apply([this](auto&&... args) { _func(args.get()...); }, _state.get());
    }

    /// @brief Run deferred logic
    void run_deferred() override {
        std::apply([this](auto&&... args) { (..., args.run_deferred()); }, _state.get());
    }

private:
    registry& _registry;
    F _func;
    system_state_trait<system_arguments>::type _state;
};

/// @brief System implementation class for generic F function-like type
///
/// @tparam F Callable type
template<typename F>
class system : public system_interface {
public:
    /// @brief Construct a new system object
    ///
    /// @param func Function object
    explicit system(F func) : _func(std::move(func)) {
    }

    /// @brief Create an executor object
    ///
    /// @param registry Registry reference
    /// @return std::unique_ptr<system_executor_interface>
    std::unique_ptr<system_executor_interface> create_executor(registry& registry) override {
        return std::make_unique<system_executor<F>>(registry, std::move(_func));
    }

private:
    F _func;
};

} // namespace cobalt::ecs