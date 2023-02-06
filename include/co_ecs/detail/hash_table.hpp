#pragma once

#include <cassert>
#include <memory>
#include <stdexcept>
#include <vector>

#include <co_ecs/detail/bits.hpp>

namespace co_ecs::detail {

/// @brief Approximately calculate 85% of passed value. 85% is used as a reserve threshold.
///
/// @param value Value
/// @return decltype(auto) Result
constexpr auto approx_85_percent(auto value) noexcept -> decltype(auto) {
    return (value * 870) >> 10U; // NOLINT(readability-magic-numbers)
}

/// @brief Approximately calculate 40% of passed value. 40% is used as a reserve threshold.
///
/// @param value Value
/// @return decltype(auto) Result
constexpr auto approx_40_percent(auto value) noexcept -> decltype(auto) {
    return (value * 409) >> 10U; // NOLINT(readability-magic-numbers)
}

/// @brief Hash Table implementation. This implementation uses open addressing hash table implementation using robin
/// hood hashing algorithm.
///
/// @tparam K Key type
/// @tparam T Mapped type
/// @tparam is_map Wether this is a map or a set
/// @tparam Hash Hash type for K
/// @tparam KeyEqual KeyEqual type for K
/// @tparam Allocator Allocator type for K
template<typename K, typename T, bool is_map, typename Hash, typename KeyEqual, typename Allocator>
class hash_table {
public:
    using key_type = K;
    using mapped_type = T;
    using value_type = std::conditional_t<is_map, std::pair<key_type, mapped_type>, key_type>;
    using allocator_type = Allocator;
    using allocator_traits = std::allocator_traits<allocator_type>;
    using hasher = Hash;
    using key_equal = KeyEqual;
    using size_type = std::size_t;

    constexpr static size_type default_bucket_count = 16; // the number of buckets the hash table is initialized with

private:
    /// @brief Bucket info
    struct bucket_info {
        bool occupied{ false };
        size_t hash{};
        size_t psl{};
    };

    using info_storage = std::vector<bucket_info>;
    using info_iterator = typename info_storage::iterator;
    using info_const_iterator = typename info_storage::const_iterator;

    /// @brief Buckets storage
    class buckets {
    public:
        /// @brief Construct from allocator and size
        ///
        /// @param alloc: Allocator object
        /// @param size: Requested size
        constexpr buckets(const allocator_type& alloc, typename allocator_type::size_type size) :
            a(alloc), b(a.allocate(size)), e(b + size), z(e) {
        }

        /// @brief Deleted copy constructor
        ///
        /// @param rhs Another buckets
        constexpr buckets(const buckets& rhs) = delete;

        /// @brief Move constructor
        ///
        /// @param rhs Another buckets
        constexpr buckets(buckets&& rhs) noexcept {
            swap(std::move(rhs));
        }

        /// @brief Deleted copy assignment operator
        ///
        /// @param rhs Another buckets
        /// @return Buckets
        constexpr auto operator=(const buckets& rhs) -> buckets& = delete;

        /// @brief Move assignment operator
        ///
        /// @param rhs Another buckets
        /// @return Buckets
        constexpr auto operator=(buckets&& rhs) noexcept -> buckets& {
            swap(std::move(rhs));
            return *this;
        }

        /// @brief Destructor
        constexpr ~buckets() {
            a.deallocate(b, e - b);
        }

        /// @brief Swap two buckets
        ///
        /// @param rhs
        void swap(buckets&& rhs) noexcept {
            std::swap(a, rhs.a);
            std::swap(b, rhs.b);
            std::swap(e, rhs.e);
            std::swap(z, rhs.z);
        }

        /// @brief Get the allocator object
        ///
        /// @return Allocator object
        constexpr auto allocator() noexcept -> allocator_type& {
            return a;
        }

        /// @brief Get the allocator object
        ///
        /// @return Allocator object
        constexpr auto allocator() const noexcept -> const allocator_type& {
            return a;
        }

