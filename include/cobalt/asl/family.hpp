#pragma once

#include <cstdint>

namespace cobalt::asl {

/// @brief Family pattern for generating unique sequential ids for types
///
/// @tparam Family type
/// @tparam _id_type Type for id
template<typename = void, typename _id_type = std::uint64_t>
class family {
public:
    using id_type = _id_type;

    /// @brief Get next ID value
    ///
    /// @return id_type Next ID
    inline static id_type next() noexcept {
        return identifier++;
    }

private:
    inline static id_type identifier{}; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

public:
    template<typename... T>
    inline static const id_type id = next();
};

} // namespace cobalt::asl