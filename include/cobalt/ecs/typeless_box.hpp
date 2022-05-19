#pragma once

#include <cobalt/ecs/type_meta.hpp>

namespace cobalt::ecs {

/// @brief Any wrapper object. Uses type erasure to hold a heap allocated object. It is like std::any, but works for
/// noncopyable types.
class typeless_box {
public:
    /// @brief Allocate R constructed from args...
    ///
    /// @tparam T Type
    /// @tparam Args Argument types
    /// @param args Arguments to construct R from
    /// @return typeless_box
    template<typename T, typename... Args>
    static typeless_box create(Args&&... args) {
        return typeless_box{
            new T{ std::forward<Args>(args)... },
            type_meta::of<T>(),
            [](void* ptr) { delete reinterpret_cast<T*>(ptr); },
        };
    }

    /// @brief Construct a new data wrapper object
    typeless_box() = default;

    /// @brief Destroy the data wrapper object
    ~typeless_box() {
        if (_data) {
            _meta->destruct(_data);
            _deallocate(_data);
        }
    }

    /// @brief Copy constructor
    ///
    /// @param rhs Right hand side
    typeless_box(const typeless_box& rhs) = delete;

    /// @brief Copy assignment operator
    ///
    /// @param rhs Right hand side
    /// @return typeless_box&
    typeless_box& operator=(const typeless_box& rhs) = delete;

    /// @brief Move constructor
    ///
    /// @param rhs Right hand side
    typeless_box(typeless_box&& rhs) {
        std::swap(_data, rhs._data);
        std::swap(_meta, rhs._meta);
        std::swap(_deallocate, rhs._deallocate);
    }

    /// @brief Move assignment operator
    ///
    /// @param rhs Right hand side
    /// @return typeless_box&
    typeless_box& operator=(typeless_box&& rhs) {
        std::swap(_data, rhs._data);
        std::swap(_meta, rhs._meta);
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
    typeless_box(void* data, const type_meta* meta, void (*deallocate)(void*)) :
        _data(data), _meta(meta), _deallocate(deallocate) {
    }

    void* _data{};
    const type_meta* _meta{};
    void (*_deallocate)(void*){};
};

} // namespace cobalt::ecs