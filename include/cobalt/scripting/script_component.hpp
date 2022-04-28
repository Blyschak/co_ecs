#pragma once

namespace cobalt::scripting {

template<typename T>
struct script_component {
    T state;
};

} // namespace cobalt::scripting