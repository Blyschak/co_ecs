#pragma once

#include <cassert>
#include <cstdint>
#include <new>
#include <optional>
#include <type_traits>

#include <co_ecs/component.hpp>
#include <co_ecs/detail/bits.hpp>
#include <co_ecs/detail/sparse_map.hpp>
#include <co_ecs/entity.hpp>
#include <co_ecs/exceptions.hpp>


namespace co_ecs {

// NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)

/// @brief Block metadata holds pointers where it begins, ends and a component metadata it holds
struct block_metadata {
    std::size_t offset{};
    component_meta meta{};

    block_metadata(std::size_t offset, const component_meta& meta) noexcept : offset(offset), meta(meta) {
    }
};

using blocks_type = detail::sparse_map<component_id_t, block_metadata>;

/// @brief Chunk holds a 16 Kb block of memory that holds components in blocks:
/// |A1|A2|A3|...padding|B1|B2|B3|...padding|C1|C2|C3...padding where A, B, C are component types and A1, B1, C1 and
/// others are components instances.
class chunk {
public:
    /// @brief Chunk size in bytes
    static constexpr std::size_t chunk_bytes = static_cast<const std::size_t>(16U * 1024); // 16 KB

    /// @brief Block allocation alignment
    static constexpr std::size_t alloc_alignment = alignof(entity);

    /// @brief Chunk buffer type
    struct alignas(alloc_alignment) chunk_buffer {
        std::byte data[chunk_bytes];
    };

    /// @brief Construct a new chunk object
    ///
    /// @param set Component metadata set
    chunk(const blocks_type& blocks, std::size_t max_size) :
        _blocks(&blocks), _max_size(max_size), _buffer((new chunk_buffer)->data) {
    }

    /// @brief Deleted copy constructor
    ///
    /// @param rhs Another chunk
    chunk(const chunk& rhs) = delete;

    /// @brief Deleted copy assignment operator
    ///
    /// @param rhs Another chunk
    auto operator=(const chunk& rhs) -> chunk& = delete;

    /// @brief Move constructor
    ///
    /// @param rhs Another chunk
    chunk(chunk&& rhs) noexcept :
        _buffer(rhs._buffer), _size(rhs._size), _max_size(rhs._max_size), _blocks(rhs._blocks) {
        rhs._buffer = nullptr;
    }

    /// @brief Move assignment operator
    ///
    /// @param rhs Right hand side chunk
    /// @return chunk& Resulting chunk
    auto operator=(chunk&& rhs) noexcept -> chunk& {
        _buffer = rhs._buffer;
        _size = rhs._size;
        _max_size = rhs._max_size;
        _blocks = rhs._blocks;

        rhs._buffer = nullptr;
        return *this;
    }

    /// @brief Destroy the chunk object
    ~chunk() {
        if (_buffer == nullptr) {
            return;
        }
        for (const auto& [id, block] : *_blocks) {
            for (std::size_t i = 0; i < _size; i++) {
                block.meta.type->destruct(_buffer + block.offset + i * block.meta.type->size);
            }
        }
        delete _buffer;
    }

    /// @brief Emplace back components into blocks
    ///
    /// @tparam Args Parameter pack
    /// @param args component arguments
    template<component... Args>
    void emplace_back(entity ent, Args&&... args) {
        assert((!full()) && "Chunk is full, cannot add more entities");
        std::construct_at(ptr_unchecked<entity>(size()), ent);
        (..., std::construct_at(ptr_mut<Args>(size()), std::move(args)));
        _size++;
    }

    /// @brief Remove back elements from blocks
    void pop_back() noexcept {
        assert((!empty()) && "Chunk is empty, cannot pop out any entity");
        _size--;
        destroy_at(_size);
    }

    /// @brief Swap end removes a components in blocks at position index and swaps it with the last element from
    /// other_chunk chunk. Returns std::optional of entity that has been moved or std::nullopt if no entities were
    /// moved
    ///
    /// @param index Index to remove components from
    /// @param other_chunk Other chunk
    /// @return std::optional<entity>
    auto swap_erase(std::size_t index, chunk& other) noexcept -> std::optional<entity> {
        assert((index < _size) && "Entity index exceeds chunk size");
        if (size() == 1 || index == _size - 1) {
            pop_back();
            return std::nullopt;
        }
        assert((!other.empty()) && "Other chunk is empty, cannot move entity out of it");
        const std::size_t other_chunk_index = other._size - 1;
        entity ent = *other.ptr_unchecked<entity>(other_chunk_index);
        for (const auto& [id, block] : *_blocks) {
            auto other_block = other._blocks->find(id)->second;
            const auto* type = block.meta.type;
            auto* ptr = other._buffer + other_block.offset + other_chunk_index * type->size;
            type->move_assign(_buffer + block.offset + index * type->size, ptr);
        }
        other.pop_back();
        return ent;
    }

