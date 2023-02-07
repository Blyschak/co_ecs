#pragma once

#include <memory>
#include <stdexcept>
#include <vector>

namespace co_ecs::detail {

/// @brief Sparse table implementation.
/// @details The underlying layout is:
///          dense:  |value_type|value_type|value_type|value_type|
///          sparse: |key_type  |          |key_type  |key_type  |
///
/// @tparam K Key type
/// @tparam T Mapped type type
/// @tparam is_map Whether this is for a map or set
template<typename K, typename T, bool is_map, typename Allocator>
class sparse_table {
public:
    using key_type = K;
    using mapped_type = T;
    using value_type = std::conditional_t<is_map, std::pair<key_type, mapped_type>, key_type>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using allocator_type = Allocator;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = typename std::allocator_traits<allocator_type>::pointer;
    using const_pointer = typename std::allocator_traits<allocator_type>::const_pointer;
    using iterator = typename std::vector<value_type>::iterator;
    using const_iterator = typename std::vector<value_type>::const_iterator;

    static_assert(sizeof(key_type) <= sizeof(std::size_t));

    /// @brief Default constructor
    constexpr sparse_table() = default;

    /// @brief Move constructor
    ///
    /// @param rhs Sparse container to move from
    constexpr sparse_table(sparse_table&& rhs) noexcept = default;

    /// @brief Copy constructor
    ///
    /// @param rhs Sparse container to copy from
    constexpr sparse_table(const sparse_table& rhs) = default;

    /// @brief Move assignment
    ///
    /// @param rhs Sparse container to move from
    /// @return Container data from rhs moved to
    constexpr auto operator=(sparse_table&& rhs) noexcept -> sparse_table& = default;

    /// @brief Copy assignment
    ///
    /// @param rhs Sparse container to copy from
    /// @return Container data from rhs copied to
    constexpr auto operator=(const sparse_table& rhs) -> sparse_table& = default;

    /// @brief Destructor
    constexpr ~sparse_table() = default;

    /// @brief Construct sparse container from initializer list.
    ///
    /// @param list Initializer list
    constexpr sparse_table(std::initializer_list<value_type> list) {
        _dense.reserve(list.size());
        for (const auto& entry : list) {
            insert(entry);
        }
    }

    /// @brief Return iterator to the beginning of the container
    ///
    /// @return Iterator pointing to the beginning
    constexpr auto begin() noexcept -> iterator {
        return _dense.begin();
    }

    /// @brief Return iterator to the end of the container
    ///
    /// @return Iterator pointing to the end
    constexpr auto end() noexcept -> iterator {
        return _dense.end();
    }

    /// @brief Return const iterator to the beginning of the container
    ///
    /// @return Const iterator pointing to the beginning
    constexpr auto begin() const noexcept -> const_iterator {
        return _dense.begin();
    }

    /// @brief Return const iterator to the end of the container
    ///
    /// @return Const iterator pointing to the end
    constexpr auto end() const noexcept -> const_iterator {
        return _dense.end();
    }

    /// @brief Return const iterator to the beginning of the container
    ///
    /// @return Const iterator pointing to the beginning
    constexpr auto cbegin() const noexcept -> const_iterator {
        return _dense.cbegin();
    }

    /// @brief Return const iterator to the end of the container
    ///
    /// @return Const iterator pointing to the end
    constexpr auto cend() const noexcept -> const_iterator {
        return _dense.cend();
    }

    /// @brief Return the size of the container
    ///
    /// @return Size of the container
    [[nodiscard]] constexpr auto size() const -> std::size_t {
        return _size;
    }

    /// @brief Return sparse vector capacity
    ///
    /// @return Sparse vector capacity
    [[nodiscard]] constexpr auto capacity() const -> std::size_t {
        return _sparse_capacity;
    }

    /// @brief Return wether container is empty
    [[nodiscard]] constexpr auto empty() const -> bool {
        return size() == 0;
    }

    /// @brief Clear elements inside container
    constexpr void clear() noexcept {
        _dense.clear();
        _size = 0;
    }

    /// @brief Reserve space in dense vector
    ///
    /// @param capacity
    constexpr void reserve_dense(std::size_t capacity) {
        _dense.reserve(capacity);
    }

    /// @brief Reserve space in sparse vector
    ///
    /// @param capacity
    constexpr void reserve_sparse(std::size_t capacity) {
        if (capacity > _sparse_capacity) {
            _sparse.resize(capacity);
            _sparse_capacity = capacity;
        }
    }

