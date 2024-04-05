#pragma once

namespace co_ecs {

/// @brief System executor interface, a system is a type that implements run() method
class system_executor_interface {
public:
    /// @brief Destroy the system executor interface object
    virtual ~system_executor_interface() = default;

    /// @brief Execute system logic
    virtual void run() = 0;

    /// @brief Run deferred system logic
    virtual void run_deferred() = 0;
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
    explicit system_executor(registry& registry, void* user_context, F func) :
        _func(std::move(func)), _state(registry, user_context) {
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
    F _func;
    typename system_state_trait<system_arguments>::type _state;
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
        return std::make_unique<system_executor<F>>(registry, user_context, std::move(_func));
    }

private:
    F _func;
};

} // namespace co_ecs
