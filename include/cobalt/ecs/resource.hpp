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
                   std::is_nothrow_move_constructible_v<T> && //
                   std::is_nothrow_move_assignable_v<T>;

/// @brief Type for family used to generated resource IDs.
using resource_family = cobalt::asl::family<struct _resource_family_t, resource_id>;

/// @brief Resource wrapper object. Uses type erasure to hold a heap allocated resource object
class resource_wrapper {
public:
    /// @brief Allocate R constructed from args...
    ///
    /// @tparam R Resource type
    /// @tparam Args Argument types
    /// @param args Arguments to construct R from
    /// @return resource_wrapper
    template<resource R, typename... Args>
    static resource_wrapper create(Args&&... args) {
        return resource_wrapper{
            new R{ std::forward<Args>(args)... },
            type_meta::of<R>(),
            [](void* ptr) { delete reinterpret_cast<R*>(ptr); },
        };
    }

    /// @brief Construct a new resource wrapper object
    resource_wrapper() = default;

    /// @brief Destroy the resource wrapper object
    ~resource_wrapper() {
        if (_resource) {
            _meta->destruct(_resource);
            _deallocate(_resource);
        }
    }

    /// @brief Copy constructor
    ///
    /// @param rhs Right hand side
    resource_wrapper(const resource_wrapper& rhs) = delete;

    /// @brief Copy assignment operator
    ///
    /// @param rhs Right hand side
    /// @return resource_wrapper&
    resource_wrapper& operator=(const resource_wrapper& rhs) = delete;

    /// @brief Move constructor
    ///
    /// @param rhs Right hand side
    resource_wrapper(resource_wrapper&& rhs) {
        std::swap(_resource, rhs._resource);
        std::swap(_meta, rhs._meta);
        std::swap(_deallocate, rhs._deallocate);
    }

    /// @brief Move assignment operator
    ///
    /// @param rhs Right hand side
    /// @return resource_wrapper&
    resource_wrapper& operator=(resource_wrapper&& rhs) {
        std::swap(_resource, rhs._resource);
        std::swap(_meta, rhs._meta);
        std::swap(_deallocate, rhs._deallocate);
        return *this;
    }

    /// @brief Return a reference to resource
    ///
    /// @tparam R Resource type
    /// @return R&
    template<resource R>
    R& get() noexcept {
        return *reinterpret_cast<R*>(_resource);
    }

    /// @brief Return a const reference to resource
    ///
    /// @tparam R Resource type
    /// @return R&
    template<resource R>
    const R& get() const noexcept {
        return *reinterpret_cast<const R*>(_resource);
    }

private:
    resource_wrapper(void* resource, const type_meta* meta, void (*deallocate)(void*)) :
        _resource(resource), _meta(meta), _deallocate(deallocate) {
    }

    void* _resource{};
    const type_meta* _meta{};
    void (*_deallocate)(void*){};
};

} // namespace cobalt::ecs
