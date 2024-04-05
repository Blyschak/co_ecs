#pragma once

#include <co_ecs/registry.hpp>
#include <co_ecs/system.hpp>
#include <co_ecs/thread_pool/thread_pool.hpp>

#include <latch>

namespace co_ecs::experimental {

/// @brief Serial executor runs systems sequentially
class serial_executor {
public:
    /// @brief Execute system set
    /// @param work_batch System set
    void execute_batch(auto&& work_batch) {
        for (auto& work_item : work_batch) {
            work_item->run();
        }
    }
};

/// @brief Parallel executor
class parallel_executor {
public:
    /// @brief Execute system set
    /// @param work_batch System set
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
};

/// @brief Simple scheduler for systems. Systems are executed in the order they are added
class scheduler {
public:
    using self_type = scheduler;

    /// @brief Run systems
    ///
    /// @param registry
    /// @param exit_flag
    /// @param user_context
    template<typename Executor>
    void run(co_ecs::registry& registry, bool& exit_flag, void* user_context = nullptr) {
        std::vector<std::vector<std::unique_ptr<system_executor_interface>>> executors;
        executors.reserve(_systems.size());

        for (auto& system_set : _systems) {
            executors.emplace_back();
            executors.back().reserve(system_set.size());
            for (auto& system : system_set) {
                executors.back().push_back(system->create_executor(registry, user_context));
            }
        }

        Executor executor;

        while (!exit_flag) {
            for (auto& executor_set : executors) {
                executor.execute_batch(executor_set);
            }
        }
    }

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
    std::vector<std::vector<std::unique_ptr<system_interface>>> _systems{};
};

} // namespace co_ecs::experimental