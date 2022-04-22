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
    auto entity1211 = registry.create<some_struct>({ "entity1211" });

    ecs::hierarchy::set_child(registry, entity1, entity11);
    ecs::hierarchy::set_child(registry, entity1, entity12);
    ecs::hierarchy::set_child(registry, entity12, entity121);
    ecs::hierarchy::set_child(registry, entity121, entity1211);

    std::map<std::string, std::set<std::string>> parent_child_relations;

    ecs::hierarchy::for_each_child(registry,
        [&](std::tuple<const some_struct&> parent_some_struct, std::tuple<const some_struct&> child_some_struct) {
            auto [parent] = parent_some_struct;
            auto [child] = child_some_struct;

            parent_child_relations[parent.name].insert(child.name);
        });

    EXPECT_EQ(parent_child_relations.size(), 3);
    EXPECT_TRUE(parent_child_relations["entity1"].contains("entity11"));
    EXPECT_TRUE(parent_child_relations["entity1"].contains("entity12"));
    EXPECT_TRUE(parent_child_relations["entity12"].contains("entity121"));
    EXPECT_TRUE(parent_child_relations["entity121"].contains("entity1211"));

    ecs::hierarchy::remove_child(registry, entity1, entity12);

    parent_child_relations.clear();
    ecs::hierarchy::for_each_child(registry,
        [&](std::tuple<const some_struct&> parent_some_struct, std::tuple<const some_struct&> child_some_struct) {
            auto [parent] = parent_some_struct;
            auto [child] = child_some_struct;

            parent_child_relations[parent.name].insert(child.name);
        });
    EXPECT_EQ(parent_child_relations.size(), 3);
    EXPECT_TRUE(parent_child_relations["entity1"].contains("entity11"));
    EXPECT_TRUE(parent_child_relations["entity12"].contains("entity121"));
    EXPECT_TRUE(parent_child_relations["entity121"].contains("entity1211"));

    ecs::hierarchy::remove_child(registry, entity121, entity1211);

    parent_child_relations.clear();
    ecs::hierarchy::for_each_child(registry,
        [&](std::tuple<const some_struct&> parent_some_struct, std::tuple<const some_struct&> child_some_struct) {
            auto [parent] = parent_some_struct;
            auto [child] = child_some_struct;

            parent_child_relations[parent.name].insert(child.name);
        });
    EXPECT_EQ(parent_child_relations.size(), 2);
    EXPECT_TRUE(parent_child_relations["entity1"].contains("entity11"));
    EXPECT_TRUE(parent_child_relations["entity12"].contains("entity121"));

    ecs::hierarchy::remove_child(registry, entity1, entity11);

    parent_child_relations.clear();
    ecs::hierarchy::for_each_child(registry,
        [&](std::tuple<const some_struct&> parent_some_struct, std::tuple<const some_struct&> child_some_struct) {
            auto [parent] = parent_some_struct;
            auto [child] = child_some_struct;

            parent_child_relations[parent.name].insert(child.name);
        });
    EXPECT_EQ(parent_child_relations.size(), 1);
    EXPECT_TRUE(parent_child_relations["entity12"].contains("entity121"));

    ecs::hierarchy::remove_child(registry, entity12, entity121);

    parent_child_relations.clear();
    ecs::hierarchy::for_each_child(registry,
        [&](std::tuple<const some_struct&> parent_some_struct, std::tuple<const some_struct&> child_some_struct) {
            auto [parent] = parent_some_struct;
            auto [child] = child_some_struct;

            parent_child_relations[parent.name].insert(child.name);
        });
    EXPECT_TRUE(parent_child_relations.empty());
}
