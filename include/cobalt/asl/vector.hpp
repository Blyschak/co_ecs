#pragma once

#include <cobalt/asl/type_traits.hpp>

#include <cstring>
#include <limits>
#include <memory>

namespace cobalt::asl {

/// @brief Default vector growing policy
///
/// @tparam T Value type
/// @tparam grow_factor Grow factor
template<typename T, std::size_t grow_factor = 2>
struct default_grow_policy {
    static constexpr std::size_t initial_size_bytes = 64;
    static constexpr std::size_t initial_size =
        std::max(static_cast<size_t>(initial_size_bytes) / sizeof(T), std::size_t(1));

    constexpr static std::size_t next_capacity(std::size_t cap) {
        return std::max(cap * grow_factor, initial_size);
    }
};

/// @brief Base vector struct implementing memory management
///
/// @tparam T Value type
/// @tparam A Allocator type
template<typename T, typename A = std::allocator<T>>
struct vector_base {
    A a{};
    T* b{};
    T* e{};
    T* z{};

    /// @brief Default constructor
    constexpr vector_base() noexcept(noexcept(A())) = default;

    /// @brief Deleted copy constructor
    ///
    /// @param rhs Another vector
    constexpr vector_base(const vector_base& rhs) = delete;

    /// @brief Deleted move constructor
    ///
    /// @param rhs Another vector
    constexpr vector_base(vector_base&& rhs) = delete;

    /// @brief Deleted copy assignment operator
    ///
    /// @param rhs Another vector
    constexpr vector_base& operator=(const vector_base& rhs) = delete;

    /// @brief Deleted copy assignment operator
    ///
    /// @param rhs Another vector
    constexpr vector_base& operator=(vector_base&& rhs) = delete;

    /// @brief Construct vector_base with an allocator
    ///
    /// @param alloc Allocator object
    constexpr explicit vector_base(const A& alloc) noexcept : a(alloc){};

    /// @brief Construct vector_base with an allocator and allocate size items
    ///
    /// @param alloc Allocator object
    /// @param size Size of a vector
    constexpr vector_base(const A& alloc, typename A::size_type size) :
        a(alloc), b(a.allocate(size)), e(b + size), z(e) {
    }

    /// @brief Construct vector_base with an allocator and allocate cap items
    ///
    /// @param alloc Allocator object
    /// @param size Capacity of a vector
    /// @param size Size of a vector
    constexpr vector_base(const A& alloc, typename A::size_type cap, typename A::size_type size) :
        a(alloc), b(a.allocate(cap)), e(b + size), z(b + cap) {
    }

    /// @brief Destructor deallocates memory
    constexpr ~vector_base() {
        if (b) {
            a.deallocate(b, e - b);
        }
    }

    /// @brief Swap with another rhs vector_base
    ///
    /// @param rhs vector
    void swap(vector_base<T, A>& rhs) {
        std::swap(a, rhs.a);
        std::swap(b, rhs.b);
        std::swap(e, rhs.e);
        std::swap(z, rhs.z);
    }
};

/// @brief Vector class
///
/// @tparam T Value type
/// @tparam A Allocator type
/// @tparam grow_policy Grow policy
template<typename T, typename A = std::allocator<T>, typename grow_policy = default_grow_policy<T>>
class vector : private vector_base<T, A> {
public:
    using value_type = T;
    using allocator_type = A;
    using reference = value_type&;
    using const_reference = const value_type&;
    using iterator = T*;
    using const_iterator = const T*;
    using size_type = std::size_t;
    using difference_type = std::make_signed_t<size_type>;
    using pointer = typename std::allocator_traits<A>::pointer;
    using const_pointer = typename std::allocator_traits<A>::const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    using vector_base<T, A>::a;
    using vector_base<T, A>::b;
    using vector_base<T, A>::e;
    using vector_base<T, A>::z;
    using vector_base<T, A>::swap;

