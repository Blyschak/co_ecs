# Cobalt ECS

![Build badge](https://github.com/Blyschak/cobalt-ecs/actions/workflows/.github.yml/badge.svg)

## Usage

```c++
#include <cobalt/ecs/registry.hpp>
#include <cobalt/ecs/view.hpp>

using namespace cobalt;

struct position {
    float x, y;
};

struct velocity {
    float x, y;
};

int main() {
    ecs::registry registry;

    for (int i = 0; i < 100; i++) {
        registry.create<position, velocity>({ i * 1.f, i * 1.5f }, { i * .3f, - i * 5.f });
    }

    for (auto [position, velocity] : ecs::view<position&, const velocity&>(registry).each()) {
        position.x += velocity.x;
        position.y += velocity.y;
    }

    for (auto [position] : ecs::view<const position&>(registry).each()) {
        std::cout << "position {" << position.x << ", " << position.y << "}\n";
    }

    return 0;
}
```
