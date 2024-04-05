#pragma once

#ifndef CO_ECS_USE_RANGE_V3
#include <ranges>
#else
#include <range/v3/all.hpp>
#endif

namespace co_ecs::detail {

#ifndef CO_ECS_USE_RANGE_V3
namespace views = std::views;
#else
namespace views = ranges::views;
#endif

} // namespace co_ecs::detail
