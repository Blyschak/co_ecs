#pragma once

#include <atomic>
#include <optional>
#include <vector>
#include <cassert>

#include <co_ecs/detail/bits.hpp>

namespace co_ecs::detail {

/// @brief Work stealing queue
/// @tparam T param type
template<typename T>
class work_stealing_queue {
public:
    /// @brief Construct work stealing queue
    /// @param capacity Capacity of the queue
    work_stealing_queue(int64_t capacity = 1024) {
        assert((mod_2n(capacity, 2) == 0) && "Capacity must be a power of two");
        _top.store(0, std::memory_order_relaxed);
        _bottom.store(0, std::memory_order_relaxed);
        _array.store(new array{capacity}, std::memory_order_relaxed);
        _garbage.reserve(32);
    }

    work_stealing_queue(const work_stealing_queue& rhs) = delete;
    work_stealing_queue& operator=(const work_stealing_queue& rhs) = delete;

    // No move semantic, idk if needed, explicitelly disabled for now

    work_stealing_queue(work_stealing_queue&& rhs) = delete;
    work_stealing_queue& operator=(work_stealing_queue&& rhs) = delete;

    /// @brief destructs the queue
    ~work_stealing_queue() {
        for(auto a : _garbage) {
            delete a;
        }
        delete _array.load();
    }

    /// @brief Test if container is empty
    /// @return true if empty
    [[nodiscard]] bool empty() const noexcept {
        auto b = _bottom.load(std::memory_order_relaxed);
        auto t = _top.load(std::memory_order_relaxed);
        return b <= t;
    }

    /// @brief Get size of the queue
    /// @return number of elements in the queue
    [[nodiscard]] size_t size() const noexcept {
        auto b = _bottom.load(std::memory_order_relaxed);
        auto t = _top.load(std::memory_order_relaxed);
        return static_cast<size_t>(b >= t ? b - t : 0);
    }

    /// @brief Push element into the queue
    /// @param o Element to push
    void push(auto&& o) {
        auto bottom = _bottom.load(std::memory_order_relaxed);
        auto top = _top.load(std::memory_order_acquire);
        auto* array = _array.load(std::memory_order::relaxed);

        if (array->capacity() - 1 < (bottom - top)) {
            auto* tmp = array->grow(bottom, top);
            _garbage.push_back(array);
            std::swap(array, tmp);
            _array.store(array, std::memory_order_relaxed);
        }

        array->push(bottom, std::forward<decltype(o)>(o));
        std::atomic_thread_fence(std::memory_order_release);
        _bottom.store(bottom + 1, std::memory_order_relaxed);
    }

    /// @brief Steal element from the top of the queue (FIFO order)
    /// @return Removed element
    std::optional<T> steal() {
        auto top = _top.load(std::memory_order_acquire);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        auto bottom = _bottom.load(std::memory_order_acquire);
        
        std::optional<T> item;

        if (top < bottom) {
            auto* array = _array.load(std::memory_order_consume);
            item = array->pop(top);
            if(!_top.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
                return std::nullopt;
            }
        }

        return item;
    }

    /// @brief Pop element from the bottom of the queue (LIFO order)
    /// @return Removed element
    std::optional<T> pop() {
        auto bottom = _bottom.load(std::memory_order_relaxed) - 1;
        auto* array = _array.load(std::memory_order_relaxed);
        _bottom.store(bottom, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        auto top = _top.load(std::memory_order_relaxed);

        std::optional<T> item;

        if (top <= bottom) {
            item = array->pop(bottom);
        
            if(top == bottom) {
                // the last item just got stolen
                if(!_top.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
                    // Failed race against steal operation
                    item = std::nullopt;
                }
                _bottom.store(bottom + 1, std::memory_order_relaxed);
            }
        } else {
            _bottom.store(bottom + 1, std::memory_order_relaxed);
        }

        return item;
    }

    /// @brief Returns capacity of the queue
    /// @return Queue capacity
    [[nodiscard]] int64_t capacity() const noexcept {
        return _array.load(std::memory_order::relaxed)->capacity();
    }

private:
    struct array {
        int64_t _capacity;
        int64_t _mask;
        std::atomic<T>* _data;

        explicit array(int64_t c) :
            _capacity(c),
            _mask(c - 1),
            _data{new std::atomic<T>[static_cast<size_t>(_capacity)]} {
        }

        ~array() {
            delete [] _data;
        }

        [[nodiscard]]
        int64_t capacity() const noexcept {
            return _capacity;
        }

        void push(int64_t i, auto&& o) noexcept {
            _data[i & _mask].store(std::forward<decltype(o)>(o), std::memory_order::relaxed);
        }

        T pop(int64_t i) noexcept {
            return _data[i & _mask].load(std::memory_order::relaxed);
        }

        array* grow(int64_t bottom, int64_t top) {
            array* ptr = new array {2 * _capacity};

            for(int64_t i = top; i != bottom; ++i) {
                ptr->push(i, pop(i));
            }

            return ptr;
        }
    };

    std::atomic<int64_t> _top;
    std::atomic<int64_t> _bottom;
    std::atomic<array*> _array;
    std::vector<array*> _garbage;
};

} // namespace co_ecs::detail
