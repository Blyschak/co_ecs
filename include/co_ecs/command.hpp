#pragma once

#include <co_ecs/entity_ref.hpp>
#include <co_ecs/registry.hpp>

#include <deque>
#include <mutex>
#include <variant>

namespace co_ecs {

/// @brief This class manages command buffers and facilitates command execution.
/// @details Command buffer manages a list of commands to be executed on the registry.
/// It has a thread local container for encoding incomming commands as well as a storage
/// used as a temporary staging buffer that holds associated data (like entity components)
class command_buffer {
public:
    /// @brief Retrieves the thread-local command buffer instance.
    /// @return Reference to the thread-local command buffer.
    static auto get() -> command_buffer& {
        static thread_local command_buffer buf;
        return buf;
    }

    /// @brief Flushes all commands in the command buffers to the given registry.
    /// @param registry Reference to the registry object to synchronize with.
    static void flush(registry& registry) {
        registry.sync();

        std::lock_guard lk{ _mutex };
        for (auto* command_buffer : _command_buffers) {
            command_buffer->play_commands(registry);
        }
    }

private:
    static inline std::mutex _mutex; ///< Mutex to synchronize access to the command buffers vector.
    static inline std::vector<command_buffer*>
        _command_buffers; ///< Vector containing all thread-local command buffers.

    friend class command_writer;
    friend class command_entity_ref;

    command_buffer() {
        std::lock_guard lk{ _mutex };
        _command_buffers.push_back(this);
    }

    template<typename T>
    void push(auto&&... args) {
        _commands.emplace_back(T{ std::forward<decltype(args)>(args)... });
    }

    auto staging() noexcept -> registry& {
        return _staging;
    }

    void play_commands(registry& registry) {
        while (!_commands.empty()) {
            auto command = std::move(_commands.front());
            _commands.pop_front();

            std::visit([&](auto&& cmd) { cmd.execute(_staging, registry); }, command);
        }
    }

    class command_create {
    public:
        command_create(entity ent, placeholder_entity reserved) : _staging_entity(ent), _reserved(reserved) {
        }

        void execute(registry& staging, registry& destination) {
            staging.get_entity(_staging_entity).move(destination, _reserved);
        }

    private:
        entity _staging_entity;
        placeholder_entity _reserved;
    };

    class command_clone {
    public:
        command_clone(entity ent, placeholder_entity reserved) : _entity(ent), _reserved(reserved) {
        }

        void execute(registry& staging, registry& destination) {
            destination.get_entity(_entity).clone(_reserved);
        }

    private:
        entity _entity;
        placeholder_entity _reserved;
    };

    class command_set {
    public:
        using set_fn_t = std::function<void(registry&, entity, registry&, entity)>;

        command_set(entity ent, entity dest, set_fn_t fn) :
            _staging_entity(ent), _destination_entity(dest), _set_fn(std::move(fn)) {
        }

        void execute(registry& staging, registry& destination) {
            _set_fn(staging, _staging_entity, destination, _destination_entity);
        }

    private:
        entity _staging_entity;
        entity _destination_entity;
        set_fn_t _set_fn;
    };

    class command_remove {
    public:
        using remove_fn_t = std::function<void(registry&, entity)>;

        command_remove(entity ent, remove_fn_t fn) : _entity(ent), _remove_fn(std::move(fn)) {
        }

        void execute(registry& staging, registry& destination) {
            _remove_fn(destination, _entity);
        }

    private:
        entity _entity;
        remove_fn_t _remove_fn;
    };

    class command_destroy {
    public:
        command_destroy(entity ent) : _entity(ent) {
        }

        void execute(registry& staging, registry& registry) {
            registry.get_entity(_entity).destroy();
        }

    private:
        entity _entity;
    };

