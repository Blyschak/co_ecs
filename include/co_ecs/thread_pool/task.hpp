#pragma once

#include <array>
#include <atomic>
#include <functional>

namespace co_ecs {

/// @brief Task structure
class task_t {
public:
    /// @brief Create an empty task
    task_t() = default;

    /// @brief Create a task from a function
    /// @param func callable object
    /// @param parent task
    task_t(auto&& func, task_t* parent = nullptr) : _func(std::forward<decltype(func)>(func)), _parent(parent) {
        if (_parent) {
            _parent->_unfinishedTasks.fetch_add(1, std::memory_order::relaxed);
        }
    }

    /// @brief Execute task
    void execute() {
        _func();
        finish();
    }

    /// @brief Check if task has been completed
    /// @return True if completed
    bool is_completed() const {
        return !_unfinishedTasks.load(std::memory_order::relaxed);
    }

    /// @brief Get parent task
    /// @return Parent task, nullptr if no parent
    task_t* parent() {
        return _parent;
    }

private:
    void finish() {
        _unfinishedTasks.fetch_sub(1, std::memory_order::relaxed);

        if (is_completed()) {
            if (_parent) {
                _parent->finish();
            }
        }
    }

private:
    std::function<void()> _func{};
    task_t* _parent{};
    // TODO: insert padding between mutable and immutable data to avoid false sharing
    std::atomic<uint16_t> _unfinishedTasks{ 1 };
};

/// @brief Task pool. Tasks are allocated from an circular array.
/// No deallocation is done explicitelly, task deallocation is run
/// when a new task replaces it.
class task_pool {
public:
    /// @brief Maximum number of tasks that can exists at any given time per worker.
    constexpr static std::size_t max_tasks = 4096;

    /// @brief Allocate a task
    /// @param func Callable
    /// @return Pointer to an allocated task
    static task_t* allocate(auto&& func, task_t* parent = nullptr) {
        auto& task = _tasks_array[_task_counter++ & (max_tasks - 1)];
        task.~task_t();
        new (&task) task_t(std::forward<decltype(func)>(func), parent);
        return &task;
    }

private:
    /// @brief Tasks counter
    static inline thread_local std::size_t _task_counter;
    /// @brief Ring buffer, use counter to index into array
    static inline thread_local std::array<task_t, max_tasks> _tasks_array;
};

} // namespace co_ecs
