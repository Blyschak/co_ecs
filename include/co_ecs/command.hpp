#pragma once

#include <co_ecs/detail/allocator/stack_allocator.hpp>
#include <co_ecs/registry.hpp>

#include <vector>

namespace co_ecs {

/// @brief Command. TODO: Try to remove vtable usage
struct command {
    /// @brief Destructor
    virtual ~command() = default;

    /// @brief Execute command on registry
    /// @param registry Registry
    virtual void run(registry& registry) = 0;
};

/// @brief Create command
/// @tparam ...Args Components
template<component... Args>
struct command_create : command {
    std::tuple<Args...> args;

    /// @brief Construct command
    /// @param ...args Components
    command_create(Args&&... args) : args(std::forward<Args>(args)...) {
    }

    /// @brief Execute command on registry
    /// @param registry Registry
    void run(registry& registry) override {
        std::apply([&registry](auto&&... args) { registry.create<Args...>(std::forward<Args>(args)...); }, args);
    }
};

/// @brief Command to add compoentn
/// @tparam C Component
template<component C>
struct command_set : command {
    entity ent;
    C c;

    /// @brief Construct command
    /// @param ent Entity
    /// @param c Component
    command_set(entity ent, C c) : ent(ent), c(std::move(c)) {
    }

    /// @brief Execute command on registry
    /// @param registry Registry
    void run(registry& registry) override {
        registry.set<C>(ent, std::move(c));
    }
};

/// @brief Command to remove component
/// @tparam C Component
template<component C>
struct command_remove : command {
    entity ent;

    /// @brief Construct command
    /// @param ent Entity
    command_remove(entity ent) : ent(ent) {
    }

    /// @brief Execute command on registry
    /// @param registry Registry
    void run(registry& registry) override {
        registry.remove<C>(ent);
    }
};

/// @brief Command to destroy entity
struct command_destroy : command {
    entity ent;

    /// @brief Construct command
    /// @param ent Entity
    command_destroy(entity ent) : ent(ent) {
    }

    /// @brief Execute command on registry
    /// @param registry Registry
    void run(registry& registry) override {
        registry.destroy(ent);
    }
};

/// @brief Command buffer. Record commands on registry and execute then later.
class command_buffer {
public:
    /// @brief Command buffer constructor
    command_buffer() {
        _commands.reserve(1024);
    }

    /// @brief Create entity
    /// @tparam ...Args Component types
    /// @param ...args Components
    template<component... Args>
    void create(Args&&... args) {
        command* ptr = (command*)_salloc.allocate(sizeof(command_create<Args...>), alignof(command_create<Args...>));
        new (ptr) command_create<Args...>(std::forward<Args>(args)...);
        _commands.push_back(ptr);
    }

    /// @brief Set component to an entity
    /// @tparam ...Args Arguments
    /// @tparam C Component types
    /// @param ent Entity
    /// @param ...args Components
    template<component C, typename... Args>
    void set(entity ent, Args&&... args) {
        command* ptr = (command*)_salloc.allocate(sizeof(command_set<C>), alignof(command_set<C>));
        new (ptr) command_set<C>(ent, C{ std::forward<Args>(args)... });
        _commands.push_back(ptr);
    }

    /// @brief Remove component
    /// @tparam C Component type
    /// @param ent Entity
    template<component C>
    void remove(entity ent) {
        command* ptr = (command*)_salloc.allocate(sizeof(command_remove<C>), alignof(command_remove<C>));
        new (ptr) command_remove<C>(ent);
        _commands.push_back(ptr);
    }

    /// @brief Destroy entity
    /// @param ent Entity
    void destroy(entity ent) {
        command* ptr = (command*)_salloc.allocate(sizeof(command_destroy), alignof(command_destroy));
        new (ptr) command_destroy(ent);
        _commands.push_back(ptr);
    }

    /// @brief Flush commands to registry
    /// @param registry Registry
    void flush(registry& registry) {
        while (!_commands.empty()) {
            auto& command = _commands.back();
            command->run(registry);
            command->~command();
            _commands.pop_back();
        }

        _salloc.free_all();
    }

private:
    detail::stack_allocator _salloc{ 16ull * 1024 * 1024 }; // TODO: Could use linear allocator as well
    std::vector<command*> _commands;
};

} // namespace co_ecs