    using command = std::variant< //
        command_create,
        command_clone,
        command_set,
        command_remove,
        command_destroy>;

private:
    registry _staging;             ///< Staging registry for intermediate command processing.
    std::deque<command> _commands; ///< Deque containing the commands to be executed.
};


/// @class command_entity_ref
/// @brief This class provides a reference to a command entity, allowing operations such as setting, removing
/// components, and cloning the entity.
class command_entity_ref {
public:
    /// @brief Sets a component of type C for the entity.
    /// @tparam C Component type to set.
    /// @tparam Args Argument types for the component constructor.
    /// @param args Arguments for the component constructor.
    /// @return A reference to the command_entity_ref for chaining.
    template<component C, typename... Args>
    auto set(Args&&... args) -> command_entity_ref& {
        auto staging_entity = _commands.staging().template create<C>(C{ std::forward<Args>(args)... });
        _commands.push<command_buffer::command_set>(staging_entity,
            _entity,
            [](auto& staging_registry, auto staging_entity, auto& dest_registry, auto dest_entity) {
                dest_registry.get_entity(dest_entity)
                    .template set<C>(std::move(staging_registry.get_entity(staging_entity).template get<C>()));
            });
        return *this;
    }

    /// @brief Removes a component of type C from the entity.
    /// @tparam C Component type to remove.
    /// @return A reference to the command_entity_ref for chaining.
    template<component C>
    auto remove() -> command_entity_ref& {
        _commands.push<command_buffer::command_remove>(
            _entity, [](auto& registry, auto entity) { registry.get_entity(entity).template remove<C>(); });
        return *this;
    }

    /// @brief Destroys the entity.
    void destroy() {
        _commands.push<command_buffer::command_destroy>(_entity);
    }

    /// @brief Clones the entity.
    /// @return A reference to the cloned command_entity_ref.
    auto clone() const -> command_entity_ref {
        auto entity = _registry.reserve();
        _commands.push<command_buffer::command_clone>(_entity, entity);
        return command_entity_ref{ _commands, _registry, entity.get_entity() };
    }

    /// @brief Conversion operator to entity.
    /// @return The underlying entity.
    [[nodiscard]] operator entity() const noexcept {
        return _entity;
    }

private:
    friend class command_writer;

    command_entity_ref(command_buffer& commands, registry& registry, entity entity) :
        _commands(commands), _registry(registry), _entity(entity) {
    }

private:
    command_buffer& _commands; ///< Reference to the command buffer object.
    registry& _registry;       ///< Reference to the registry object.
    entity _entity;            ///< The entity being referenced.
};

/// @brief This class is responsible for writing commands to a command_buffer.
///
/// @section Example
/// @code
/// registry reg;
/// command_writer cmd{reg};
///
/// auto e = cmd.create<position>({/* position data */});
/// e.set<velocity>({/* velocity data */});
///
/// command_buffer::flush(reg);
///
/// reg.get_entity(e).alive(); // true
/// reg.get_entity(e).has<position, velocity>(); // true
/// @endcode
///
class command_writer {
public:
    /// @brief Constructs a command_writer with a given registry.
    /// @param reg Reference to a registry object.
    command_writer(registry& reg) : _reg(reg), _cmds(command_buffer::get()) {
    }

    /// @brief Retrieves a reference to a command entity.
    /// @param ent The entity to retrieve.
    /// @return A reference to the command entity.
    auto get_entity(entity ent) -> command_entity_ref {
        return command_entity_ref{ _cmds, _reg, ent };
    }

    /// @brief Creates a new entity with the given components.
    /// @tparam Args Component types to be added to the entity.
    /// @param args Components to be added to the entity.
    /// @return A reference to the newly created command entity.
    template<component... Args>
    auto create(Args&&... args) -> command_entity_ref {
        auto entity = _reg.reserve();
        auto staging_entity = _cmds.staging().template create<Args...>(std::forward<Args>(args)...);
        _cmds.push<command_buffer::command_create>(staging_entity, entity);
        return command_entity_ref{ _cmds, _reg, entity.get_entity() };
    }

    /// @brief Destroys an existing entity.
    /// @param ent The entity to be destroyed.
    void destroy(entity ent) {
        _cmds.push<command_buffer::command_destroy>(ent);
    };

private:
    registry& _reg;        ///< Reference to the registry object.
    command_buffer& _cmds; ///< Reference to the command buffer object.
};


} // namespace co_ecs
