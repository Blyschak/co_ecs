#pragma once

#include <array>
#include <atomic>
#include <functional>

namespace co_ecs {

/// @brief Represents a task that can be executed, monitored for completion, and linked to a parent task.
class task_t {
public:
    /// @brief Constructs an empty task.
    task_t() = default;

    /// @brief Constructs a task from a callable function and optionally links it to a parent task.
    /// @param func A callable object to be executed as the task.
    /// @param parent Optional pointer to a parent task, defaults to nullptr if no parent is specified.
    task_t(auto&& func, task_t* parent = nullptr) : _func(std::forward<decltype(func)>(func)), _parent(parent) {
        _unfinishedTasks.store(1, std::memory_order::relaxed);
        if (_parent) {
            _parent->_unfinishedTasks.fetch_add(1, std::memory_order::relaxed);
        }
    }

    /// @brief Executes the task's function and marks it as completed.
    void execute() {
        _func();
        finish();
    }

    /// @brief Checks if the task has been completed.
    /// @return True if the task is completed, otherwise false.
    bool is_completed() const noexcept {
        return _unfinishedTasks.load(std::memory_order::relaxed) == 0;
    }

    /// @brief Retrieves the parent task if it exists.
    /// @return Pointer to the parent task, or nullptr if there is no parent.
    task_t* parent() const noexcept {
        return _parent;
    }

private:
    void finish() {
        _unfinishedTasks.fetch_sub(1, std::memory_order::relaxed);

        if (is_completed() && _parent) {
            _parent->finish();
        }
    }

    std::function<void()> _func{};          ///< The function that the task executes.
    task_t* _parent{};                      ///< Optional pointer to the parent task.
    std::atomic<uint16_t> _unfinishedTasks; ///< Atomic counter for tracking unfinished tasks.
};

/// @brief Manages a pool of tasks, allocated from a circular array. Tasks are reused instead of being deallocated
/// explicitly.
class task_pool {
public:
    /// @brief Maximum number of tasks that can exist at any given time per worker.
    constexpr static std::size_t max_tasks = 4096;

    /// @brief Allocates a task with the specified function and parent, placing it in a circular buffer.
    /// @param func A callable object to be executed by the task.
    /// @param parent Optional pointer to a parent task.
    /// @return Pointer to the newly allocated task.
    static task_t* allocate(auto&& func, task_t* parent = nullptr) {
        auto& task = _tasks_array[_task_counter++ & (max_tasks - 1)];
        task.~task_t();
        new (&task) task_t(std::forward<decltype(func)>(func), parent);
        return &task;
    }

private:
    static inline thread_local std::size_t _task_counter;                  ///< Counter to index into the tasks array.
    static inline thread_local std::array<task_t, max_tasks> _tasks_array; ///< Circular buffer of tasks.
};

} // namespace co_ecs
