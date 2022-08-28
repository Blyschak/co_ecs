#pragma once

#include <co_ecs/component.hpp>
#include <co_ecs/entity.hpp>

#include <sstream>
#include <stdexcept>
#include <string>

namespace co_ecs {

/// @brief Exception raised when accessing non existing entity
class entity_not_found : public std::exception {
public:
    /// @brief Construct a new entity not found exception object
    ///
    /// @param ent Entity
    explicit entity_not_found(entity ent) {
        std::stringstream ss;
        ss << "entity (" << ent.id() << ", " << ent.generation() << ")"
           << " does not exist";
        _msg = ss.str();
    }

    /// @brief Message to the client
    ///
    /// @return const char*
    [[nodiscard]] const char* what() const noexcept override {
        return _msg.c_str();
    }

private:
    std::string _msg;
};

/// @brief Exception raised when accessing entities component which was not assigned
class component_not_found : public std::exception {
public:
    /// @brief Construct a new component not found exception object
    ///
    /// @param meta Type metadata
    explicit component_not_found(const type_meta* meta) {
        std::stringstream ss;
        ss << "component \"" << meta->name << "\" not found";
        _msg = ss.str();
    }

    /// @brief Message to the client
    ///
    /// @return const char*
    [[nodiscard]] const char* what() const noexcept override {
        return _msg.c_str();
    }

private:
    std::string _msg;
};

} // namespace cobalt::ecs
