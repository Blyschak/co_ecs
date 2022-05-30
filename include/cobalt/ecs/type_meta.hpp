#pragma once

#include <cobalt/asl/type_name.hpp>

#include <string_view>

namespace cobalt::ecs {

/// @brief Type meta information
struct type_meta {
    /// @brief Move constructor callback for type T
    ///
    /// @tparam T Target type
    /// @param ptr Place to construct at
    /// @param rhs Pointer to an object to construct from
    template<typename T>
    static void move_constructor(void* ptr, void* rhs) {
        std::construct_at(static_cast<T*>(ptr), std::move(*static_cast<T*>(rhs)));
    }

    /// @brief Move assignment callback for type T
    ///
    /// @tparam T Target type
    /// @param lhs Pointer to an object to assign to
    /// @param rhs Pointer to an object to assign from
    template<typename T>
    static void move_assignment(void* lhs, void* rhs) {
        *static_cast<T*>(lhs) = std::move(*static_cast<T*>(rhs));
    }

    /// @brief Destructor callback for type T
    ///
    /// @tparam T Target type
    /// @param ptr Pointer to an object to delete
    template<typename T>
    static void destructor(void* ptr) {
        static_cast<T*>(ptr)->~T();
    }

    /// @brief Constructs type_meta for type T
    ///
    /// @tparam T Target type
    /// @return const type_meta* Target type meta
    template<typename T>
    static const type_meta* of() noexcept {
        static const type_meta meta{
            sizeof(T),
            alignof(T),
            asl::type_name_v<T>,
            &move_constructor<T>,
            &move_assignment<T>,
            &destructor<T>,
        };
        return &meta;
    }

    std::size_t size;
    std::size_t align;
    std::string_view name;
    void (*move_construct)(void*, void*);
    void (*move_assign)(void*, void*);
    void (*destruct)(void*);
};


} // namespace cobalt::ecs