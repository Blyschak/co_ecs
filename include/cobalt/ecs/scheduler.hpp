#pragma once

#include <cobalt/ecs/registry.hpp>
#include <cobalt/ecs/system.hpp>

#include <vector>

namespace cobalt::ecs {

/// @brief Simple scheduler for systems. Systems are executed in the order they are added
class simple_scheduler {
public:
    using self_type = simple_scheduler;

    /// @brief Construct a new simple scheduler object
    ///
    /// @param registry Registry reference
    explicit simple_scheduler(registry& registry) noexcept : _registry(registry) {
    }

    /// @brief Run systems
    void run() {
        for (auto& system : _systems) {
            system->run();
        }
    }

    /// @brief Add system to schedule
    ///
    /// @tparam F System function type
    /// @param func System function
    /// @return self_type&
    template<typename F>
    self_type& add_system(F&& func) {
        _systems.emplace_back(std::make_unique<system<F>>(_registry, std::move(func)));
        return *this;
    }

private:
    registry& _registry;
    std::vector<std::unique_ptr<system_interface>> _systems;
};

} // namespace cobalt::ecs