    /// @brief Default constructor
    constexpr vector() noexcept(noexcept(A())) = default;

    /// @brief Construct vector from an allocator object
    ///
    /// @param alloc Allocator object
    constexpr explicit vector(const A& alloc) noexcept : vector_base<T, A>(alloc) {
    }

    /// @brief Construct vector with count copies of value
    ///
    /// @param count Number of elements
    /// @param value Value to initialize vector with
    /// @param alloc Allocator object
    constexpr vector(size_type count, const T& value, const A& alloc = A()) : vector_base<T, A>(alloc, count) {
        std::uninitialized_fill(b, e, value);
    }

    /// @brief Construct vector with count default constructed values
    ///
    /// @param count Number of elements
    /// @param alloc Allocator object
    constexpr explicit vector(size_type count, const A& alloc = A()) : vector_base<T, A>(alloc, count) {
    }

    /// @brief Constructs the container with the contents of the range
    /// first..last
    ///
    /// @param first First iterator
    /// @param last Last iterator
    /// @param alloc Allocator object
    template<std::input_iterator input_it>
    constexpr vector(input_it first, input_it last, const A& alloc = A()) :
        vector_base<T, A>(alloc, std::distance(first, last)) {
        std::uninitialized_copy(first, last, b);
    }

    /// @brief Copy constructor
    ///
    /// @param rhs Another vector
    constexpr vector(const vector& rhs) : vector_base<T, A>(rhs.a, rhs.size()) {
        std::uninitialized_copy(rhs.begin(), rhs.end(), b);
    }

    /// @brief Copy constructor with a different allocator
    ///
    /// @param rhs Another vector
    /// @param alloc Allocator object
    constexpr vector(const vector& rhs, const A& alloc) : vector_base<T, A>(alloc, rhs.size()) {
        std::uninitialized_copy(rhs.begin(), rhs.end(), b);
    }

    /// @brief Move constructor
    ///
    /// @param rhs Another vector
    constexpr vector(vector&& rhs) noexcept {
        swap(rhs);
    }

    /// @brief Move constructor with a different allocator
    ///
    /// @param rhs Another vector
    /// @param alloc Allocator object
    constexpr vector(vector&& rhs, const A& alloc) : vector_base<T, A>(alloc, rhs.size()) {
        if (rhs.a == a) {
            swap(rhs);
        } else {
            std::uninitialized_move(rhs.begin(), rhs.end(), b);
        }
    }

    /// @brief Construct vector from initializer list
    ///
    /// @param list Initializer list
    /// @param alloc Allocator object
    constexpr vector(std::initializer_list<T> list, const A& alloc = A()) : vector_base<T, A>(alloc, list.size()) {
        std::uninitialized_copy(list.begin(), list.end(), b);
    }

    /// @brief Destructor
    constexpr ~vector() {
        free();
    }

    /// @brief Copy assignment operator
    ///
    /// @param rhs Another vector
    constexpr vector& operator=(const vector& rhs) {
        // protect against self-assignment
        if (this == &rhs) {
            return *this;
        }

        assign_impl(
            rhs.size(),
            [&]() { return vector(rhs); },
            [&]() {
                std::copy(rhs.begin(), rhs.end(), begin());
                std::destroy_n(begin() + rhs.size(), capacity() - size());
            },
            [&]() {
                std::copy(rhs.begin(), rhs.begin() + size(), begin());
                std::uninitialized_copy(rhs.begin() + size(), rhs.end(), end());
            });
        return *this;
    }

    /// @brief Move assignment operator
    ///
    /// @param rhs Another vector
    constexpr vector& operator=(vector&& rhs) noexcept {
        swap(rhs);
        return *this;
    }

    /// @brief Assign from an initializer list
    ///
    /// @param list Initializer list
    constexpr vector& operator=(std::initializer_list<T> list) {
        assign(list);
        return *this;
    }

