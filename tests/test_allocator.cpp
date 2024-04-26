#include <catch2/catch_all.hpp>

#include <co_ecs/detail/allocator/linear_allocator.hpp>
#include <co_ecs/detail/allocator/stack_allocator.hpp>

using namespace co_ecs::detail;

TEST_CASE("Linear allocator") {
    void* ptr;
    const auto buffer_size = 1024;
    std::unique_ptr<uint8_t[]> buffer{ new uint8_t[buffer_size] };

    linear_allocator alloc{ buffer.get(), buffer_size };

    const auto alignment = GENERATE(1, 2, 4, 8, 16, 32);
    const auto size = GENERATE(1, 4, 8, 10, 12, 16, 32);

    ptr = alloc.allocate(size, alignment);
    REQUIRE((std::size_t(ptr) % alignment) == 0);

    ptr = alloc.allocate(1024, 16);
    REQUIRE(ptr == nullptr);
}

TEST_CASE("Stack allocator") {
    void *ptr, *ptr2, *ptr3;
    std::size_t remaining;

    const auto buffer_size = 1024;
    std::unique_ptr<uint8_t[]> buffer{ new uint8_t[buffer_size] };

    stack_allocator alloc{ buffer.get(), buffer_size };

    const auto alignment = GENERATE(1, 2, 4, 8, 16, 32);
    const auto size = GENERATE(1, 4, 8, 10, 12, 16, 32);

    ptr = alloc.allocate(size, alignment);
    REQUIRE((std::size_t(ptr) % alignment) == 0);

    remaining = alloc.remaining();

    const auto alignment2 = GENERATE(1, 2, 4, 8, 16, 32);
    const auto size2 = GENERATE(1, 4, 8, 10, 12, 16, 32);

    ptr2 = alloc.allocate(size2, alignment2);
    REQUIRE((std::size_t(ptr2) % alignment2) == 0);

    REQUIRE(alloc.remaining() < remaining);

    alloc.deallocate(ptr2);

    REQUIRE(alloc.remaining() == remaining);

    alloc.deallocate(ptr);
}