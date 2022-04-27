#include <cobalt/core/logging.hpp>
#include <cobalt/ecs/registry.hpp>
#include <cobalt/scripting/lua_engine.hpp>

#include <sstream>

namespace cobalt::scripting {

namespace {
    void lua_log(const sol::state& lua, core::log_level level, const sol::variadic_args& args) {
        // format output as a space seprated output of args
        std::stringstream ss;
        for (const auto& arg : args) {
            ss << lua["tostring"](arg).get<std::string>() << " ";
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

    lua.new_usertype<ecs::registry>("registry",
        sol::constructors<ecs::registry()>(),
        "create",
        &ecs::registry::create<>,
        "destroy",
        &ecs::registry::destroy,
        "alive",
        &ecs::registry::alive,
        "view",
        [](ecs::registry& self, const sol::variadic_args& args) {
            auto rng = (args | std::views::transform([](const sol::table& obj) {
                return obj["id"].get<sol::function>()().get<ecs::component_id>();
            }));
            return self.runtime_view(rng);
        });
}

void lua_engine::script(const std::string& script) {
    lua.script(script);
}

} // namespace cobalt::scripting