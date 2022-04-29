#include <cobalt/core/logging.hpp>
#include <cobalt/ecs/registry.hpp>
#include <cobalt/scripting/lua_engine.hpp>

#include <sstream>

namespace cobalt::scripting {

namespace {
    std::string lua_to_string(const sol::state& lua, const sol::object& obj) {
        return lua["tostring"](obj).get<std::string>();
    }

    void lua_log(const sol::state& lua, core::log_level level, const sol::variadic_args& args) {
        // format output as a space seprated output of args
        std::stringstream ss;
        for (const auto& arg : args) {
            ss << lua_to_string(lua, arg) << " ";
        }
        // log using core logger
        core::get_logger()->log(level, "{}", ss.str());
    }
} // namespace

lua_engine::lua_engine() {
    // open required libraries
    lua.open_libraries(sol::lib::base);

    // register required features

    // logging utilities

    lua.create_named_table(
        "log",
        "err",
        [&](const sol::table& log, const sol::variadic_args& args) { lua_log(lua, core::log_level::err, args); },
        "warn",
        [&](const sol::table& log, const sol::variadic_args& args) { lua_log(lua, core::log_level::warn, args); },
        "info",
        [&](const sol::table& log, const sol::variadic_args& args) { lua_log(lua, core::log_level::info, args); },
        "debug",
        [&](const sol::table& log, const sol::variadic_args& args) { lua_log(lua, core::log_level::debug, args); },
        "trace",
        [&](const sol::table& log, const sol::variadic_args& args) { lua_log(lua, core::log_level::trace, args); });


    // ecs types

    lua.new_usertype<ecs::entity>(
        "entity", sol::constructors<ecs::entity()>(), "id", &ecs::entity::id, "generation", &ecs::entity::generation);

    lua.new_usertype<ecs::runtime_view>(
        "view", sol::no_constructor, "each", [](ecs::runtime_view& self, const sol::function& func) {
            return self.each(func);
        });

    lua.new_usertype<ecs::registry>(
        "registry",
        sol::constructors<ecs::registry()>(),
        "create",
        [&](ecs::registry& self, const sol::variadic_args& args) {
            auto ent = self.create<>();
            // TODO: make lua create method immidiatelly detect the right archetype and write components to it, instead
            // of emulating C++ create behaviour
            for (const sol::table& table : args) {
                auto set_func = table[lua_set_method];
                if (!set_func.valid()) {
                    core::log_err("Table {} is not a registered component", lua_to_string(lua, table));
                    return ent;
                }
                set_func(self, ent, table);
            }
            return ent;
        },
        "destroy",
        &ecs::registry::destroy,
        "alive",
        &ecs::registry::alive,
        "has",
        [&](ecs::registry& self, ecs::entity ent, const sol::table& table) {
            auto has_func = table[lua_has_method];
            if (!has_func.valid()) {
                core::log_err("Table {} is not a registered component", lua_to_string(lua, table));
                return false;
            }
            return has_func(self, table).get<bool>();
        },
        "set",
        [&](ecs::registry& self, ecs::entity ent, const sol::table& table) {
            auto set_func = table[lua_set_method];
            if (!set_func.valid()) {
                core::log_err("Table {} is not a registered component", lua_to_string(lua, table));
                return;
            }
            set_func(self, ent, table);
        },
        "get",
        [&](ecs::registry& self, ecs::entity ent, const sol::table& table) {
            auto get_func = table[lua_get_method];
            if (!get_func.valid()) {
                core::log_err("Table {} is not a registered component", lua_to_string(lua, table));
                return sol::table();
            }
            return get_func(self, ent, table).get<sol::table>();
        },
        "view",
        [&](ecs::registry& self, const sol::variadic_args& args) {
            auto rng = (args | std::views::transform([](const sol::table& table) {
                return table["id"].get<sol::function>()().get<ecs::component_id>();
            }));
            return self.runtime_view(rng);
        },
        "register_component",
        [&](ecs::registry& self, sol::table table) {
            auto meta = ecs::component_meta::external<sol::table>();
            auto id = meta.id;

            table[lua_id_method] = [id]() { return id; };
            table[lua_meta_method] = [meta]() { return &meta; };
            table[lua_set_method] = [meta](ecs::registry& r, ecs::entity e, const sol::table& table) {
                return r.set<sol::table>(&meta, e, table);
            };
            table[lua_get_method] = [meta](ecs::registry& r, ecs::entity ent) { return r.get<sol::table>(&meta, ent); };
            table[lua_has_method] = [meta](ecs::registry& r, ecs::entity ent) { return r.has(meta.id, ent); };
        });
}

void lua_engine::script(const std::string& script) {
    lua.script(script);
}

} // namespace cobalt::scripting