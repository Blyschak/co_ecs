#pragma once

#include <functional>
#include <tuple>
#include <type_traits>

namespace co_ecs::detail {

/// @brief Extracts Nth type parameter from parameter pack
///
/// @tparam I Index in parameter pack
/// @tparam Args Parameter pack
template<std::size_t I, typename... Args>
using nth_type_t = std::tuple_element_t<I, std::tuple<Args...>>;

/// @brief Extracts the first type parameter in parameter pack
///
/// @tparam Args Parameter pack
template<typename... Args>
using first_type_t = nth_type_t<0, Args...>;

/// @brief Extracts the second type parameter in parameter pack
///
/// @tparam Args Parameter pack
template<typename... Args>
using second_type_t = nth_type_t<1, Args...>;

/// @brief Function traits primary template
///
/// @tparam F Function type
template<typename F>
struct function_traits : public function_traits<decltype(&std::remove_reference<F>::type::operator())> {};

/// @brief Function trait, specialization for const member types
///
/// @tparam C Class type
/// @tparam R Return type
/// @tparam Args Argument types
template<typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) const> : function_traits<R (*)(Args...)> {};

/// @brief Function trait, specialization for member types
///
/// @tparam C Class type
/// @tparam R Return type
/// @tparam Args Argument types
template<typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...)> : function_traits<R (*)(Args...)> {};

/// @brief Function trait, specialization for function pointers
///
/// @tparam R Return type
/// @tparam Args Argument types
template<typename R, typename... Args>
struct function_traits<R (*)(Args...)> {
    using return_type = R;

    using arguments_tuple_type = std::tuple<Args...>;

    template<std::size_t I>
    using argument_type = nth_type_t<I, Args...>;

    static constexpr std::size_t arity = sizeof...(Args);
};

template<typename... Rest>
struct unique_types;

template<typename T1, typename T2, typename... Rest>
struct unique_types<T1, T2, Rest...>
    : unique_types<T1, T2>
    , unique_types<T1, Rest...>
    , unique_types<T2, Rest...> {};

template<>
struct unique_types<> {};

template<typename T1>
struct unique_types<T1> {};

template<class T1, class T2>
struct unique_types<T1, T2> {
    static_assert(!std::is_same<T1, T2>::value, "Types must be unique within parameter pack");
};

} // namespace co_ecs::detail
