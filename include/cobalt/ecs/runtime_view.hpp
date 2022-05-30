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
        for (auto chunk : chunks(_registry.get_archetypes(), _ids.begin(), _ids.end())) {
            for (auto [entity] : chunk) {
                func(entity);
            }
        }
    }

private:
    /// @brief Return a range of chunks that match given ids in [first, last]
    ///
    /// @param archetypes Archetypes
    /// @param first First iterator
    /// @param last Last iterator
    /// @return decltype(auto)
    template<std::forward_iterator iter_t>
    decltype(auto) chunks(auto&& archetypes, iter_t first, iter_t last) {
        auto filter_archetypes = [first, last](auto& archetype) mutable {
            for (; first != last; first++) {
                if (!archetype->contains(*first)) {
                    return false;
                }
            }
            return true;
        };
        auto into_chunks = [](auto& archetype) -> decltype(auto) { return archetype->chunks(); };
        auto as_typed_chunk = [](auto& chunk) -> decltype(auto) { return chunk_view<const entity&>(chunk); };

        return archetypes                               // for each archetype entry in archetype map
               | std::views::values                     // for each value, a pointer to archetype
               | std::views::filter(filter_archetypes)  // filter archetype by requested components
               | std::views::transform(into_chunks)     // fetch chunks vector
               | std::views::join                       // join chunks together
               | std::views::transform(as_typed_chunk); // each chunk casted to a typed chunk view range-like type
    }

    registry& _registry;
    std::vector<component_id> _ids;
};

} // namespace cobalt::ecs