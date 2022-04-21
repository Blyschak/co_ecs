#include <cobalt/ecs/hierarchy.hpp>

#include <gtest/gtest.h>

using namespace cobalt;

struct some_struct {
    std::string name;
};

TEST(hierarchy, basic) {
    ecs::registry registry;
    auto entity1 = registry.create<some_struct>({ "entity1" });
    auto entity11 = registry.create<some_struct>({ "entity11" });
    auto entity12 = registry.create<some_struct>({ "entity12" });
    auto entity121 = registry.create<some_struct>({ "entity121" });

    ecs::entity_set_child(registry, entity1, entity11);
    ecs::entity_set_child(registry, entity1, entity12);
    ecs::entity_set_child(registry, entity12, entity121);

    ecs::entity_for_each_child<some_struct>(registry, [&](auto tuple) {
        auto& parent = std::get<0>(tuple);
        auto& child = std::get<1>(tuple);
        // std::cout << "entity " << child.name << " parent " << registry.get<some_struct>(parent.entity).name
        //           << std::endl;
    });

    // std::cout << "==============\n";

    ecs::entity_set_child(registry, entity11, entity121);

    ecs::entity_for_each_child<some_struct>(registry, [&](auto tuple) {
        auto& parent = std::get<0>(tuple);
        auto& child = std::get<1>(tuple);
        // std::cout << "entity " << child.name << " parent " << registry.get<some_struct>(parent.entity).name
        //           << std::endl;
    });

    // std::cout << "==============\n";
}