    /// @brief Assign count copies of value
    ///
    /// @param count Number of elements
    /// @param value Value
    constexpr void assign(size_type count, const T& value) {
        assign_impl(
            count,
            [&]() { return vector(count, value); },
            [&]() {
                std::fill_n(begin(), count, value);
                std::destroy(begin(), end());
            },
            [&]() {
                std::fill_n(begin(), count, value);
                std::uninitialized_fill_n(end(), count, value);
            });
    }

    /// @brief Assign with the contents of the range first..last
    ///
    /// @param first First iterator
    /// @param last Last iterator
    template<std::input_iterator input_it>
    constexpr void assign(input_it first, input_it last) {
        size_type count = std::distance(first, last);
        assign_impl(
            count,
            [&]() { return vector(first, last); },
            [&]() {
                std::copy(first, last, begin());
                std::destroy_n(begin() + count, capacity() - size());
            },
            [&]() {
                std::copy(first, first + size(), begin());
                std::uninitialized_copy(first + size(), last, end());
            });
    }

    /// @brief Assign with the contents of an initializer list
    ///
    /// @param list Initializer list
    constexpr void assign(std::initializer_list<T> list) {
        assign_impl(
            list.size(),
            [&]() { return vector(list); },
            [&]() {
                std::copy(list.begin(), list.end(), begin());
                std::destroy_n(begin() + list.size(), capacity() - list.size());
            },
            [&]() {
                std::copy(list.begin(), list.begin() + size(), begin());
                std::uninitialized_copy(list.begin() + size(), list.end(), end());
            });
    }

    /// @brief Get the allocator object
    ///
    /// @return Allocator object
    constexpr allocator_type get_allocator() const noexcept {
        return a;
    }

    /// @brief Reserve memory to hold n elements
    ///
    /// @param n Number of items to reserve memory for
    constexpr void reserve(size_type n) {
        if (n <= capacity()) {
            return;
        }

        vector_base<T, A> temp(a, n, 0);
        realloc_no_overlap(begin(), end(), temp.b);
        swap(temp);
    }

    /// @brief Resize vector, default construct additional count elements
    ///
    /// @param count Number of elements
    constexpr void resize(size_type count) {
        if (count < size()) {
            std::destroy_n(begin(), count);
            e = begin() + count;
        } else {
            size_type sz = size();
            vector_base<T, A> temp(a, count);
            realloc_no_overlap(begin(), end(), temp.b);
            swap(temp);
            std::uninitialized_value_construct(begin() + sz, begin() + count);
        }
    }

    /// @brief Resize vector with count copies of value
    ///
    /// @param count Number of elements
    /// @param value Value to insert
    constexpr void resize(size_type count, const value_type& value) {
        if (count < size()) {
            std::destroy_n(begin(), count);
        } else {
            reserve(count);
            for (auto it = begin() + size(); it != begin() + capacity(); it++) {
                std::construct_at(it, value);
            }
        }
        e = begin() + count;
    }

    /// @brief Return maximum size of a vector
    ///
    /// @return Maximum vector size
    [[nodiscard]] constexpr size_type max_size() const noexcept {
        return std::numeric_limits<std::size_t>::max() / sizeof(T);
    }

    /// @brief Clear the vector
    constexpr void clear() noexcept {
        free();
        e = b;
    }

    /// @brief Insert value at position pos
    ///
    /// @param pos Position where to insert
    /// @param value Value to insert
    /// @return Iterator pointing to inserted value
    constexpr iterator insert(const_iterator pos, const T& value) {
        return insert_impl(
            pos,
            1,
            [&value](iterator pos, size_type count [[maybe_unused]]) { std::construct_at(pos, value); },
            [&value](iterator pos, size_type count [[maybe_unused]]) { *pos = value; });
    }