    /// @brief Find key in the container
    ///
    /// @param key Key to find
    /// @return Const iterator pointing to found key
    ///         or end of the container if the key is
    ///         not present in the container
    constexpr auto find(key_type key) const noexcept -> const_iterator {
        return find_impl(*this, key);
    }

    /// @brief Find key in the container
    ///
    /// @param key Key to find
    /// @return Iterator pointing to found key
    ///         or end of the container if the key is
    ///         not present in the container
    constexpr auto find(key_type key) noexcept -> iterator {
        return find_impl(*this, key);
    }

    /// @brief Test whether container has key
    ///
    /// @param key Key to test
    /// @return True if key is found in the container
    constexpr auto contains(key_type key) const noexcept -> bool {
        return find(key) != end();
    }

    /// @brief Insert a value inside the container
    ///
    /// @param entry Entry to insert
    /// @return A pair of iterator pointing to the inserted element and a
    /// boolean
    ///         set to true in case the insertion happened
    constexpr auto insert(const value_type& entry) -> std::pair<iterator, bool> {
        return emplace(get_key(entry), std::move(get_value(entry)));
    }

    /// @brief Emplace if the key does not exist yet
    ///
    /// @tparam ...Args Type of arguments
    /// @param key Key to insert
    /// @param ...args args to perfectly forward to values constructor
    /// @return A pair of iterator pointing to the inserted element and a
    /// boolean
    ///         set to true in case the insertion happened
    template<typename... Args>
    constexpr auto emplace(key_type key, Args&&... args) -> std::pair<iterator, bool> {
        if (contains(key)) {
            return std::pair(_dense.begin() + _sparse[key], false);
        }

        reserve_sparse(static_cast<std::size_t>(key) + 1);

        if constexpr (is_map) {
            _dense.emplace_back(std::piecewise_construct,
                std::forward_as_tuple(key),
                std::forward_as_tuple(std::forward<Args>(args)...));
        } else {
            _dense.emplace_back(key);
        }
        _sparse[key] = _size;
        ++_size;
        return std::pair(_dense.begin() + _sparse[key], true);
    }

    /// @brief Find an element and return a reference to it
    ///
    /// @param key Key to look for
    /// @return Reference to element found;
    ///         std::out_of_range is thrown if key does not exist in the
    ///         container
    constexpr auto at(const key_type& key) -> mapped_type&
        requires(is_map)
    {
        if (!contains(key)) {
            throw std::out_of_range("Key is not in the BaseSparseSet");
        }
        return get_value(_dense[_sparse[key]]);
    }

    /// @brief Find an element and return a const reference to it
    ///
    /// @param key Key to look for
    /// @return Reference to element found;
    ///         std::out_of_range is thrown if key does not exist in the
    ///         container
    constexpr auto at(const key_type& key) const -> const mapped_type&
        requires(is_map)
    {
        if (!contains(key)) {
            throw std::out_of_range("Key is not in the BaseSparseSet");
        }
        return get_value(_dense[_sparse[key]]);
    }

    /// @brief Find or default construct and insert an element and return a
    /// reference to it
    ///
    /// @param key Key to look for
    /// @return Reference to element found or inserted
    constexpr auto operator[](const key_type& key) noexcept -> mapped_type&
        requires(is_map)
    {
        auto [iter, _] = emplace(key);
        mapped_type& value = get_value(*iter);
        return value;
    }

    /// @brief Erase the key from the container
    ///
    /// @param key Key to erase
    /// @return Number of elements erased
    constexpr auto erase(key_type key) noexcept -> std::size_t {
        if (contains(key)) {
            _dense[_sparse[key]] = std::move(_dense.back());
            _sparse[get_key(_dense[_size - 1])] = _sparse[key];
            _dense.pop_back();
            _size--;
            return 1;
        }
        return 0;
    }

private:
    constexpr static auto find_impl(auto& self, auto& key) -> decltype(auto) {
        if (key < self._sparse_capacity && self._sparse[key] < self._size
            && get_key(self._dense[self._sparse[key]]) == key) {
            return self.begin() + self._sparse[key];
        }
        return self.end();
    }

    constexpr static auto get_key(auto& entry) -> auto& {
        if constexpr (is_map) {
            return entry.first;
        } else {
            return entry;
        }
    }

    constexpr static auto get_value(auto& entry) -> auto& {
        if constexpr (is_map) {
            return entry.second;
        } else {
            return entry;
        }
    }

    std::vector<value_type, Allocator> _dense;
    std::vector<key_type> _sparse;

    key_type _size{};
    size_t _sparse_capacity{};
};

} // namespace co_ecs::detail
