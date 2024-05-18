#pragma once

#include <co_ecs/command.hpp>
#include <co_ecs/registry.hpp>
#include <co_ecs/thread_pool/thread_pool.hpp>
#include <co_ecs/view.hpp>

#include <co_ecs/system/access.hpp>

namespace co_ecs {

/// @brief System executor interface, a system is a type that implements run() method
class system_executor_interface {
public:
    /// @brief Destroy the system executor interface object
    virtual ~system_executor_interface() = default;

    /// @brief Execute system logic
    virtual void run() = 0;

    /// @brief Get the name of the entity.
    ///
    /// @return The name of the entity.
    virtual auto name() const -> std::string_view = 0;

    /// @brief Get the type name of the entity.
    ///
    /// @return The type name of the entity.
    virtual auto type_name() const -> std::string_view = 0;

    /// @brief Get the access pattern of the entity.
    ///
    /// @return The access pattern of the entity.
    virtual auto access_pattern() const -> access_pattern_t = 0;
};

/// @brief System interface
class system_interface {
public:
    /// @brief Destroy the system interface object
    virtual ~system_interface() = default;

    /// @brief Create a system executor object
    ///
    /// @param registry Registry
    /// @param user_context User provided context to fetch data from and provide to the system
    /// @return std::unique_ptr<system_executor_interface>
    virtual std::unique_ptr<system_executor_interface> create_executor(registry& registry, void* user_context) = 0;
};

/// @cond TURN_OFF_DOXYGEN

/// @brief System argument state trait primary template trait. Helps to get corresponding state class type based on the
/// input type
///
/// @tparam T Input type
template<typename T>
class system_argument_state_trait;

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
    /// @param user_context User provided context to fetch data from and provide to the system
    explicit system_state(registry& registry, void* user_context) :
        _state(typename system_argument_state_trait<Args>::state_type(registry, user_context)...){};

    /// @brief Get the tuple of states per argument
    ///
    /// @return system_state_tuple&
    [[nodiscard]] system_state_tuple& get() noexcept {
        return _state;
    }

    /// @brief Get the access pattern of the entity.
    ///
    /// @return The access pattern of the entity.
    auto access_pattern() const -> access_pattern_t {
        access_pattern_t pattern;
        if constexpr (0 < std::tuple_size_v<system_state_tuple>) {
            pattern &= std::apply([](auto&&... args) { return (args.access_pattern() & ...); }, _state);
        }
        return pattern;
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
    using system_arguments = typename detail::function_traits<F>::arguments_tuple_type;

    /// @brief Construct a new system object
    ///
    /// @param registry Registry reference
    /// @param func Function object
    explicit system_executor(registry& registry, void* user_context, F func, std::string_view name = {}) :
        _func(std::move(func)), _state(registry, user_context), _name(name) {
    }

    /// @brief Execute system
    void run() override {
        std::apply([this](auto&&... args) { _func(args.get()...); }, _state.get());
    }

    /// @brief Get the name of the entity.
    ///
    /// @return The name of the entity.
    auto name() const -> std::string_view override {
        return _name;
    }

    /// @brief Get the type name of the entity.
    ///
    /// @return The type name of the entity.
    auto type_name() const -> std::string_view override {
        return co_ecs::type_name<F>();
    }

    /// @brief Get the access pattern of the entity.
    ///
    /// @return The access pattern of the entity.
    auto access_pattern() const -> access_pattern_t override {
        return _state.access_pattern();
    }

private:
    F _func;
    typename system_state_trait<system_arguments>::type _state;
    std::string_view _name;
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
    /// @param user_context User provided context to fetch data from and provide to the system
    /// @return std::unique_ptr<system_executor_interface>
    std::unique_ptr<system_executor_interface> create_executor(registry& registry, void* user_context) override {
        return std::make_unique<system_executor<F>>(registry, user_context, _func);
    }

private:
    F _func;
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

    /// @brief Get the access pattern of the entity.
    ///
    /// @return The access pattern of the entity.
    auto access_pattern() const -> access_pattern_t {
        return access_pattern_t(access_type::write);
    }

private:
    registry& _registry;
};

/// @brief Specialization for registry
template<>
class system_argument_state_trait<registry&> {
public:
    /// @brief Actual state type for T
    using state_type = system_registry_state;
};

/// @brief Registry system state
///
class system_const_registry_state {
public:
    /// @brief Construct a new system registry state object
    ///
    /// @param registry Registry reference
    /// @param user_context User provided context to fetch data from and provide to the system
    explicit system_const_registry_state(registry& registry, void* user_context) noexcept : _registry(registry) {
    }

    /// @brief Returns the actual state inside to pass to the system
    ///
    /// @return Registry
    [[nodiscard]] const registry& get() noexcept {
        return _registry;
    }

    /// @brief Get the access pattern of the entity.
    ///
    /// @return The access pattern of the entity.
    auto access_pattern() const -> access_pattern_t {
        return access_pattern_t(access_type::read);
    }

private:
    const registry& _registry;
};

/// @brief Specialization for registry
template<>
class system_argument_state_trait<const registry&> {
public:
    /// @brief Actual state type for T
    using state_type = system_registry_state;
};

/// @brief System command writer state
///
class system_command_writer_state {
public:
    /// @brief Constructor
    ///
    /// @param registry Registry reference
    /// @param user_context User provided context to fetch data from and provide to the system
    explicit system_command_writer_state(registry& registry, void* user_context) noexcept : _registry(registry) {
    }

    /// @brief Returns the actual state inside to pass to the system
    ///
    /// @return Command buffer
    [[nodiscard]] command_writer get() noexcept {
        return command_writer(_registry);
    }

    /// @brief Get the access pattern of the entity.
    ///
    /// @return The access pattern of the entity.
    auto access_pattern() const -> access_pattern_t {
        return access_pattern_t{};
    }

private:
    registry& _registry;
};

/// @brief Specialization for command_buffer
template<>
class system_argument_state_trait<command_writer> {
public:
    /// @brief Actual state type for T
    using state_type = system_command_writer_state;
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
    /// @param user_context User provided context to fetch data from and provide to the system
    explicit system_view_state(registry& registry, void* user_context) noexcept : _view(registry) {
    }

    /// @brief Returns the actual state inside to pass to the system
    ///
    /// @return View object
    [[nodiscard]] view_t& get() noexcept {
        return _view;
    }

private:
    template<typename T>
    struct access_pattern_helper {};

    template<component_reference... Args>
    struct access_pattern_helper<view<Args...>> {
        static auto access_pattern() -> access_pattern_t {
            return (access_pattern_t(
                        std::is_const_v<std::remove_reference_t<Args>> ? access_type::read : access_type::write,
                        component_meta::of<decay_component_t<Args>>())
                    & ...);
        }
    };

public:
    /// @brief Get the access pattern of the entity.
    ///
    /// @return The access pattern of the entity.
    auto access_pattern() const -> access_pattern_t {
        return access_pattern_helper<view_t>::access_pattern();
    }

private:
    view_t _view;
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

/// @endcond TURN_OFF_DOXYGEN

} // namespace co_ecs
