#include <cobalt/asl/defer.hpp>

#include <gtest/gtest.h>

#include <vector>

double foo() {
    return 0.0;
}

class bar {
public:
    explicit bar(int* ptr) : _ptr(ptr) {
    }

    void operator()() const {
        ++*_ptr;
    }

private:
    // mutable to test const bar invocable
    mutable int* _ptr;
};

TEST(defer, different_bindings) {
    cobalt::asl::defer{ foo };

    cobalt::asl::defer{ &foo };

    auto* const baz = &foo;
    cobalt::asl::defer{ baz };

    // moved
    int c = 0;
    {
        auto l = [&c] { c++; };
        auto defer = cobalt::asl::defer{ l };
        auto new_defer = std::move(defer);
    }
    EXPECT_EQ(1, c);

    std::vector<int> v;
    void (std::vector<int>::*push_back)(int const&) = &std::vector<int>::push_back;

    v.push_back(1);
    { cobalt::asl::defer{ std::bind(&std::vector<int>::pop_back, &v) }; }
    EXPECT_EQ(0, v.size());

    { cobalt::asl::defer(std::bind(push_back, v, 2)); }
    EXPECT_EQ(0, v.size());

    { cobalt::asl::defer(std::bind(push_back, &v, 4)); }
    EXPECT_EQ(1, v.size());

    { cobalt::asl::defer(std::bind(push_back, std::ref(v), 4)); }
    EXPECT_EQ(2, v.size());

    // lambda with a reference to v
    {
        cobalt::asl::defer([&] { v.push_back(5); });
    }
    EXPECT_EQ(3, v.size());

    // lambda with a copy of v
    {
        cobalt::asl::defer([v]() mutable { v.push_back(6); });
    }
    EXPECT_EQ(3, v.size());

    // functor object
    int n = 0;
    {
        bar f(&n);
        cobalt::asl::defer{ f };
    }
    EXPECT_EQ(1, n);

    // const functor object
    n = 0;
    {
        const bar f(&n);
        cobalt::asl::defer{ f };
    }
    EXPECT_EQ(1, n);

    // temporary functor object
    n = 0;
    { cobalt::asl::defer(bar(&n)); }
    EXPECT_EQ(1, n);

    // Use const auto& instead of ScopeGuard
    n = 10;
    {
        const auto& g = cobalt::asl::defer(bar(&n));
        (void)g;
    }
    EXPECT_EQ(11, n);

    n = 0;
    std::function a = [&n]() { n = 1; };
    { cobalt::asl::defer{ a }; }
    EXPECT_EQ(1, n);
}