        /// @brief Get iterator to the begin of the vector
        ///
        /// @return Iterator
        constexpr auto begin() noexcept -> value_type* {
            return b;
        }

        /// @brief Get const iterator to the begin of the vector
        ///
        /// @return Iterator
        constexpr auto begin() const noexcept -> const value_type* {
            return b;
        }

        /// @brief Get iterator to the end of the vector
        ///
        /// @return Iterator
        constexpr auto end() noexcept -> value_type* {
            return e;
        }

        /// @brief Get const iterator to the end of the vector
        ///
        /// @return Iterator
        constexpr auto end() const noexcept -> const value_type* {
            return e;
        }

        /// @brief Returns the size of the vector
        ///
        /// @return Size of the vector
        [[nodiscard]] constexpr auto size() const noexcept -> size_type {
            return e - b;
        }

    private:
        allocator_type a{};
        value_type* b{};
        value_type* e{};
        value_type* z{};
    };

    /// @brief Hash table iterator implementation
    ///
    /// @tparam is_const Wether this implementation is for const_iterator or iterator
    template<bool is_const = true>
    class iterator_impl {
    public:
        using iterator_concept = std::forward_iterator_tag;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = int;
        using value_type = hash_table::value_type;
        using element_type = hash_table::value_type;
        using reference = std::conditional_t<is_const, const value_type&, value_type&>;
        using pointer = std::conditional_t<is_const, const value_type*, value_type*>;
        using info_iter = std::conditional_t<is_const, info_const_iterator, info_iterator>;

        constexpr iterator_impl() = default;
        constexpr ~iterator_impl() = default;

        /// @brief Iterator constructor
        ///
        /// @param ptr Pointer in the bucket
        /// @param end End pointer in the bucket
        /// @param info Info in the bucket
        constexpr explicit iterator_impl(pointer ptr, pointer end, info_iter info) noexcept :
            _ptr(ptr), _end(end), _info(info) {
            // fast forward to the next occupied entry in the buckets array
            // if not occupied already or it is not the end.
            if (_ptr != _end && !_info->occupied) {
                fast_forward();
            }
        }

        constexpr iterator_impl(const iterator_impl& other) noexcept = default;
        constexpr auto operator=(const iterator_impl& other) noexcept -> iterator_impl& = default;
        constexpr iterator_impl(iterator_impl&& other) noexcept = default;
        constexpr auto operator=(iterator_impl&& other) noexcept -> iterator_impl& = default;

        /// @brief Pre-increment iterator
        ///
        /// @return iterator_impl& Incremented iterator
        constexpr auto operator++() noexcept -> iterator_impl& {
            _info++;
            _ptr++;
            fast_forward();
            return *this;
        }

        /// @brief Post-increment iterator
        ///
        /// @return iterator_impl& Iterator
        constexpr auto operator++(int) noexcept -> iterator_impl {
            iterator_impl tmp(*this);
            *this ++;
            return tmp;
        }

        /// @brief Dereference iterator
        ///
        /// @return reference Reference to value
        constexpr auto operator*() const noexcept -> reference {
            return *_ptr;
        }

        /// @brief Dereference iterator
        ///
        /// @return reference Pointer to value
        constexpr auto operator->() const noexcept -> pointer {
            return _ptr;
        }

        /// @brief Spaceship operator
        ///
        /// @param rhs Right hand side
        /// @return auto Result of comparison
        constexpr auto operator<=>(const iterator_impl& rhs) const noexcept = default;

    private:
        constexpr void fast_forward() noexcept {
            while (_ptr != _end && !_info->occupied) {
                _info++;
                _ptr++;
            }
        }

        pointer _ptr{};
        pointer _end{};
        info_iter _info{};
    };

public:
    using iterator = iterator_impl<false>;
    using const_iterator = iterator_impl<true>;

    /// @brief Default construct hash table
    constexpr hash_table() : _buckets(allocator_type(), default_bucket_count), _info(default_bucket_count) {
    }

