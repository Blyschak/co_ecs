#include <cobalt/asl/graph.hpp>

#include <gtest/gtest.h>

using namespace cobalt::asl;

struct my_node : public node_base<std::set<char>, my_node> {
    my_node(const std::set<char>& s = {}) : node_base<std::set<char>, my_node>(s) {
    }
};

TEST(graph, graph_basic) {
    graph<my_node> g;
    g.emplace({ 'A', 'B', 'C' });
    g.emplace({ 'A', 'C' });
    g.emplace({ 'A', 'B' });
    g.emplace({ 'B' });
    g.emplace({ 'A', 'C', 'D' });
    g.emplace({ 'A', 'B', 'D' });
    g.emplace({ 'C' });
    g.emplace({ 'A' });
    g.emplace({ 'A', 'B', 'C', 'D' });

    auto [node, inserted] = g.emplace({ 'A', 'B' });
    EXPECT_FALSE(inserted);
    ASSERT_TRUE(node);
    EXPECT_EQ(node->key(), std::set({ 'A', 'B' }));

    std::tie(node, inserted) = g.emplace({ 'A', 'B', 'C' });
    EXPECT_FALSE(inserted);
    ASSERT_TRUE(node);
    EXPECT_EQ(node->key(), std::set({ 'A', 'B', 'C' }));

    std::tie(node, inserted) = g.emplace({ 'A', 'B', 'C', 'D' });
    EXPECT_FALSE(inserted);
    ASSERT_TRUE(node);
    EXPECT_EQ(node->key(), std::set({ 'A', 'B', 'C', 'D' }));

    auto [node1, inserted1] = g.emplace_right(node, 'D');
    EXPECT_FALSE(inserted1);
    ASSERT_TRUE(node1);
    EXPECT_EQ(node, node1);
    EXPECT_EQ(node1->key(), std::set({ 'A', 'B', 'C', 'D' }));

    std::tie(node, inserted) = g.emplace({ 'A', 'C', 'D' });
    ASSERT_TRUE(node);
    EXPECT_EQ(node->key(), std::set({ 'A', 'C', 'D' }));

    auto [node2, inserted2] = g.emplace_right(node, 'B');
    ASSERT_TRUE(node2);
    EXPECT_EQ(node1, node2);
    EXPECT_EQ(node2->key(), std::set({ 'A', 'B', 'C', 'D' }));

    auto [node3, inserted3] = g.emplace_right(node, 'B');
    ASSERT_TRUE(node3);
    EXPECT_EQ(node2, node3);

    auto [_, erased] = g.erase({ 'A' });
    EXPECT_TRUE(erased);

    auto node4 = g.find({ 'A' });
    ASSERT_FALSE(node4);
}

TEST(graph, graph_move) {
    graph<my_node> g1;
    g1.emplace({ 'A', 'B', 'C' });
    g1.emplace({ 'A', 'C' });
    g1.emplace({ 'A', 'B' });
    g1.emplace({ 'B' });

    auto* node1 = g1.find({ 'A', 'B', 'C' });
    EXPECT_TRUE(node1);

    graph<my_node> g2 = std::move(g1);

    auto* node2 = g2.find({ 'A', 'B', 'C' });
    EXPECT_TRUE(node2);
}
