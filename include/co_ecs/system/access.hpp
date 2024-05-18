#pragma once

#include <co_ecs/component.hpp>

namespace co_ecs {

/// @brief Enumeration representing the type of access.
///
/// This enum defines the different types of access permissions: none, read, and write.
enum access_type {
    none, ///< No access.
    read, ///< Read access.
    write ///< Write access.
};

/// @brief Class representing an access pattern for components.
///
/// This class is used to manage and check access permissions for components within a registry.
class access_pattern_t {
public:
    /// @brief Default constructor.
    ///
    /// Constructs an access pattern with no registry access.
    access_pattern_t() = default;

    /// @brief Constructs an access pattern with specified registry access.
    ///
    /// @param registry_access The type of access for the registry.
    access_pattern_t(access_type registry_access) : _registry_access(registry_access) {
    }

    /// @brief Constructs an access pattern for a specific component.
    ///
    /// @param access The type of access for the component.
    /// @param meta Metadata for the component.
    access_pattern_t(access_type access, component_meta meta) {
        _component_access.emplace(meta.id, access);
    }

    /// @brief Checks if this access pattern allows another access pattern.
    ///
    /// @param other The other access pattern to check against.
    /// @return True if this access pattern allows the other, false otherwise.
    [[nodiscard]] auto allows(const access_pattern_t& other) const noexcept -> bool {
        if ((writes_all() && other.reads_all()) || (reads_all() && !other.reads_all())) {
            return false;
        }

        for (auto [component_id, access] : _component_access) {
            if ((access == access_type::write && other.reads(component_id)) || other.writes(component_id)) {
                return false;
            }
        }

        return true;
    }

    /// @brief Checks if this access pattern writes to all components.
    ///
    /// @return True if this access pattern writes to all components, false otherwise.
    [[nodiscard]] auto writes_all() const noexcept -> bool {
        return _registry_access == access_type::write;
    }

    /// @brief Checks if this access pattern reads from all components.
    ///
    /// @return True if this access pattern reads from all components, false otherwise.

    [[nodiscard]] auto reads_all() const noexcept -> bool {
        return _registry_access != access_type::none;
    }

    /// @brief Checks if this access pattern writes to a specific component.
    ///
    /// @param id The ID of the component.
    /// @return True if this access pattern writes to the component, false otherwise.
    [[nodiscard]] auto writes(component_id_t id) const noexcept -> bool {
        if (_registry_access == access_type::write) {
            return true;
        }

        if (auto iter = _component_access.find(id); iter != _component_access.end()) {
            auto [_, access] = *iter;
            if (access == access_type::write) {
                return true;
            }
        }

        return false;
    }

    /// @brief Checks if this access pattern reads from a specific component.
    ///
    /// @param id The ID of the component.
    /// @return True if this access pattern reads from the component, false otherwise.
    [[nodiscard]] auto reads(component_id_t id) const noexcept -> bool {
        if (_registry_access != access_type::none) {
            return true;
        }

        if (auto iter = _component_access.find(id); iter != _component_access.end()) {
            return true;
        }

        return false;
    }

    /// @brief Combines two access patterns using the bitwise AND operator.
    ///
    /// @param rhs The other access pattern to combine with.
    /// @return A new access pattern that is the result of combining the two access patterns.
    auto operator&(const access_pattern_t& rhs) -> access_pattern_t {
        access_pattern_t res = *this;
        res &= rhs;
        return res;
    }

    /// @brief Combines this access pattern with another using the bitwise AND assignment operator.
    ///
    /// @param rhs The other access pattern to combine with.
    /// @return A reference to this access pattern after combining.
    auto operator&=(const access_pattern_t& rhs) -> access_pattern_t& {
        _registry_access = std::max(_registry_access, rhs._registry_access);

        for (auto [component_id, access] : rhs._component_access) {
            _component_access[component_id] = std::max(access, _component_access[component_id]);
        }

        return *this;
    }

private:
    access_type _registry_access{ access_type::none };
    detail::sparse_map<component_id_t, access_type> _component_access;
};

} // namespace co_ecs
