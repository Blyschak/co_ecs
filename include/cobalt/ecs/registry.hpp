#pragma once

#include <cassert>
#include <ranges>

#include <cobalt/asl/hash_map.hpp>
#include <cobalt/asl/sparse_map.hpp>
#include <cobalt/asl/zip.hpp>
#include <cobalt/ecs/archetype.hpp>
#include <cobalt/ecs/entity.hpp>
#include <cobalt/ecs/entity_location.hpp>
#include <cobalt/ecs/view.hpp>

namespace cobalt::ecs {

class registry {
public:
    template<component... Args>
    entity create(Args&&... args) {
        auto entity = _entity_pool.create();
        auto components = component_meta_set::create<Args...>();
        auto& archetype = _archetypes[components.bitset()];
        if (!archetype) {
            archetype = std::make_unique<ecs::archetype>(components);
        }
        auto location = archetype->template allocate<Args...>(entity, std::forward<Args>(args)...);
        set_location(entity.id(), location);
        return entity;
    }

    void destroy(entity ent) {
        assert(alive(ent));
        auto location = get_location(ent.id());
        // returns the entity ID that has been moved to a new location
        auto moved_id = location.arch->deallocate(location).id();
        remove_location(ent.id());
        if (moved_id != entity::invalid_id) {
            set_location(moved_id, location);
        }
        _entity_pool.recycle(ent);
    }

    template<component C, typename... Args>
    void set(entity_id id, Args&&... args) {
        auto& location = get_location(id);
        auto*& archetype = location.arch;

        if (archetype->template contains<C>()) {
            archetype->template write<C>(location, std::forward<Args>(args)...);
        } else {
            auto components = archetype->key();
            components.insert<C>();
            auto& new_archetype = _archetypes[components.bitset()];
            if (!new_archetype) {
                new_archetype = std::make_unique<ecs::archetype>(components);
            }
            auto [new_location, moved] = archetype->move(location, *new_archetype);
            if (moved.id() != entity::invalid_id) {
                set_location(moved.id(), location);
            }

            new_archetype->template construct<C>(new_location, std::forward<Args>(args)...);

            archetype = new_archetype.get();
            set_location(id, new_location);
        }
    }

    template<component C>
    void remove(entity_id id) {
        auto& location = get_location(id);
        auto*& archetype = location.arch;

        if (!archetype->template contains<C>()) {
            return;
        }

        auto components = archetype->key();
        components.erase<C>();
        auto& new_archetype = _archetypes[components.bitset()];
        if (!new_archetype) {
            new_archetype = std::make_unique<ecs::archetype>(components);
        }
        auto [new_location, moved] = archetype->move(location, *new_archetype);
        if (moved.id() != entity::invalid_id) {
            set_location(moved.id(), location);
        }

        set_location(id, new_location);
    }

    bool alive(entity handle) const noexcept {
        return _entity_pool.alive(handle);
    }

    template<component C>
    C& get(entity_id id) {
        auto& location = get_location(id);
        return location.arch->template read<C>(location);
    }

    template<component C>
    const C& get(entity_id id) const {
        auto& location = get_location(id);
        return location.arch->template read<C>(location);
    }

    template<component C>
    bool has(entity_id id) const {
        auto& location = get_location(id);
        return location.arch->template contains<C>();
    }

    template<component_or_reference... Args>
    decltype(auto) view() {
        return ecs::view<Args...>(*this);
    }

    asl::hash_map<component_set, std::unique_ptr<archetype>, component_set_hasher>& archetypes() {
        return _archetypes;
    }

private:
    [[nodiscard]] const entity_location& get_location(entity_id id) const {
        return _entity_archetype_map.at(id);
    }

    [[nodiscard]] entity_location& get_location(entity_id id) {
        return _entity_archetype_map.at(id);
    }

    void set_location(entity_id id, const entity_location& location) {
        _entity_archetype_map[id] = location;
    }

    void remove_location(entity_id id) {
        _entity_archetype_map.erase(id);
    }

    entity_pool _entity_pool;
    asl::sparse_map<entity_id, entity_location> _entity_archetype_map;
    asl::hash_map<component_set, std::unique_ptr<archetype>, component_set_hasher> _archetypes;
};

template<component_or_reference... Args>
auto filter_archetypes(registry& registry) -> decltype(auto) {
    return registry.archetypes() | std::views::values
           | std::views::filter([set = component_set::create<decay_component_t<Args>...>()](auto& archetype) {
                 bool matched = archetype->match(set);
                 return matched;
             });
}

template<component_or_reference... Args>
auto each_impl(auto archetypes) -> decltype(auto) {
    if constexpr (sizeof...(Args) == 1) {
        return archetypes | std::views::transform([](auto& archetype) -> decltype(auto) { return archetype->chunks(); })
               | std::views::join
               | std::views::transform([](auto& c) -> decltype(auto) { return c.template as_container_of<Args...>(); })
               | std::views::join;
    } else if constexpr (sizeof...(Args) == 2) {
        return asl::zip_view(each_impl<std::tuple_element_t<0, std::tuple<Args...>>>(archetypes),
            each_impl<std::tuple_element_t<1, std::tuple<Args...>>>(archetypes));
    } else {
        return [archetypes]<component_or_reference T, component_or_reference U, component_or_reference... Other>() {
            return asl::zip_view(
                each_impl<T>(archetypes), each_impl<U>(archetypes), (..., each_impl<Other>(archetypes)));
        }
        .template operator()<Args...>();
    }
}

template<component_or_reference... Args>
auto view<Args...>::each() -> decltype(auto) {
    return each_impl<Args...>(filter_archetypes<Args...>(_registry));
}

} // namespace cobalt::ecs