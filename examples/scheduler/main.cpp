#include <co_ecs/co_ecs.hpp>

#include <iostream>

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

void end_frame(co_ecs::command_writer c, co_ecs::view<const co_ecs::entity&, const pos&, const rot&> v) {
    v.each([&](const auto& entity, const auto& pos, const auto& rot) {
        std::cout << "Entity {" << entity.id() << ", " << entity.generation() << "} " << "Position {" << pos.x << " "
                  << pos.y << "} " << "Rotation {" << rot.angle << "}\n";
        c.destroy(entity);
    });
}

void setup(co_ecs::registry& r, co_ecs::command_writer commands) {
    static int i;
    commands.create<pos, rot, vel, tan_vel>(
        {}, {}, { .x = -1.0f + 0.005f * i, .y = -2.0f + 0.001f * i }, { .speed = 0.0003f * i });
    i++;
}

int main() {
    co_ecs::registry registry;

    auto schedule = co_ecs::schedule()
                        .add_init_system([]() { std::cout << "Init...\n"; })
                        .add_init_system([](co_ecs::registry& reg) { std::cout << "Post-Init...\n"; })
                        .begin_stage("Setup")
                        .add_system(co_ecs::main_thread_execution_policy, setup)
                        .add_system(start_frame)
                        .end_stage()
                        .begin_stage("Update")
                        .add_system(end_frame)
                        .add_system(update_pos)
                        .add_system(update_rot)
                        .end_stage()
                        .create_executor(registry);

    for (auto i = 0; i < 5; i++) {
        schedule->run_once();
    }

    return 0;
}