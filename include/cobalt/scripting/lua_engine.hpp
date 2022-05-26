#pragma once

#include <cobalt/core/logging.hpp>
#include <cobalt/ecs/registry.hpp>

#include <sol/sol.hpp>

namespace cobalt::scripting {

/// @brief Lua table callbacks method names
class lua_callbacks {
public:
    constexpr static auto id = "__id";
    constexpr static auto set = "__set";
    constexpr static auto get = "__get";
    constexpr static auto has = "__has";
};


/// @brief Exception thrown when lua script execution fails
class lua_execution_error : public std::exception {
public:
    /// @brief Construct a new lua execution error object
    ///
    /// @param error sol::error object
    explicit lua_execution_error(const sol::error& error) : _error(error) {
    }

    /// @brief Description of an error
    ///
    /// @return const char*
    const char* what() const noexcept override {
        return _error.what();
    }

private:
    sol::error _error;
};

/// @brief Exception throw when trying to use un-registered table as a component
class lua_component_error : public std::exception {
public:
    /// @brief Construct a new lua component error object
    ///
    /// @param name Component name
    explicit lua_component_error(std::string name) {
        std::stringstream ss;
        ss << "Table " << name << " is not a registered component";
        _msg = ss.str();
    }

    /// @brief Description of an error
    ///
    /// @return const char*
    const char* what() const noexcept override {
        return _msg.c_str();
    }

private:
    std::string _msg;
};

/// @brief Lua scripting engine
class lua_engine {
public:
    /// @brief Construct a new lua engine object
    lua_engine();

    /// @brief Execute script passed in this function
    ///
    /// @param script Lua script code
    void script(const std::string& script);

    /// @brief Exposes component to the scripting world
    ///
    /// @tparam C Component type
    /// @tparam Ctors Constructor types
    /// @param name Component name
    /// @param args Args to pass to new_usertype<C>
    template<ecs::component C, typename... Ctors>
    void expose_component(std::string name, auto&&... args) {
        // allocate new ID for the component
        ecs::component_id id = ecs::component_family::id<C>;

        // lua callbacks
        auto id_callback = [id]() { return id; };
        auto set_callback = [](ecs::registry& r, ecs::entity e, const sol::table& t) { r.set<C>(e, t.as<C>()); };
        auto get_callback = [](ecs::registry& r, ecs::entity e) { return r.get<C>(e); };
        auto has_callback = [](ecs::registry& r, ecs::entity e) { return r.has<C>(e); };

        // Register component type with added lua methods
        lua.new_usertype<C>(name,
            sol::constructors<Ctors...>(),
            lua_callbacks::id,
            id_callback,
            lua_callbacks::set,
            set_callback,
            lua_callbacks::get,
            get_callback,
            lua_callbacks::has,
            has_callback,
            std::forward<decltype(args)>(args)...);
    }

    /// @brief Get the component callback object
    ///
    /// @param table Lua Table
    /// @param callback_name Callback name
    /// @return sol::function
    [[nodiscard]] sol::function get_component_callback(const sol::table& table, std::string callback_name) const;

private:
    [[nodiscard]] std::string to_string(const sol::object& obj) const;
    void log_callback(core::log_level level, const sol::variadic_args& args) const;

    sol::state lua;
};

} // namespace cobalt::scripting