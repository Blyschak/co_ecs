#include <co_ecs/co_ecs.hpp>

#include <iostream>

constexpr auto num_entities = 1000;

static co_ecs::registry registry;
static co_ecs::experimental::scheduler scheduler;

static bool exit_flag;
static int frame;

struct pos {
    float x, y;
};

struct rot {
    float angle;
};

struct vel {
    float x, y;
};

struct tan_vel {
    float speed;
};

void update_pos(co_ecs::view<pos&, const vel&> v) {
    v.each([](auto& pos, const auto& vel) {
        pos.x += vel.x;
        pos.y += vel.y;
    });
}

void update_rot(co_ecs::view<rot&, const tan_vel&> v) {
    v.each([](auto& rot, const auto& vel) { rot.angle += vel.speed; });
}

void start_frame() {
    frame++;
}

void end_frame(co_ecs::view<const co_ecs::entity&, const pos&, const rot&> v) {
    if (frame % 100000 == 0) {
        v.each([](const auto& entity, const auto& pos, const auto& rot) {
            std::cout << "Entity {" << entity.id() << ", " << entity.generation() << "} " << "Position {" << pos.x
                      << " " << pos.y << "} " << "Rotation {" << rot.angle << "}\n";
        });

        exit_flag = true;
    }
}

void setup() {
    for (int i = 0; i < num_entities; i++) {
        registry.create<pos, rot, vel, tan_vel>(
            {}, {}, { .x = -1.0f + 0.005f * i, .y = -2.0f + 0.001f * i }, { .speed = 0.0003f * i });
    }
}

int main() {
    setup();

    scheduler.add_system(&start_frame)
        .barrier()
        .add_system(&update_pos) // Two systems running in parallel
        .add_system(&update_rot)
        .barrier()
        .add_system(&end_frame);

    scheduler.run<co_ecs::experimental::parallel_executor>(registry, exit_flag);

    return 0;
}