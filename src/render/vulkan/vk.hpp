#pragma once

#include <cobalt/core/assert.hpp>
#include <vulkan/vulkan.h>

namespace cobalt {

static inline void vk_check(VkResult result, auto&& msg) {
    assert_with_message(result == VK_SUCCESS, msg);
}

} // namespace cobalt