#pragma once

#include <co_ecs/registry.hpp>
#include <co_ecs/system.hpp>
#include <co_ecs/thread_pool/thread_pool.hpp>

#include <latch>

namespace co_ecs::experimental {

/// @brief Schedule of systems
class schedule {
public:
    using self_type = schedule;

    /// @brief Add system to schedule
    ///
    /// @tparam F System function type
    /// @param func System function
    /// @return self_type&
    template<typename F>
    self_type& add_system(F&& func) {
        if (_systems.empty())
            _systems.emplace_back();
        _systems.back().emplace_back(std::make_unique<system<F>>(std::forward<F>(func)));
        return *this;
    }

    /// @brief Barrier
    ///
    /// @return self_type&
    self_type& barrier() {
        _systems.emplace_back();
        return *this;
    }

private:
    friend class executor;

private:
    std::vector<std::vector<std::unique_ptr<system_interface>>> _systems{};
};

/// @brief Executes the scehdule
class executor {
public:
    /// @brief Construct an executor
    /// @param schedule Schedule
    /// @param registry Registry
    /// @param user_context User provided context
    executor(schedule& schedule, registry& registry, void* user_context = nullptr) : _registry(registry) {
        _system_executors.reserve(schedule._systems.size());

        for (auto& system_set : schedule._systems) {
            _system_executors.emplace_back();
            _system_executors.back().reserve(system_set.size());
            for (auto& system : system_set) {
                _system_executors.back().push_back(system->create_executor(registry, user_context));
            }
        }
    }

    /// @brief Exeucte schedule once
    void run_once() {
        for (auto& executor_set : _system_executors) {
            execute_batch(executor_set);
        }

        _registry.flush_reserved();

        // flush commands
        for (auto i = 0; i < _thread_pool.num_workers(); i++) {
            _thread_pool.get_worker_by_id(i).get_command_buffer().flush(_registry);
        }
    }

private:
    void execute_batch(auto&& work_batch) {
        auto* parent = _thread_pool.allocate([]() {});

        for (auto& work_item : work_batch) {
            auto* task = _thread_pool.allocate([&work_item]() { work_item->run(); }, parent);

            _thread_pool.submit(task);
        }

        _thread_pool.submit(parent);
        _thread_pool.wait(parent);
    }

private:
    thread_pool _thread_pool;
    registry& _registry;
    std::vector<std::vector<std::unique_ptr<system_executor_interface>>> _system_executors;
};

} // namespace co_ecs::experimental