    /// @brief Insert value at position pos
    ///
    /// @param pos Position where to insert
    /// @param value Value to insert
    /// @return Iterator pointing to inserted value
    constexpr iterator insert(const_iterator pos, T&& value) {
        return insert_impl(
            pos,
            1,
            [&value](iterator pos, size_type count [[maybe_unused]]) { std::construct_at(pos, std::move(value)); },
            [&value](iterator pos, size_type count [[maybe_unused]]) { *pos = std::move(value); });
    }

    /// @brief Insert count values at position pos
    ///
    /// @param pos Position where to insert
    /// @param count Number of elements to insert
    /// @param value Value to insert
    /// @return Iterator pointing to first inserted value
    constexpr iterator insert(const_iterator pos, size_type count, const T& value) {
        return insert_impl(
            pos,
            count,
            [&value](iterator pos, size_type count) {
                for (iterator it = pos; it != pos + count; it++) {
                    std::construct_at(it, value);
                }
            },
            [&value](iterator pos, size_type count) {
                for (iterator it = pos; it != pos + count; it++) {
                    *it = value;
                }
            });
    }

    /// @brief Insert copied values from first to last at position pos
    ///
    /// @param pos Position where to insert
    /// @param first First iterator
    /// @param last Last iterator
    /// @return Iterator pointing to first inserted value
    template<std::input_iterator input_it>
    constexpr iterator insert(const_iterator pos, input_it first, input_it last) {
        return insert_impl(
            pos,
            std::distance(first, last),
            [first](iterator pos, size_type count) {
                for (iterator it = pos; it != pos + count; first++, it++) {
                    std::construct_at(it, *first);
                }
            },
            [first](iterator pos, size_type count) {
                for (iterator it = pos; it != pos + count; it++) {
                    *it = first;
                }
            });
    }

    /// @brief Insert copied values from initializer list at position pos
    ///
    /// @param pos Position where to insert
    /// @param list Initializer list
    /// @return Iterator pointing to first inserted value
    constexpr iterator insert(const_iterator pos, std::initializer_list<T> list) {
        return insert_impl(
            pos,
            list.size(),
            [&list](iterator pos, size_type count) {
                auto first = list.begin();
                for (iterator it = pos; it != pos + count; first++, it++) {
                    std::construct_at(it, *first);
                }
            },
            [&list](iterator pos, size_type count) {
                auto first = list.begin();
                for (iterator it = pos; it != pos + count; first++, it++) {
                    *it = *first;
                }
            });
    }

    /// @brief Emplace value at position pos
    ///
    /// @tparam Args Parameter pack
    /// @param pos Position where to insert
    /// @param args Arguments
    /// @return Iterator pointing to first inserted value
    template<typename... Args>
    constexpr iterator emplace(const_iterator pos, Args&&... args) {
        return insert_impl(
            pos,
            1,
            [... args = std::forward<decltype(args)>(args)](iterator pos, size_type count [[maybe_unused]]) mutable {
                std::construct_at(pos, std::forward<decltype(args)>(args)...);
            },
            [... args = std::forward<decltype(args)>(args)](iterator pos, size_type count [[maybe_unused]]) mutable {
                *pos = T{ std::forward<decltype(args)>(args)... };
            });
    }

    /// @brief Erase element at position pos
    ///
    /// @param pos Position where to erase
    /// @return Position next to inserted element
    constexpr iterator erase(const_iterator pos) {
        return erase(pos, pos + 1);
    }

    /// @brief Erase elements from first to last
    ///
    /// @param first First iterator
    /// @param last Last iterator
    /// @return Position next to inserted element
    constexpr iterator erase(const_iterator first, const_iterator last) {
        std::move(begin() + (last - begin()), end(), begin() + (first - begin()));
        std::destroy_n(begin() + (last - begin()) + 1, std::distance(first, last));
        e -= std::distance(first, last);
        return begin() + (last - begin());
    }

    /// @brief Push back a copy of value
    ///
    /// @param value Value to push
    constexpr void push_back(const T& value) {
        insert_end_impl(1, [&value](iterator pos, size_type count [[maybe_unused]]) { std::construct_at(pos, value); });
    }

