#pragma once

#include <cobalt/ecs/component.hpp>

namespace cobalt::ecs {

class registry;

template<component_or_reference... Args>
class view {
public:
    view(registry& registry) : _registry(registry) {
    }

    decltype(auto) each();

private:
    registry& _registry;
};


} // namespace cobalt::ecs