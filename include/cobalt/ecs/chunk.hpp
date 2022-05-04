#pragma once

#include <cassert>
#include <cstdint>
#include <optional>
#include <type_traits>

#include <cobalt/asl/algorithm.hpp>
#include <cobalt/asl/sparse_map.hpp>
#include <cobalt/ecs/component.hpp>
#include <cobalt/ecs/entity.hpp>
#include <cobalt/ecs/exceptions.hpp>

namespace cobalt::ecs {

/// @brief Chunk holds a 16 Kb block of memory that holds components in blocks:
/// |A1|A2|A3|...padding|B1|B2|B3|...padding|C1|C2|C3...padding where A, B, C are component types and A1, B1, C1 and
/// others are components instances.
class chunk {
public:
    /// @brief Chunk size in bytes
    static constexpr std::size_t chunk_bytes = 16 * 1024; // 16 KB

    /// @brief Block allocation alignment
    static constexpr std::size_t alloc_alignment = alignof(entity);

    /// @brief Block metadata holds pointers where it begins, ends and a component metadata it holds
    struct block_metadata {
        std::byte* begin{};
        component_meta meta{};
    };

    /// @brief Construct a new chunk object
    ///
    /// @param set Component metadata set
    chunk(const component_meta_set& components_meta) {
        _buffer = reinterpret_cast<std::byte*>(std::aligned_alloc(alloc_alignment, chunk_bytes));
        _max_size = get_max_size(components_meta);
        init_blocks(components_meta);
    }

    /// @brief Deleted copy constructor
    ///
    /// @param rhs Another chunk
    chunk(const chunk& rhs) = delete;

    /// @brief Deleted copy assignment operator
    ///
    /// @param rhs Another chunk
    chunk& operator=(const chunk& rhs) = delete;

    /// @brief Move constructor
    ///
    /// @param rhs Another chunk
    chunk(chunk&& rhs) noexcept {
        _buffer = rhs._buffer;
        _size = rhs._size;
        _max_size = rhs._max_size;
        _blocks = rhs._blocks;

        rhs._buffer = nullptr;
    }

    /// @brief Move assignment operator
    ///
    /// @param rhs Right hand side chunk
    /// @return chunk& Resulting chunk
    chunk& operator=(chunk&& rhs) noexcept {
        _buffer = rhs._buffer;
        _size = rhs._size;
        _max_size = rhs._max_size;
        _blocks = rhs._blocks;

        rhs._buffer = nullptr;
        return *this;
    }

    /// @brief Destroy the chunk object
    ~chunk() {
        if (!_buffer) {
            return;
        }
        for (const auto& [id, block] : _blocks) {
            for (std::size_t i = 0; i < _size; i++) {
                block.meta.type->destruct(block.begin + i * block.meta.type->size);
            }
        }
        std::free(_buffer);
    }

    /// @brief Emplace back components into blocks
    ///
    /// @tparam Args Paramter pack
    /// @param args component arguments
    template<component... Args>
    void emplace_back(entity ent, Args&&... args) {
        assert(!full());
        std::construct_at(ptr_unchecked<entity>(size()), ent);
        (..., std::construct_at(ptr_unchecked<Args>(size()), std::move(args)));
        _size++;
    }

    /// @brief Remove back elements from blocks
    void pop_back() noexcept {
        assert(!empty());
        _size--;
        destroy_at(_size);
    }

    /// @brief Swap end removes a components in blocks at position index and swaps it with the last element from
    /// other_chunk chunk. Returns std::optional of entity that has been moved or std::nullopt if no entities were moved
    ///
    /// @param index Index to remove components from
    /// @param other_chunk Other chunk
    /// @return std::optional<entity>
    std::optional<entity> swap_erase(std::size_t index, chunk& other) {
        assert(index < _size);
        if (size() == 1 || index == _size - 1) {
            pop_back();
            return std::nullopt;
        }
        assert(!other_chunk.empty());
        std::size_t other_chunk_index = other._size - 1;
        entity ent = *other.ptr_unchecked<entity>(other_chunk_index);
        for (const auto& [id, block] : _blocks) {
            auto* type = block.meta.type;
            auto ptr = other._blocks[id].begin + other_chunk_index * type->size;
            type->move_assign(block.begin + index * type->size, ptr);
        }
        other.pop_back();
        return ent;
    }

