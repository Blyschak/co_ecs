#pragma once

#include <co_ecs/system/system.hpp>

// Helper macro to construct a named system from a function
#define CO_ECS_MAKE_NAMED_SYSTEM(func) \
    std::make_unique<::co_ecs::named_system<decltype(std::function{ func })>>(func, #func)

namespace co_ecs {

/// @brief System implementation class for generic F function-like type with it's name
///
/// @tparam F Callable type
template<typename F>
class named_system : public system_interface {
public:
    /// @brief Construct a new named system object
    ///
    /// @param func Function object
    explicit named_system(F func, const char* name) : _func(std::move(func)), _name(name) {
    }

    /// @brief Create an executor object
    ///
    /// @param registry Registry reference
    /// @param user_context User provided context to fetch data from and provide to the system
    /// @return Executor object
    std::unique_ptr<system_executor_interface> create_executor(registry& registry, void* user_context) override {
        return std::make_unique<system_executor<F>>(registry, user_context, _func, _name);
    }

private:
    F _func;
    std::string_view _name;
};

} // namespace co_ecs