    /// @brief Push back an r-value of value
    ///
    /// @param value Value to push
    constexpr void push_back(T&& value) {
        insert_end_impl(1, [value = std::forward<T>(value)](iterator pos, size_type count [[maybe_unused]]) mutable {
            std::construct_at(pos, std::move(value));
        });
    }

    /// @brief Emplace back an element constructed from args
    ///
    /// @tparam Args Parameter pack
    /// @param args Arguments for construction
    /// @return Reference to emplaced element
    template<typename... Args>
    constexpr reference emplace_back(Args&&... args) {
        insert_end_impl(
            1, [... args = std::forward<decltype(args)>(args)](iterator pos, size_type count [[maybe_unused]]) mutable {
                std::construct_at(pos, std::forward<decltype(args)>(args)...);
            });
        return *(end() - 1);
    }

    /// @brief Remove element from the end of the vector
    constexpr void pop_back() noexcept {
        e--;
        std::destroy_at(end());
    }

    /// @brief Erase an element at pos and swap the last element with erased
    ///
    /// @param pos Position where to erase
    constexpr void swap_erase(const_iterator pos) noexcept(std::is_nothrow_move_constructible_v<T>) {
        auto _pos = begin() + (pos - begin());
        std::swap(*_pos, back());
        std::destroy_at(&back());
        e--;
    }

    /// @brief Swap vector with rhs vector
    ///
    /// @param rhs Another vector
    constexpr void swap(vector& rhs) noexcept {
        vector_base<T, A>::swap(rhs);
    }

    /// @brief Decrease the capacity of the vector to fit exactly all elements
    constexpr void shrink_to_fit() {
        if (size() < capacity()) {
            vector temp(*this);
            swap(temp);
        }
    }