    /// @brief Move components in blocks at position index into other_chunk
    ///
    /// @param index Index to move from
    /// @param other_chunk Chunk to move to
    /// @return std::size_t Other chunk index
    auto move(std::size_t index, chunk& other_chunk) -> std::size_t {
        assert((index < _size) && "Entity index exceeds chunk size");
        assert((!other_chunk.full()) && "Other chunk is full, cannot move entity to it");
        const std::size_t other_chunk_index = other_chunk._size;
        for (const auto& [id, block] : *_blocks) {
            const auto* type = block.meta.type;
            if (!other_chunk._blocks->contains(id)) {
                continue;
            }
            auto* ptr = other_chunk._buffer + other_chunk._blocks->at(id).offset + other_chunk_index * type->size;
            type->move_construct(ptr, _buffer + block.offset + index * type->size);
        }
        other_chunk._size++;
        return other_chunk_index;
    }

    /// @brief Give a const pointer to a component T at index
    ///
    /// @tparam T Component type
    /// @param index Index
    /// @return const T* Resulting pointer
    template<component T>
    inline auto ptr_const(std::size_t index) const -> const T* {
        return ptr_unchecked_impl<const T*>(*this, index);
    }

    /// @brief Give a pointer to a component T at index
    ///
    /// @tparam T Component type
    /// @param index Index
    /// @return const T* Resulting pointer
    template<component T>
    inline auto ptr_mut(std::size_t index) -> T* {
        static_assert(!std::is_same_v<T, entity>, "Cannot give a mutable pointer/reference to the entity");
        return ptr_unchecked_impl<T*>(*this, index);
    }

    /// @brief Get max size, how many elements can this chunk hold
    ///
    /// @return std::size_t Max size of this chunk
    [[nodiscard]] constexpr auto max_size() const noexcept -> std::size_t {
        return _max_size;
    }

    /// @brief Return size
    ///
    /// @return std::size_t Size
    [[nodiscard]] constexpr auto size() const noexcept -> std::size_t {
        return _size;
    }

    /// @brief Check if chunk is full
    ///
    /// @return true If it is full
    /// @return false If it is not full
    [[nodiscard]] constexpr auto full() const noexcept -> bool {
        return size() == max_size();
    }

    /// @brief Check if chunk is empty
    ///
    /// @return true If it is empty
    /// @return false If it is not empty
    [[nodiscard]] constexpr auto empty() const noexcept -> bool {
        return size() == 0;
    }

private:
    template<component T>
    [[nodiscard]] inline auto ptr_unchecked(std::size_t index) -> T* {
        return ptr_unchecked_impl<T*>(*this, index);
    }

    template<component T>
    [[nodiscard]] inline auto ptr_unchecked(std::size_t index) const noexcept -> const T* {
        return ptr_unchecked_impl<const T*>(*this, index);
    }

    template<typename P>
    [[nodiscard]] static inline auto ptr_unchecked_impl(auto&& self, std::size_t index) -> P {
        using component_type = std::remove_const_t<std::remove_pointer_t<P>>;
        const auto& block = self.get_block(component_id::value<component_type>);
        return (reinterpret_cast<P>(self._buffer + block.offset) + index);
    }

    [[nodiscard]] auto get_block(component_id_t id) const -> const block_metadata& {
        return _blocks->at(id);
    }

    inline void destroy_at(std::size_t index) noexcept {
        for (const auto& [id, block] : *_blocks) {
            block.meta.type->destruct(_buffer + block.offset + index * block.meta.type->size);
        }
    }

    std::byte* _buffer{};
    std::size_t _size{};
    std::size_t _max_size{};
    const blocks_type* _blocks;
};

/// @brief Component fetch is a namespace for routines that figure out based on input component_reference how to fetch
/// the component from the chunk
struct component_fetch {
    // clang-format off

    /// @brief Fetches pointer for const component reference
    ///
    /// @tparam C Component reference
    template<component_reference C>
    static auto fetch_pointer(auto&& chunk, std::size_t index)
        -> const decay_component_t<C>* requires(const_component_reference_v<C>) {
            try {
                return chunk.template ptr_const<decay_component_t<C>>(index);
            } catch (const std::out_of_range&) {
                throw component_not_found{ type_meta::of<decay_component_t<C>>() };
            }
        }

