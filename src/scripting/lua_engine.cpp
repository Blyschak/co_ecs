#include <cobalt/ecs/runtime_view.hpp>
#include <cobalt/scripting/lua_engine.hpp>

#include <sstream>

namespace cobalt {

namespace {} // namespace

class lua_runtime_view {
public:
    explicit lua_runtime_view(lua_engine& engine, ecs::registry& registry, const sol::variadic_args& components) :
        _engine(engine), _registry(registry), _view(view_from_lua_components(_registry, components)) {
        for (const sol::table& component : components) {
            _components.emplace_back(component);
        }
    }

    void each(auto&& func) {
        _view.each([&](ecs::entity ent) {
            std::vector<sol::table> arguments;
            for (const sol::table& component : _components) {
                auto get_func = _engine.get_component_callback(component, lua_callbacks::get);
                auto component_data = get_func(_registry, ent, component).get<sol::table>();
                arguments.emplace_back(component_data);
            }
            func(sol::as_args(arguments));
        });
    }

private:
    ecs::runtime_view view_from_lua_components(ecs::registry& registry, const sol::variadic_args& components) {
        auto rng = (components | std::views::transform([&](const sol::table& table) {
            auto id_func = _engine.get_component_callback(table, lua_callbacks::id);
            return id_func().get<ecs::component_id>();
        }));
        return ecs::runtime_view(registry, rng);
    }

    lua_engine& _engine;
    ecs::registry& _registry;
    ecs::runtime_view _view;
    std::vector<sol::table> _components;
};

lua_engine::lua_engine() {
    // open required libraries
    lua.open_libraries(sol::lib::base);

    // register required features

    // logging utilities

    lua.create_named_table(
        "log",
        "err",
        [&](const sol::table& log, const sol::variadic_args& args) { log_callback(log_level::err, args); },
        "warn",
        [&](const sol::table& log, const sol::variadic_args& args) { log_callback(log_level::warn, args); },
        "info",
        [&](const sol::table& log, const sol::variadic_args& args) { log_callback(log_level::info, args); },
        "debug",
        [&](const sol::table& log, const sol::variadic_args& args) { log_callback(log_level::debug, args); },
        "trace",
        [&](const sol::table& log, const sol::variadic_args& args) { log_callback(log_level::trace, args); });


    // ecs types

    lua.new_usertype<ecs::entity>(
        "entity", sol::constructors<ecs::entity()>(), "id", &ecs::entity::id, "generation", &ecs::entity::generation);

    lua.new_usertype<lua_runtime_view>(
        "view", sol::no_constructor, "each", [](lua_runtime_view& self, const sol::function& func) {
            return self.each(func);
        });

    lua.new_usertype<ecs::registry>(
        "registry",
        sol::constructors<ecs::registry()>(),
        "create",
        [&](ecs::registry& self, const sol::variadic_args& args) {
            auto ent = self.create<>();
            // TODO: make lua create method immediately detect the right archetype and write components to it, instead
            // of emulating C++ create behaviour
            for (const sol::table& table : args) {
                auto set_func = get_component_callback(table, lua_callbacks::set);
                set_func(self, ent, table);
            }
            return ent;
        },
        "destroy",
        &ecs::registry::destroy,
        "alive",
        &ecs::registry::alive,
        "set",
        [&](ecs::registry& self, ecs::entity ent, const sol::table& table) {
            auto set_func = get_component_callback(table, lua_callbacks::set);
            set_func(self, ent, table);
        },
        "has",
        [&](ecs::registry& self, ecs::entity ent, const sol::table& table) {
            auto has_func = get_component_callback(table, lua_callbacks::has);
            return has_func(self, ent, table).get<bool>();
        },
        "get",
        [&](ecs::registry& self, ecs::entity ent, const sol::table& table) {
            auto get_func = get_component_callback(table, lua_callbacks::get);
            return get_func(self, ent, table).get<sol::table>();
        },
        "view",
        [&](ecs::registry& self, const sol::variadic_args& args) { return lua_runtime_view(*this, self, args); });
}

sol::function lua_engine::get_component_callback(const sol::table& table, std::string callback_name) const {
    auto func = table[callback_name];
    if (!func.valid()) {
        throw lua_component_error(to_string(table));
    }
    return func;
}

void lua_engine::log_callback(log_level level, const sol::variadic_args& args) const {
    // format output as a space seprated output of args
    std::stringstream ss;
    for (const auto& arg : args) {
        ss << to_string(arg) << " ";
    }
    // log using core logger
    get_logger()->log(level, "{}", ss.str());
}

std::string lua_engine::to_string(const sol::object& obj) const {
    return lua["tostring"](obj).get<std::string>();
}

void lua_engine::script(const std::string& script) {
    try {
        lua.script(script);
    } catch (const sol::error& error) {
        throw lua_execution_error(error);
    }
}

} // namespace cobalt