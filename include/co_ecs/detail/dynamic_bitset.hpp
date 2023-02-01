#pragma once

#include <co_ecs/detail/bits.hpp>

#include <algorithm>
#include <climits>
#include <cstdint>
#include <ranges>
#include <vector>

namespace co_ecs::detail {

/// @brief Dynamically growing bitset
///
/// TODO: Small size optimization
///
/// @tparam A Allocator type
template<typename T = std::uint64_t, typename A = std::allocator<T>>
class dynamic_bitset {
public:
    using block_type = T;
    using allocator_type = A;
    using storage_type = std::vector<block_type, allocator_type>;

    /// @brief Construct a new dynamic bitset object
    ///
    /// @param initial_bits Initial size of the bitset
    explicit dynamic_bitset(std::size_t initial_blocks = 1) {
        _blocks.resize(initial_blocks);
    }

    /// @brief Check if bit at given position is set
    ///
    /// @param pos Position of the bit
    /// @return true If bit is set
    /// @return false If bit is unset
    [[nodiscard]] inline bool test(std::size_t pos) const noexcept {
        const auto [block_index, bit_pos] = block_and_bit(pos);
        if (block_index < _blocks.size()) {
            return _blocks[block_index] & (block_type{ 1 } << bit_pos);
        }
        return false;
    }

    /// @brief Set the bit value
    ///
    /// @param pos Position of the bit to set
    /// @param value Value to set
    /// @return dynamic_bitset&
    inline dynamic_bitset& set(std::size_t pos, bool value = true) {
        const auto [block_index, bit_pos] = block_and_bit(pos);
        if (block_index >= _blocks.size()) {
            _blocks.resize(block_index + 1);
        }
        if (value) {
            _blocks[block_index] |= (block_type{ 1 } << bit_pos);
        } else {
            _blocks[block_index] &= ~(block_type{ 1 } << bit_pos);

            std::size_t i = _blocks.size() - 1;
            while (!_blocks[i] && i != 0) {
                i--;
            }
            _blocks.resize(i + 1);
        }
        return *this;
    }

    /// @brief Clear all bits
    inline void clear() noexcept {
        _blocks.resize(1);
        _blocks[0] = 0;
    }

    /// @brief Equality operator
    ///
    /// @param rhs Right hand side bitset
    /// @return true If bitsets are equal
    /// @return false If bitsets aren't equal
    bool operator==(const dynamic_bitset& rhs) const noexcept {
        return std::ranges::equal(_blocks, rhs._blocks);
    }

private:
    friend struct std::hash<dynamic_bitset>;

    static inline std::pair<std::size_t, std::size_t> block_and_bit(std::size_t pos) {
        const auto bit_pos = pos % (sizeof(block_type) * CHAR_BIT);
        const auto block_index = pos / (sizeof(block_type) * CHAR_BIT);
        return std::make_pair(block_index, bit_pos);
    }

    storage_type _blocks;
};

} // namespace co_ecs::detail

namespace std {

/// @brief Hash implementation for dynamic_bitset
template<typename A>
struct hash<co_ecs::detail::dynamic_bitset<A>> {
    std::size_t operator()(const co_ecs::detail::dynamic_bitset<A>& bitset) const {
        std::size_t hash = 0;
        for (auto block : bitset._blocks) {
            hash ^= block;
        }
        return hash;
    }
};

} // namespace std
