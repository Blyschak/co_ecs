#pragma once

#include <compare>
#include <concepts>
#include <cstdint>
#include <numeric>

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

    static const handle invalid;

    /// @brief Default construct a handle
    constexpr handle() = default;

    /// @brief Construct handle from id and generation
    ///
    /// @param id ID number
    /// @param generation Generation number
    /// @return Handle
    explicit constexpr handle(id_t id, generation_t generation = 0) noexcept : _id(id), _generation(generation) {
    }

    /// @brief Test if handle is valid
    ///
    /// @return True if handle is valid
    [[nodiscard]] constexpr bool valid() const noexcept {
        return *this != handle::invalid;
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

template<typename H, typename G>
const handle<H, G> handle<H, G>::invalid{ handle<H, G>::invalid_id, handle<H, G>::invalid_generation };

} // namespace cobalt::asl