    /// @brief Hash table destructor
    constexpr ~hash_table() {
        for (auto& value : *this) {
            allocator_traits::destroy(_buckets.allocator(), std::addressof(value));
        }
    }

    /// @brief Construct hash table
    ///
    /// @param bucket_count Initial bucket count to create hash table with
    /// @param hash Hash object
    /// @param key_equal KeyEqual object
    /// @param alloc Allocator object
    constexpr explicit hash_table(size_type bucket_count,
        const Hash& hash = Hash(),
        const key_equal& equal = key_equal(),
        const Allocator& alloc = Allocator()) :
        _buckets(alloc, bucket_count),
        _info(bucket_count), _hash(hash), _equal(equal) {
        assert((bucket_count % 2 == 0) && "Bucket count must be a power of two");
    }

    /// @brief Construct hash table
    ///
    /// @param alloc Allocator object
    constexpr explicit hash_table(const Allocator& alloc) : hash_table(default_bucket_count, alloc) {
    }

    /// @brief Construct hash table
    ///
    /// @param bucket_count Initial bucket count to create hash table with
    /// @param alloc Allocator object
    constexpr hash_table(size_type bucket_count, const Allocator& alloc) : hash_table(bucket_count, Hash(), alloc) {
    }

    /// @brief Construct hash table
    ///
    /// @param bucket_count Initial bucket count to create hash table with
    /// @param hash Hash object
    /// @param alloc Allocator object
    constexpr hash_table(size_type bucket_count, const Hash& hash, const Allocator& alloc) :
        hash_table(bucket_count, hash, key_equal(), alloc) {
    }

    /// @brief Construct hash table from iterator pair
    ///
    /// @tparam Input iterator type
    ///
    /// @param first First iterator
    /// @param last Last iterator
    /// @param bucket_count Initial bucket count to create hash table with
    /// @param hash Hash object
    /// @param key_equal KeyEqual object
    /// @param alloc Allocator object
    template<typename input_iterator>
    constexpr hash_table(input_iterator first,
        input_iterator last,
        size_type bucket_count = default_bucket_count,
        const Hash& hash = Hash(),
        const key_equal& equal = key_equal(),
        const Allocator& alloc = Allocator()) :
        _buckets(alloc, bucket_count),
        _info(default_bucket_count), _equal(equal), _hash(hash) {
        assert((bucket_count % 2 == 0) && "Bucket count must be a power of two");
        for (; first != last; ++first) {
            emplace_or_assign_impl(std::move(*first));
        }
    }

    /// @brief Construct hash table from iterator pair
    ///
    /// @tparam Input iterator type
    ///
    /// @param first First iterator
    /// @param last Last iterator
    /// @param bucket_count Initial bucket count to create hash table with
    /// @param alloc Allocator object
    template<typename input_iterator>
    constexpr hash_table(input_iterator first,
        input_iterator last,
        size_type bucket_count,
        const Allocator& alloc = Allocator()) :
        hash_table(first, last, bucket_count, Hash(), key_equal(), alloc) {
    }

    /// @brief Construct hash table from iterator pair
    ///
    /// @tparam Input iterator type
    ///
    /// @param first First iterator
    /// @param last Last iterator
    /// @param bucket_count Initial bucket count to create hash table with
    /// @param hash Hash object
    /// @param alloc Allocator object
    template<typename input_iterator>
    constexpr hash_table(input_iterator first,
        input_iterator last,
        size_type bucket_count,
        const Hash& hash,
        const Allocator& alloc = Allocator()) :
        hash_table(first, last, bucket_count, hash, key_equal(), alloc) {
    }

    /// @brief Construct hash table from initializer list
    ///
    /// @param init Initializer list
    /// @param bucket_count Initial bucket count to create hash table with
    /// @param hash Hash object
    /// @param alloc Allocator object
    constexpr hash_table(std::initializer_list<value_type> init,
        size_type bucket_count = default_bucket_count,
        const Hash& hash = Hash(),
        const KeyEqual& equal = KeyEqual(),
        const Allocator& alloc = Allocator()) :
        hash_table(init.begin(), init.end(), bucket_count, hash, equal, alloc) {
    }

