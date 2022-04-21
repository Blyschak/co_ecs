#pragma once

#include <cstdint>

#include <cobalt/ecs/entity.hpp>
#include <cobalt/ecs/registry.hpp>

namespace cobalt::ecs {

using depth_t = std::uint32_t;

struct parent {
    depth_t depth{};
    ecs::entity entity{};
};

void entity_set_child(ecs::registry& registry, ecs::entity parent_entity, ecs::entity child_entity) {
    depth_t depth = 0;
    if (registry.has<parent>(parent_entity)) {
        depth = registry.get<parent>(parent_entity).depth;
    }
    depth++;
    registry.set<parent>(child_entity, depth, parent_entity);
}

template<component_reference... Args>
void entity_for_each_child(ecs::registry& registry, auto&& func) {
    depth_t current_depth{};
    depth_t max_depth{};
    do {
        for (auto value : registry.each<const parent&, Args...>()) {
            const auto& parent = std::get<0>(value);
            if (parent.depth > max_depth) {
                max_depth = parent.depth;
            }
            if (parent.depth != current_depth) {
                continue;
            }
            func(value);
        }
        current_depth++;
    } while (current_depth <= max_depth);
}

} // namespace cobalt::ecs