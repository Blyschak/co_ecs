#include "vk_device.hpp"
#include "vk.hpp"

#include <GLFW/glfw3.h>

#include <ranges>

#include <cobalt/asl/check.hpp>

namespace cobalt::render {

constexpr const char* required_validation_layers[] = { "VK_LAYER_KHRONOS_validation" };

// local callback functions
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    core::log_debug("vulkan validation layer: {}", pCallbackData->pMessage);
    return VK_FALSE;
}

vk_device::vk_device(platform::window& window) : _window(window) {
    query_validation_layers();
    query_extensions();
    create_instance();
}

vk_device::~vk_device() {
}

void vk_device::create_instance() {
    asl::check(enable_validation_layers() && are_validation_layers_supported(), "validation layers aren't available");

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "App";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    create_info.enabledExtensionCount = static_cast<uint32_t>(_extensions.size());
    create_info.ppEnabledExtensionNames = _extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info;
    if (enable_validation_layers()) {
        create_info.enabledLayerCount = sizeof(required_validation_layers) / sizeof(required_validation_layers[0]);
        create_info.ppEnabledLayerNames = required_validation_layers;

        debug_create_info = {};
        debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                                            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                                        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                        | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_create_info.pfnUserCallback = debug_callback;
        debug_create_info.pUserData = nullptr; // Optional
        debug_create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;
    } else {
        create_info.enabledLayerCount = 0;
        create_info.pNext = nullptr;
    }

    vk_check(vkCreateInstance(&create_info, nullptr, &_instance), "failed to create instance");
}

void vk_device::query_validation_layers() {
    std::uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    _validation_layers.resize(static_cast<std::size_t>(layer_count));
    vkEnumerateInstanceLayerProperties(&layer_count, _validation_layers.data());

    for (const auto& layer : _validation_layers) {
        core::log_debug("vulkan validation layer supported: {}", layer.layerName);
    }
}

void vk_device::query_extensions() {
    std::uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    if (enable_validation_layers()) {
        _extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
}

bool vk_device::are_validation_layers_supported() const noexcept {
    for (const auto& layer : required_validation_layers) {
        auto iter = std::ranges::find_if(
            _validation_layers, [](const auto& layer) { return layer.layerName == "VK_LAYER_KHRONOS_validation"; });
        if (iter == _validation_layers.end()) {
            return false;
        }
    }
    return true;
}

} // namespace cobalt::render