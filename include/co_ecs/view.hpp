#pragma once

#include <co_ecs/registry.hpp>
#include <co_ecs/system.hpp>

#include <co_ecs/detail/views.hpp>

#include <co_ecs/thread_pool/parallel_for.hpp>

#include <type_traits>

namespace co_ecs {

namespace detail {

/// @brief Helper function to return true if all of the passed arguments are true
///
/// @tparam Args Parameter pack types
/// @param args Parameter pack
template<typename... Args>
inline auto all(Args... args) -> bool {
    return (... && args);
}

} // namespace detail

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
        return chunks(_registry.get_archetypes()) | detail::views::join; // join all chunks together
    }

    /// @brief Returns an iterator that yields a std::tuple<Args...>
    ///
    /// @return decltype(auto) Iterator
    auto each() const -> decltype(auto)
        requires(is_const)
    {
        return chunks(_registry.get_archetypes()) | detail::views::join; // join all chunks together
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

    /// @brief Run func on every entity that matches the Args requirement in parallel
    ///
    /// @param func A callable to run on entity components
    void par_each(auto&& func)
        requires(!is_const)
    {
        co_ecs::parallel_for(chunks(),
            [&func](auto chunk) { std::ranges::for_each(chunk, [&](auto&& elem) { std::apply(func, elem); }); });
    }

    /// @brief Run func on every entity that matches the Args requirement in parallel. Constant version
    ///
    /// NOTE: See the note on non-const each()
    ///
    /// @param func A callable to run on entity components
    void par_each(auto&& func) const
        requires(is_const)
    {
        co_ecs::parallel_for(chunks(),
            [&func](auto chunk) { std::ranges::for_each(chunk, [&](auto&& elem) { std::apply(func, elem); }); });
    }

    auto chunks() -> decltype(auto) {
        return chunks(_registry.get_archetypes());
    }

    auto chunks() const -> decltype(auto) {
        return chunks(_registry.get_archetypes());
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
        auto filter_archetypes = [](auto& archetype) -> bool {
            return detail::all(archetype->template contains<decay_component_t<Args>>()...);
        };
        auto into_chunks = [](auto& archetype) -> decltype(auto) { return archetype->chunks(); };
        auto as_typed_chunk = [](auto& chunk) -> decltype(auto) { return chunk_view<Args...>(chunk); };

        return archetypes                                  // for each archetype entry in archetype map
               | detail::views::values                     // for each value, a pointer to archetype
               | detail::views::filter(filter_archetypes)  // filter archetype by requested components
               | detail::views::transform(into_chunks)     // fetch chunks vector
               | detail::views::join                       // join chunks together
               | detail::views::transform(as_typed_chunk); // each chunk casted to a typed chunk view range-like type
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

/// @brief System view state, pre-creates a view object needed for components iteration
///
/// @tparam view_t View type
template<typename view>
class system_view_state {
public:
    /// @brief Construct a new system view state object
    ///
    /// @param registry Registry reference
    /// @param user_context User provided context to fetch data from and provide to the system
    explicit system_view_state(registry& registry, void* user_context) noexcept : _view(registry) {
    }

    /// @brief Returns the actual state inside to pass to the system
    ///
    /// @return view_t&
    [[nodiscard]] view& get() noexcept {
        return _view;
    }

    /// @brief Run deferred logic
    void run_deferred() const noexcept {
    }

private:
    view _view;
};

/// @brief Specialization for ecs::view<Args...>
///
/// @tparam Args View argument types
template<component_reference... Args>
class system_argument_state_trait<view<Args...>> {
public:
    /// @brief Actual state type for T
    using state_type = system_view_state<view<Args...>>;
};

} // namespace co_ecs