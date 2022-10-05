#include <co_ecs/co_ecs.hpp>

#include <iostream>

struct position {
    float x, y;
};

struct velocity {
    float x, y;
};

void print_positions(const co_ecs::registry& registry) {
    registry.each(
        [](const position& position) { std::cout << "position {" << position.x << ", " << position.y << "}\n"; });
}

int main() {
    co_ecs::registry registry;

    for (int i = 0; i < 100; i++) {
        registry.create<position, velocity>({ i * 1.f, i * 1.5f }, { i * .3f, -i * 5.f });
    }

    for (auto [position, velocity] : registry.view<position&, const velocity&>().each()) {
        position.x += velocity.x;
        position.y += velocity.y;
    }

    print_positions(registry);

    return 0;
}