#include <cobalt/core/logging.hpp>
#include <cobalt/ecs/registry.hpp>
#include <cobalt/ecs/view.hpp>

using namespace cobalt;

struct transform {
    float position[3];
    float rotation[3];
    float scale[3];
};

struct velocity {
    float vector[3];
};

int main() {
    scoped_log scope("main()");
    ecs::registry registry;

    {
        scoped_timer_log scope("creation");
        for (int i = 0; i < 100; i++) {
            registry.create<transform, velocity>(
                { { 0.f + i * 0.1f, -5.f + i * 0.1f, 0.f }, { i * 0.1f, i * 0.1f }, { 1.f, 1.f, 1.f } },
                { 1.f, 2.f, 3.f });
        }
    }

    for (auto [transform, velocity] : ecs::view<transform&, const velocity&>(registry).each()) {
        transform.position[0] += velocity.vector[0];
        transform.position[1] += velocity.vector[1];
        transform.position[2] += velocity.vector[2];
    }

    for (auto [transform] : ecs::view<const transform&>(registry).each()) {
        log_info("transform {} {} {}", transform.position[0], transform.position[1], transform.position[2]);
    }

    return 0;
}