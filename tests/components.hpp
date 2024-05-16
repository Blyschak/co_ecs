#pragma once

#include <cstddef>
#include <utility>

// This header contains components structs that are going to be used for tests.
// Different components are simply generated through meta-programming with use of component_generator class.

template<std::size_t I>
struct foo {
    int a{};
    int b{};

    foo() = default;
    foo(int a, int b) noexcept : a(a), b(b) {
    }

    [[nodiscard]]
    auto operator==(const foo& rhs) const noexcept -> bool {
        return (rhs.a == a) && (rhs.b == b);
    }
};

template<std::size_t I>
struct bar {
    int a{};
    int b{};

    bar() = default;
    bar(int a, int b) noexcept : a(a), b(b) {
    }

    [[nodiscard]]
    auto operator==(const bar& rhs) const noexcept -> bool {
        return (rhs.a == a) && (rhs.b == b);
    }
};

struct foo_creator {
    template<std::size_t I>
    using type = foo<I>;
};

struct bar_creator {
    template<std::size_t I>
    using type = foo<I>;
};

template<typename T, std::size_t N>
class components_generator {
    template<typename = std::make_index_sequence<N>>
    struct impl;

    template<std::size_t... Is>
    struct impl<std::index_sequence<Is...>> {
        template<std::size_t II>
        using wrap = typename T::template type<II>;
        using type = std::tuple<wrap<Is>...>;
    };

public:
    using type = typename impl<>::type;
};
