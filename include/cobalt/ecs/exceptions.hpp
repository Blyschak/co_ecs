#pragma once

#include <cobalt/ecs/component.hpp>
#include <cobalt/ecs/entity.hpp>

#include <cobalt/asl/demangle.hpp>

#include <sstream>
#include <stdexcept>
#include <string>

namespace cobalt::ecs {

/// @brief Exception raised when accessing non existing entity
class entity_not_found : public std::exception {
public:
    /// @brief Construct a new entity not found exception object
    ///
    /// @param ent Entity
    explicit entity_not_found(entity ent) {
        std::stringstream ss;
        ss << "entity " << ent << " does not exist";
        _msg = ss.str();
    }

    /// @brief Message to the client
    ///
    /// @return const char*
    const char* what() const noexcept override {
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
    /// @param id Component ID
    explicit component_not_found(component_id id) {
        std::stringstream ss;
        ss << "component [id=" << id << "] not found";
        _msg = ss.str();
    }

    /// @brief Message to the client
    ///
    /// @return const char*
    const char* what() const noexcept override {
        return _msg.c_str();
    }

private:
    std::string _msg;
};

/// @brief Exception raised when accessing a resource which was not set
class resource_not_found : public std::exception {
public:
    /// @brief Construct a new resource not found exception object
    ///
    /// @param meta Type metadata
    explicit resource_not_found(const type_meta* meta) {
        std::stringstream ss;
        ss << "resource \"" << asl::demangle(meta->name) << "\" not found";
        _msg = ss.str();
    }

    /// @brief Message to the client
    ///
    /// @return const char*
    const char* what() const noexcept override {
        return _msg.c_str();
    }

private:
    std::string _msg;
};

} // namespace cobalt::ecs