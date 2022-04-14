#include <cobalt/ecs/archetype.hpp>
#include <cobalt/ecs/entity.hpp>

#include <gtest/gtest.h>

using namespace cobalt;

namespace components {

struct a {
    int foo;
    bool bar;

    a() : foo(15), bar(false) {
    }

    a(int foo, bool bar) : foo(foo), bar(bar) {
    }
};
struct b {
    bool c{};
};

} // namespace components

TEST(archetype, basic) {
    asl::graph<ecs::archetype> g;
    auto set = ecs::component_meta_set::create<components::a, components::b>();
    auto [archetype, _] = g.emplace(set);

    auto location = archetype->allocate(ecs::entity{ 0, 0 }, components::a(16, true), components::b(true));
    EXPECT_EQ(location.arch, archetype);
    EXPECT_EQ(location.chunk_index, 0);
    EXPECT_EQ(location.entry_index, 0);

    auto location1 = archetype->allocate(ecs::entity{ 1, 0 }, components::a(17, true), components::b(false));
    EXPECT_EQ(location1.arch, archetype);
    EXPECT_EQ(location1.chunk_index, 0);
    EXPECT_EQ(location1.entry_index, 1);

    auto& a = archetype->read<components::a>(location1);
    EXPECT_EQ(a.foo, 17);
    EXPECT_EQ(a.bar, true);

    archetype->write<components::a>(location1, 18, false);

    a = archetype->read<components::a>(location1);
    EXPECT_EQ(a.foo, 18);
    EXPECT_EQ(a.bar, false);

    auto entity = archetype->deallocate(location);
    EXPECT_EQ(entity, ecs::entity(1, 0));

    location1.entry_index = 0;
    auto entity1 = archetype->deallocate(location1);
    EXPECT_EQ(entity1, ecs::entity::invalid());
}

TEST(chunk, basic) {
    auto set = ecs::component_meta_set();
    set.insert<components::a>();
    set.insert<components::b>();
    ecs::chunk chunk(set);
    EXPECT_EQ(chunk.max_size(), 963);
}

TEST(chunk, basic_emplace) {
    auto set = ecs::component_meta_set();
    set.insert<components::a>();
    set.insert<components::b>();
    ecs::chunk chunk(set);

    chunk.emplace_back(components::a(15, false), components::b(false));
    EXPECT_EQ(chunk.at<components::a>(0).foo, 15);
    EXPECT_EQ(chunk.at<components::a>(0).bar, false);
    EXPECT_EQ(chunk.at<components::b>(0).c, false);
}
