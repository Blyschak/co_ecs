#pragma once

#include <cobalt/ecs/registry.hpp>

#include <sol/sol.hpp>

namespace cobalt::scripting {

/// @brief Lua scripting engine
class lua_engine {
public:
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
            "name",
            [name]() { return name; },
            "id",
            []() { return ecs::component_family::id<C>; },
            std::forward<decltype(args)>(args)...);
        lua["registry"][std::string("_set_") + name] =
            [&](ecs::registry& self, ecs::entity ent, const sol::table& table) {
                return self.set<C>(ent, table.as<C>());
            };
        lua["registry"][std::string("_get_") + name] = [&](ecs::registry& self, ecs::entity ent) {
            return self.get<C>(ent);
        };
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