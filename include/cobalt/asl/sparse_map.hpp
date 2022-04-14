#pragma once

#include <cobalt/asl/sparse_table.hpp>
#include <cobalt/asl/type_traits.hpp>

namespace cobalt::asl {

/// @brief Sparse map
/// @tparam K Key type
/// @tparam T Value type
/// @tparam Allocator Allocator type
template<std::unsigned_integral K, typename T, typename Allocator = std::allocator<std::pair<K, T>>>
using sparse_map = sparse_table<K, T, true, Allocator>;

template<typename K, typename T, typename Allocator>
struct is_relocatable<sparse_map<K, T, Allocator>> : std::true_type {};

} // namespace cobalt::asl