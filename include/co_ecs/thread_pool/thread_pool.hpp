#pragma once

#include <co_ecs/detail/work_stealing_queue.hpp>
#include <co_ecs/thread_pool/task.hpp>

#include <random>
#include <semaphore>
#include <thread>

namespace co_ecs {

/// @brief Generic thread pool implementation.
///
/// Creates N worker threads. Each thread has its own local task queue
/// that it can push/pop task items to/from. Once there's no tasks in
/// local queue a worker thread tries to steal a task from a random worker.
class thread_pool {
public:
    using thread_t = std::thread;

    /// @brief Thread pool worker
    class worker {
    public:
#ifdef CO_ECS_WORKER_STATS
        /// @brief Worker stats
        struct worker_stats {
            std::atomic<uint64_t> task_count;
            std::atomic<uint64_t> steal_count;
            std::atomic<uint64_t> idle_count;

            void inc_task() {
                task_count.fetch_add(1, std::memory_order::relaxed);
            }

            void inc_steal() {
                steal_count.fetch_add(1, std::memory_order::relaxed);
            }

            void inc_idle() {
                idle_count.fetch_add(1, std::memory_order::relaxed);
            }
        };
#endif

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
        /// @param func Function
        /// @param parent Parent task pointer
        task_t* submit(auto&& func, task_t* parent = nullptr) {
            task_t* task = task_pool::allocate(std::forward<decltype(func)>(func), parent);
            submit(task);
            return task;
        }

        /// @brief Submit a task into local workers queue
        /// @param task Task
        void submit(task_t* task) {
            get_queue().push(task);
            _pool.wake_worker();
        }

        /// @brief Wait for task completion
        /// @param task Task
        void wait(task_t* task) {
            while (!task->is_completed()) {
                auto* next_task = get_task();
                if (next_task) {
                    execute(next_task);
                } else {
#ifdef CO_ECS_WORKER_STATS
                    _stats.inc_idle();
#endif
                }
                _pool.wake_worker();
            }
        }

#ifdef CO_ECS_WORKER_STATS
        /// @brief Get worker stats
        /// @return Stats
        const worker_stats& stats() const noexcept {
            return _stats;
        }
#endif

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
                        execute(task);
                    } else {
                        idle();
                    }
                } while (task);

                if (!is_active()) {
                    break;
                }
            }
        }

        void start() {
            _thread = thread_t([this]() { run(); });
        }

        void stop() {
            _active.store(false, std::memory_order::relaxed);
        }

        void join() {
            if (_thread.joinable()) {
                _thread.join();
            }
        }

        auto is_active() const noexcept -> bool {
            return _active.load(std::memory_order::relaxed);
        }

        [[nodiscard]] task_t* get_task() {
            // First, attempt to retrieve a task from the worker's own local queue.
            if (auto maybe_task = get_queue().pop()) {
                return *maybe_task;
            }

            // No tasks in the local queue; attempt to steal from the main worker queue, if not the main worker.
            if (worker* main_worker = &_pool.main_worker(); main_worker != this) {
                if (auto maybe_task = steal(*main_worker)) {
                    return *maybe_task;
                }
            }

            // If stealing from the main worker fails, attempt to steal from a random worker.
            // This method is optimal for smaller numbers of workers (e.g., 4-8).
            if (worker* random_worker = _pool.random_worker(); random_worker && random_worker != this) {
                if (auto maybe_task = steal(*random_worker)) {
                    return *maybe_task;
                }
            }

            return nullptr;
        }

        [[nodiscard]]
        std::optional<task_t*> steal(worker& worker) {
            auto maybe_task = worker.get_queue().steal();
#ifdef CO_ECS_WORKER_STATS
            if (maybe_task) {
                _stats.inc_steal();
            }
#endif
            return maybe_task;
        }

        void execute(task_t* task) {
            task->execute();
#ifdef CO_ECS_WORKER_STATS
            _stats.inc_task();
#endif
        }

        void idle() {
            _pool.wait();

#ifdef CO_ECS_WORKER_STATS
            _stats.inc_idle();
#endif
        }

        [[nodiscard]]
        detail::work_stealing_queue<task_t*>& get_queue() noexcept {
            return _queue;
        }

    private:
        detail::work_stealing_queue<task_t*> _queue;
        thread_pool& _pool;

        std::atomic<bool> _active{ true };
        thread_t _thread{};
        std::size_t _id;
