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

class resource_wrapper {
public:
    template<resource R, typename... Args>
    static resource_wrapper create(Args&&... args) {
        return resource_wrapper{
            new R{ std::forward<Args>(args)... },
            type_meta::of<R>(),
        };
    }

    resource_wrapper() = default;

    resource_wrapper(const resource_wrapper& rhs) = delete;
    resource_wrapper& operator=(const resource_wrapper& rhs) = delete;
    resource_wrapper(resource_wrapper&& rhs) {
        std::swap(_resource, rhs._resource);
        std::swap(_meta, rhs._meta);
    }

    resource_wrapper& operator=(resource_wrapper&& rhs) {
        std::swap(_resource, rhs._resource);
        std::swap(_meta, rhs._meta);
        return *this;
    }

    template<resource R>
    R& get() noexcept {
        return *reinterpret_cast<R*>(_resource);
    }

    template<resource R>
    const R& get() const noexcept {
        return *reinterpret_cast<const R*>(_resource);
    }

    ~resource_wrapper() {
        if (_resource) {
            _meta->destruct(_resource);
        }
    }

private:
    resource_wrapper(void* resource, const type_meta* meta) : _resource(resource), _meta(meta) {
    }

    void* _resource{};
    const type_meta* _meta{};
};

} // namespace cobalt::ecs