    /// @brief Copy constructor
    ///
    /// @param rhs Right hand side
    constexpr hash_table(const hash_table& rhs) :
        _info(rhs._info), _size(rhs._size), _hash(rhs._hash), _equal(rhs._equal),
        _buckets(rhs._buckets.allocator(), rhs._buckets.size()) {
        for (std::size_t idx{}; const auto& info : _info) {
            if (info.occupied) {
                std::uninitialized_copy_n(rhs._buckets.begin() + idx, 1, _buckets.begin() + idx);
            }
            idx++;
        }
    }

    /// @brief Copy assignment operator
    ///
    /// @param rhs Right hand side
    constexpr auto operator=(const hash_table& rhs) -> hash_table& {
        hash_table copy(rhs);
        swap(std::move(copy));
        return *this;
    }

    /// @brief Move copy constructor
    ///
    /// @param rhs Right hand side
    constexpr hash_table(hash_table&& rhs) noexcept {
        swap(std::move(rhs));
    }

    /// @brief Move assignment operator
    ///
    /// @param rhs Right hand side
    constexpr auto operator=(hash_table&& rhs) noexcept -> hash_table& {
        swap(std::move(rhs));
        return *this;
    }

    /// @brief Swap with other hash table
    ///
    /// @param rhs Other hash table
    constexpr void swap(hash_table&& rhs) {
        std::swap(_buckets, rhs._buckets);
        std::swap(_info, rhs._info);
        std::swap(_size, rhs._size);
        std::swap(_equal, rhs._equal);
        std::swap(_hash, rhs._hash);
    }

    /// @brief Get the allocator object
    ///
    /// @return allocator_type Allocator object
    constexpr auto get_allocator() const noexcept -> allocator_type {
        return _buckets.allocator();
    }

    /// @brief Get or default construct mapped type value associated with key
    ///
    /// @param key Key to insert
    /// @return mapped_type& Reference to the mapped type
    auto operator[](const key_type& key) -> mapped_type&
        requires(is_map)
    {
        if (auto iter = find(key); iter != end()) {
            return iter->second;
        }
        auto [iter, _] = emplace_impl(std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple());
        return iter->second;
    }

    /// @brief Get mapped type value associated with key
    ///
    /// @throws std::out_of_range Out of range error
    ///
    /// @param key Key to insert
    /// @return mapped_type& Reference to the mapped type
    auto at(const key_type& key) -> mapped_type&
        requires(is_map)
    {
        if (auto iter = find_impl(*this, key); iter != end()) {
            return iter->second;
        }
        throw std::out_of_range{ "key not found in hash_table" };
    }

    /// @brief Get mapped type value associated with key
    ///
    /// @throws std::out_of_range Out of range error
    ///
    /// @param key Key to insert
    /// @return const mapped_type& Reference to the mapped type
    auto at(const key_type& key) const -> const mapped_type&
        requires(is_map)
    {
        if (auto iter = find_impl(*this, key); iter != end()) {
            return iter->second;
        }
        throw std::out_of_range{ "key not found in hash_table" };
    }

    /// @brief Clear all values in the table
    void clear() noexcept {
        // destroy held elements
        for (auto& value : *this) {
            allocator_traits::destroy(_buckets.allocator(), std::addressof(value));
        }
        // clear the bucket info
        for (auto& info : _info) {
            info = bucket_info{};
        }
        _size = 0;
    }

    /// @brief Insert value into the hash table if it does not exist yet
    ///
    /// @param value Value to insert
    /// @return std::pair<iterator, bool> Pair of iterator to the inserted/found value and a boolean indicating whether
    /// insertion happened
    auto insert(const value_type& value) -> std::pair<iterator, bool> {
        return emplace_impl(value);
    }