    /// @brief Move components in blocks at position index into other_chunk
    ///
    /// @param index Index to move from
    /// @param other_chunk Chunk to move to
    /// @return std::size_t Other chunk index
    std::size_t move(std::size_t index, chunk& other_chunk) {
        assert(index < _size);
        assert(!other_chunk.full());
        std::size_t other_chunk_index = other_chunk._size;
        for (const auto& [id, block] : _blocks) {
            auto* type = block.meta.type;
            if (!other_chunk._blocks.contains(id)) {
                continue;
            }
            auto ptr = other_chunk._blocks.at(id).begin + other_chunk_index * type->size;
            type->move_construct(ptr, block.begin + index * type->size);
        }
        other_chunk._size++;
        return other_chunk_index;
    }

    template<component_reference T>
    inline decay_component_t<T>* ptr(std::size_t index) {
        if constexpr (mutable_component_reference_v<T>) {
            static_assert(
                !std::is_same_v<decay_component_t<T>, entity>, "Cannot give a mutable reference to the entity");
        }
        return ptr<decay_component_t<T>>(index);
    }

    template<component_reference T>
    inline const decay_component_t<T>* ptr(std::size_t index) const {
        return ptr<decay_component_t<T>>(index);
    }

    template<component T>
    inline const T* ptr(std::size_t index) const {
        return ptr_unchecked_impl<const T*>(*this, index);
    }

    template<component T>
    inline T* ptr(std::size_t index) {
        return ptr_unchecked_impl<T*>(*this, index);
    }

    /// @brief Get max size, how many elements can this chunk hold
    ///
    /// @return std::size_t Max size of this chunk
    [[nodiscard]] constexpr std::size_t max_size() const noexcept {
        return _max_size;
    }

    /// @brief Return size
    ///
    /// @return std::size_t Size
    [[nodiscard]] constexpr std::size_t size() const noexcept {
        return _size;
    }

    /// @brief Check if chunk is full
    ///
    /// @return true If it is full
    /// @return false If it is not full
    [[nodiscard]] constexpr bool full() const noexcept {
        return size() == max_size();
    }

    /// @brief Check if chunk is empty
    ///
    /// @return true If it is empty
    /// @return false If it is not empty
    [[nodiscard]] constexpr bool empty() const noexcept {
        return size() == 0;
    }

private:
    template<component T>
    inline T* ptr_unchecked(std::size_t index) {
        return ptr_unchecked_impl<T*>(*this, index);
    }

    template<component T>
    inline const T* ptr_unchecked(std::size_t index) const {
        return ptr_unchecked_impl<const T*>(*this, index);
    }

    template<typename P>
    static inline P ptr_unchecked_impl(auto&& self, std::size_t index) {
        const auto& block = self.get_block(component_family::id<std::remove_pointer_t<std::remove_const_t<P>>>);
        return (reinterpret_cast<P>(block.begin) + index);
    }

    block_metadata& get_block(component_id id) {
        try {
            return _blocks.at(id);
        } catch (std::out_of_range) {
            throw component_not_found{ id };
        }
    }

    const block_metadata& get_block(component_id id) const {
        try {
            return _blocks.at(id);
        } catch (std::out_of_range) {
            throw component_not_found{ id };
        }
    }

    inline void destroy_at(std::size_t index) noexcept {
        for (const auto& [id, block] : _blocks) {
            block.meta.type->destruct(block.begin + index * block.meta.type->size);
        }
    }

    void init_blocks(const component_meta_set& components_meta) {
        std::byte* ptr = _buffer;

        // make space for entity
        ptr = add_block(ptr, component_meta::of<entity>());

        // space for all components
        for (const auto& meta : components_meta) {
            ptr = add_block(ptr, meta);
        }

        // sanity assertion
        assert(ptr <= _buffer + chunk_bytes);
    }

    std::byte* add_block(std::byte* ptr, const component_meta& meta) {
        std::size_t size_in_bytes = _max_size * meta.type->size;
        std::size_t align = meta.type->align;

        _blocks.emplace(meta.id, ptr, meta);

        ptr += asl::mod_2n(std::bit_cast<std::size_t>(ptr), align) + size_in_bytes;

        return ptr;
    }

    // Calculates the maxium size of individual components this chunk buffer can hold
    static std::size_t get_max_size(const component_meta_set& components_meta) {
        // Calculate packed structure size
        auto netto_size = aligned_components_size(components_meta);
        // Remaining size for packed components
        auto remaining_space = chunk_bytes - netto_size;
        // Calculate how much components we can pack into remaining space
        auto remaining_elements_count = remaining_space / packed_components_size(components_meta);
        //
        return remaining_elements_count + 1;
    }

