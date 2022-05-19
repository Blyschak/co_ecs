#pragma once

#include <cobalt/ecs/registry.hpp>

#include <type_traits>

namespace cobalt::ecs {

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
    decltype(auto) each() requires(!is_const) {
        return _registry.template each<Args...>();
    }

    /// @brief Returns an iterator that yields a std::tuple<Args...>
    ///
    /// @return decltype(auto) Iterator
    decltype(auto) each() const requires(is_const) {
        return _registry.template each<Args...>();
    }

    /// @brief Run func on every entity that matches the Args requirement
    ///
    /// NOTE: This kind of iteration might be faster and better optimized by the compiler since the func can operate on
    /// a chunk that yields two tuples of pointers to the actual data whereas an each() variant returns an iterator over
    /// iterator over iterator to the actual data which is a challenge for compiler to optimize. Look at the benchmarks
    /// to see the actual difference.
    ///
    /// @param func A callable to run on entity components
    void each(auto&& func) requires(!is_const) {
        _registry.template each<Args...>(std::forward<decltype(func)>(func));
    }

    /// @brief Run func on every entity that matches the Args requirement. Constant version
    ///
    /// NOTE: This kind of iteration might be faster and better optimized by the compiler since the func can operate on
    /// a chunk that yields two tuples of pointers to the actual data whereas an each() variant returns an iterator over
    /// iterator over iterator to the actual data which is a challenge for compiler to optimize. Look at the benchmarks
    /// to see the actual difference.
    ///
    /// @param func A callable to run on entity components
    void each(auto&& func) const requires(is_const) {
        _registry.template each<Args...>(std::forward<decltype(func)>(func));
    }

    /// @brief Get components for a single entity
    ///
    /// @param ent Entity to query
    /// @return value_type Components tuple
    value_type get(entity ent) requires(!is_const) {
        return _registry.template get<Args...>(ent);
    }

    /// @brief Get components for a single entity
    ///
    /// @param ent Entity to query
    /// @return value_type Components tuple
    value_type get(entity ent) const requires(is_const) {
        return _registry.template get<Args...>(ent);
    }

private:
    registry_type _registry;
};

} // namespace cobalt::ecs