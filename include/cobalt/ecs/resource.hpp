#pragma once

#include <type_traits>

#include <cobalt/asl/family.hpp>

namespace cobalt::ecs {

/// @brief Type for resource ID
using resource_id = std::uint32_t;

/// @brief Resource concept
///
/// @tparam T Resource type
template<typename T>
concept resource = !std::is_reference_v<T> &&                 //
                   !std::is_pointer_v<T> &&                   //
                   !std::is_const_v<T> &&                     //
                   std::is_class_v<T> &&                      //
                   std::is_nothrow_move_constructible_v<T> && //
                   std::is_nothrow_move_assignable_v<T>;

/// @brief Type for family used to generated resource IDs.
using resource_family = cobalt::asl::family<struct _resource_family_t, resource_id>;

/// @brief Resource wrapper object. Uses type erasure to hold a heap allocated object. It is like std::any, but works
/// for noncopyable types.
class resource_container {
public:
    /// @brief Allocate R constructed from args...
    ///
    /// @tparam T Type
    /// @tparam Args Argument types
    /// @param args Arguments to construct R from
    /// @return resource_container
    template<typename T, typename... Args>
    static resource_container create(Args&&... args) {
        return resource_container{
            new T{ std::forward<Args>(args)... },
            [](void* ptr) { delete reinterpret_cast<T*>(ptr); },
        };
    }

    /// @brief Construct a new data wrapper object
    resource_container() = default;

    /// @brief Destroy the data wrapper object
    ~resource_container() {
        if (_data) {
            _deallocate(_data);
        }
    }

    /// @brief Copy constructor
    ///
    /// @param rhs Right hand side
    resource_container(const resource_container& rhs) = delete;

    /// @brief Copy assignment operator
    ///
    /// @param rhs Right hand side
    /// @return resource_container&
    resource_container& operator=(const resource_container& rhs) = delete;

    /// @brief Move constructor
    ///
    /// @param rhs Right hand side
    resource_container(resource_container&& rhs) {
        std::swap(_data, rhs._data);
        std::swap(_deallocate, rhs._deallocate);
    }

    /// @brief Move assignment operator
    ///
    /// @param rhs Right hand side
    /// @return resource_container&
    resource_container& operator=(resource_container&& rhs) {
        std::swap(_data, rhs._data);
        std::swap(_deallocate, rhs._deallocate);
        return *this;
    }

    /// @brief Return a reference
    ///
    /// @tparam R Type
    /// @return T&
    template<typename T>
    T& get() noexcept {
        return *reinterpret_cast<T*>(_data);
    }

    /// @brief Return a const reference
    ///
    /// @tparam T Type
    /// @return T&
    template<typename T>
    const T& get() const noexcept {
        return *reinterpret_cast<const T*>(_data);
    }

private:
    resource_container(void* data, void (*deallocate)(void*)) : _data(data), _deallocate(deallocate) {
    }

    void* _data{};
    void (*_deallocate)(void*){};
};

} // namespace cobalt::ecs
