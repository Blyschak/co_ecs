#pragma once

#include <co_ecs/system/system.hpp>

namespace co_ecs {

/// @brief Vector of systems.
///
/// This type alias defines a vector of unique pointers to system executor interfaces.
using vector_of_executors_t = std::vector<std::unique_ptr<system_executor_interface>>;

class stage_executor;
class schedule;

/// @brief Structure representing the main thread execution policy.
///
/// This structure is used to indicate that a system should be executed on the main thread.
struct main_thread_execution_policty_t {
    /// @brief Default constructor.
    explicit main_thread_execution_policty_t() = default;
};

/// @brief Constant instance of the main thread execution policy.
inline constexpr main_thread_execution_policty_t main_thread_execution_policy{};

/// @brief Class representing a stage in the schedule.
///
/// A stage contains systems that are executed in sequence.
class stage {
private:
    using self_type = stage; ///< Type alias for the stage class.

public:
    /// @brief Constructs a new stage object.
    ///
    /// @param schedule Reference to the schedule object.
    /// @param name Name of the stage.
    stage(schedule& schedule, std::string_view name = {}) : _schedule(schedule), _name(name) {
    }

    /// @brief Ends the current stage and returns the schedule.
    ///
    /// @return Reference to the schedule.
    auto end_stage() noexcept -> schedule& {
        return _schedule;
    }

    /// @brief Adds a system to the main thread execution list.
    ///
    /// @param policy The main thread execution policy.
    /// @param args Arguments for the system to be added.
    /// @return Reference to this stage object.
    auto add_system(main_thread_execution_policty_t policy, auto&&... args) -> self_type& {
        _main_thread_systems.emplace_back(into_system_interface(std::forward<decltype(args)>(args)...));
        return *this;
    }

    /// @brief Adds a system to the stage.
    ///
    /// @param args Arguments for the system to be added.
    /// @return Reference to this stage object.
    auto add_system(auto&&... args) -> self_type& {
        _systems.emplace_back(into_system_interface(std::forward<decltype(args)>(args)...));
        return *this;
    }

    /// @brief Creates an executor for the stage.
    ///
    /// This function creates a stage executor, which is responsible for running the systems in the stage.
    ///
    /// @param registry Reference to the registry object.
    /// @param user_context Optional user context.
    /// @return Unique pointer to the created stage executor.
    auto create_executor(registry& registry, void* user_context = nullptr) -> std::unique_ptr<stage_executor> {
        std::vector<vector_of_executors_t> executors{};

        while (!_systems.empty()) {
            access_pattern_t access_pattern;
            vector_of_executors_t& executor_set = executors.emplace_back();

            for (size_t i = 0; i < _systems.size();) {
                auto executor = _systems[i]->create_executor(registry, user_context);
                auto system_access_pattern = executor->access_pattern();

                if (!access_pattern.allows(system_access_pattern)) {
                    ++i;
                    continue;
                }

                access_pattern &= system_access_pattern;
                executor_set.emplace_back(std::move(executor));
                _systems.erase(_systems.begin() + i);
            }
        }

        // Main thread systems
        vector_of_executors_t main_thread_executors;
        for (auto& system : _main_thread_systems) {
            main_thread_executors.emplace_back(system->create_executor(registry, user_context));
        }

        return std::make_unique<stage_executor>(_name, std::move(executors), std::move(main_thread_executors));
    }

private:
    /// @brief Converts a callable object to a system interface.
    ///
    /// @tparam F Type of the callable object.
    /// @param func The callable object.
    /// @return Unique pointer to the system interface.
    template<typename F>
    static auto into_system_interface(F&& func) -> std::unique_ptr<system_interface> {
        return std::make_unique<system<F>>(std::forward<F>(func));
    }

    /// @brief Converts a unique pointer to a system to a system interface.
    ///
    /// @tparam S Type of the system.
    /// @param system Unique pointer to the system.
    /// @return Unique pointer to the system interface.
    template<typename S>
    static auto into_system_interface(std::unique_ptr<S> system) -> std::unique_ptr<system_interface> {
        return system;
    }

private:
    schedule& _schedule;
    std::string_view _name;
    std::vector<std::unique_ptr<system_interface>> _systems{};
    std::vector<std::unique_ptr<system_interface>> _main_thread_systems{};
};

/// @brief Class representing a stage executor.
///
/// A stage executor is responsible for running all systems in a stage.
class stage_executor {
public:
    using builder = stage; ///< Type alias for the stage builder.

    /// @brief Constructs a new stage executor.
    ///
    /// @param name Name of the stage.
    /// @param executor_set Set of system executors.
    /// @param main_thread_executors Main thread system executors.
    explicit stage_executor(std::string_view name,
        std::vector<vector_of_executors_t> executor_set,
        vector_of_executors_t main_thread_executors) :
        _executor_set(std::move(executor_set)), _main_thread_executors(std::move(main_thread_executors)), _name(name) {
    }

    /// @brief Runs all systems in the stage.
    void run() {
        for (auto& executors : _executor_set) {
            execute_batch(executors);
        }
    }

private:
    /// @brief Executes a batch of systems.
    ///
    /// @tparam WorkBatch Type of the work batch.
    /// @param work_batch The batch of work to be executed.
    template<typename WorkBatch>
    void execute_batch(WorkBatch&& work_batch) {
        task_t* parent{};
        thread_pool& _thread_pool = thread_pool::get();

        for (auto& work_item : work_batch) {
            auto task = _thread_pool.submit([&work_item]() { work_item->run(); }, parent);
            if (!parent) {
                parent = task;
            }
        }

        // in the meantime execute main thread systems
        for (auto& executor : _main_thread_executors) {
            executor->run();
        }

        if (parent) {
            _thread_pool.wait(parent);
        }
    }

private:
    std::vector<vector_of_executors_t> _executor_set;
    vector_of_executors_t _main_thread_executors;
    std::string_view _name;
};


} // namespace co_ecs
