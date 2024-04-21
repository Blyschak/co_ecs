#include "components.hpp"

#include <catch2/catch_all.hpp>
#include <co_ecs/detail/allocator/linear_allocator.hpp>
#include <co_ecs/detail/allocator/stack_allocator.hpp>

using namespace co_ecs::detail;

TEST_CASE("Linear allocator") {
    void* ptr;
    char* buffer = reinterpret_cast<char*>(GENERATE(0x1, 0x2, 0x7, 0x8, 0x100));

    linear_allocator alloc{ buffer, 1024 };

    const auto alignment = GENERATE(1, 2, 4, 8, 16, 32);
    const auto size = GENERATE(1, 4, 8, 10, 12, 16, 32);

    ptr = alloc.allocate(size, alignment);
    REQUIRE((std::size_t(ptr) % alignment) == 0);

    ptr = alloc.allocate(1024, 16);
    REQUIRE(ptr == nullptr);
}