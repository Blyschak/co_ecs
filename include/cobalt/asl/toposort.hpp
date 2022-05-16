#pragma once

#include <algorithm>
#include <ranges>
#include <set>
#include <vector>

/// @brief Implements a topological sort algorithm.

namespace cobalt::asl {

namespace detail {

    template<typename G, typename T = G::key_type, typename M = G::mapped_type>
    void fill_empty_dependency(G& graph) {
        std::vector<T> diff;

        auto keys = graph | std::views::keys;
        auto deps = graph | std::views::transform([](const auto& entry) {
            const auto& [_, deps] = entry;
            return deps;
        }) | std::views::join;

        std::ranges::set_difference(deps, keys, std::back_inserter(diff));

        auto empty_deps = diff | std::views::transform([](const auto& key) { return std::pair{ key, M{} }; });

        std::ranges::copy(empty_deps, std::inserter(graph, graph.begin()));
    }


} // namespace detail

/// @brief The input to the toposort function is a graph describing the dependencies among the input nodes. Each key is
/// a dependent node, the corresponding value is a set containing the dependent nodes.
///
/// @tparam G Input dependency graph
/// @tparam O Ouput iterator, this function will insert G::mapped_type values
/// @param graph Input dependency graph
/// @param output Output iterator
/// @return true Successfully sorted
/// @return false Input dependency graph has circular dependency
template<typename G, typename O>
bool topological_sort(G graph, O&& output) {
    using M = G::mapped_type;

    detail::fill_empty_dependency(graph);

    while (true) {
        auto ordered_view = graph //
                            | std::views::filter([](const auto& entry) { return entry.second.empty(); })
                            | std::views::keys;

        M ordered{ ordered_view.begin(), ordered_view.end() };

        if (ordered.empty()) {
            break;
        }

        *output = ordered;
        output++;

        for (const auto& key : ordered) {
            graph.erase(key);
        }

        for (auto& [key, deps] : graph) {
            M new_deps;
            std::ranges::set_difference(deps, ordered, std::inserter(new_deps, new_deps.begin()));
            deps = new_deps;
        }
    }

    return graph.empty();
}

} // namespace cobalt::asl