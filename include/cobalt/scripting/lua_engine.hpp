#pragma once

#include <cobalt/ecs/registry.hpp>

#include <sol/sol.hpp>

namespace cobalt::scripting {

/// @brief Lua scripting engine
class lua_engine {
public:
    constexpr static auto lua_name_method = "__name";
    constexpr static auto lua_id_method = "__id";
    constexpr static auto lua_set_method = "__set";
    constexpr static auto lua_get_method = "__get";
    constexpr static auto lua_has_method = "__has";

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
        // allocate new ID for the component
        ecs::component_id id = ecs::component_family::id<C>;

        // Register component type with added lua methods
        lua.new_usertype<C>(
            name,
            sol::constructors<Ctors...>(),
            lua_name_method,
            [name]() { return name; },
            lua_id_method,
            [id]() { return id; },
            lua_set_method,
            [id](ecs::registry& registry, ecs::entity ent, const sol::table& table) {
                return registry.set<sol::table>(id, ent, std::move(table));
            },
            lua_get_method,
            [id](ecs::registry& registry, ecs::entity ent) { return registry.get<sol::table>(id, ent); },
            lua_has_method,
            [id](ecs::registry& registry, ecs::entity ent) { return registry.has(id, ent); },
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
};

} // namespace cobalt::scripting