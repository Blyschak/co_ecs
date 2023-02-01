#pragma once

#include <co_ecs/detail/hash_table.hpp>

namespace co_ecs::detail {

/// @brief Hash map
///
/// @tparam K Key type
/// @tparam T Mapped value type
/// @tparam Hash Hash type for K
/// @tparam KeyEqual KeyEqual type for K
/// @tparam Allocator Allocator type for std::pair<K, T>
template<typename K,
    typename T,
    typename Hash = std::hash<K>,
    typename KeyEqual = std::equal_to<K>,
    typename Allocator = std::allocator<std::pair<K, T>>>
using hash_map = hash_table<K, T, true, Hash, KeyEqual, Allocator>;

} // namespace co_ecs::detail