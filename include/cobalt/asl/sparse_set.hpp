#pragma once

#include <cobalt/asl/sparse_table.hpp>
#include <cobalt/asl/type_traits.hpp>

namespace cobalt::asl {

/// @brief Sparse set
///
/// @tparam K Key type
/// @tparam Allocator Allocator type
template<std::unsigned_integral K, typename Allocator = std::allocator<K>>
using sparse_set = sparse_table<K, K, false, Allocator>;

template<typename K, typename Allocator>
struct is_relocatable<sparse_set<K, Allocator>> : std::true_type {};

} // namespace cobalt::asl