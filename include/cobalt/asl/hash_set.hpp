#pragma once

#include <cobalt/asl/hash_table.hpp>
#include <cobalt/asl/type_traits.hpp>

namespace cobalt::asl {

/// @brief Hash set class
///
/// @tparam K Key type
/// @tparam Hash Hash class type for K
/// @tparam KeyEqual Key equality type for K
/// @tparam Allocator Allocator type for K
template<typename K,
    typename Hash = std::hash<K>,
    typename KeyEqual = std::equal_to<K>,
    typename Allocator = std::allocator<K>>
using hash_set = hash_table<K, K, false, Hash, KeyEqual, Allocator>;

template<typename K>
struct is_relocatable<hash_set<K>> : std::true_type {};

} // namespace cobalt::asl