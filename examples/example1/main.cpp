#include <co_ecs/registry.hpp>
#include <co_ecs/view.hpp>

#include <iostream>

struct position {
    float x, y;
};

struct velocity {
    float x, y;
};

int main() {
    co_ecs::registry registry;

    for (int i = 0; i < 100; i++) {
        registry.create<position, velocity>({ i * 1.f, i * 1.5f }, { i * .3f, -i * 5.f });
    }

    for (auto [position, velocity] : co_ecs::view<position&, const velocity&>(registry).each()) {
        position.x += velocity.x;
        position.y += velocity.y;
    }

    for (auto [position] : co_ecs::view<const position&>(registry).each()) {
        std::cout << "position {" << position.x << ", " << position.y << "}\n";
    }

    return 0;
}