#ifdef CO_ECS_WORKER_STATS
        worker_stats _stats;
#endif
    };

    /// @brief Construct thread pool with num_workers workers
    /// @param num_workers The number of workers to create
    thread_pool(std::size_t num_workers = std::thread::hardware_concurrency()) {
        assert(num_workers > 0 && "Number of workers should be > 0");
        _workers.reserve(num_workers);

        if (_instance) {
            throw std::logic_error("Thread pool already created");
        }
        _instance = this;

        // create main worker that will execute tasks in main thread
        _workers.emplace_back(std::make_unique<worker>(*this, 0));
        worker::current_worker = _workers[0].get();

        // create background workers
        for (auto i = 1; i < num_workers; i++) {
            _workers.emplace_back(std::make_unique<worker>(*this, i));
        }

        // start workers
        for (auto i = 1; i < num_workers; i++) {
            _workers[i]->start();
        }
    }

    /// @brief Destroy thread pool, worker threads are notified to exit and joined.
    ~thread_pool() {
        // stop workers
        for (auto i = 1; i < _workers.size(); i++) {
            _workers[i]->stop();
        }

        // join worker threads
        for (auto i = 1; i < _workers.size(); i++) {
            _workers[i]->join();
        }

        _instance = nullptr;
    }

    thread_pool(const thread_pool&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;

    thread_pool(thread_pool&&) = delete;
    thread_pool& operator=(thread_pool&&) = delete;

    /// @brief Get thread pool instance
    /// @return thread_pool
    static thread_pool& get() {
        if (!_instance) {
            static thread_pool tp;
            _instance = &tp;
        }

        return *_instance;
    }

    /// @brief Submit a task to a thread pool
    /// @param func Function
    /// @param parent Parent task pointer
    task_t* submit(auto&& func, task_t* parent = nullptr) {
        return current_worker().submit(std::forward<decltype(func)>(func), parent);
    }

    /// @brief Wait a task to complete
    /// @param task
    void wait(task_t* task) {
        current_worker().wait(task);
    }

    /// @brief Get worker by ID
    /// @param id Worker ID
    /// @return Worker
    worker& get_worker_by_id(std::size_t id) noexcept {
        return *_workers.at(id);
    }

    /// @brief Return the number of workers
    /// @return Number of workers
    [[nodiscard]]
    std::size_t num_workers() const noexcept {
        return _workers.size();
    }

    /// @brief Get current worker
    /// @return Worker
    static worker& current_worker() noexcept {
        return worker::current();
    }

private:
    worker& main_worker() noexcept {
        return *_workers[0];
    }

    worker* random_worker() noexcept {
        if (num_workers() == 1) {
            // no other workers than main
            return nullptr;
        }

        std::uniform_int_distribution<std::size_t> dist{ 1, num_workers() - 1 };
        std::default_random_engine random_engine{ std::random_device()() };

        auto random_index = dist(random_engine);

        return _workers[random_index].get();
    }

    void wake_worker() {
        _worker_wait_semaphore.release();
    }

    void wait() {
        constexpr auto wait_time = std::chrono::milliseconds(5);
        _worker_wait_semaphore.try_acquire_for(wait_time);
    }

private:
    static inline thread_pool* _instance;

    std::vector<std::unique_ptr<worker>> _workers;
    std::counting_semaphore<> _worker_wait_semaphore{ 0 };
};

} // namespace co_ecs
