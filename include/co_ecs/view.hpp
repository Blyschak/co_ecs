#pragma once

#include <co_ecs/registry.hpp>

#include <type_traits>

namespace co_ecs {

/// @brief A view lets you get a viewable range over components of Args out of a registry
///
/// A view isn't invalidated when there are changes made to the registry which lets a create one an re-use over time.
///
/// @tparam Args Component references types
template<component_reference... Args>
class view {
public:
    /// @brief Const when all component references are const
    static constexpr bool is_const = const_component_references_v<Args...>;

    using value_type = std::tuple<Args...>;
    using registry_type = std::conditional_t<is_const, const registry&, registry&>;

    /// @brief Construct a new view object
    ///
    /// @param registry Reference to the registry
    explicit view(registry_type registry) noexcept : _registry(registry) {
    }

    /// @brief Returns an iterator that yields a std::tuple<Args...>
    ///
    /// @return decltype(auto) Iterator
    decltype(auto) each()
        requires(!is_const)
    {
        return chunks(_registry.get_archetypes()) | std::views::join; // join all chunks together
    }

    /// @brief Returns an iterator that yields a std::tuple<Args...>
    ///
    /// @return decltype(auto) Iterator
    decltype(auto) each() const
        requires(is_const)
    {
        return chunks(_registry.get_archetypes()) | std::views::join; // join all chunks together
    }

    /// @brief Run func on every entity that matches the Args requirement
    ///
    /// NOTE: This kind of iteration might be faster and better optimized by the compiler since the func can operate on
    /// a chunk that yields two tuples of pointers to the actual data whereas an each() variant returns an iterator over
    /// iterator over iterator to the actual data which is a challenge for compiler to optimize. Look at the benchmarks
    /// to see the actual difference.
    ///
    /// @param func A callable to run on entity components
    void each(auto&& func)
        requires(!is_const)
    {
        for (auto chunk : chunks(_registry.get_archetypes())) {
            for (auto entry : chunk) {
                std::apply([func = std::forward<decltype(func)>(func)](
                               auto&&... args) { func(std::forward<decltype(args)>(args)...); },
                    entry);
            }
        }
    }

    /// @brief Run func on every entity that matches the Args requirement. Constant version
    ///
    /// NOTE: This kind of iteration might be faster and better optimized by the compiler since the func can operate on
    /// a chunk that yields two tuples of pointers to the actual data whereas an each() variant returns an iterator over
    /// iterator over iterator to the actual data which is a challenge for compiler to optimize. Look at the benchmarks
    /// to see the actual difference.
    ///
    /// @param func A callable to run on entity components
    void each(auto&& func) const
        requires(is_const)
    {
        for (auto chunk : chunks(_registry.get_archetypes())) {
            for (auto entry : chunk) {
                std::apply([func = std::forward<decltype(func)>(func)](
                               auto&&... args) { func(std::forward<decltype(args)>(args)...); },
                    entry);
            }
        }
    }

    /// @brief Get components for a single entity
    ///
    /// @param ent Entity to query
    /// @return value_type Components tuple
    value_type get(entity ent)
        requires(!is_const)
    {
        return _registry.template get<Args...>(ent);
    }

    /// @brief Get components for a single entity
    ///
    /// @param ent Entity to query
    /// @return value_type Components tuple
    value_type get(entity ent) const
        requires(is_const)
    {
        return _registry.template get<Args...>(ent);
    }

private:
    /// @brief Return a range of chunks that match given component set in Args
    ///
    /// @param archetypes Archetypes
    /// @return decltype(auto)
    static decltype(auto) chunks(auto&& archetypes) {
        auto filter_archetypes = [](auto& archetype) {
            return (... && archetype->template contains<decay_component_t<Args>>());
        };
        auto into_chunks = [](auto& archetype) -> decltype(auto) { return archetype->chunks(); };
        auto as_typed_chunk = [](auto& chunk) -> decltype(auto) { return chunk_view<Args...>(chunk); };

        return archetypes                               // for each archetype entry in archetype map
               | std::views::values                     // for each value, a pointer to archetype
               | std::views::filter(filter_archetypes)  // filter archetype by requested components
               | std::views::transform(into_chunks)     // fetch chunks vector
               | std::views::join                       // join chunks together
               | std::views::transform(as_typed_chunk); // each chunk casted to a typed chunk view range-like type
    }

    registry_type _registry;
};

} // namespace cobalt::ecs