#pragma once

#include <co_ecs/registry.hpp>

#include <type_traits>

namespace co_ecs {

/// @brief A view lets you get a range over components of Args out of a registry
///
/// A view isn't invalidated when there are changes made to the registry which lets a create one an re-use over time.
///
/// @tparam Args Component references types
template<component_reference... Args>
class view {
public:
    /// @brief Const when all component references are const
    static constexpr bool is_const = view_arguments<Args...>::is_const;

    /// @brief Iteration value type
    using value_type = std::tuple<Args...>;

    /// @brief Registry type deduced based on input component reference types
    using registry_type = std::conditional_t<is_const, const registry&, registry&>;

    /// @brief Construct a new view object
    ///
    /// @param registry Reference to the registry
    explicit view(registry_type registry) noexcept : _registry(registry) {
    }

    /// @brief Returns an iterator that yields a std::tuple<Args...>
    ///
    /// @return decltype(auto) Iterator
    auto each() -> decltype(auto)
        requires(!is_const)
    {
        return chunks(_registry.get_archetypes()) | std::views::join; // join all chunks together
    }

    /// @brief Returns an iterator that yields a std::tuple<Args...>
    ///
    /// @return decltype(auto) Iterator
    auto each() const -> decltype(auto)
        requires(is_const)
    {
        return chunks(_registry.get_archetypes()) | std::views::join; // join all chunks together
    }

    /// @brief Run func on every entity that matches the Args requirement
    ///
    /// @param func A callable to run on entity components
    void each(auto&& func)
        requires(!is_const)
    {
        for (auto chunk : chunks(_registry.get_archetypes())) {
            for (auto entry : chunk) {
                std::apply(func, entry);
            }
        }
    }

    /// @brief Run func on every entity that matches the Args requirement. Constant version
    ///
    /// NOTE: See the note on non-const each()
    ///
    /// @param func A callable to run on entity components
    void each(auto&& func) const
        requires(is_const)
    {
        for (auto chunk : chunks(_registry.get_archetypes())) {
            for (auto entry : chunk) {
                std::apply(func, entry);
            }
        }
    }

    /// @brief Get components for a single entity
    ///
    /// @param ent Entity to query
    /// @return value_type Components tuple
    auto get(entity ent) -> value_type
        requires(!is_const)
    {
        return _registry.template get<Args...>(ent);
    }

    /// @brief Get components for a single entity
    ///
    /// @param ent Entity to query
    /// @return value_type Components tuple
    auto get(entity ent) const -> value_type
        requires(is_const)
    {
        return _registry.template get<Args...>(ent);
    }

private:
    /// @brief Return a range of chunks that match given component set in Args
    ///
    /// @param archetypes Archetypes
    /// @return decltype(auto)
    static auto chunks(auto&& archetypes) -> decltype(auto) {
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

// Implement registry methods after we have view class defined

template<component_reference... Args>
auto registry::view() -> co_ecs::view<Args...>
    requires(!const_component_references_v<Args...>)
{
    return co_ecs::view<Args...>{ *this };
}

template<component_reference... Args>
auto registry::view() const -> co_ecs::view<Args...>
    requires const_component_references_v<Args...>
{
    return co_ecs::view<Args...>{ *this };
}

template<typename F>
void registry::each(F&& func)
    requires(!detail::func_decomposer<F>::is_const)
{
    using view_t = typename detail::func_decomposer<F>::view_t;
    view_t{ *this }.each(std::forward<F>(func));
}

template<typename F>
void registry::each(F&& func) const
    requires(detail::func_decomposer<F>::is_const)
{
    using view_t = typename detail::func_decomposer<F>::view_t;
    view_t{ *this }.each(std::forward<F>(func));
}

} // namespace co_ecs