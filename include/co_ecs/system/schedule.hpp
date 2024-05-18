#pragma once

#include <co_ecs/registry.hpp>
#include <co_ecs/thread_pool/thread_pool.hpp>

#include <co_ecs/system/stage.hpp>
#include <co_ecs/system/system.hpp>

namespace co_ecs {

class schedule_executor;

/// @class schedule
/// @brief Manages the execution of systems in different stages.
class schedule {
private:
    using self_type = schedule;

public:
    /// @brief Adds an initialization system to the initial stage.
    ///
    /// This function adds a system to the initialization stage, which is executed before other stages.
    ///
    /// @param args Arguments for the system to be added.
    /// @return Reference to this schedule object.
    template<typename... Args>
    auto add_init_system(Args&&... args) -> self_type& {
        _init_stage.add_system(std::forward<Args>(args)...);
        return *this;
    }

    /// @brief Begins a new stage in the schedule.
    ///
    /// This function creates a new stage with the given name and adds it to the schedule.
    ///
    /// @param name Name of the stage to be created.
    /// @return Reference to the newly created stage.
    auto begin_stage(std::string_view name = {}) -> stage& {
        return _stages.emplace_back(*this, name);
    }

    /// @brief Creates an executor for the schedule.
    ///
    /// This function creates a schedule executor, which is responsible for running the stages of the schedule.
    ///
    /// @param registry Reference to the registry object.
    /// @param user_context Optional user context.
    /// @return Unique pointer to the created schedule executor.
    auto create_executor(registry& registry, void* user_context = nullptr) -> std::unique_ptr<schedule_executor> {
        std::vector<std::unique_ptr<stage_executor>> stage_executors;
        for (auto& stage : _stages) {
            stage_executors.emplace_back(stage.create_executor(registry, user_context));
        }

        return std::make_unique<schedule_executor>(
            registry, std::move(stage_executors), _init_stage.create_executor(registry, user_context));
    }

private:
    stage _init_stage{ *this };
    std::vector<stage> _stages;
};

/// @class schedule_executor
/// @brief Executes the schedule by running all stages.
class schedule_executor {
public:
    /// @brief Constructs a schedule executor.
    ///
    /// This constructor initializes the schedule executor with the given registry, stages, and initial stage.
    /// It runs the initial stage and flushes the command buffer.
    ///
    /// @param registry Reference to the registry object.
    /// @param stages Vector of unique pointers to stage executors.
    /// @param init Unique pointer to the initial stage executor.
    schedule_executor(registry& registry,
        std::vector<std::unique_ptr<stage_executor>> stages,
        std::unique_ptr<stage_executor> init) :
        _registry(registry), _stages(std::move(stages)), _init(std::move(init)) {
        // Run initial systems
        _init->run();

        // Flush commands
        command_buffer::flush(_registry);
    }

    /// @brief Executes the schedule once.
    ///
    /// This function runs all stages in the schedule and then flushes the command buffer.
    void run_once() {
        for (auto& stage : _stages) {
            stage->run();
        }

        // Flush commands
        command_buffer::flush(_registry);
    }

private:
    registry& _registry;
    std::vector<std::unique_ptr<stage_executor>> _stages;
    std::unique_ptr<stage_executor> _init;
};

} // namespace co_ecs