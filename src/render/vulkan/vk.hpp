#pragma once

#include <cobalt/asl/check.hpp>
#include <vulkan/vulkan.h>

namespace cobalt::renderer {

static inline void vk_check(VkResult result, auto&& msg) {
    asl::check(result == VK_SUCCESS, msg);
}

} // namespace cobalt::renderer