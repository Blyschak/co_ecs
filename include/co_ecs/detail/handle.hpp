#pragma once

#include <atomic>
#include <cstdint>
#include <numeric>
#include <vector>

namespace co_ecs::detail {

/// @brief Represents a handle with an ID and a generation index.
/// @tparam Tag The type used to distinguish between handles.
template<typename Tag>
class handle {
public:
    using id_t = std::uint32_t;
    using generation_t = std::uint32_t;

    /// @brief Invalid ID number for handles.
    static constexpr auto invalid_id = std::numeric_limits<id_t>::max();

    /// @brief Invalid generation number for handles.
    static constexpr auto invalid_generation = std::numeric_limits<generation_t>::max();

    /// @brief Returns an invalid handle.
    /// @return Invalid handle
    [[nodiscard]] static constexpr handle invalid() noexcept {
        return handle{ invalid_id, invalid_generation };
    }

    /// @brief Default constructor for handle.
    constexpr handle() = default;

    /// @brief Constructs a handle from an ID and a generation.
    /// @param id ID number
    /// @param generation Generation number
    explicit constexpr handle(id_t id, generation_t generation = 0) noexcept : _id(id), _generation(generation) {
    }

    /// @brief Checks if the handle is valid.
    /// @return True if the handle is valid
    [[nodiscard]] constexpr auto valid() const noexcept -> bool {
        return *this != handle::invalid();
    }

    /// @brief Gets the ID number of the handle.
    /// @return ID number
    [[nodiscard]] constexpr auto id() const noexcept {
        return _id;
    }

    /// @brief Gets the generation number of the handle.
    /// @return Generation number
    [[nodiscard]] constexpr auto generation() const noexcept {
        return _generation;
    }

    /// @brief Spaceship operator for handles.
    /// @param rhs Other handle
    /// @return Comparison result
    [[nodiscard]] constexpr auto operator<=>(const handle& rhs) const = default;

private:
    id_t _id{ invalid_id };
    generation_t _generation{ invalid_generation };
};

/// @brief Pool of handles that generates and recycles handle IDs.
/// @tparam H Handle type
template<typename H>
class handle_pool {
public:
    /// @brief Creates a new handle.
    /// @return Handle
    [[nodiscard]] constexpr auto create() -> H {
        if (!_free_ids.empty()) {
            auto id = _free_ids.back();
            _free_ids.pop_back();
            _free_cursor.store(_free_ids.size(), std::memory_order::relaxed);
            return H{ id, _generations[id] };
        }
        auto handle = H{ _next_id.fetch_add(1, std::memory_order::relaxed) };
        _generations.emplace_back();
        return handle;
    };

    /// @brief Checks if a handle is still alive.
    /// @param handle Handle to check
    /// @return True if the handle is alive
    [[nodiscard]] constexpr auto alive(H handle) const noexcept -> bool {
        if (handle.id() < _generations.size()) {
            return _generations[handle.id()] == handle.generation();
        }
        return false;
    }

    /// @brief Recycles a handle for reuse in future creations.
    /// @param handle Handle to recycle
    constexpr void recycle(H handle) {
        if (!alive(handle)) {
            return;
        }
        _generations[handle.id()]++;
        _free_ids.push_back(handle.id());
        _free_cursor.fetch_add(1, std::memory_order::relaxed);
    }

    /// @brief Reserves a handle.
    /// @details This call is thread-safe.
    /// @return Reserved handle
    constexpr auto reserve() -> H {
        auto n = _free_cursor.fetch_sub(1, std::memory_order::relaxed);
        if (n > 0) {
            auto id = _free_ids[n - 1];
            return H{ id, _generations[id] };
        }
        auto handle = H{ _next_id.fetch_add(1, std::memory_order::relaxed) };
        return handle;
    }

    /// @brief Flushes reserved handles.
    constexpr void flush() {
        auto free_cursor = _free_cursor.load(std::memory_order::relaxed);

        while (free_cursor < 0) {
            _generations.emplace_back();
            free_cursor++;
        }

        while (_free_ids.size() > free_cursor) {
            _free_ids.pop_back();
        }

        _free_cursor.store(free_cursor, std::memory_order::relaxed);
    }

private:
    std::atomic<typename H::id_t> _next_id{};
    std::atomic<std::int64_t> _free_cursor{};
    std::vector<typename H::generation_t> _generations;
    std::vector<typename H::id_t> _free_ids;
};

} // namespace co_ecs::detail
