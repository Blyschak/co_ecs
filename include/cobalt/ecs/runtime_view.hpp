#pragma once

#include <cobalt/ecs/registry.hpp>

namespace cobalt::ecs {

/// @brief Runtime view, used for scripting
class runtime_view {
public:
    /// @brief Construct a new runtime view object
    ///
    /// @tparam container_t Container type holding component IDs
    /// @param registry Registry reference
    /// @param ids IDs to match archetypes
    template<typename container_t>
    runtime_view(registry& registry, container_t ids) : _registry(registry), _ids(ids.begin(), ids.end()) {
    }

    /// @brief Iterate over entities that match given IDs
    ///
    /// @param func Function to call on every entity
    void each(auto&& func) {
        _registry.runtime_each(_ids, func);
    }

private:
    registry& _registry;
    std::vector<component_id> _ids;
};

} // namespace cobalt::ecs