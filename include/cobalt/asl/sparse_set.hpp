#pragma once

#include <cobalt/asl/sparse_table.hpp>

namespace cobalt::asl {

/// @brief Sparse set
///
/// @tparam K Key type
/// @tparam Allocator Allocator type
template<std::unsigned_integral K, typename Allocator = std::allocator<K>>
using sparse_set = sparse_table<K, K, false, Allocator>;

} // namespace cobalt::asl