    /// @brief Insert value into the hash table if it does not exist yet. Hint is ignored in the implementation.
    ///
    /// @param hint Hint to where insert
    /// @param value Value to insert
    /// @return iterator Iterator to the inserted/found value
    auto insert([[maybe_unused]] iterator hint, value_type&& value) -> iterator {
        return emplace_impl(std::move(value)).first;
    }

    /// @brief Insert value into the hash table if it does not exist yet. Hint is ignored in the implementation.
    ///
    /// @param hint Hint to where insert
    /// @param value Value to insert
    /// @return iterator Iterator to the inserted/found value
    auto insert([[maybe_unused]] iterator hint, const value_type& value) -> iterator {
        return emplace_impl(value).first;
    }

    /// @brief Insert value into the hash table or assign to existing one
    ///
    /// @param value Value to insert
    /// @return std::pair<iterator, bool> Pair of iterator to the inserted/found value and a boolean indicating whether
    /// insertion happened
    auto insert_or_assign(const value_type& value) -> std::pair<iterator, bool> {
        return emplace_or_assign_impl(value);
    }

    /// @brief Inserts a new element into the container constructed in-place with the given args if there is no element
    /// with the key in the container.
    ///
    /// @tparam Args Parameter pack
    /// @param args args to construct value from
    /// @return std::pair<iterator, bool> Returns a pair consisting of an iterator to the inserted element, or the
    /// already-existing element if no insertion happened, and a bool denoting whether the insertion took place
    template<typename... Args>
    auto emplace(Args&&... args) -> std::pair<iterator, bool> {
        return emplace_impl(std::forward<Args>(args)...);
    }

    /// @brief Erase key from the container
    ///
    /// @param key Key
    void erase(const key_type& key) {
        return erase_impl(key);
    }

    /// @brief Erase key that compares to c
    ///
    /// @tparam C Type
    /// @param comparable_key Value key compares to
    template<typename C>
    void erase(C&& comparable_key) {
        return erase_impl(std::forward<C>(comparable_key));
    }

    /// @brief Erase entry at position
    ///
    /// @param pos Position to erase
    void erase(const_iterator pos) {
        return erase_impl(get_key(*pos));
    }

    /// @brief Erase entry at position
    ///
    /// @param pos Position to erase
    void erase(iterator pos) {
        return erase_impl(get_key(*pos));
    }

    /// @brief Returns the number of elements with key that compares equal to the specified argument key, which is
    /// either 1 or 0 since this container does not allow duplicates.
    ///
    /// @param key key to compare to
    /// @return size_type Number of elements (1 or 0)
    [[nodiscard]] auto count(const key_type& key) const noexcept -> size_type {
        return find_impl(*this, key) != end() ? 1 : 0;
    }

    /// @brief Check if key exists in the container
    ///
    /// @param key Key to check
    /// @return true If key exists in the container
    /// @return false If key does not exist in the container
    [[nodiscard]] auto contains(const key_type& key) const noexcept -> bool {
        return count(key) > 0;
    }

    /// @brief Find key in the container and return an iterator pointing to it
    ///
    /// @param key Key to find
    /// @return iterator Iterator result
    [[nodiscard]] auto find(const auto& key) noexcept -> iterator {
        return find_impl(*this, key);
    }

    /// @brief Find key in the container and return an iterator pointing to it
    ///
    /// @param key Key to find
    /// @return iterator Iterator result
    [[nodiscard]] auto find(const key_type& key) const noexcept -> const_iterator {
        return find_impl(*this, key);
    }

    /// @brief Reserve space in the bucket array
    ///
    /// @param new_size New size
    void reserve(size_type new_size) {
        if (new_size < _size) {
            return;
        }

        hash_table h_table(new_size, _hash, _equal, _buckets.allocator());

        for (auto& value : *this) {
            h_table._emplace_impl(std::move(value));
        }

        std::swap(_buckets, h_table._buckets);
        std::swap(_info, h_table._info);
        std::swap(_size, h_table._size);
        std::swap(_equal, h_table._equal);
        std::swap(_hash, h_table._hash);
    }

