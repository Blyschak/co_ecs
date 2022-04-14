#pragma once

#include <memory>
#include <vector>

namespace cobalt::asl {

/// @brief Calculate the value % 2^n
///
/// @param value Value
/// @param n Power of 2
/// @return decltype(auto) Result
constexpr decltype(auto) mod(auto value, auto n) noexcept {
    return value & ((1 << n) - 1);
}

/// @brief Approximatelly calculate 85% of passed value. 85% is used as a reserve threshold.
///
/// @param value Value
/// @return decltype(auto) Result
constexpr decltype(auto) approx_85_percent(auto value) noexcept {
    return (value * 870) >> 10;
}

/// @brief Approximatelly calculate 40% of passed value. 40% is used as a reserve threshold.
///
/// @param value Value
/// @return decltype(auto) Result
constexpr decltype(auto) approx_40_percent(auto value) noexcept {
    return (value * 409) >> 10;
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
    using difference_type = std::ptrdiff_t;

    constexpr static size_type default_bucket_count = 16; // the number of buckets the hash table is initialized with

private:
    /// @brief Bucket info
    struct bucket_info {
        bool occupied{ false };
        size_t hash{};
        size_t psl{};
    };

    using info_storage = std::vector<bucket_info>;
    using info_storage_iterator = typename info_storage::iterator;
    using info_storage_const_iterator = typename info_storage::const_iterator;

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
        constexpr buckets(buckets&& rhs) {
            swap(rhs);
        }

        /// @brief Deleted copy assignment operator
        ///
        /// @param rhs Another buckets
        /// @return Buckets
        constexpr buckets& operator=(const buckets& rhs) = delete;

        /// @brief Move assignment operator
        ///
        /// @param rhs Another buckets
        /// @return Buckets
        constexpr buckets& operator=(buckets&& rhs) {
            swap(rhs);
            return *this;
        }

        /// @brief Destructor
        constexpr ~buckets() {
            a.deallocate(b, e - b);
        }

        /// @brief Swap two buckets
        ///
        /// @param rhs
        void swap(buckets& rhs) {
            std::swap(a, rhs.a);
            std::swap(b, rhs.b);
            std::swap(e, rhs.e);
            std::swap(z, rhs.z);
        }

        /// @brief Get the allocator object
        ///
        /// @return Allocator object
        constexpr allocator_type& allocator() noexcept {
            return a;
        }

        /// @brief Get the allocator object
        ///
        /// @return Allocator object
        constexpr const allocator_type& allocator() const noexcept {
            return a;
        }

        /// @brief Get iterator to the begin of the vector
        ///
        /// @return Iterator
        constexpr value_type* begin() noexcept {
            return b;
        }

        /// @brief Get const iterator to the begin of the vector
        ///
        /// @return Iterator
        constexpr const value_type* begin() const noexcept {
            return b;
        }

        /// @brief Get iterator to the end of the vector
        ///
        /// @return Iterator
        constexpr value_type* end() noexcept {
            return e;
        }

        /// @brief Get const iterator to the end of the vector
        ///
        /// @return Iterator
        constexpr const value_type* end() const noexcept {
            return e;
        }

        /// @brief Returns the size of the vector
        ///
        /// @return Size of the vector
        [[nodiscard]] constexpr size_type size() const noexcept {
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
        using difference_type = size_type;
        using value_type = hash_table::value_type;
        using element_type = hash_table::value_type;
        using reference = std::conditional_t<is_const, const value_type&, value_type&>;
        using pointer = std::conditional_t<is_const, const value_type*, value_type*>;
        using info_iterator = std::conditional_t<is_const, info_storage_const_iterator, info_storage_iterator>;

        /// @brief Iterator constructor
        ///
        /// @param ptr Pointer in the bucket
        /// @param end End pointer in the bucket
        /// @param info Info in the bucket
        constexpr explicit iterator_impl(pointer ptr, pointer end, info_iterator info) noexcept :
            _ptr(ptr), _end(end), _info(info) {
            // fast forward to the next occupied entry in the buckets array
            // if not occupied already or it is not the end.
            if (_ptr != _end && !_info->occupied) {
                fast_forward();
            }
        }

        /// @brief Pre-increment iterator
        ///
        /// @return iterator_impl& Incremented iterator
        constexpr iterator_impl& operator++() noexcept {
            fast_forward();
            return *this;
        }

        /// @brief Post-increment iterator
        ///
        /// @return iterator_impl& Iterator
        constexpr iterator_impl operator++(int) noexcept {
            iterator_impl tmp(*this);
            _ptr++;
            return tmp;
        }

        /// @brief Dereference iterator
        ///
        /// @return reference Reference to value
        constexpr reference operator*() const noexcept {
            return *_ptr;
        }

        /// @brief Dereference iterator
        ///
        /// @return reference Pointer to value
        constexpr pointer operator->() const noexcept {
            return _ptr;
        }

        /// @brief Spaceship operator
        ///
        /// @param rhs Right hand side
        /// @return auto Result of comparison
        constexpr auto operator<=>(const iterator_impl& rhs) const noexcept = default;

    private:
        constexpr void fast_forward() noexcept {
            while (_ptr != _end) {
                _info++;
                _ptr++;
                if (_info->occupied) {
                    break;
                }
            }
        }

        pointer _ptr;
        pointer _end;
        info_iterator _info;
    };

public:
    using iterator = iterator_impl<false>;
    using const_iterator = iterator_impl<true>;

    /// @brief Default construct hash table
    constexpr hash_table() : _size(0), _buckets(allocator_type(), default_bucket_count), _info(default_bucket_count) {
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
        _size(0),
        _buckets(allocator_type(), bucket_count), _info(bucket_count), _hash(hash), _equal(equal) {
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
        _size(0),
        _buckets(alloc, bucket_count), _info(default_bucket_count), _equal(equal), _hash(hash) {
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
    constexpr hash_table(const hash_table& rhs) {
        size_type idx{};
        _buckets = buckets(_buckets.allocator(), _buckets.size());

        for (const auto& info : rhs._info) {
            if (info.occupied) {
                std::uninitialized_copy_n(rhs._buckets.begin() + idx, 1, _buckets.begin() + idx);
            }
            idx++;
        }

        _info = rhs._info;
        _size = rhs._size;
        _hash = rhs._hash;
        _equal = rhs._equal;
    }

    /// @brief Copy assignment operator
    ///
    /// @param rhs Right hand side
    constexpr hash_table& operator=(const hash_table& rhs) {
        hash_table copy(rhs);
        swap(copy);
        return *this;
    }

    /// @brief Move copy constructor
    ///
    /// @param rhs Right hand side
    constexpr hash_table(hash_table&& rhs) = default;

    /// @brief Move assignment operator
    ///
    /// @param rhs Right hand side
    constexpr hash_table& operator=(hash_table&& rhs) = default;

    /// @brief Get the allocator object
    ///
    /// @return allocator_type Allocator object
    constexpr allocator_type get_allocator() const noexcept {
        return _buckets.allocator();
    }

    /// @brief Get or default construct mapped type value associated with key
    ///
    /// @param key Key to insert
    /// @return mapped_type& Reference to the mapped type
    mapped_type& operator[](const key_type& key) requires(is_map) {
        auto [iter, _] = emplace_impl(std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple());
        return iter->second;
    }

    /// @brief Get mapped type value associated with key
    ///
    /// @throws std::out_of_range Out of range error
    ///
    /// @param key Key to insert
    /// @return mapped_type& Reference to the mapped type
    mapped_type& at(const key_type& key) requires(is_map) {
        if (auto iter = find_impl(*this, key); iter != end()) {
            return *iter;
        }
        throw std::out_of_range{ "key not found in hash_table" };
    }

    /// @brief Get mapped type value associated with key
    ///
    /// @throws std::out_of_range Out of range error
    ///
    /// @param key Key to insert
    /// @return const mapped_type& Reference to the mapped type
    const mapped_type& at(const key_type& key) const requires(is_map) {
        if (auto iter = find_impl(*this, key); iter != end()) {
            return *iter;
        }
        throw std::out_of_range{ "key not found in hash_table" };
    }

    /// @brief Clear all values in the table
    void clear() {
        for (auto& value : *this) {
            allocator_traits::destroy(_buckets.allocator(), std::addressof(value));
        }
        _info.clear();
        _size = 0;
    }

    /// @brief Insert value into the hash table if it does not exist yet
    ///
    /// @param value Value to insert
    /// @return std::pair<iterator, bool> Pair of iterator to the inserted/found value and a boolean indicating whether
    /// insertion happened
    std::pair<iterator, bool> insert(const value_type& value) {
        return emplace_impl(std::move(value));
    }

    /// @brief Insert value into the hash table or assign to existing one
    ///
    /// @param value Value to insert
    /// @return std::pair<iterator, bool> Pair of iterator to the inserted/found value and a boolean indicating whether
    /// insertion happened
    std::pair<iterator, bool> insert_or_assign(const value_type& value) {
        return emplace_or_assign_impl(std::move(value));
    }

    /// @brief Inserts a new element into the container constructed in-place with the given args if there is no element
    /// with the key in the container.
    ///
    /// @tparam Args Parameter pack
    /// @param args args to construct value from
    /// @return std::pair<iterator, bool> Returns a pair consisting of an iterator to the inserted element, or the
    /// already-existing element if no insertion happened, and a bool denoting whether the insertion took place
    template<typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
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
    /// @param c Value key compares to
    template<typename C>
    void erase(C&& c) {
        return erase_impl(std::forward<C>(c));
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
    [[nodiscard]] size_type count(const key_type& key) const noexcept {
        return find_impl(*this, key) != end() ? 1 : 0;
    }

    /// @brief Check if key exists in the container
    ///
    /// @param key Key to check
    /// @return true If key exists in the container
    /// @return false If key does not exist in the container
    [[nodiscard]] bool contains(const key_type& key) const noexcept {
        return count(key) > 0;
    }

    /// @brief Find key in the container and return an iterator pointing to it
    ///
    /// @param key Key to find
    /// @return iterator Iterator result
    [[nodiscard]] iterator find(const key_type& key) noexcept {
        return find_impl(*this, key);
    }

    /// @brief Find key in the container and return an iterator pointing to it
    ///
    /// @param key Key to find
    /// @return iterator Iterator result
    [[nodiscard]] const_iterator find(const key_type& key) const noexcept {
        return find_impl(*this, key);
    }

    /// @brief Reserve space in the bucket array
    ///
    /// @param new_size New size
    void reserve(size_type new_size) {
        if (new_size < _size) {
            return;
        }

        hash_table ht(new_size, _hash, _equal, _buckets.allocator());

        for (auto& value : *this) {
            ht._emplace_impl(std::move(value));
        }

        std::swap(_buckets, ht._buckets);
        std::swap(_info, ht._info);
        std::swap(_size, ht._size);
        std::swap(_equal, ht._equal);
        std::swap(_hash, ht._hash);
    }

    /// @brief Get iterator to the begin of the container
    ///
    /// @return Iterator
    [[nodiscard]] constexpr iterator begin() noexcept {
        return iterator{ _buckets.begin(), _buckets.end(), _info.begin() };
    }

    /// @brief Get iterator to the end of the container
    ///
    /// @return Iterator
    [[nodiscard]] constexpr iterator end() noexcept {
        return iterator{ _buckets.end(), _buckets.end(), _info.end() };
    }

    /// @brief Get iterator to the begin of the container
    ///
    /// @return Iterator
    [[nodiscard]] constexpr const_iterator begin() const noexcept {
        return const_iterator{ _buckets.begin(), _buckets.end(), _info.begin() };
    }

    /// @brief Get iterator to the end of the container
    ///
    /// @return Iterator
    [[nodiscard]] constexpr const_iterator end() const noexcept {
        return const_iterator{ _buckets.end(), _buckets.end(), _info.end() };
    }

    /// @brief Get iterator to the begin of the container
    ///
    /// @return Iterator
    [[nodiscard]] constexpr iterator cbegin() const noexcept {
        return begin();
    }

    /// @brief Get iterator to the end of the container
    ///
    /// @return Iterator
    [[nodiscard]] constexpr iterator cend() const noexcept {
        return end();
    }

    /// @brief Return size of container
    ///
    /// @return size_type Number of elements in the container
    [[nodiscard]] constexpr size_type size() const noexcept {
        return _size;
    }

    /// @brief Check if container is empty
    ///
    /// @return true If it is empty
    /// @return false If it is not empty
    [[nodiscard]] constexpr bool empty() const noexcept {
        return size() == 0;
    }

private:
    template<typename C>
    void erase_impl(C&& key) {
        size_type bs = _buckets.size();

        size_type hash = _hash(key);
        size_type psl = 0;
        size_type i = hash % bs;

        value_type* ptr;
        auto info = _info.begin();
        while (true) {
            ptr = _buckets.begin() + i;
            info = _info.begin() + i;

            if (!info->occupied || psl > info->psl) {
                return;
            }

            if (info->hash != hash || !_equal(get_key(*ptr), key)) {
                i = (i + 1) % bs;
                psl++;
                continue;
            }

            break;
        }

        value_type temp = std::move(*ptr);
        _size--;

        while (true) {
            info->occupied = false;

            i = (i + 1) % bs;
            auto nptr = _buckets.begin() + i;
            auto ninfo = _info.begin() + i;

            if (!ninfo->occupied || ninfo->psl == 0) {
                break;
            }

            ninfo->psl--;
            *ptr = std::move(*nptr);
            ptr = nptr;
            *info = std::move(*ninfo);
            info = ninfo;
        }

        const size_type threshold = approx_40_percent(bs);
        if (_size > default_bucket_count && _size < threshold) {
            const auto new_size = bs >> 1;
            reserve(new_size);
        }
    }

    template<typename... Args>
    constexpr std::pair<iterator, bool> emplace_impl(Args&&... args) {
        size_type buckets_size = _buckets.size();
        const size_type threshold = approx_85_percent(buckets_size);
        if (_size > threshold) {
            const auto new_size = buckets_size << 1;
            reserve(new_size);
        }

        return _emplace_impl(std::forward<Args>(args)...);
    }

    template<typename... Args>
    constexpr std::pair<iterator, bool> emplace_or_assign_impl(Args&&... args) {
        size_type buckets_size = _buckets.size();
        const size_type threshold = approx_85_percent(buckets_size);
        if (_size > threshold) {
            const auto new_size = buckets_size * 2;
            reserve(new_size);
        }
        return _emplace_or_assign_impl(std::forward<Args>(args)...);
    }

    template<typename... Args>
    constexpr std::pair<iterator, bool> _emplace_or_assign_impl(Args&&... args) {
        size_type buckets_size = _buckets.size();
        value_type n(std::forward<Args>(args)...);
        size_type hash = _hash(get_key(n));
        size_type psl = 0;
        bucket_info ninfo = { true, hash, psl };
        size_type i = hash % buckets_size;

        while (true) {
            value_type* ptr = _buckets.begin() + i;
            auto info = _info.begin() + i;
            if (info->occupied) {
                if (info->hash == hash && _equal(get_key(*ptr), get_key(n))) {
                    get_value(*ptr) = std::move(get_value(n));
                    return std::make_pair(iterator(ptr, _buckets.end(), info), false);
                }

                if (ninfo.psl > info->psl) {
                    std::swap(n, *ptr);
                    std::swap(ninfo, *info);
                }
                ninfo.psl++;
                i = (i + 1) % buckets_size;
                continue;
            }
            allocator_traits::construct(_buckets.allocator(), ptr, std::move(n));
            *info = ninfo;
            _size++;
            return std::make_pair(iterator(ptr, _buckets.end(), info), true);
        }
    }

    template<typename... Args>
    constexpr decltype(auto) _emplace_impl(Args&&... args) {
        size_type buckets_size = _buckets.size();
        value_type n(std::forward<Args>(args)...);
        size_type hash = _hash(get_key(n));
        size_type psl = 0;
        bucket_info ninfo = { true, hash, psl };
        size_type i = hash % buckets_size;

        while (true) {
            value_type* ptr = _buckets.begin() + i;
            auto info = _info.begin() + i;
            if (info->occupied) {
                if (info->hash == hash && _equal(get_key(*ptr), get_key(n))) {
                    return std::make_pair(iterator(ptr, _buckets.end(), info), false);
                }

                if (ninfo.psl > info->psl) {
                    std::swap(n, *ptr);
                    std::swap(ninfo, *info);
                }
                ninfo.psl++;
                i = (i + 1) % buckets_size;
                continue;
            }
            allocator_traits::construct(_buckets.allocator(), ptr, std::move(n));
            *info = ninfo;
            _size++;
            return std::make_pair(iterator(ptr, _buckets.end(), info), true);
        }
    }

    static constexpr decltype(auto) find_impl(auto& self, const key_type& key) {
        size_type buckets_size = self._buckets.size();
        if (!buckets_size) {
            return self.end();
        }
        size_type hash = self._hash(key);
        size_type i = hash % buckets_size;

        size_type probes = 0;
        while (true) {
            auto ptr = self._buckets.begin() + i;
            auto info = self._info.begin() + i;
            if (info->occupied && info->hash == hash && self._equal(get_key(*ptr), key)) {
                return self.get_iterator_to(ptr, info);
            }
            if (!info->occupied || probes > info->psl) {
                return self.end();
            }
            probes++;
            i = (i + 1) % buckets_size;
        }
    }

    constexpr iterator get_iterator_to(value_type* ptr, std::vector<bucket_info>::iterator info) noexcept {
        return iterator{ ptr, _buckets.end(), info };
    }

    constexpr const_iterator get_iterator_to(const value_type* ptr,
        std::vector<bucket_info>::const_iterator info) const noexcept {
        return const_iterator{ ptr, _buckets.end(), info };
    }

    constexpr static auto& get_key(auto& v) {
        if constexpr (is_map) {
            return v.first;
        } else {
            return v;
        }
    }

    constexpr static auto& get_value(auto& v) {
        if constexpr (is_map) {
            return v.second;
        } else {
            return v;
        }
    }

    size_t _size{};
    buckets _buckets{ allocator_type(), default_bucket_count };
    std::vector<bucket_info> _info{ default_bucket_count };
    key_equal _equal{};
    hasher _hash{};
};

} // namespace cobalt::asl