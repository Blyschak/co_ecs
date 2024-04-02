#pragma once

#include <co_ecs/detail/work_stealing_queue.hpp>

#include <vector>
#include <random>
#include <condition_variable>

namespace co_ecs::detail {

/// @brief Generic thread pool implementation.
/// 
/// Creates N worker threads. Each thread has its own local task queue
/// that it can push/pop task items to/from. Once there's no tasks in 
/// local queue a worker thread tries to steal a task from a random other
/// worker. If there's no local work or remote work a worker waits until 
/// there are tasks in a global queue.
class thread_pool {
public:
    /// @brief Task structure
    class task {
    public:
        /// @brief Create an empty task
        task() = default;

        /// @brief Create a task from a function
        /// @param f Callable
        task(auto&& f) : task_inner(std::forward<decltype(f)>(f)) {}

        task(const task& rhs) = delete; 
        task& operator=(const task& rhs) = delete; 

        task(task&&) = default; 
        task& operator=(task&&) = default; 

        /// @brief Execute task
        void execute() {
            task_inner.func();
        }
    
    private:
        struct {
            std::function<void()> func;
        } task_inner;
    };

    /// @brief Task pool. Tasks are allocated from a thread local array.
    /// No deallocation is done explicitelly, task deallocation is run
    /// when a new task replaces it.
    ///
    /// NOTE: This assumes the tasks we are allocating are simple free functions or
    /// simple lambdas capturing only few variables that fit into std::function.
    /// Otherwise, if std::function allocates memory, we will end up with max_tasks
    /// std::function objects that aren't going to be deallocated till the program ends.
    struct task_pool {
        /// @brief Maximum number of tasks that can exists at any given time per worker.
        constexpr static std::size_t max_tasks = 4096;

        /// @brief Tasks counter
        static inline thread_local std::size_t task_counter;

        /// @brief Ring buffer, use counter to index into array
        static inline thread_local std::array<task, max_tasks> tasks_array;

        /// @brief Allocate a task
        /// @param f Callable
        /// @return Pointer to an allocated task
        static task* create(auto&& f) {
            auto& t = tasks_array[task_counter++ & (max_tasks - 1)];
            t = task(std::forward<decltype(f)>(f));
            return &t;
        }
    };

    /// @brief Thread pool worker
    class worker {
    public:
        /// @brief Create a thread pool worker
        /// @param pool 
        /// @param id
        worker(thread_pool& pool, uint16_t id): _thread_pool(pool), _id(id) {}

        /// @brief Get current thread worker
        /// @return Returns a worker dedicated to the thread this method is invoked from
        static worker& current() noexcept {
            return *current_worker;
        }

        /// @brief Return ID of the worker
        /// @return Worker ID
        [[nodiscard]] uint16_t id() const noexcept {
            return _id;
        }

        /// @brief Submit a task into local workers queue.
        /// @param f Callable
        void submit(auto&& f) {
            get_queue().push(task_pool::create(std::forward<decltype(f)>(f)));
        }

    private:
        static inline thread_local worker* current_worker;

        friend class thread_pool;

        void start() {
            _thread = std::thread([this]() { run(); });
        }

        void stop() {
            _active.store(false, std::memory_order::relaxed);
        }

        void join() {
            _thread.join();
        }

        void run() {
            current_worker = this;

            while (_active.load(std::memory_order::relaxed)) {
                auto* task = get_task();
                if (task) {
                    task->execute();
                }
            }
        }

        [[nodiscard]]
        task* get_task() {
            auto maybe_task = get_queue().pop();
            if (!maybe_task) {
                auto& random_worker = _thread_pool.random_worker();
                if (&random_worker == this) {
                    std::this_thread::yield();
                    return nullptr;
                }

                maybe_task = random_worker.get_queue().steal();
                if (!maybe_task) {
                    maybe_task = _thread_pool.get_global_queue().steal();
                    if (!maybe_task) {
                        std::unique_lock lock(_thread_pool._mutex);
                        if (_active.load(std::memory_order::relaxed)) {
                            _thread_pool._condition.wait(lock);
                        }

                        return nullptr;
                    }
                }
            }

            return *maybe_task;
        }

        [[nodiscard]]
        work_stealing_queue<task*>& get_queue() noexcept { return _queue; }

    private:
        std::atomic_bool _active {true};
        work_stealing_queue<task*> _queue;
        thread_pool& _thread_pool;
        std::thread _thread;
        uint16_t _id;
    };

    /// @brief Construct thread pool with num_workers workers
    /// @param num_workers The number of workers to create
    thread_pool(std::size_t num_workers = std::thread::hardware_concurrency()) {
        assert(num_workers > 1 && "Number of workers should be > 1");
        _workers.reserve(num_workers);

        for (auto i = 0; i < num_workers; i++) {
            _workers.emplace_back(std::make_unique<worker>(*this, i));
        }

        for (auto& worker: _workers) {
            worker->start();
        }
    }

    thread_pool(const thread_pool&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;

    thread_pool(thread_pool&&) = delete;
    thread_pool& operator=(thread_pool&&) = delete;

    /// @brief Destroy thread pool, worker threads are notified to exit and joined.
    ~thread_pool() {
        {
            std::lock_guard lock(_mutex);

            for (auto& worker: _workers) {
                worker->stop();
            }
        }

        notify_all();

        for (auto& worker: _workers) {
            worker->join();
        }
    }

    /// @brief Submit a task to a thread pool
    /// @param f Callable
    void submit(auto&& f) {
        auto t = task_pool::create(std::forward<decltype(f)>(f));
        
        _queue.push(t);
    }

    /// @brief Notifu workers to process submitted tasks
    void notify_all() {
        _condition.notify_all();
    }

private:
    friend class worker;

    worker& random_worker() noexcept {
        std::uniform_int_distribution<std::size_t> dist{0, _workers.size() - 1};
        std::default_random_engine random_engine{std::random_device()()};

        auto random_index = dist(random_engine);

        return *_workers[random_index];
    }
    
    [[nodiscard]]
    work_stealing_queue<task*>& get_global_queue() noexcept { return _queue; }

private:
    std::mutex _mutex;
    std::condition_variable _condition;
    std::vector<std::unique_ptr<worker>> _workers;
    work_stealing_queue<task*> _queue;
};
    
} // namespace co_ecs::detail
