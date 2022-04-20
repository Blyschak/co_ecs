#pragma once

#include <vulkan/vulkan.h>

#include <cobalt/asl/vector.hpp>
#include <cobalt/core/logging.hpp>
#include <cobalt/platform/window.hpp>

namespace cobalt::render {

struct swap_chain_support_details {
    VkSurfaceCapabilitiesKHR capabilities;
    asl::vector<VkSurfaceFormatKHR> formats;
    asl::vector<VkPresentModeKHR> present_modes;
};

struct queue_family_indices {
    std::uint32_t graphics_family;
    std::uint32_t present_family;
    bool graphics_family_has_value = false;
    bool present_family_has_value = false;

    bool isComplete() {
        return graphics_family_has_value && present_family_has_value;
    }
};

class vk_device {
public:
    vk_device(platform::window& window);
    ~vk_device();

    vk_device(const vk_device&) = delete;
    void operator=(const vk_device&) = delete;
    vk_device(vk_device&&) = delete;
    vk_device& operator=(vk_device&&) = delete;

private:
    void create_instance();

    [[nodiscard]] bool are_validation_layers_supported() const noexcept;
    void query_validation_layers();
    void query_extensions();

    [[nodiscard]] bool enable_validation_layers() const noexcept {
        return _enable_validation_layers;
    }

    asl::vector<VkLayerProperties> _validation_layers;
    asl::vector<const char*> _extensions;

    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debug_messenger;
    VkPhysicalDevice _physical_device = VK_NULL_HANDLE;
    VkCommandPool _command_pool;

    VkDevice _device;
    VkSurfaceKHR _surface;
    VkQueue _graphics_queue;
    VkQueue _present_queue;

    bool _enable_validation_layers{ true };
    platform::window& _window;
};

} // namespace cobalt::render