    /// @brief Get iterator to the begin of the container
    ///
    /// @return Iterator
    [[nodiscard]] constexpr auto begin() noexcept -> iterator {
        return iterator{ _buckets.begin(), _buckets.end(), _info.begin() };
    }

    /// @brief Get iterator to the end of the container
    ///
    /// @return Iterator
    [[nodiscard]] constexpr auto end() noexcept -> iterator {
        return iterator{ _buckets.end(), _buckets.end(), _info.end() };
    }

    /// @brief Get iterator to the begin of the container
    ///
    /// @return Iterator
    [[nodiscard]] constexpr auto begin() const noexcept -> const_iterator {
        return const_iterator{ _buckets.begin(), _buckets.end(), _info.begin() };
    }

    /// @brief Get iterator to the end of the container
    ///
    /// @return Iterator
    [[nodiscard]] constexpr auto end() const noexcept -> const_iterator {
        return const_iterator{ _buckets.end(), _buckets.end(), _info.end() };
    }

    /// @brief Get iterator to the begin of the container
    ///
    /// @return Iterator
    [[nodiscard]] constexpr auto cbegin() const noexcept -> iterator {
        return begin();
    }

    /// @brief Get iterator to the end of the container
    ///
    /// @return Iterator
    [[nodiscard]] constexpr auto cend() const noexcept -> iterator {
        return end();
    }

    /// @brief Return size of container
    ///
    /// @return size_type Number of elements in the container
    [[nodiscard]] constexpr auto size() const noexcept -> size_type {
        return _size;
    }

    /// @brief Check if container is empty
    ///
    /// @return true If it is empty
    /// @return false If it is not empty
    [[nodiscard]] constexpr auto empty() const noexcept -> bool {
        return size() == 0;
    }

private:
    template<typename C>
    void erase_impl(C&& key) {
        size_type bucket_size = _buckets.size();

        size_type hash = _hash(key);
        size_type psl = 0;
        size_type index = mod_2n(hash, bucket_size);

        value_type* ptr{};
        auto info = _info.begin();
        while (true) {
            ptr = _buckets.begin() + index;
            info = _info.begin() + index;

            if (!info->occupied || psl > info->psl) {
                return;
            }

            if (info->hash != hash || !_equal(get_key(*ptr), key)) {
                index = mod_2n(index + 1, bucket_size);
                psl++;
                continue;
            }

            break;
        }

        value_type temp = std::move(*ptr);
        _size--;

        while (true) {
            info->occupied = false;

            index = mod_2n(index + 1, bucket_size);
            auto nptr = _buckets.begin() + index;
            auto n_info = _info.begin() + index;

            if (!n_info->occupied || n_info->psl == 0) {
                break;
            }

            n_info->psl--;
            *ptr = std::move(*nptr);
            ptr = nptr;
            *info = std::move(*n_info);
            info = n_info;
        }

        const size_type threshold = approx_40_percent(bucket_size);
        if (_size > default_bucket_count && _size < threshold) {
            const auto new_size = bucket_size >> size_type{ 1 };
            reserve(new_size);
        }
    }

    template<typename... Args>
    constexpr auto emplace_impl(Args&&... args) -> std::pair<iterator, bool> {
        size_type buckets_size = _buckets.size();
        const size_type threshold = approx_85_percent(buckets_size);
        if (_size > threshold) {
            const auto new_size = buckets_size << size_type{ 1 };
            reserve(new_size);
        }

        return _emplace_impl(std::forward<Args>(args)...);
    }

    template<typename... Args>
    constexpr auto emplace_or_assign_impl(Args&&... args) -> std::pair<iterator, bool> {
        size_type buckets_size = _buckets.size();
        const size_type threshold = approx_85_percent(buckets_size);
        if (_size > threshold) {
            const auto new_size = buckets_size << size_type{ 1 };
            reserve(new_size);
        }
        return _emplace_or_assign_impl(std::forward<Args>(args)...);
    }

