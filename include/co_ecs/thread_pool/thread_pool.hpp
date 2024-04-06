#pragma once

#include <co_ecs/detail/work_stealing_queue.hpp>
#include <co_ecs/thread_pool/task.hpp>

#include <co_ecs/command.hpp>

#include <random>
#include <thread>

namespace co_ecs {

/// @brief Generic thread pool implementation.
///
/// Creates N worker threads. Each thread has its own local task queue
/// that it can push/pop task items to/from. Once there's no tasks in
/// local queue a worker thread tries to steal a task from a random worker.
class thread_pool {
public:
    /// @brief Thread pool worker
    class worker {
    public:
        /// @brief Worker stats
        struct worker_stats {
            std::atomic<uint64_t> task_count;
            std::atomic<uint64_t> steal_count;
            std::atomic<uint64_t> yield_count;
            std::atomic<uint64_t> idle_count;

        private:
            friend class worker;

            void inc_task() {
                task_count.fetch_add(1, std::memory_order::relaxed);
            }

            void inc_steal() {
                steal_count.fetch_add(1, std::memory_order::relaxed);
            }

            void inc_yield() {
                yield_count.fetch_add(1, std::memory_order::relaxed);
            }

            void inc_idle() {
                idle_count.fetch_add(1, std::memory_order::relaxed);
            }
        };

        /// @brief Create a thread pool worker
        /// @param pool
        /// @param id
        worker(thread_pool& pool, uint16_t id) : _pool(pool), _id(id) {
        }

        /// @brief Return ID of the worker
        /// @return Worker ID
        [[nodiscard]] std::size_t id() const noexcept {
            return _id;
        }

        /// @brief Get current thread worker
        /// @return Returns a worker dedicated to the thread this method is invoked from
        static worker& current() noexcept {
            return *current_worker;
        }

        /// @brief Submit a task into local workers queue
        /// @param task Task
        void submit(task_t* task) {
            _queue.push(task);
        }

        /// @brief Wait for task completion
        /// @param task Task
        void wait(task_t* task) {
            while (!task->is_completed()) {
                auto* next_task = get_task();
                if (next_task) {
                    next_task->execute();
                    _stats.inc_task();
                } else {
                    _stats.inc_idle();
                }
            }

            // TODO: do I need a thread fence here?
        }

        /// @brief Allocate task
        /// @param func Callable
        /// @param parent Parent task
        /// @return Allocated task
        task_t* allocate(auto&& func, task_t* parent = nullptr) {
            return task_pool::allocate(std::forward<decltype(func)>(func), parent);
        }

        /// @brief Get worker stats
        /// @return Stats
        const worker_stats& stats() const noexcept {
            return _stats;
        }

        /// @brief Get command buffer
        /// @return Command buffer
        command_buffer& get_command_buffer() noexcept {
            return _command_buffer;
        }

    private:
        friend class thread_pool;

        static inline thread_local worker* current_worker;

        void run() {
            current_worker = this;

            while (true) {
                task_t* task;

                // fetch and execute tasks while we can
                do {
                    task = get_task();
                    if (task) {
                        task->execute();
                        _stats.inc_task();
                    } else {
                        _stats.inc_idle();
                    }
                } while (task);

                if (!_active.load(std::memory_order::relaxed)) {
                    break;
                }
            }
        }

        [[nodiscard]] task_t* get_task() {
            auto maybe_task = _queue.pop();
            if (!maybe_task) {
                auto& random_worker = _pool.random_worker();
                if (&random_worker == this) {
                    yield();
                    return nullptr;
                }

                maybe_task = random_worker._queue.steal();
                if (!maybe_task) {
                    yield();
                    return nullptr;
                }
                _stats.inc_steal();
            }

            return *maybe_task;
        }

        void yield() {
            _stats.inc_yield();
            std::this_thread::yield();
        }

        void start() {
            _thread = std::thread([this]() { run(); });
        }

        void stop() {
            _active.store(false, std::memory_order::relaxed);
        }

        void join() {
            _thread.join();
        }

    private:
        std::atomic<bool> _active{ true };
        detail::work_stealing_queue<task_t*> _queue;
        command_buffer _command_buffer;
        thread_pool& _pool;
        std::thread _thread{};
        std::size_t _id;
        worker_stats _stats;
    };


    /// @brief Construct thread pool with num_workers workers
    /// @param num_workers The number of workers to create
    thread_pool(std::size_t num_workers = std::thread::hardware_concurrency()) {
        assert(num_workers > 0 && "Number of workers should be > 0");
        _workers.reserve(num_workers);

        // create main worker that will execute tasks in main thread
        _workers.emplace_back(std::make_unique<worker>(*this, 0));
        worker::current_worker = _workers[0].get();

        // create background workers
        for (auto i = 1; i < num_workers; i++) {
            _workers.emplace_back(std::make_unique<worker>(*this, i));
        }

        for (auto i = 1; i < num_workers; i++) {
            _workers[i]->start();
        }
    }

    thread_pool(const thread_pool&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;

    thread_pool(thread_pool&&) = delete;
    thread_pool& operator=(thread_pool&&) = delete;

    /// @brief Destroy thread pool, worker threads are notified to exit and joined.
    ~thread_pool() {
        for (auto i = 1; i < _workers.size(); i++) {
            _workers[i]->stop();
        }

        for (auto i = 1; i < _workers.size(); i++) {
            _workers[i]->join();
        }
    }

    /// @brief Allocate a task
    /// @param func Callable
    /// @return Pointer to an allocated task
    task_t* allocate(auto&& func, task_t* parent = nullptr) {
        return current_worker().allocate(std::forward<decltype(func)>(func), parent);
    }

    /// @brief Submit a task to a thread pool
    /// @param task
    void submit(task_t* task) {
        current_worker().submit(task);
    }

    /// @brief Wait a task to complete
    /// @param task
    void wait(task_t* task) {
        current_worker().wait(task);
    }

    /// @brief Get random worker
    /// @return Worker
    worker& random_worker() noexcept {
        std::uniform_int_distribution<std::size_t> dist{ 0, _workers.size() - 1 };
        std::default_random_engine random_engine{ std::random_device()() };

        auto random_index = dist(random_engine);

        return *_workers[random_index];
    }

    /// @brief Get worker by ID
    /// @param id Worker ID
    /// @return Worker
    worker& get_worker_by_id(std::size_t id) noexcept {
        return *_workers.at(id);
    }

    /// @brief Get current worker
    /// @return Worker
    static worker& current_worker() noexcept {
        return worker::current();
    }

    /// @brief Return the number of workers
    /// @return Number of workers
    std::size_t num_workers() const noexcept {
        return _workers.size();
    }

private:
    std::vector<std::unique_ptr<worker>> _workers;
};

} // namespace co_ecs