    /// @brief Get a reference to element at position pos. Bounds are checked
    ///
    /// @param pos Position
    /// @return reference to element at position pos
    constexpr reference at(size_type pos) {
        if (!(begin() + pos >= end())) {
            throw std::out_of_range{ "position out of range" };
        }
        return begin()[pos]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

    /// @brief Get a const reference to element at position pos. Bounds are
    /// checked
    ///
    /// @param pos Position
    /// @return Constant reference to element at position pos
    constexpr const_reference at(size_type pos) const {
        if (!(pos < size())) {
            throw std::out_of_range{ "position out of range" };
        }
        return *this[pos]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

    /// @brief Get a reference to element at position pos
    ///
    /// @param pos Position
    /// @return Reference to element at position pos
    constexpr reference operator[](size_type pos) {
        return begin()[pos]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

    /// @brief Get a const reference to element at position pos
    ///
    /// @param pos Position
    /// @return Constant reference to element at position pos
    constexpr const_reference operator[](size_type pos) const {
        return begin()[pos]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

    /// @brief Get a reference to the first element
    ///
    /// @return Reference to the first element
    constexpr reference front() {
        return *begin();
    }

    /// @brief Get a const reference to the first element
    ///
    /// @return Constant reference to the first element
    constexpr const_reference front() const {
        return *begin();
    }

    /// @brief Get a reference to the last element
    ///
    /// @return Reference to the last element
    constexpr reference back() {
        return begin()[size() - 1]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

    /// @brief Get a const reference to the last element
    ///
    /// @return Constant reference to the last element
    constexpr const_reference back() const {
        return begin()[size() - 1]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

    /// @brief Get a pointer to data buffer
    ///
    /// @return Pointer to data buffer
    constexpr value_type* data() noexcept {
        return begin();
    }

    /// @brief Get a const pointer to data buffer
    ///
    /// @return Constant pointer to data buffer
    constexpr const value_type* data() const noexcept {
        return begin();
    }

    /// @brief Get iterator to the begin of the vector
    ///
    /// @return Iterator
    constexpr iterator begin() noexcept {
        return b;
    }

    /// @brief Get const iterator to the begin of the vector
    ///
    /// @return Iterator
    constexpr const_iterator begin() const noexcept {
        return b;
    }

    /// @brief Get const iterator to the begin of the vector
    ///
    /// @return Iterator
    constexpr const_iterator cbegin() const noexcept {
        return b;
    }

    /// @brief Get iterator to the end of the vector
    ///
    /// @return Iterator
    constexpr iterator end() noexcept {
        return e;
    }

    /// @brief Get const iterator to the end of the vector
    ///
    /// @return Iterator
    constexpr const_iterator end() const noexcept {
        return e;
    }

    /// @brief Get const iterator to the end of the vector
    ///
    /// @return Iterator
    constexpr const_iterator cend() const noexcept {
        return e;
    }

    /// @brief Get reverse iterator from end of the vector
    ///
    /// @return Iterator
    constexpr reverse_iterator rbegin() noexcept {
        return std::make_reverse_iterator(end());
    }

    /// @brief Get const reverse iterator from end of the vector
    ///
    /// @return Iterator
    constexpr const_reverse_iterator rbegin() const noexcept {
        return std::make_reverse_iterator(end());
    }

    /// @brief Get const reverse iterator from end of the vector
    ///
    /// @return Iterator
    constexpr const_reverse_iterator crbegin() const noexcept {
        return std::make_reverse_iterator(end());
    }

    /// @brief Get reverse iterator from begin of the vector
    ///
    /// @return Iterator
    constexpr reverse_iterator rend() noexcept {
        return std::make_reverse_iterator(begin());
    }

    /// @brief Get const reverse iterator from begin of the vector
    ///
    /// @return Iterator
    constexpr const_reverse_iterator rend() const noexcept {
        return std::make_reverse_iterator(begin());
    }

    /// @brief Get const reverse iterator from begin of the vector
    ///
    /// @return Iterator
    constexpr const_reverse_iterator crend() const noexcept {
        return std::make_reverse_iterator(begin());
    }

    /// @brief Check if vector is empty
    ///
    /// @return true When the vector is empty
    /// @return false When the vector is not empty
    [[nodiscard]] constexpr bool empty() const noexcept {
        return b == e;
    }

    /// @brief Returns the capacity of the vector
    ///
    /// @return Capacity of the vector
    [[nodiscard]] constexpr size_type capacity() const noexcept {
        return z - b;
    }

    /// @brief Returns the size of the vector
    ///
    /// @return Size of the vector
    [[nodiscard]] constexpr size_type size() const noexcept {
        return e - b;
    }

private:
    static constexpr bool has_nothrow_move_ctor = std::is_nothrow_move_constructible_v<value_type>;
    static constexpr bool has_move_ctor = std::is_move_constructible_v<value_type>;
    static constexpr bool has_copy_ctor = std::is_copy_constructible_v<value_type>;

    static constexpr bool has_nothrow_move_assign = std::is_nothrow_move_assignable_v<value_type>;
    static constexpr bool has_move_assign = std::is_move_assignable_v<value_type>;
    static constexpr bool has_copy_assign = std::is_copy_assignable_v<value_type>;

    static constexpr bool use_move_ctor = has_nothrow_move_ctor || (has_move_ctor && !has_copy_ctor);
    static constexpr bool use_move_assign = has_nothrow_move_assign || (has_move_assign && !has_copy_assign);

    constexpr void free() noexcept {
        std::destroy(begin(), end());
    }

    constexpr void assign_impl(size_type count, auto create, auto insert, auto additional) {
        // not enough memory to hold elements from the other vector
        if (capacity() < count) {
            auto temp = create();
            swap(temp);
            return;
        }

        if (count <= size()) { // copy over old elements and destroy surplus elements
            insert();
            std::destroy_n(begin() + count, capacity() - count);
        } else { // copy over old elements and construct additional elements:
            additional();
        }

        e = begin() + count;
    }

    constexpr iterator insert_impl(const_iterator pos, size_type count, auto insert, auto assign) {
        if (pos == end()) {
            insert_end_impl(count, insert);
            return end();
        } else {
            return insert_middle_impl(pos, count, assign);
        }
    }

    constexpr void insert_end_impl(size_type count, auto insert) {
        if (e + count > z) {
            vector_base<T, A> temp(a, count == 1 ? next_capacity() : size() + count, size() + count);
            insert(temp.b + size(), count);
            realloc_no_overlap(begin(), end(), temp.b);
            swap(temp);
        } else {
            insert(end(), count);
            e += count;
        }
    }

    constexpr iterator insert_middle_impl(const_iterator pos, size_type count, auto assign) {
        if (size() + count > capacity()) {
            return insert_middle_realloc_impl(pos, count, assign);
        } else {
            return insert_middle_no_realloc_impl(pos, count, assign);
        }
    }

    constexpr iterator insert_middle_realloc_impl(const_iterator pos, size_type count, auto assign) {
        size_type index = pos - begin();
        vector_base<T, A> temp(a, count == 1 ? next_capacity() : size() + count, size() + count);
        assign(temp.b + index, count);
        realloc_no_overlap(begin(), begin() + index, temp.b);
        realloc_no_overlap(begin() + index, end(), temp.b + index + count);
        swap(temp);
        return begin() + index;
    }

    constexpr iterator insert_middle_no_realloc_impl(const_iterator pos, size_type count, auto assign) {
        iterator insert_pos = (pos - begin()) + begin();
        realloc_overlap(insert_pos, end(), insert_pos + count);
        assign(insert_pos, count);
        e += count;
        return insert_pos;
    }

    static constexpr void realloc_overlap(auto first, auto last, auto output) {
        auto count = std::distance(first, last);
        if constexpr (is_relocatable_v<value_type>) {
            std::memmove(output, first, count * sizeof(T));
        } else {
            iterator new_end = last + count - 1;
            for (iterator it = new_end; it != last - 1; it--) {
                iterator moving = it - count;
                if constexpr (use_move_ctor) {
                    std::construct_at(it, std::move(*moving));
                } else {
                    std::construct_at(it, *moving);
                }
                std::destroy_at(moving);
            }
            for (iterator it = last - 1; it != output; it--) {
                iterator moving = it - count;
                if constexpr (use_move_assign) {
                    *it = std::move(*moving);
                } else {
                    *it = *moving;
                }
                std::destroy_at(moving);
            }
        }
    }

    static constexpr void realloc_no_overlap(auto first, auto last, auto output) {
        if constexpr (is_relocatable_v<value_type>) {
            std::memcpy(output, first, std::distance(first, last) * sizeof(T));
        } else {
            if constexpr (use_move_ctor) {
                std::uninitialized_move(first, last, output);
            } else {
                std::uninitialized_copy(first, last, output);
            }
            std::destroy(first, last);
        }
    }

    [[nodiscard]] constexpr size_type next_capacity() const noexcept {
        return grow_policy::next_capacity(capacity());
    }
};


template<typename T, typename alloc_t>
constexpr bool operator==(const vector<T, alloc_t>& lhs, const vector<T, alloc_t>& rhs) {
    return lhs.b == rhs.b;
}

template<typename T, typename alloc_t>
constexpr auto operator<=>(const vector<T, alloc_t>& lhs, const vector<T, alloc_t>& rhs);


template<typename input_it, typename alloc_t = std::allocator<typename std::iterator_traits<input_it>::value_type>>
vector(input_it, input_it, alloc_t = alloc_t()) -> vector<typename std::iterator_traits<input_it>::value_type, alloc_t>;

template<typename T>
struct is_relocatable<vector<T>> : std::true_type {};

} // namespace cobalt::asl
