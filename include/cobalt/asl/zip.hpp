#pragma once

#include <memory>
#include <ranges>
#include <tuple>

namespace cobalt::asl {

/// @brief Simple zip_view until C++ 23
/// @tparam ...Ranges Range types
template<typename... Ranges>
requires(std::ranges::view<Ranges>&&...) class zip_view : public std::ranges::view_interface<zip_view<Ranges...>> {
public:
    static_assert(sizeof...(Ranges) > 1);

    /// @brief Simple iterator over a zip_view
    /// @tparam ...Iters Iterator types
    template<typename... Iters>
    class iterator {
    public:
        using iterator_category = std::common_type_t<typename std::iterator_traits<Iters>::iterator_category...>;
        using difference_type = std::common_type_t<typename std::iterator_traits<Iters>::difference_type...>;
        using value_type = std::tuple<typename std::iterator_traits<Iters>::value_type...>;
        using pointer = std::tuple<typename std::iterator_traits<Iters>::pointer...>;
        using reference = std::tuple<typename std::iterator_traits<Iters>::reference...>;

        /// @brief Default constructor
        constexpr iterator() = default;

        /// @brief Constructs zip_view iterator from a pack of iterators
        /// @param ...iters Iterators to zip
        constexpr explicit iterator(const Iters&... iters) : _iters(iters...) {
        }

        /// @brief Advance iterators
        /// @param rhs By how much advance iterators
        /// @return Reference to this zip_view iterator
        iterator& operator+=(const difference_type rhs) {
            std::apply([&rhs](auto&&... iters) { ((std::advance(iters, rhs)), ...); }, _iters);
            return *this;
        }

        /// @brief Retreat iterators
        /// @param rhs By how much retreat iterators
        /// @return Reference to this zip_view iterator
        iterator& operator-=(const difference_type rhs) {
            return operator+=(-rhs);
        }

        /// @brief Dereference zip_view iterator
        /// @return Tuple of references returned by dereferencing zipped
        /// iterators
        reference operator*() const {
            return std::apply([](auto&&... args) { return std::make_tuple(std::ref(args.operator*())...); }, _iters);
        }

        /// @brief Dereference zip_view iterator
        /// @return Tuple of pointers returned by dereferencing zipped iterators
        pointer operator->() const {
            return std::apply([](auto&&... args) { return std::make_tuple(std::ref(args.operator->())...); }, _iters);
        }

        /// @brief Index zip_view iterator
        /// @return Tuple of references returned by indexing zipped iterators
        reference operator[](difference_type rhs) const {
            return std::apply(
                [&rhs](auto&&... args) { return std::make_tuple(std::ref(args.operator[](rhs))...); }, _iters);
        }

        /// @brief Pre-increment zip_view iterator
        /// @return Reference to this zip_view iterator
        iterator& operator++() {
            return operator+=(1);
        }

        /// @brief Pre-decrement zip_view iterator
        /// @return Reference to this zip_view iterator
        iterator& operator--() {
            return operator+=(-1);
        }

        /// @brief Post-increment zip_view iterator
        /// @return New iterator that points to location prior to increment
        iterator operator++(int) { // NOLINT(cert-dcl21-cpp)
            iterator tmp(*this);
            operator++();
            return tmp;
        }

        /// @brief Post-decrement zip_view iterator
        /// @return New iterator that points to location prior to decrement
        iterator operator--(int) { // NOLINT(cert-dcl21-cpp)
            iterator tmp(*this);
            operator--();
            return tmp;
        }

        /// @brief Distance between iterators
        /// @param rhs Another iterator
        /// @return Distance between iterators
        int operator-(const iterator& rhs) const {
            return std::get<0>(_iters) - std::get<0>(rhs._iters);
        }

        /// @brief Advance iterator by rhs
        /// @param rhs By how much advance
        /// @return New advanced iterator
        iterator operator+(const difference_type rhs) const {
            iterator tmp(*this);
            tmp += rhs;
            return tmp;
        }

        /// @brief Retreat iterator by rhs
        /// @param rhs By how much retreat
        /// @return New retreated iterator
        iterator operator-(const difference_type rhs) const {
            iterator tmp(*this);
            tmp -= rhs;
            return tmp;
        }

        /// @brief Plus operator for iterators
        /// @param rhs By how much advance
        /// @param iter Iterator to advance
        /// @return New advanced iterator
        friend iterator operator+(const difference_type rhs, const iterator& iter) {
            return iter + rhs;
        }

        /// @brief Minus operator for iterators
        /// @param rhs By how much retreat
        /// @param iter Iterator to retreat
        /// @return New retreated iterator
        friend iterator operator-(const difference_type rhs, const iterator& iter) {
            return iter - rhs;
        }

        /// @brief Equality operator for iterators
        /// @param rhs Iterator to compare with
        /// @return True if iterators are equal
        bool operator==(const iterator& rhs) const {
            return at_least_one_of_iters_is_same(rhs);
        }

        /// @brief Not equal operator for iterators
        /// @param rhs Iterator to compare with
        /// @return True if iterators are not equal
        bool operator!=(const iterator& rhs) const {
            return none_of_iters_is_same(rhs);
        }

        /// @brief Spaceship operator for iterators
        /// @param rhs Iterator to compare with
        /// @return Automatic
        constexpr auto operator<=>(const iterator&) const = default;

    private:
        template<std::size_t I = 0>
        bool at_least_one_of_iters_is_same(const iterator& rhs) const {
            if (std::get<I>(_iters) == std::get<I>(rhs._iters)) {
                return true;
            }
            if constexpr (I + 1 < sizeof...(Iters)) {
                return at_least_one_of_iters_is_same<I + 1>(rhs);
            }
            return false;
        }

        template<std::size_t I = 0>
        bool none_of_iters_is_same(const iterator& rhs) const {
            if (std::get<I>(_iters) == std::get<I>(rhs._iters)) {
                return false;
            }
            if constexpr (I + 1 < sizeof...(Iters)) {
                return none_of_iters_is_same<I + 1>(rhs);
            }
            return true;
        }

        std::tuple<Iters...> _iters;

        template<typename... Args>
        friend class iterator;
    };

    /// @brief Construct zip_view from a pack of ranges
    /// @tparam ...R Range types
    /// @param ...ranges Range objects
    template<typename... R>
    explicit constexpr zip_view(R&&... ranges) : _ranges(std::views::all(ranges)...) {
    }

    /// @brief Return iterator pointing to the beginning of the zip_view range
    /// @return Iterator
    constexpr decltype(auto) begin() {
        return std::apply([](auto&&... ranges) { return iterator((ranges.begin())...); }, _ranges);
    }

    /// @brief Return iterator pointing to the end of the zip_view range
    /// @return Iterator
    constexpr decltype(auto) end() {
        return std::apply([](auto&&... ranges) { return iterator((ranges.end())...); }, _ranges);
    }

    /// @brief Return reverse iterator pointing to the end of the zip_view range
    /// @return Iterator
    constexpr decltype(auto) rbegin() {
        return std::apply([](auto&&... ranges) { return iterator((ranges.rbegin())...); }, _ranges);
    }

    /// @brief Return reverse iterator pointing to the beginning of the zip_view
    /// range
    /// @return Iterator
    constexpr decltype(auto) rend() {
        return std::apply([](auto&&... ranges) { return iterator((ranges.rend())...); }, _ranges);
    }

    /// @brief Return reverse iterator pointing to the const end of the zip_view
    /// range
    /// @return Iterator
    constexpr decltype(auto) crbegin() const {
        return std::apply([](auto&&... ranges) { return iterator((ranges.crbegin())...); }, _ranges);
    }

    /// @brief Return reverse iterator pointing to the const beginning of the
    /// zip_view range
    /// @return Iterator
    constexpr decltype(auto) crend() const {
        return std::apply([](auto&&... ranges) { return iterator((ranges.crend())...); }, _ranges);
    }

    /// @brief Return iterator pointing to the const beginning of the zip_view
    /// range
    /// @return Iterator
    constexpr decltype(auto) cbegin() const {
        return std::apply([](auto&&... ranges) { return iterator((ranges.cbegin())...); }, _ranges);
    }

    /// @brief Return iterator pointing to the const end of the zip_view range
    /// @return Iterator
    constexpr decltype(auto) cend() const {
        return std::apply([](auto&&... ranges) { return iterator((ranges.cend())...); }, _ranges);
    }

    /// @brief Return iterator pointing to the const beginning of the zip_view
    /// range
    /// @return Iterator
    constexpr decltype(auto) begin() const {
        return cbegin();
    }

    /// @brief Return iterator pointing to the const end of the zip_view range
    /// @return Iterator
    constexpr decltype(auto) end() const {
        return cend();
    }

    /// @brief Return reverse iterator pointing to the const end of the zip_view
    /// range
    /// @return Iterator
    constexpr decltype(auto) rbegin() const {
        return crbegin();
    }

    /// @brief Return reverse iterator pointing to the const beginning of the
    /// zip_view range
    /// @return Iterator
    constexpr decltype(auto) rend() const {
        return crend();
    }

private:
    std::tuple<Ranges...> _ranges;
};

// Deduction guide for zip_view;
// Basically Ranges... -> std::views::all_t<Ranges>...
// So that we are taking a view into the actual container
// and not taking a copy of the container itself.
template<typename... Ranges>
zip_view(Ranges&&...) -> zip_view<std::views::all_t<Ranges>...>;

} // namespace cobalt::asl