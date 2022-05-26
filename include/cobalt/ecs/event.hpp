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

/// @brief Event vector type
///
/// @tparam T Event type
template<event T>
using event_vector = std::vector<T>;

/// @brief Event storage
class event_storage {
public:
    /// @brief Construct event_storage for an event type T
    ///
    /// @tparam T Event type
    /// @return event_storage Event storage
    template<event T>
    static event_storage create() {
        return event_storage{
            new event_vector<T>{},
            [](void* ptr) { reinterpret_cast<event_vector<T>*>(ptr)->clear(); },
            [](void* ptr) { delete reinterpret_cast<event_vector<T>*>(ptr); },
        };
    }

    /// @brief Destroy the data wrapper object
    ~event_storage() {
        if (_data) {
            _deallocate(_data);
        }
    }

    /// @brief Copy constructor
    ///
    /// @param rhs Right hand side
    event_storage(const event_storage& rhs) = delete;

    /// @brief Copy assignment operator
    ///
    /// @param rhs Right hand side
    /// @return event_storage&
    event_storage& operator=(const event_storage& rhs) = delete;

    /// @brief Move constructor
    ///
    /// @param rhs Right hand side
    event_storage(event_storage&& rhs) {
        std::swap(_data, rhs._data);
        std::swap(_deallocate, rhs._deallocate);
        std::swap(_clear, rhs._clear);
    }

    /// @brief Move assignment operator
    ///
    /// @param rhs Right hand side
    /// @return event_storage&
    event_storage& operator=(event_storage&& rhs) {
        std::swap(_data, rhs._data);
        std::swap(_deallocate, rhs._deallocate);
        std::swap(_clear, rhs._clear);
        return *this;
    }

    /// @brief Emplace back event
    ///
    /// @tparam T Event type
    /// @tparam Args Argument types to construct event from
    /// @param args Arguments to construct event from
    template<event T, typename... Args>
    void emplace_back(Args&&... args) {
        auto* vec = get<T>();
        vec->emplace_back(std::forward<Args>(args)...);
    }

    /// @brief Clear event storage
    void clear() {
        _clear(_data);
    }

    /// @brief Get event vector for type T
    ///
    /// @tparam T Event type
    /// @return event_vector<T>& Event vector
    template<event T>
    event_vector<T>& get() {
        return *reinterpret_cast<event_vector<T>*>(_data);
    }

    /// @brief Get const event vector for type T
    ///
    /// @tparam T Event type
    /// @return const event_vector<T>& Const event vector
    template<event T>
    const event_vector<T>& get() const {
        return *reinterpret_cast<const event_vector<T>*>(_data);
    }

private:
    event_storage(void* data, void (*clear)(void*), void (*deallocate)(void*)) :
        _data(data), _clear(clear), _deallocate(deallocate) {
    }

    void* _data{};
    void (*_clear)(void*){};
    void (*_deallocate)(void*){};
};

/// @brief Event publisher to push events into an event storage
///
/// @tparam T Event type
template<event T>
class event_publisher {
public:
    /// @brief Event type
    using event_type = T;

    /// @brief Construct a new event publisher object
    ///
    /// @param vec Event vector reference
    event_publisher(event_vector<T>& vec) : _queue(vec) {
    }

    /// @brief Publish new event
    ///
    /// @tparam Args Argument types to construct event from
    /// @param args Arguments to construct event from
    template<typename... Args>
    void publish(Args&&... args) {
        _queue.emplace_back(std::forward<Args>(args)...);
    }

private:
    event_vector<T>& _queue;
};

/// @brief Event reader reads events from event vector of type T
///
/// @tparam T Event type
template<event T>
class event_reader {
public:
    /// @brief Event type
    using event_type = T;

    /// @brief Construct a new event reader object
    ///
    /// @param vec Event vector reference
    event_reader(const event_vector<T>& vec) : _queue(vec) {
    }

    /// @brief Return iterator to the beginning of the event vector
    ///
    /// @return event_vector<T>::const_iterator Resulting iterator
    event_vector<T>::const_iterator begin() const {
        return _queue.begin();
    }

    /// @brief Return iterator to the end of the event vector
    ///
    /// @return event_vector<T>::const_iterator Resulting iterator
    event_vector<T>::const_iterator end() const {
        return _queue.end();
    }

private:
    const event_vector<T>& _queue;
};

} // namespace cobalt::ecs
