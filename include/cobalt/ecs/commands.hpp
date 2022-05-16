#pragma once

#include <cobalt/ecs/registry.hpp>

#include <memory>
#include <vector>

namespace cobalt::ecs {

/// @brief Command abstract interface class
class command {
public:
    /// @brief Destroy the command object
    virtual ~command() {
    }

    /// @brief Execute command interface method
    ///
    /// @param registry Registry reference
    virtual void execute(registry& registry) = 0;
};

/// @brief Registry command
///
/// @tparam F Function type
template<typename F>
class registry_command : public command {
public:
    /// @brief Construct a new registry command object
    ///
    /// @param f Callable object
    registry_command(F&& f) : _f(std::move(f)) {
    }

    /// @brief Execute command, executes a callable object
    ///
    /// @param registry Registry reference
    void execute(registry& registry) override {
        _f(registry);
    }

private:
    F _f;
};

/// @brief Commands queue. Commands queue is used to cache commands that should be executed on the registry from
/// multiple threads from different systems without any locking. Everything in the command queue has to be flushed from
/// the main thread at the end of a single update.
class command_queue {
public:
    /// @brief Create entity in the registry
    ///
    /// @tparam Args Component types
    /// @param args Components
    template<component... Args>
    void create(Args&&... args) {
        add_command([... args = std::forward<decltype(args)>(args)](
                        registry& registry) mutable { registry.create<Args...>(std::forward<Args>(args)...); });
    }

    /// @brief Set component to the entity
    ///
    /// @tparam C Component
    /// @tparam Args Argument types
    /// @param ent Entity to set component to
    /// @param args Arguments to construct component from
    template<component C, typename... Args>
    void set(entity ent, Args&&... args) {
        add_command([ent, ... args = std::forward<decltype(args)>(args)](
                        registry& registry) mutable { registry.set<C>(ent, std::forward<Args>(args)...); });
    }

    /// @brief Remove component from the entity
    ///
    /// @tparam C Component type
    /// @param ent Entity to remove component from
    template<component C>
    void remove(entity ent) {
        add_command([ent](registry& registry) { registry.remove<C>(ent); });
    }

    /// @brief Destroy entity
    ///
    /// @param ent Entity to destroy
    void destroy(entity ent) {
        add_command([ent](registry& registry) { registry.destroy(ent); });
    }

    /// @brief Set the resource object
    ///
    /// @tparam R Resource type
    /// @tparam Args Argument types
    /// @param args Arguments to construct resource from
    template<resource R, typename... Args>
    void set_resource(Args&&... args) {
        add_command([... args = std::forward<decltype(args)>(args)](
                        registry& registry) mutable { registry.set_resource<R>(std::forward<Args>(args)...); });
    }

    /// @brief Remove resource from the registry
    ///
    /// @tparam R Resource type
    template<resource R>
    void remove_resource() {
        add_command([](registry& registry) { registry.remove_resource<R>(); });
    }

    /// @brief Flush commands from the queue and execute them on the registry
    ///
    /// @param registry Registry reference
    void execute(registry& registry) {
        while (!_commands.empty()) {
            auto& command = _commands.back();
            command->execute(registry);
            _commands.pop_back();
        }
    }

private:
    template<typename F>
    void add_command(F&& f) {
        _commands.emplace_back(std::make_unique<registry_command<F>>(std::move(f)));
    }

    std::vector<std::unique_ptr<command>> _commands;
};

} // namespace cobalt::ecs