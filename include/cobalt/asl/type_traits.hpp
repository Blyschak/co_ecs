#pragma once

#include <tuple>
#include <type_traits>

namespace cobalt::asl {

template<typename T>
struct is_relocatable : std::is_trivially_copy_constructible<T> {};

template<typename T>
inline constexpr bool is_relocatable_v = is_relocatable<T>::value;

} // namespace cobalt::asl
