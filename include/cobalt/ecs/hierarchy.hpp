#pragma once

#include <cstdint>

#include <cobalt/asl/type_traits.hpp>
#include <cobalt/ecs/entity.hpp>
#include <cobalt/ecs/registry.hpp>

namespace cobalt::ecs {

using depth_t = std::uint32_t;

/// @brief Parent component
struct parent_component {
    ecs::entity entity{};

    /// @brief Construct a new parent component object
    ///
    /// @param entity Entity
    /// @param depth Hierarchy depth
    parent_component(ecs::entity entity, depth_t depth) : entity(entity), depth(depth) {
    }

private:
    friend class hierarchy;
    depth_t depth{};
};

namespace detail {
    template<typename T>
    struct extract_arguments_tuple_type;

    template<typename... Args>
    struct extract_arguments_tuple_type<std::tuple<Args...>> {
        static view<Args...> get_view(ecs::registry& registry) {
            return registry.view<Args...>();
        }
    };

    template<typename T>
    struct extract_arguments_view_type;

    template<typename... Args>
    struct extract_arguments_view_type<std::tuple<Args...>> {
        static view<const parent_component&, Args...> get_view(ecs::registry& registry) {
            return registry.view<const parent_component&, Args...>();
        }
    };

    template<typename T, std::size_t... Is>
    static auto pop_front_impl(const T& tuple, std::index_sequence<Is...>) {
        return std::make_tuple(std::ref(std::get<1 + Is>(tuple))...);
    }

    template<typename T>
    static auto pop_front(const T& tuple) {
        return pop_front_impl(tuple, std::make_index_sequence<std::tuple_size<T>::value - 1>());
    }
} // namespace detail

/// @brief Hierarchy helper with static methods to change parent-child relationship and iterate over child entities
class hierarchy {
public:
    /// @brief Iterate over childs recursivelly in depth first order starting from parent_entity
    ///
    /// @tparam F Function type to apply to the found child
    /// @param registry Registry reference
    /// @param parent_entity Parent entity
    /// @param func Function to apply to the found child
    template<typename F>
    static void for_each_child(ecs::registry& registry, ecs::entity parent_entity, F&& func) {
        using arguments_tuple_type = typename asl::function_traits<F>::arguments_tuple_type;
        using parent_arguments_tuple_type = typename std::tuple_element_t<0, arguments_tuple_type>;
        using child_arguments_view_type = typename std::tuple_element_t<1, arguments_tuple_type>;

        depth_t current_depth{};
        depth_t max_depth{};
        do {
            auto child_view = detail::extract_arguments_view_type<child_arguments_view_type>::get_view(registry);
            for (auto value : child_view.each()) {
                const auto& parent = std::get<0>(value);
                if (parent.depth > max_depth) {
                    max_depth = parent.depth;
                }
                if (parent.depth != current_depth) {
                    continue;
                }
                auto parent_view =
                    detail::extract_arguments_tuple_type<parent_arguments_tuple_type>::get_view(registry);
                if (parent.entity == parent_entity) {
                    func(parent_view.get(parent.entity), detail::pop_front(value));
                }
            }
            current_depth++;
        } while (current_depth <= max_depth);
    }

    /// @brief Iterate over childs recursivelly in depth first order
    ///
    /// @tparam F Function type to apply to the found child
    /// @param registry Registry reference
    /// @param func Function to apply to the found child
    template<typename F>
    static void for_each_child(ecs::registry& registry, F&& func) {
        using arguments_tuple_type = typename asl::function_traits<F>::arguments_tuple_type;
        using parent_arguments_tuple_type = typename std::tuple_element_t<0, arguments_tuple_type>;
        using child_arguments_view_type = typename std::tuple_element_t<1, arguments_tuple_type>;

        depth_t current_depth{};
        depth_t max_depth{};
        do {
            auto child_view = detail::extract_arguments_view_type<child_arguments_view_type>::get_view(registry);
            for (auto value : child_view.each()) {
                const auto& parent = std::get<0>(value);
                if (parent.depth > max_depth) {
                    max_depth = parent.depth;
                }
                if (parent.depth != current_depth) {
                    continue;
                }
                auto parent_view =
                    detail::extract_arguments_tuple_type<parent_arguments_tuple_type>::get_view(registry);
                func(parent_view.get(parent.entity), detail::pop_front(value));
            }
            current_depth++;
        } while (current_depth <= max_depth);
    }

    /// @brief Set the child to the parent entity
    ///
    /// @param registry Registry reference
    /// @param parent_entity Parent entity
    /// @param child_entity Child entity
    static void set_child(ecs::registry& registry, ecs::entity parent_entity, ecs::entity child_entity) {
        depth_t depth = 0;
        if (registry.has<parent_component>(parent_entity)) {
            depth = registry.get<parent_component>(parent_entity).depth;
        }
        if (!registry.has<parent_component>(child_entity)) {
            depth++;
        }
        registry.set<parent_component>(child_entity, parent_component(parent_entity, depth));
    }

    /// @brief Remove child from the parent entity
    ///
    /// @param registry Registry reference
    /// @param parent_entity Parent entity
    /// @param child_entity Child entity
    static void remove_child(ecs::registry& registry, ecs::entity parent_entity, ecs::entity child_entity) {
        registry.remove<parent_component>(child_entity);
        for_each_child(registry,
            parent_entity,
            [parent_entity](std::tuple<ecs::entity&> parent, std::tuple<parent_component&> child) {
                auto [child_parent_component] = child;
                child_parent_component.depth--;
            });
    }
};
} // namespace cobalt::ecs