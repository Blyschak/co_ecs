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
struct function_traits : public function_traits<decltype(std::function{ std::declval<F>() })> {};

/// @brief Function trait, specialization for std::function
///
/// @tparam R Return type
/// @tparam Args Argument types
template<typename R, typename... Args>
struct function_traits<std::function<R(Args...)>> {
    using return_type = R;

    using arguments_tuple_type = std::tuple<Args...>;

    template<std::size_t I>
    using argument_type = nth_type_t<I, Args...>;

    static constexpr std::size_t arity = sizeof...(Args);
};

/// @brief Unique type is a structure that helps to statically check whether all types are unique
///
/// @tparam Rest Types to check for uniqueness
template<typename... Rest>
struct unique_types;

/// @brief Unique type specialization for 0 types
template<>
struct unique_types<> {};

/// @brief Unique type specialization for at least 1 type
///
/// @tparam T1 First type
template<typename T1>
struct unique_types<T1> {};

/// @brief Unique type specialization for at 2 types
///
/// @tparam T1 First type
/// @tparam T2 Second type
template<class T1, class T2>
struct unique_types<T1, T2> {
    static_assert(!std::is_same<T1, T2>::value, "Types must be unique within parameter pack");
};

/// @brief Unique type specialization for at least 3 types
///
/// @tparam T1 First type
/// @tparam T2 Second type
/// @tparam Rest Other types
template<typename T1, typename T2, typename... Rest>
struct unique_types<T1, T2, Rest...>
    : unique_types<T1, T2>
    , unique_types<T1, Rest...>
    , unique_types<T2, Rest...> {};

} // namespace co_ecs::detail
