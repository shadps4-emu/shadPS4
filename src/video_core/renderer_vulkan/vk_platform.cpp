// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// Include the vulkan platform specific header
#if defined(ANDROID)
#define VK_USE_PLATFORM_ANDROID_KHR
#elif defined(_WIN64)
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__APPLE__)
#define VK_USE_PLATFORM_METAL_EXT
#else
#define VK_USE_PLATFORM_WAYLAND_KHR
#define VK_USE_PLATFORM_XLIB_KHR
#endif

#include <vector>
#include "common/assert.h"
#include "common/config.h"
#include "common/logging/log.h"
#include "sdl_window.h"
#include "video_core/renderer_vulkan/vk_platform.h"

namespace Vulkan {

static const char* const VALIDATION_LAYER_NAME = "VK_LAYER_KHRONOS_validation";
static const char* const API_DUMP_LAYER_NAME = "VK_LAYER_LUNARG_api_dump";

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtilsCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data) {

    switch (static_cast<u32>(callback_data->messageIdNumber)) {
    case 0x609a13b: // Vertex attribute at location not consumed by shader
    case 0xc81ad50e:
        return VK_FALSE;
    default:
        break;
    }

    Common::Log::Level level{};
    switch (severity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        level = Common::Log::Level::Error;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        level = Common::Log::Level::Info;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        level = Common::Log::Level::Debug;
        break;
    default:
        level = Common::Log::Level::Info;
    }

    LOG_GENERIC(Common::Log::Class::Render_Vulkan, level, "{}: {}",
                callback_data->pMessageIdName ? callback_data->pMessageIdName : "<null>",
                callback_data->pMessage ? callback_data->pMessage : "<null>");

    return VK_FALSE;
}

vk::SurfaceKHR CreateSurface(vk::Instance instance, const Frontend::WindowSDL& emu_window) {
    const auto& window_info = emu_window.getWindowInfo();
    vk::SurfaceKHR surface{};

#if defined(VK_USE_PLATFORM_WIN32_KHR)
    if (window_info.type == Frontend::WindowSystemType::Windows) {
        const vk::Win32SurfaceCreateInfoKHR win32_ci = {
            .hinstance = nullptr,
            .hwnd = static_cast<HWND>(window_info.render_surface),
        };

        if (instance.createWin32SurfaceKHR(&win32_ci, nullptr, &surface) != vk::Result::eSuccess) {
            LOG_CRITICAL(Render_Vulkan, "Failed to initialize Win32 surface");
            UNREACHABLE();
        }
    }
#elif defined(VK_USE_PLATFORM_XLIB_KHR) || defined(VK_USE_PLATFORM_WAYLAND_KHR)
    if (window_info.type == Frontend::WindowSystemType::X11) {
        const vk::XlibSurfaceCreateInfoKHR xlib_ci = {
            .dpy = static_cast<Display*>(window_info.display_connection),
            .window = reinterpret_cast<Window>(window_info.render_surface),
        };

        if (instance.createXlibSurfaceKHR(&xlib_ci, nullptr, &surface) != vk::Result::eSuccess) {
            LOG_ERROR(Render_Vulkan, "Failed to initialize Xlib surface");
            UNREACHABLE();
        }
    } else if (window_info.type == Frontend::WindowSystemType::Wayland) {
        const vk::WaylandSurfaceCreateInfoKHR wayland_ci = {
            .display = static_cast<wl_display*>(window_info.display_connection),
            .surface = static_cast<wl_surface*>(window_info.render_surface),
        };

        if (instance.createWaylandSurfaceKHR(&wayland_ci, nullptr, &surface) !=
            vk::Result::eSuccess) {
            LOG_ERROR(Render_Vulkan, "Failed to initialize Wayland surface");
            UNREACHABLE();
        }
    }
#elif defined(VK_USE_PLATFORM_METAL_EXT)
    if (window_info.type == Frontend::WindowSystemType::Metal) {
        const vk::MetalSurfaceCreateInfoEXT macos_ci = {
            .pLayer = static_cast<const CAMetalLayer*>(window_info.render_surface),
        };

        if (instance.createMetalSurfaceEXT(&macos_ci, nullptr, &surface) != vk::Result::eSuccess) {
            LOG_CRITICAL(Render_Vulkan, "Failed to initialize MacOS surface");
            UNREACHABLE();
        }
    }
#endif

    if (!surface) {
        LOG_CRITICAL(Render_Vulkan, "Presentation not supported on this platform");
        UNREACHABLE();
    }

    return surface;
}

