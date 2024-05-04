#pragma once

#include <co_ecs/detail/allocator/linear_allocator.hpp>
#include <co_ecs/registry.hpp>

#include <iostream>
#include <vector>

namespace co_ecs {

/// @brief Command. TODO: Try to remove vtable usage
struct command {
    /// @brief Pointer to the next command in command buffer
    command* next_command{ nullptr };

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
    entity ent;
    std::tuple<Args...> args;

    /// @brief Construct command
    /// @param entity Entity handle
    /// @param ...args Components
    command_create(entity ent, Args&&... args) : ent(ent), args(std::forward<Args>(args)...) {
    }

    /// @brief Execute command on registry
    /// @param registry Registry
    void run(registry& registry) override {
        std::apply([&registry, this](
                       auto&&... args) { registry.create_with_entity<Args...>(ent, std::forward<Args>(args)...); },
            args);
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
private:
    class command_buffer_chunk {
    public:
        static const std::size_t chunk_size = 16ull * 1024 * 1024;

        auto allocate(std::size_t size, std::size_t alignment) -> command* {
            return static_cast<command*>(_arena.allocate(size, alignment));
        }

        void reset() {
            _arena.reset();
        }

    private:
        std::unique_ptr<uint8_t[]> _buffer{ new uint8_t[chunk_size] };
        detail::linear_allocator _arena{ _buffer.get(), chunk_size };
    };

public:
    /// @brief Command buffer constructor
    command_buffer() {
        _chunks.resize(1);
    }

    /// @brief Create entity
    /// @tparam ...Args Component types
    /// @param reg Registry
    /// @param ...args Components
    /// @return Entity handle
    template<component... Args>
    auto create(const registry& reg, Args&&... args) -> entity {
        auto ent = reg.reserve();
        push_command<command_create<Args...>>(ent, std::forward<Args>(args)...);
        return ent;
    }

    /// @brief Set component to an entity
    /// @tparam ...Args Arguments
    /// @tparam C Component types
    /// @param ent Entity
    /// @param ...args Components
    template<component C, typename... Args>
    void set(entity ent, Args&&... args) {
        push_command<command_set<C>>(ent, std::forward<Args>(args)...);
    }

    /// @brief Remove component
    /// @tparam C Component type
    /// @param ent Entity
    template<component C>
    void remove(entity ent) {
        push_command<command_remove<C>>(ent);
    }

    /// @brief Destroy entity
    /// @param ent Entity
    void destroy(entity ent) {
        push_command<command_destroy>(ent);
    }

    /// @brief Flush commands to registry
    /// @param registry Registry
    void flush(registry& registry) {
        registry.flush_reserved();
        flush_commands(registry);
    }

    /// @brief Flush commands to registry, reserved entities are not flushed
    /// @param registry Registry
    void flush_commands(registry& registry) {
        // Iterate the linked list of commands
        auto* iter = _base;

        while (iter) {
            iter->run(registry);
            auto* next = iter->next_command;
            iter->~command();
            iter = next;
        }

        _base = _top = nullptr;

        for (auto& chunk : _chunks) {
            chunk.reset();
            _current_chunk = 0;
        }
    }


private:
    command* allocate_command(std::size_t size, std::size_t alignment) {
        auto ptr = _chunks[_current_chunk].allocate(size, alignment);
        if (!ptr) {
            _current_chunk++;
            if (_current_chunk == _chunks.size()) {
                _chunks.emplace_back();
            }
            ptr = _chunks[_current_chunk].allocate(size, alignment);
        }
        return ptr;
    }

    template<typename C, typename... Args>
    void push_command(Args&&... args) {
        command* ptr = allocate_command(sizeof(C), alignof(C));
        new (ptr) C{ std::forward<Args>(args)... };
        push_command(ptr);
    }

    void push_command(command* cmd) {
        if (_top) {
            _top->next_command = cmd;
        } else {
            _base = cmd;
        }
        _top = cmd;
    }

private:
    std::vector<command_buffer_chunk> _chunks;
    command* _top{};
    command* _base{};
    std::size_t _current_chunk{};
};

/// @brief Command writer used by systems to record commands
class command_writer {
public:
    /// @brief Constructor
    /// @param reg Registry
    /// @param cmds Command buffer
    command_writer(const registry& reg, command_buffer& cmds) : _cmds(cmds), _reg(reg) {
    }

    /// @brief Create entity
    /// @tparam ...Args Component types
    /// @param ...args Components
    /// @return Entity handle
    template<component... Args>
    auto create(Args&&... args) -> entity {
        return _cmds.create(_reg, std::forward<Args>(args)...);
    }

    /// @brief Set component to an entity
    /// @tparam ...Args Arguments
    /// @tparam C Component types
    /// @param ent Entity
    /// @param ...args Components
    template<component C, typename... Args>
    void set(entity ent, Args&&... args) {
        _cmds.set(ent, std::forward<Args>(args)...);
    }

    /// @brief Remove component
    /// @tparam C Component type
    /// @param ent Entity
    template<component C>
    void remove(entity ent) {
        _cmds.remove<C>(ent);
    }

    /// @brief Destroy entity
    /// @param ent Entity
    void destroy(entity ent) {
        _cmds.destroy(ent);
    };

private:
    const registry& _reg;
    command_buffer& _cmds;
};

} // namespace co_ecs
