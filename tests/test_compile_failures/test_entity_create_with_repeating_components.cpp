#include <co_ecs/co_ecs.hpp>

struct test1 {};
struct test2 {};

int main() {
    co_ecs::registry registry;
    auto entity = registry.create<test1, test2, test1>({}, {}, {});

    return 0;
}