    // Calculate size of packed structure of components
    static std::size_t packed_components_size(const component_meta_set& components_meta) noexcept {
        return std::accumulate(components_meta.begin(),
            components_meta.end(),
            component_meta::of<entity>().type->size,
            [](const auto& res, const auto& meta) { return res + meta.type->size; });
    }

    // Calculate size of properly aligned structure of components
    static std::size_t aligned_components_size(const component_meta_set& components_meta) noexcept {
        auto begin = alloc_alignment;
        auto end = begin;

        // Add single component element size accounting for its alignment
        auto add_elements = [&end](const component_meta& meta) {
            end += asl::mod_2n(std::bit_cast<std::size_t>(end), meta.type->align);
            end += meta.type->size;
        };

        add_elements(component_meta::of<entity>());
        for (const auto& meta : components_meta) {
            add_elements(meta);
        }

        return end - begin;
    }

    std::byte* _buffer;
    std::size_t _size{};
    std::size_t _max_size{};
    asl::sparse_map<component_id, block_metadata> _blocks;
};

/// @brief A type aware view into a chunk components
///
/// @tparam Args Components
template<component_reference... Args>
class chunk_view {
public:
    /// @brief Chunk view iterator
    class iterator {
    public:
        using iterator_concept = std::forward_iterator_tag;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = int;
        using value_type = std::tuple<Args...>;
        using reference = std::tuple<Args...>;
        using element_type = value_type;

        /// @brief Default constructor
        constexpr iterator() = default;

        /// @brief Create iterator out of chunk pointing to the index
        ///
        /// @param c Chunk reference
        /// @param index Index this iterator is pointing to
        constexpr iterator(chunk& c, std::size_t index = 0) : _ptrs(std::make_tuple(c.ptr<Args>(index)...)) {
        }

        /// @brief Default copy constructor
        ///
        /// @param rhs Right hand side iterator
        constexpr iterator(const iterator& rhs) = default;

        /// @brief Default copy assignment operator
        ///
        /// @param rhs Right hand side iterator
        /// @return iterator& this iterator
        constexpr iterator& operator=(const iterator& rhs) = default;

        /// @brief Default move constructor
        ///
        /// @param rhs Right hand side iterator
        constexpr iterator(iterator&& rhs) = default;

        /// @brief Default move assignment operator
        ///
        /// @param rhs Right hand side iterator
        /// @return iterator& this iterator
        constexpr iterator& operator=(iterator&& rhs) = default;

        /// @brief Pre-increment iterator
        ///
        /// @return iterator_impl& Incremented iterator
        constexpr iterator& operator++() noexcept {
            std::apply([](auto&&... args) { (args++, ...); }, _ptrs);
            return *this;
        }

        /// @brief Post-increment iterator
        ///
        /// @return iterator_impl& Iterator
        constexpr iterator operator++(int) noexcept {
            iterator tmp(*this);
            std::apply([](auto&&... args) { (args++, ...); }, _ptrs);
            return tmp;
        }

        /// @brief Dereference iterator
        ///
        /// @return reference Reference to value
        constexpr reference operator*() const noexcept {
            return std::apply([](auto&&... args) { return std::make_tuple(std::ref(*args)...); }, _ptrs);
        }

        /// @brief Spaceship operator
        ///
        /// @param rhs Right hand side
        /// @return auto Result of comparison
        constexpr auto operator<=>(const iterator& rhs) const noexcept = default;

    private:
        std::tuple<std::add_pointer_t<decay_component_t<Args>>...> _ptrs;
    };

    /// @brief Construct a new chunk view object
    ///
    /// @param c Chunk reference
    chunk_view(chunk& c) : _chunk(c) {
    }

    /// @brief Return iterator to the beginning of a chunk
    ///
    /// @return constexpr iterator
    [[nodiscard]] constexpr iterator begin() noexcept {
        return iterator(_chunk, 0);
    }

    /// @brief Return iterator to the end of a chunk
    ///
    /// @return constexpr iterator
    [[nodiscard]] constexpr iterator end() noexcept {
        return iterator(_chunk, _chunk.size());
    }

private:
    chunk& _chunk;
};

} // namespace cobalt::ecs