    /// @brief Fetches pointer for mutable component reference
    ///
    /// @tparam C Component reference
    template<component_reference C>
    static auto fetch_pointer(auto&& chunk, std::size_t index)
        -> decay_component_t<C>* requires(mutable_component_reference_v<C>) {
            try {
                return chunk.template ptr_mut<decay_component_t<C>>(index);
            } catch (const std::out_of_range&) {
                throw component_not_found{ type_meta::of<decay_component_t<C>>() };
            }
        }

    // clang-format on
};

/// @brief A type aware view into a chunk components
///
/// @tparam Args Components
template<component_reference... Args>
class chunk_view {
public:
    /// @brief Const when all component references are const
    static constexpr bool is_const = const_component_references_v<Args...>;

    using chunk_type = std::conditional_t<is_const, const chunk&, chunk&>;

    /// @brief Chunk view iterator
    class iterator {
    public:
        using iterator_concept = std::forward_iterator_tag;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = int;
        using value_type = std::tuple<Args...>;
        using reference = std::tuple<Args...>;
        using element_type = reference;

        /// @brief Default constructor
        constexpr iterator() = default;

        /// @brief Default destructor
        constexpr ~iterator() = default;

        /// @brief Create iterator out of chunk pointing to the index
        ///
        /// @param c Chunk reference
        /// @param index Index this iterator is pointing to
        explicit constexpr iterator(chunk_type c, std::size_t index = 0) :
            _ptrs(std::make_tuple(component_fetch::fetch_pointer<Args>(c, index)...)) {
        }

        /// @brief Default copy constructor
        ///
        /// @param rhs Right hand side iterator
        constexpr iterator(const iterator& rhs) noexcept = default;

        /// @brief Default copy assignment operator
        ///
        /// @param rhs Right hand side iterator
        /// @return iterator& this iterator
        constexpr auto operator=(const iterator& rhs) noexcept -> iterator& = default;

        /// @brief Default move constructor
        ///
        /// @param rhs Right hand side iterator
        constexpr iterator(iterator&& rhs) noexcept = default;

        /// @brief Default move assignment operator
        ///
        /// @param rhs Right hand side iterator
        /// @return iterator& this iterator
        constexpr auto operator=(iterator&& rhs) noexcept -> iterator& = default;

        /// @brief Pre-increment iterator
        ///
        /// @return iterator_impl& Incremented iterator
        constexpr auto operator++() noexcept -> iterator& {
            std::apply([](auto&&... args) { (args++, ...); }, _ptrs);
            return *this;
        }

        /// @brief Post-increment iterator
        ///
        /// @return iterator_impl& Iterator
        constexpr auto operator++(int) noexcept -> iterator {
            iterator tmp(*this);
            std::apply([](auto&&... args) { (args++, ...); }, _ptrs);
            return tmp;
        }

        /// @brief Dereference iterator
        ///
        /// @return reference Reference to value
        constexpr auto operator*() const noexcept -> reference {
            return std::apply([](auto&&... args) { return std::make_tuple(std::ref(*args)...); }, _ptrs);
        }

        /// @brief Equality operator
        ///
        /// @param rhs Right hand side
        /// @return auto Result of comparison
        constexpr bool operator==(const iterator& rhs) const noexcept {
            return std::get<0>(_ptrs) == std::get<0>(rhs._ptrs);
        }

        /// @brief Spaceship operator
        ///
        /// @param rhs Right hand side
        /// @return auto Result of comparison
        constexpr auto operator<=>(const iterator& rhs) const noexcept {
            return std::get<0>(_ptrs) <=> std::get<0>(rhs._ptrs);
        }

    private:
        std::tuple<std::add_pointer_t<std::remove_reference_t<Args>>...> _ptrs;
    };

    /// @brief Construct a new chunk view object
    ///
    /// @param c Chunk reference
    explicit chunk_view(chunk_type c) : _chunk(c) {
    }

    /// @brief Return iterator to the beginning of a chunk
    ///
    /// @return constexpr iterator
    [[nodiscard]] constexpr auto begin() noexcept -> iterator {
        return iterator(_chunk, 0);
    }

    /// @brief Return iterator to the end of a chunk
    ///
    /// @return constexpr iterator
    [[nodiscard]] constexpr auto end() noexcept -> iterator {
        return iterator(_chunk, _chunk.size());
    }

private:
    chunk_type _chunk;
};

// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
// NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)

} // namespace co_ecs