    template<typename... Args>
    constexpr auto _emplace_or_assign_impl(Args&&... args) -> std::pair<iterator, bool> {
        size_type buckets_size = _buckets.size();
        value_type temp(std::forward<Args>(args)...);
        size_type hash = _hash(get_key(temp));
        size_type psl = 0;
        bucket_info n_info = { true, hash, psl };
        size_type index = mod_2n(hash, buckets_size);

        value_type* ret{ nullptr };
        info_iterator ret_info{};

        while (true) {
            value_type* ptr = _buckets.begin() + index;
            auto info = _info.begin() + index;
            if (info->occupied) {
                if (info->hash == hash && _equal(get_key(*ptr), get_key(temp))) {
                    get_value(*ptr) = std::move(get_value(temp));
                    return std::make_pair(create_iterator(ptr, info), false);
                }

                if (n_info.psl > info->psl) {
                    std::swap(temp, *ptr);
                    std::swap(n_info, *info);
                    if (!ret) {
                        ret = ptr;
                        ret_info = info;
                    }
                }
                n_info.psl++;
                index = mod_2n(index + 1, buckets_size);
                continue;
            }
            allocator_traits::construct(_buckets.allocator(), ptr, std::move(temp));
            *info = n_info;
            if (!ret) {
                ret = ptr;
                ret_info = info;
            }
            _size++;
            return std::make_pair(create_iterator(ret, ret_info), true);
        }
    }

    template<typename... Args>
    constexpr auto _emplace_impl(Args&&... args) -> decltype(auto) {
        size_type buckets_size = _buckets.size();
        value_type temp(std::forward<Args>(args)...);
        size_type hash = _hash(get_key(temp));
        size_type psl = 0;
        bucket_info n_info = { true, hash, psl };
        size_type index = mod_2n(hash, buckets_size);

        value_type* ret{ nullptr };
        info_iterator ret_info{};

        while (true) {
            value_type* ptr = _buckets.begin() + index;
            auto info = _info.begin() + index;
            if (info->occupied) {
                if (info->hash == hash && _equal(get_key(*ptr), get_key(temp))) {
                    return std::make_pair(create_iterator(ptr, info), false);
                }

                if (n_info.psl > info->psl) {
                    std::swap(temp, *ptr);
                    std::swap(n_info, *info);
                    if (!ret) {
                        ret = ptr;
                        ret_info = info;
                    }
                }
                n_info.psl++;
                index = mod_2n(index + 1, buckets_size);
                continue;
            }
            allocator_traits::construct(_buckets.allocator(), ptr, std::move(temp));
            *info = n_info;
            if (!ret) {
                ret = ptr;
                ret_info = info;
            }
            _size++;
            return std::make_pair(create_iterator(ret, ret_info), true);
        }
    }

    static constexpr auto find_impl(auto& self, const key_type& key) -> decltype(auto) {
        size_type buckets_size = self._buckets.size();
        if (!buckets_size) {
            return self.end();
        }
        size_type hash = self._hash(key);
        size_type index = mod_2n(hash, buckets_size);

        size_type probes = 0;
        while (true) {
            auto ptr = self._buckets.begin() + index;
            auto info = self._info.begin() + index;
            if (info->occupied && info->hash == hash && self._equal(get_key(*ptr), key)) {
                return self.create_iterator(ptr, info);
            }
            if (!info->occupied || probes > info->psl) {
                return self.end();
            }
            probes++;
            index = mod_2n(index + 1, buckets_size);
        }
    }

    constexpr auto create_iterator(value_type* ptr, info_iterator info) noexcept -> iterator {
        return iterator{ ptr, _buckets.end(), info };
    }

    constexpr auto create_iterator(const value_type* ptr, info_const_iterator info) const noexcept -> const_iterator {
        return const_iterator{ ptr, _buckets.end(), info };
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

    size_t _size{};
    buckets _buckets{ allocator_type(), default_bucket_count };
    info_storage _info{ default_bucket_count };
    key_equal _equal{};
    hasher _hash{};
};

} // namespace co_ecs::detail