std::vector<const char*> GetInstanceExtensions(Frontend::WindowSystemType window_type,
                                               bool enable_debug_utils) {
    const auto properties = vk::enumerateInstanceExtensionProperties();
    if (properties.empty()) {
        LOG_ERROR(Render_Vulkan, "Failed to query extension properties");
        return {};
    }

    // Add the windowing system specific extension
    std::vector<const char*> extensions;
    extensions.reserve(7);

    switch (window_type) {
    case Frontend::WindowSystemType::Headless:
        break;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    case Frontend::WindowSystemType::Windows:
        extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
        break;
#elif defined(VK_USE_PLATFORM_XLIB_KHR) || defined(VK_USE_PLATFORM_WAYLAND_KHR)
    case Frontend::WindowSystemType::X11:
        extensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
        break;
    case Frontend::WindowSystemType::Wayland:
        extensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
        break;
#elif defined(VK_USE_PLATFORM_METAL_EXT)
    case Frontend::WindowSystemType::Metal:
        extensions.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
        break;
#endif
    default:
        LOG_ERROR(Render_Vulkan, "Presentation not supported on this platform");
        break;
    }

    if (window_type != Frontend::WindowSystemType::Headless) {
        extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    }

    if (enable_debug_utils) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    // Sanitize extension list
    std::erase_if(extensions, [&](const char* extension) -> bool {
        const auto it =
            std::find_if(properties.begin(), properties.end(), [extension](const auto& prop) {
                return std::strcmp(extension, prop.extensionName) == 0;
            });

        if (it == properties.end()) {
            LOG_INFO(Render_Vulkan, "Candidate instance extension {} is not available", extension);
            return true;
        }
        return false;
    });

    return extensions;
}

vk::UniqueInstance CreateInstance(vk::DynamicLoader& dl, Frontend::WindowSystemType window_type,
                                  bool enable_validation, bool dump_command_buffers) {
    LOG_INFO(Render_Vulkan, "Creating vulkan instance");

    auto vkGetInstanceProcAddr =
        dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    const u32 available_version = VULKAN_HPP_DEFAULT_DISPATCHER.vkEnumerateInstanceVersion
                                      ? vk::enumerateInstanceVersion()
                                      : VK_API_VERSION_1_0;

    ASSERT_MSG(available_version >= TargetVulkanApiVersion,
               "Vulkan {}.{} is required, but only {}.{} is supported by instance!",
               VK_VERSION_MAJOR(TargetVulkanApiVersion), VK_VERSION_MINOR(TargetVulkanApiVersion),
               VK_VERSION_MAJOR(available_version), VK_VERSION_MINOR(available_version));

    const auto extensions = GetInstanceExtensions(window_type, true);

    const vk::ApplicationInfo application_info = {
        .pApplicationName = "shadPS4",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "shadPS4 Vulkan",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = available_version,
    };

    u32 num_layers = 0;
    std::array<const char*, 2> layers;

    if (enable_validation) {
        layers[num_layers++] = VALIDATION_LAYER_NAME;
    }
    if (dump_command_buffers) {
        layers[num_layers++] = API_DUMP_LAYER_NAME;
    }

    vk::Bool32 enable_sync =
        enable_validation && Config::vkValidationSyncEnabled() ? vk::True : vk::False;
    vk::LayerSettingEXT layer_set = {
        .pLayerName = VALIDATION_LAYER_NAME,
        .pSettingName = "validate_sync",
        .type = vk::LayerSettingTypeEXT::eBool32,
        .valueCount = 1,
        .pValues = &enable_sync,
    };

    vk::StructureChain<vk::InstanceCreateInfo, vk::LayerSettingsCreateInfoEXT> instance_ci_chain = {
        vk::InstanceCreateInfo{
            .pApplicationInfo = &application_info,
            .enabledLayerCount = num_layers,
            .ppEnabledLayerNames = layers.data(),
            .enabledExtensionCount = static_cast<u32>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data(),
        },
        vk::LayerSettingsCreateInfoEXT{
            .settingCount = 1,
            .pSettings = &layer_set,
        },
    };

    auto instance = vk::createInstanceUnique(instance_ci_chain.get());

    VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);

    return instance;
}

vk::UniqueDebugUtilsMessengerEXT CreateDebugCallback(vk::Instance instance) {
    const vk::DebugUtilsMessengerCreateInfoEXT msg_ci = {
        .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                           vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                           vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                           vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose,
        .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                       vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                       vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        .pfnUserCallback = DebugUtilsCallback,
    };
    return instance.createDebugUtilsMessengerEXTUnique(msg_ci);
}

} // namespace Vulkan
