#pragma once

#include <atomic>
#include <cstdint>
#include <numeric>
#include <vector>

namespace co_ecs {

/// @brief Entity ID type, 32 bit value should be sufficient for all use cases
using entity_id_t = std::uint32_t;

/// @brief Generation ID type
using generation_id_t = std::uint32_t;

/// @brief Entity is an ID and generation
class entity {
public:
    using id_t = entity_id_t;
    using generation_t = generation_id_t;

    /// @brief Invalid ID number
    static constexpr auto invalid_id = std::numeric_limits<id_t>::max();

    /// @brief Invalid generation number
    static constexpr auto invalid_generation = std::numeric_limits<generation_t>::max();

    static const entity invalid;

    /// @brief Default construct a handle
    constexpr entity() = default;

    /// @brief Construct handle from id and generation
    ///
    /// @param id ID number
    /// @param generation Generation number
    /// @return Handle
    explicit constexpr entity(id_t id, generation_t generation = 0) // NOLINT(bugprone-easily-swappable-parameters)
        noexcept : _id(id), _generation(generation) {
    }

    /// @brief Test if handle is valid
    ///
    /// @return True if handle is valid
    [[nodiscard]] constexpr auto valid() const noexcept -> bool {
        return *this != entity::invalid;
    }

    /// @brief Return ID number
    ///
    /// @return ID number
    [[nodiscard]] constexpr auto id() const noexcept {
        return _id;
    }

    /// @brief Return generation number
    ///
    /// @return Generation number
    [[nodiscard]] constexpr auto generation() const noexcept {
        return _generation;
    }

    /// @brief Spaceship operator
    ///
    /// @param rhs Other handle
    /// @return Automatic
    constexpr auto operator<=>(const entity& rsh) const = default;

private:
    id_t _id{ invalid_id };
    generation_t _generation{ invalid_generation };
};

inline const entity entity::invalid{ entity::invalid_id, entity::invalid_generation };


/// @brief Pool of entities, generates entity ids, recycles ids.
class entity_pool {
public:
    /// @brief Create new handle
    ///
    /// @return Handle
    [[nodiscard]] auto create() -> entity {
        if (!_free_ids.empty()) {
            auto id = _free_ids.back();
            _free_ids.pop_back();
            return entity{ id, _generations[id] };
        }
        auto handle = entity{ _next_id.fetch_add(1, std::memory_order::relaxed) };
        _generations.emplace_back();
        return handle;
    };

    /// @brief Check if handle is still alive
    ///
    /// @param handle Handle to check
    /// @return True if handle is alive
    [[nodiscard]] auto alive(entity handle) const noexcept -> bool {
        if (handle.id() < _generations.size()) {
            return _generations[handle.id()] == handle.generation();
        }
        return false;
    }

    /// @brief Recycle the handle, handle will be reused in next create()
    ///
    /// @param handle Handle to recycle
    void recycle(entity handle) {
        if (!alive(handle)) {
            return;
        }
        _generations[handle.id()]++;
        _free_ids.push_back(handle.id());
    }

    /// @brief Reserve an entity handle
    /// This call is thread safe.
    ///
    /// @return Entity handle
    auto reserve() -> entity {
        auto handle = entity{ _next_id.fetch_add(1, std::memory_order::relaxed) };
        _reserved.fetch_add(1, std::memory_order::relaxed);

        // Synchronize memory consistency with the below flush().
        // Make sure both atomic changes are visible to another thread.
        std::atomic_thread_fence(std::memory_order::release);

        return handle;
    }

    /// @brief Flushes reserved entities.
    void flush() {
        // Synchronizes with the above reserve() function.
        std::atomic_thread_fence(std::memory_order::acquire);

        auto reserved = _reserved.load(std::memory_order::relaxed);
        while (reserved--) {
            _generations.emplace_back();
        }

        _reserved.store(0, std::memory_order::relaxed);
    }

private:
    typename std::atomic<entity::id_t> _next_id{};
    typename std::atomic<entity::id_t> _reserved{};
    std::vector<typename entity::generation_t> _generations;
    std::vector<typename entity::id_t> _free_ids;
};

} // namespace co_ecs
