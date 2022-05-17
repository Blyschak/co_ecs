#pragma once

#include <compare>
#include <concepts>
#include <cstdint>
#include <numeric>

#ifdef __cpp_lib_format
#include <format>
#endif

namespace cobalt::asl {

/// @brief Handle as an ID and generation
template<std::integral _id_t = std::uint32_t, std::integral _generation_t = std::uint32_t>
class handle {
public:
    using id_t = _id_t;
    using generation_t = _generation_t;

    /// @brief Invalid ID number
    static constexpr auto invalid_id = std::numeric_limits<id_t>::max();

    /// @brief Invalid generation number
    static constexpr auto invalid_generation = std::numeric_limits<generation_t>::max();

    /// @brief Default construct a handle
    constexpr handle() = default;

    /// @brief Construct handle from id and generation
    ///
    /// @param id ID number
    /// @param generation Generation number
    /// @return Handle
    explicit constexpr handle(id_t id, generation_t generation = 0) noexcept : _id(id), _generation(generation) {
    }

    /// @brief Returns invalid handle
    ///
    /// @return Invalid handle
    [[nodiscard]] constexpr static auto invalid() {
        return handle{ invalid_id, invalid_generation };
    }

    /// @brief Test if handle is valid
    ///
    /// @return True if handle is valid
    [[nodiscard]] constexpr bool valid() const noexcept {
        return *this != handle::invalid();
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
    constexpr auto operator<=>(const handle& rsh) const = default;

private:
    id_t _id{ invalid_id };
    generation_t _generation{ invalid_generation };
};

} // namespace cobalt::asl

#ifdef __cpp_lib_format
template<typename T, typename U>
struct std::formatter<cobalt::handle<T, U>> : std::formatter<std::string> {
    auto format(cobalt::handle<T, U> h, format_context& ctx) {
        return formatter<string>::format(std::format("[{}, {}]", h.id(), h.generation()), ctx);
    }
};
#endif
