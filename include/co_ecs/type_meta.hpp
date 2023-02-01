#pragma once

#include <co_ecs/macro.hpp>

#include <string_view>

namespace co_ecs {

/// @brief Utility to get type name at compile type
///
/// @tparam T Type to get name for
/// @return std::string_view Type name string
template<typename T>
constexpr static std::string_view type_name() noexcept {
#if defined CO_ECS_PRETTY_FUNCTION
    std::string_view pretty_function{ CO_ECS_PRETTY_FUNCTION };
    auto first =
        pretty_function.find_first_not_of(' ', pretty_function.find_first_of(CO_ECS_PRETTY_FUNCTION_PREFIX) + 1);
    auto value = pretty_function.substr(first, pretty_function.find_last_of(CO_ECS_PRETTY_FUNCTION_SUFFIX) - first);
    return value;
#else
    return typeid(T).name();
#endif
}

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
            type_name<T>(),
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


} // namespace co_ecs