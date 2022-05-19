#pragma once

#include <cobalt/asl/family.hpp>

#include <type_traits>

namespace cobalt::ecs {

/// @brief Type for event ID
using event_id = std::uint32_t;

/// @brief event concept
///
/// @tparam T event type
template<typename T>
concept event = !std::is_reference_v<T> &&                 //
                !std::is_pointer_v<T> &&                   //
                !std::is_const_v<T> &&                     //
                std::is_class_v<T> &&                      //
                std::is_nothrow_move_constructible_v<T> && //
                std::is_nothrow_move_assignable_v<T>;

/// @brief Type for family used to generated event IDs.
using event_family = cobalt::asl::family<struct _event_family_t, event_id>;

class event_queue {
public:
    template<event T>
    static event_queue create() {
        return event_queue{
            new std::vector<T>{},
            [](void* ptr) { reinterpret_cast<std::vector<T>*>(ptr)->clear(); },
            [](void* ptr) { delete reinterpret_cast<std::vector<T>*>(ptr); },
        };
    }

    /// @brief Destroy the data wrapper object
    ~event_queue() {
        if (_data) {
            _deallocate(_data);
        }
    }

    /// @brief Copy constructor
    ///
    /// @param rhs Right hand side
    event_queue(const event_queue& rhs) = delete;

    /// @brief Copy assignment operator
    ///
    /// @param rhs Right hand side
    /// @return event_queue&
    event_queue& operator=(const event_queue& rhs) = delete;

    /// @brief Move constructor
    ///
    /// @param rhs Right hand side
    event_queue(event_queue&& rhs) {
        std::swap(_data, rhs._data);
        std::swap(_deallocate, rhs._deallocate);
        std::swap(_clear, rhs._clear);
    }

    /// @brief Move assignment operator
    ///
    /// @param rhs Right hand side
    /// @return event_queue&
    event_queue& operator=(event_queue&& rhs) {
        std::swap(_data, rhs._data);
        std::swap(_deallocate, rhs._deallocate);
        std::swap(_clear, rhs._clear);
        return *this;
    }

    template<event T, typename... Args>
    void emplace_back(Args&&... args) {
        auto* vec = get<T>();
        vec->emplace_back(std::forward<Args>(args)...);
    }

    template<event T>
    T pop_back() {
        auto* vec = get<T>();
        auto e = std::move(vec->back());
        vec->pop_back();
        return e;
    }

    void clear() {
        _clear(_data);
    }

    template<event T>
    std::vector<T>& get() {
        return *reinterpret_cast<std::vector<T>*>(_data);
    }

    template<event T>
    const std::vector<T>& get() const {
        return *reinterpret_cast<const std::vector<T>*>(_data);
    }

private:
    event_queue(void* data, void (*clear)(void*), void (*deallocate)(void*)) :
        _data(data), _clear(clear), _deallocate(deallocate) {
    }

    void* _data{};
    void (*_clear)(void*){};
    void (*_deallocate)(void*){};
};

template<event T>
class event_publisher {
public:
    using event_type = T;

    event_publisher(std::vector<T>& queue) : _queue(queue) {
    }

    template<typename... Args>
    void publish(Args&&... args) {
        _queue.emplace_back(std::forward<Args>(args)...);
    }

private:
    std::vector<T>& _queue;
};

template<event T>
class event_reader {
public:
    using event_type = T;

    event_reader(const std::vector<T>& queue) : _queue(queue) {
    }

    std::vector<T>::const_iterator begin() const {
        return _queue.begin();
    }

    std::vector<T>::const_iterator end() const {
        return _queue.end();
    }

private:
    const std::vector<T>& _queue;
};

} // namespace cobalt::ecs
