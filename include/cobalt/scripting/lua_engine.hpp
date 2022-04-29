#pragma once

#include <cobalt/asl/hash_map.hpp>
#include <cobalt/ecs/registry.hpp>
#include <cobalt/scripting/script_component.hpp>

#include <sol/sol.hpp>

namespace cobalt::scripting {

/// @brief Lua scripting engine
class lua_engine {
public:
    constexpr static auto lua_name_method = "name";
    constexpr static auto lua_id_method = "id";
    constexpr static auto lua_meta_method = "meta";
    constexpr static auto lua_set_method = "__set";
    constexpr static auto lua_get_method = "__get";
    constexpr static auto lua_has_method = "__has";

    struct lua_external_component {
        const ecs::component_meta* _meta{};
        sol::table table{};

        const ecs::component_meta* meta() const {
            return _meta;
        }
    };

    /// @brief Construct a new lua engine object
    lua_engine();

    /// @brief Execute script passed in this function
    ///
    /// @param script Lua script code
    void script(const std::string& script);

    /// @brief Register component
    ///
    /// @tparam C Component type
    /// @tparam Ctors Constructor types
    /// @param name Component name
    /// @param args Args to pass to new_usertype<C>
    template<ecs::component C, typename... Ctors>
    void register_component(std::string name, auto&&... args) {
        lua.new_usertype<C>(
            name,
            sol::constructors<Ctors...>(),
            lua_name_method,
            [name]() { return name; },
            lua_id_method,
            []() { return ecs::component_traits<C>::id(); },
            lua_meta_method,
            []() { return ecs::component_traits<C>::meta(); },
            lua_set_method,
            [](ecs::registry& registry, ecs::entity ent, const sol::table& table) {
                return registry.set<C>(ent, table.as<C>());
            },
            lua_get_method,
            [](ecs::registry& registry, ecs::entity ent) { return registry.get<C>(ent); },
            lua_has_method,
            [](ecs::registry& registry, ecs::entity ent) { return registry.has<C>(ent); },
            std::forward<decltype(args)>(args)...);
    }

    /// @brief Get the state object
    ///
    /// @return sol::state&
    [[nodiscard]] sol::state& state() noexcept {
        return lua;
    }

    /// @brief Get the state object
    ///
    /// @return const sol::state&
    [[nodiscard]] const sol::state& state() const noexcept {
        return lua;
    }

private:
    sol::state lua;
    asl::hash_map<std::string, ecs::component_meta> lua_components;
};

} // namespace cobalt::scripting