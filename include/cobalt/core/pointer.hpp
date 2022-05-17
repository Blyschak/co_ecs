#pragma once

#include <memory>

namespace cobalt {

template<typename T>
using owned = std::unique_ptr<T>;

template<typename T>
using shared = std::shared_ptr<T>;

} // namespace cobalt