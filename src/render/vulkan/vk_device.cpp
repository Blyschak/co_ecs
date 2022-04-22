#include "vk_device.hpp"
#include "../../platform/glfw/glfw_window.hpp"
#include "vk.hpp"

#include <GLFW/glfw3.h>

#include <ranges>

#include <cobalt/asl/check.hpp>
#include <cobalt/asl/hash_set.hpp>

namespace cobalt::render {

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    core::log_debug("vulkan validation layer: {}", pCallbackData->pMessage);
    return VK_FALSE;
}

vk_device::vk_device(platform::window& window) : _window(window) {
    init_required_validation_layers();
    init_required_instance_extensions();
    query_validation_layers();
    query_instance_extensions();
    create_instance();
    create_surface();
    choose_physical_device();
    create_logical_device();
    create_command_pool();
}

vk_device::~vk_device() {
    vkDestroyCommandPool(_device, _command_pool, nullptr);
    vkDestroyDevice(_device, nullptr);

    if (enable_validation_layers()) {
        auto destroy_fn = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (destroy_fn) {
            destroy_fn(_instance, _debug_messenger, nullptr);
        }
    }

    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyInstance(_instance, nullptr);
}

void vk_device::create_instance() {
    asl::check(are_required_validation_layers_supported(), "validation layers aren't available");
    asl::check(are_required_instance_extensions_supported(), "required instance extensions aren't supported");

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

    create_info.enabledExtensionCount = static_cast<uint32_t>(_required_extensions.size());
    create_info.ppEnabledExtensionNames = _required_extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info;
    if (enable_validation_layers()) {
        create_info.enabledLayerCount = _required_validation_layers.size();
        create_info.ppEnabledLayerNames = _required_validation_layers.data();

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

void vk_device::create_surface() {
    auto* glfw_window = dynamic_cast<platform::glfw_window*>(&_window);
    asl::check(glfw_window, "GLFW window implementation is required");
    glfw_window->create_surface(_instance, &_surface);
}

void vk_device::choose_physical_device() {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(_instance, &device_count, nullptr);
    asl::check(device_count, "vulkan physical devices not found");

    core::log_info("vulkan found {} physical devices", device_count);

    asl::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(_instance, &device_count, devices.data());

    for (const auto& device : devices) {
        if (is_physical_device_suitable(device)) {
            _physical_device = device;
            break;
        }
    }

    asl::check(_physical_device != VK_NULL_HANDLE, "failed to find a suitable GPU");

    vkGetPhysicalDeviceProperties(_physical_device, &_properties);

    core::log_info("physical device: {}", _properties.deviceName);
}

void vk_device::create_logical_device() {
    queue_family_indices indices = find_queue_families(_physical_device);

    asl::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    asl::hash_set<uint32_t> unique_queue_families = { indices.graphics_family, indices.present_family };

    float queue_priority = 1.0f;
    for (uint32_t queueFamily : unique_queue_families) {
        VkDeviceQueueCreateInfo queue_create_info = {};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queueFamily;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.emplace_back(queue_create_info);
    }

    VkPhysicalDeviceFeatures device_features = {};
    device_features.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    create_info.pQueueCreateInfos = queue_create_infos.data();

    create_info.pEnabledFeatures = &device_features;
    create_info.enabledExtensionCount = static_cast<uint32_t>(_required_extensions.size());
    create_info.ppEnabledExtensionNames = _required_extensions.data();

    if (enable_validation_layers()) {
        create_info.enabledLayerCount = static_cast<uint32_t>(_required_validation_layers.size());
        create_info.ppEnabledLayerNames = _required_validation_layers.data();
    } else {
        create_info.enabledLayerCount = 0;
    }

    vk_check(vkCreateDevice(_physical_device, &create_info, nullptr, &_device), "failed to create logical device");

    vkGetDeviceQueue(_device, indices.graphics_family, 0, &_graphics_queue);
    vkGetDeviceQueue(_device, indices.present_family, 0, &_present_queue);
}

void vk_device::create_command_pool() {
    queue_family_indices queue_family_indices = find_queue_families(_physical_device);

    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = queue_family_indices.graphics_family;
    pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    vk_check(vkCreateCommandPool(_device, &pool_info, nullptr, &_command_pool), "failed to create command pool");
}

void vk_device::query_validation_layers() {
    std::uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    core::log_debug("vulkan {} validation layers supported", layer_count);

    _validation_layers.resize(static_cast<std::size_t>(layer_count));
    vkEnumerateInstanceLayerProperties(&layer_count, _validation_layers.data());

    for (const auto& layer : _validation_layers) {
        core::log_debug("vulkan validation layer supported: {}", layer.layerName);
    }
}

void vk_device::query_instance_extensions() {
    std::uint32_t extension_count;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

    core::log_debug("vulkan {} instance extensions supported", extension_count);

    _instance_extensions.resize(static_cast<std::size_t>(extension_count));
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, _instance_extensions.data());

    for (const auto& extension : _instance_extensions) {
        core::log_debug("vulkan instance extensions supported: {}", extension.extensionName);
    }
}

void vk_device::init_required_instance_extensions() {
    auto* glfw_window = dynamic_cast<platform::glfw_window*>(&_window);
    asl::check(glfw_window, "GLFW window implementation is required");
    _required_extensions = glfw_window->get_glfw_required_extensions();

    if (enable_validation_layers()) {
        _required_extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    for (const auto& extension : _required_extensions) {
        core::log_debug("vulkan instance extensions required: {}", extension);
    }
}

void vk_device::init_required_validation_layers() {
    if (enable_validation_layers()) {
        _required_validation_layers.emplace_back("VK_LAYER_LUNARG_standard_validation");
    }
}

bool vk_device::are_required_validation_layers_supported() const noexcept {
    for (const auto& required_layer : _required_validation_layers) {
        auto iter = std::ranges::find_if(_validation_layers,
            [required_layer](const auto& layer) { return std::string_view(layer.layerName) == required_layer; });
        if (iter == _validation_layers.end()) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool vk_device::are_required_instance_extensions_supported() const noexcept {
    for (const auto& required_extension : _required_extensions) {
        auto iter = std::ranges::find_if(_instance_extensions, [required_extension](const auto& instance_extension) {
            return std::string_view(instance_extension.extensionName) == required_extension;
        });
        if (iter == _instance_extensions.end()) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool vk_device::is_physical_device_suitable(VkPhysicalDevice device) const noexcept {
    queue_family_indices indices = find_queue_families(device);

    bool extensions_supported = are_extensions_supported_by_device(device);

    bool swap_chain_adequate = false;
    if (extensions_supported) {
        swap_chain_support_details swap_chain_support = query_swap_chain_support(device);
        swap_chain_adequate = !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.isComplete() && extensions_supported && swap_chain_adequate && supportedFeatures.samplerAnisotropy;
}

queue_family_indices vk_device::find_queue_families(VkPhysicalDevice device) const {
    queue_family_indices indices;

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

    asl::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

    int i = 0;
    for (const auto& queue_family : queue_families) {
        if (queue_family.queueCount > 0 && queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics_family = i;
            indices.graphics_family_has_value = true;
        }
        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _surface, &present_support);
        if (queue_family.queueCount > 0 && present_support) {
            indices.present_family = i;
            indices.present_family_has_value = true;
        }
        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

bool vk_device::are_extensions_supported_by_device(VkPhysicalDevice device) const {
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

    asl::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

    auto iter = std::ranges::find_if(available_extensions,
        [](const auto& extension) { return extension.extensionName == VK_KHR_SWAPCHAIN_EXTENSION_NAME; });
    return iter != available_extensions.end();
}

swap_chain_support_details vk_device::query_swap_chain_support(VkPhysicalDevice device) const {
    swap_chain_support_details details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, details.formats.data());
    }

    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &present_mode_count, nullptr);

    if (present_mode_count != 0) {
        details.present_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &present_mode_count, details.present_modes.data());
    }
    return details;
}

} // namespace cobalt::render