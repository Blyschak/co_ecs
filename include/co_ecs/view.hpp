#pragma once

#include <co_ecs/detail/views.hpp>
#include <co_ecs/registry.hpp>
#include <co_ecs/thread_pool/parallel_for.hpp>

#include <type_traits>

namespace co_ecs {

/// @brief A view lets you get a range over components of Args out of a registry.
///
/// Views represent a slice of a registry restricting the access to a particular components.
///
/// @code
/// co_ecs::view<position&, const velocity&> view{ registry };
///
/// // Iterate using iterators:
/// for (auto [pos, vel] : view.each()) {
///     pos.x += vel.x;
///     pos.y += vel.y;
/// }
///
/// // Alternativelly, iterate using a callback
/// view.each([](auto& pos, const auto& vel) {
///     pos.x += vel.x;
///     pos.y += vel.y;
/// });
/// @endcode
///
/// @note A view isn't invalidated when there are changes made to the registry, which allows creating one and re-using
/// it over time.
///
/// @tparam Args Component reference types
template<component_reference... Args>
class view {
private:
    /// @brief Indicates if the view is const when all component references are const.
    static constexpr bool is_const = detail::view_arguments<Args...>::is_const;

    /// @brief The type of values iterated over by the view.
    using value_type = std::tuple<Args...>;

    /// @brief The type of the registry, deduced based on the input component reference types.
    using registry_type = std::conditional_t<is_const, const registry&, registry&>;

public:
    /// @brief Constructs a new view object.
    /// @param registry Reference to the registry.
    explicit view(registry_type registry) noexcept : _registry(registry) {
    }

    /// @brief Returns a single tuple of components matching Args, if available in the view.
    ///
    /// This method is available in const views and allows accessing a single tuple of components matching Args.
    /// It returns an optional tuple, which is empty if no entities in the view match the component requirements.
    /// @code
    /// co_ecs::view<directional_light&> view{ registry };
    /// // Only one instance with directional_light component is supported
    /// auto [dir_light] = *view.single();
    /// @endcode
    ///
    /// @return Optional tuple of components if found, otherwise empty optional.
    auto single() -> std::optional<std::tuple<Args...>>
        requires(!is_const)
    {
        for (auto chunk : chunks(_registry.archetypes())) {
            for (auto entry : chunk) {
                return entry;
            }
        }
        return {};
    }

    /// @brief Returns a single tuple of components matching Args, if available in the view (const version).
    /// @return Optional tuple of components if found, otherwise empty optional.
    auto single() const -> std::optional<std::tuple<Args...>>
        requires(is_const)
    {
        for (auto chunk : chunks(_registry.archetypes())) {
            for (auto entry : chunk) {
                return entry;
            }
        }
        return {};
    }

    /// @brief Returns an iterator that yields a std::tuple<Args...>.
    /// @return decltype(auto) Iterator.
    auto each() -> decltype(auto)
        requires(!is_const)
    {
        return chunks(_registry.archetypes()) | detail::views::join; // join all chunks together
    }

    /// @brief Returns an iterator that yields a std::tuple<Args...> (const version).
    /// @return decltype(auto) Iterator.
    auto each() const -> decltype(auto)
        requires(is_const)
    {
        return chunks(_registry.archetypes()) | detail::views::join; // join all chunks together
    }

    /// @brief Runs a function on every entity that matches the Args requirement.
    /// @param func A callable to run on entity components.
    void each(auto&& func)
        requires(!is_const)
    {
        for (auto chunk : chunks(_registry.archetypes())) {
            for (auto entry : chunk) {
                std::apply(func, entry);
            }
        }
    }

    /// @brief Runs a function on every entity that matches the Args requirement (const version).
    ///
    /// @param func A callable to run on entity components.
    /// @note This method is similar to the non-const each() but is available in const views.
    void each(auto&& func) const
        requires(is_const)
    {
        for (auto chunk : chunks(_registry.archetypes())) {
            for (auto entry : chunk) {
                std::apply(func, entry);
            }
        }
    }

    /// @brief Runs a function on every entity that matches the Args requirement in parallel.
    /// @param func A callable to run on entity components.
    void par_each(auto&& func)
        requires(!is_const)
    {
        co_ecs::parallel_for(chunks(),
            [&func](auto chunk) { std::ranges::for_each(chunk, [&](auto&& elem) { std::apply(func, elem); }); });
    }

    /// @brief Runs a function on every entity that matches the Args requirement in parallel (const version).
    ///
    /// @param func A callable to run on entity components.
    /// @note This method is similar to the non-const par_each() but is available in const views.
    void par_each(auto&& func) const
        requires(is_const)
    {
        co_ecs::parallel_for(chunks(),
            [&func](auto chunk) { std::ranges::for_each(chunk, [&](auto&& elem) { std::apply(func, elem); }); });
    }

    /// @brief Gets the chunks range.
    /// @return Chunks.
    auto chunks() -> decltype(auto) {
        return chunks(_registry.archetypes());
    }

    /// @brief Gets the const chunks range.
    /// @return Chunks.
    auto chunks() const -> decltype(auto) {
        return chunks(_registry.archetypes());
    }

private:
    template<component C>
    constexpr static bool match(auto& archetype) {
        if constexpr (std::is_same_v<C, entity>) {
            return true;
        } else {
            return archetype->template contains<C>();
        }
    }

    constexpr static auto chunks(auto&& archetypes) -> decltype(auto) {
        auto filter_archetypes = [](auto& archetype) -> bool {
            return (match<decay_component_t<Args>>(archetype) && ...);
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

    registry_type _registry; ///< Reference to the registry.
};


} // namespace co_ecs