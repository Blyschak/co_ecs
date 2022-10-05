#pragma once

#include <co_ecs/component.hpp>

namespace co_ecs {

// Forward declaration of a view class
template<component_reference... Args>
class view;

/// @brief view_arguments metadata, like a flag whether all references are const, etc.
template<component_reference... Args>
struct view_arguments {
    /// @brief Const when all component references are const
    static constexpr bool is_const = const_component_references_v<Args...>;
};

namespace detail {

// Help to convert std::tuple<Args...> into view<Args...>

template<typename T>
struct view_converter {};

template<typename... Args>
struct view_converter<std::tuple<Args...>> {
    using view_t = typename view<Args...>;
    using view_arguments_t = typename view_arguments<Args...>;
};

// Helper to decompose function type arguments
template<typename F>
struct func_decomposer {
    using view_converter_t = typename view_converter<typename function_traits<F>::arguments_tuple_type>;
    using view_t = view_converter_t::view_t;
    static constexpr bool is_const = view_converter_t::view_arguments_t::is_const;
};

} // namespace detail
} // namespace co_ecs