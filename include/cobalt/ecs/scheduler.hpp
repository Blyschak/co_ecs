#pragma once

#include <cobalt/ecs/registry.hpp>
#include <cobalt/ecs/system.hpp>

#include <vector>

namespace cobalt::ecs {

/// @brief Used as resource to let scheduler exit
struct scheduler_control {
    bool should_exit{ false };
};

/// @brief Simple scheduler for systems. Systems are executed in the order they are added
class simple_scheduler {
public:
    using self_type = simple_scheduler;

    /// @brief Construct a new simple scheduler object
    ///
    /// @param registry Registry reference
    explicit simple_scheduler(registry& registry) noexcept :
        _registry(registry), _ctrl(registry.set_resource<scheduler_control>()) {
    }

    /// @brief Run systems
    void run() {
        // run init systems
        init();

        auto& ctrl = _registry.get_resource<scheduler_control>();

        // run till exit
        while (!ctrl.should_exit) {
            for (auto& executor : _executors) {
                executor->run();
            }
            for (auto& executor : _executors) {
                executor->run_deferred();
            }
            _registry.flush_event_queues();
        }
    }

    /// @brief Run init systems and execute update once
    void init() {
        // clear previous executors if any
        _init_executors.clear();
        _executors.clear();

        // run init systems
        for (const auto& system : _init_systems) {
            auto executor = system->create_executor(_registry);
            executor->run();
            // run deferred task from init system right away since next init system may be dependent on the result of
            // deferred tasks
            executor->run_deferred();
            _init_executors.emplace_back(std::move(executor));
        }

        _registry.flush_event_queues();

        // run update systems once first time and cache executors
        for (const auto& system : _systems) {
            auto executor = system->create_executor(_registry);
            executor->run();
            _executors.emplace_back(std::move(executor));
        }

        for (auto& executor : _executors) {
            executor->run_deferred();
        }

        _registry.flush_event_queues();
    }

    /// @brief Add system to schedule
    ///
    /// @tparam F System function type
    /// @param func System function
    /// @return self_type&
    template<typename F>
    self_type& add_system(F&& func) {
        _systems.emplace_back(std::make_unique<system<F>>(std::move(func)));
        return *this;
    }

    /// @brief Add init system to scheduler
    ///
    /// @tparam F System function type
    /// @param func Init system function
    /// @return self_type&
    template<typename F>
    self_type& add_init_system(F&& func) {
        _init_systems.emplace_back(std::make_unique<system<F>>(std::move(func)));
        return *this;
    }

private:
    registry& _registry;
    scheduler_control& _ctrl;
    std::vector<std::unique_ptr<system_interface>> _init_systems;
    std::vector<std::unique_ptr<system_executor_interface>> _init_executors;
    std::vector<std::unique_ptr<system_interface>> _systems;
    std::vector<std::unique_ptr<system_executor_interface>> _executors;
};

} // namespace cobalt::ecs