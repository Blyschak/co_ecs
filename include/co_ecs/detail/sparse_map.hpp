#pragma once

#include <co_ecs/detail/sparse_table.hpp>

namespace co_ecs::detail {

/// @brief Sparse map
/// @tparam K Key type
/// @tparam T Value type
/// @tparam Allocator Allocator type
template<std::unsigned_integral K, typename T, typename Allocator = std::allocator<std::pair<K, T>>>
using sparse_map = sparse_table<K, T, true, Allocator>;

} // namespace co_ecs::detail