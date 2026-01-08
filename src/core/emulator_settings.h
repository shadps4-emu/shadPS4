// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "common/types.h"
#include "core/user_manager.h"

// -------------------------------
// Generic Setting wrapper
// -------------------------------
template <typename T>
struct Setting {
    T value{};
};

template <typename T>
void to_json(nlohmann::json& j, const Setting<T>& s) {
    j = s.value;
}

template <typename T>
void from_json(const nlohmann::json& j, Setting<T>& s) {
    s.value = j.get<T>();
}

// -------------------------------
// Helper to describe a per-field override action
// -------------------------------
struct OverrideItem {
    const char* key;
    // apply(basePtrToStruct, jsonEntry, changedFields)
    std::function<void(void*, const nlohmann::json&, std::vector<std::string>&)> apply;
};

// Helper factory: create an OverrideItem binding a pointer-to-member
template <typename Struct, typename T>
inline OverrideItem make_override(const char* key, Setting<T> Struct::* member) {
    return OverrideItem{key, [member, key](void* base, const nlohmann::json& entry,
                                           std::vector<std::string>& changed) {
                            if (!entry.is_object())
                                return;

                            Struct* obj = reinterpret_cast<Struct*>(base);
                            Setting<T>& dst = obj->*member;

                            Setting<T> tmp = entry.get<Setting<T>>();

                            if (dst.value != tmp.value) {
                                changed.push_back(std::string(key) + " ( " +
                                                  nlohmann::json(dst.value).dump() + " → " +
                                                  nlohmann::json(tmp.value).dump() + " )");
                            }

                            dst.value = tmp.value;
                        }};
}

// -------------------------------
// Support types
// -------------------------------
struct GameInstallDir {
    std::filesystem::path path;
    bool enabled;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GameInstallDir, path, enabled)

// -------------------------------
// General settings
// -------------------------------
struct GeneralSettings {
    Setting<std::vector<GameInstallDir>> install_dirs;
    Setting<std::filesystem::path> addon_install_dir;
    Setting<std::filesystem::path> home_dir;
    Setting<std::filesystem::path> sys_modules_dir;

    Setting<int> volume_slider{100};
    Setting<bool> neo_mode{false};
    Setting<bool> dev_kit_mode{false};
    Setting<int> extra_dmem_in_mbytes{0};
    Setting<bool> psn_signed_in{false};
    Setting<bool> trophy_popup_disabled{false};
    Setting<double> trophy_notification_duration{6.0};
    Setting<std::string> trophy_notification_side{"right"};
    Setting<std::string> log_filter{""};
    Setting<std::string> log_type{"sync"};
    Setting<bool> show_splash{false};
    Setting<bool> connected_to_network{false};
    Setting<bool> discord_rpc_enabled{false};

    // return a vector of override descriptors (runtime, but tiny)
    std::vector<OverrideItem> GetOverrideableFields() const {
        return std::vector<OverrideItem>{
            make_override<GeneralSettings>("volume_slider", &GeneralSettings::volume_slider),
            make_override<GeneralSettings>("neo_mode", &GeneralSettings::neo_mode),
            make_override<GeneralSettings>("dev_kit_mode", &GeneralSettings::dev_kit_mode),
            make_override<GeneralSettings>("extra_dmem_in_mbytes",
                                           &GeneralSettings::extra_dmem_in_mbytes),
            make_override<GeneralSettings>("psn_signed_in", &GeneralSettings::psn_signed_in),
            make_override<GeneralSettings>("trophy_popup_disabled",
                                           &GeneralSettings::trophy_popup_disabled),
            make_override<GeneralSettings>("trophy_notification_duration",
                                           &GeneralSettings::trophy_notification_duration),
            make_override<GeneralSettings>("log_filter", &GeneralSettings::log_filter),
            make_override<GeneralSettings>("log_type", &GeneralSettings::log_type),
            make_override<GeneralSettings>("show_splash", &GeneralSettings::show_splash),
            make_override<GeneralSettings>("trophy_notification_side",
                                           &GeneralSettings::trophy_notification_side),
            make_override<GeneralSettings>("connected_to_network",
                                           &GeneralSettings::connected_to_network)};
    }
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GeneralSettings, install_dirs, addon_install_dir, home_dir,
                                   sys_modules_dir, volume_slider, neo_mode, dev_kit_mode,
                                   extra_dmem_in_mbytes, psn_signed_in, trophy_popup_disabled,
                                   trophy_notification_duration, log_filter, log_type, show_splash,
                                   trophy_notification_side, connected_to_network,
                                   discord_rpc_enabled)

// -------------------------------
// Debug settings
// -------------------------------
struct DebugSettings {
    Setting<bool> separate_logging_enabled{false}; // specific
    Setting<bool> debug_dump{false};               // specific
    Setting<bool> shader_collect{false};           // specific
    Setting<bool> fps_color{true};
    Setting<bool> log_enabled{true}; // specific

    std::vector<OverrideItem> GetOverrideableFields() const {
        return std::vector<OverrideItem>{
            make_override<DebugSettings>("debug_dump", &DebugSettings::debug_dump),
            make_override<DebugSettings>("shader_collect", &DebugSettings::shader_collect),
            make_override<DebugSettings>("separate_logging_enabled",
                                         &DebugSettings::separate_logging_enabled),
            make_override<DebugSettings>("log_enabled", &DebugSettings::log_enabled)};
    }
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DebugSettings, separate_logging_enabled, debug_dump,
                                   shader_collect, fps_color, log_enabled)

// -------------------------------
// Input settings
// -------------------------------
enum HideCursorState : int { Never, Idle, Always };
enum UsbDevice : int { Real, SkylanderPortal, InfinityBase, DimensionsToypad };

struct InputSettings {
    Setting<int> cursor_state{HideCursorState::Idle}; // specific
    Setting<int> cursor_hide_timeout{5};              // specific
    Setting<int> usb_device{UsbDevice::Real};         // specific
    Setting<bool> use_special_pad{false};
    Setting<int> special_pad_class{1};
    Setting<bool> motion_controls_enabled{true}; // specific
    Setting<bool> use_unified_Input_Config{true};
    Setting<std::string> default_controller_id{""};
    Setting<bool> background_controller_input{false}; // specific

    std::vector<OverrideItem> GetOverrideableFields() const {
        return std::vector<OverrideItem>{
            make_override<InputSettings>("cursor_state", &InputSettings::cursor_state),
            make_override<InputSettings>("cursor_hide_timeout",
                                         &InputSettings::cursor_hide_timeout),
            make_override<InputSettings>("usb_device", &InputSettings::usb_device),
            make_override<InputSettings>("motion_controls_enabled",
                                         &InputSettings::motion_controls_enabled),
            make_override<InputSettings>("background_controller_input",
                                         &InputSettings::background_controller_input)};
    }
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(InputSettings, cursor_state, cursor_hide_timeout, usb_device,
                                   use_special_pad, special_pad_class, motion_controls_enabled,
                                   use_unified_Input_Config, default_controller_id,
                                   background_controller_input)
// -------------------------------
// Audio settings
// -------------------------------
struct AudioSettings {
    Setting<std::string> mic_device{"Default Device"};
    Setting<std::string> main_output_device{"Default Device"};
    Setting<std::string> padSpk_output_device{"Default Device"};

    // TODO add overrides
    std::vector<OverrideItem> GetOverrideableFields() const {
        return std::vector<OverrideItem>{
            make_override<AudioSettings>("mic_device", &AudioSettings::mic_device),
            make_override<AudioSettings>("main_output_device", &AudioSettings::main_output_device),
            make_override<AudioSettings>("padSpk_output_device",
                                         &AudioSettings::padSpk_output_device)};
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AudioSettings, mic_device, main_output_device,
                                   padSpk_output_device)

// -------------------------------
// GPU settings
// -------------------------------
struct GPUSettings {
    Setting<u32> window_width{1280};
    Setting<u32> window_height{720};
    Setting<u32> internal_screen_width{1280};
    Setting<u32> internal_screen_height{720};
    Setting<bool> null_gpu{false};
    Setting<bool> copy_gpu_buffers{false};
    Setting<bool> readbacks_enabled{false};
    Setting<bool> readback_linear_images_enabled{false};
    Setting<bool> direct_memory_access_enabled{false};
    Setting<bool> dump_shaders{false};
    Setting<bool> patch_shaders{false};
    Setting<u32> vblank_frequency{60};
    Setting<bool> full_screen{false};
    Setting<std::string> full_screen_mode{"Windowed"};
    Setting<std::string> present_mode{"Mailbox"};
    Setting<bool> hdr_allowed{false};
    Setting<bool> fsr_enabled{false};
    Setting<bool> rcas_enabled{true};
    Setting<int> rcas_attenuation{250};
    // TODO add overrides
    std::vector<OverrideItem> GetOverrideableFields() const {
        return std::vector<OverrideItem>{
            make_override<GPUSettings>("null_gpu", &GPUSettings::null_gpu),
            make_override<GPUSettings>("copy_gpu_buffers", &GPUSettings::copy_gpu_buffers),
            make_override<GPUSettings>("full_screen", &GPUSettings::full_screen),
            make_override<GPUSettings>("full_screen_mode", &GPUSettings::full_screen_mode),
            make_override<GPUSettings>("present_mode", &GPUSettings::present_mode),
            make_override<GPUSettings>("window_height", &GPUSettings::window_height),
            make_override<GPUSettings>("window_width", &GPUSettings::window_width),
            make_override<GPUSettings>("hdr_allowed", &GPUSettings::hdr_allowed),
            make_override<GPUSettings>("fsr_enabled", &GPUSettings::fsr_enabled),
            make_override<GPUSettings>("rcas_enabled", &GPUSettings::rcas_enabled),
            make_override<GPUSettings>("rcas_attenuation", &GPUSettings::rcas_attenuation),
            make_override<GPUSettings>("dump_shaders", &GPUSettings::dump_shaders),
            make_override<GPUSettings>("patch_shaders", &GPUSettings::patch_shaders),
            make_override<GPUSettings>("readbacks_enabled", &GPUSettings::readbacks_enabled),
            make_override<GPUSettings>("readback_linear_images_enabled",
                                       &GPUSettings::readback_linear_images_enabled),
            make_override<GPUSettings>("direct_memory_access_enabled",
                                       &GPUSettings::direct_memory_access_enabled),
        };
    }
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GPUSettings, window_width, window_height, internal_screen_width,
                                   internal_screen_height, null_gpu, copy_gpu_buffers,
                                   readbacks_enabled, readback_linear_images_enabled,
                                   direct_memory_access_enabled, dump_shaders, patch_shaders,
                                   vblank_frequency, full_screen, full_screen_mode, present_mode,
                                   hdr_allowed, fsr_enabled, rcas_enabled, rcas_attenuation)
// -------------------------------
// Vulkan settings
// -------------------------------
struct VulkanSettings {
    Setting<s32> gpu_id{-1};
    Setting<bool> renderdoc_enabled{false};
    Setting<bool> vkvalidation_enabled{false};
    Setting<bool> vkvalidation_core_enabled{true};
    Setting<bool> vkvalidation_sync_enabled{false};
    Setting<bool> vkvalidation_gpu_enabled{false};
    Setting<bool> vkcrash_diagnostic_enabled{false};
    Setting<bool> vkhost_markers{false};
    Setting<bool> vkguest_markers{false};
    Setting<bool> pipeline_cache_enabled{false};
    Setting<bool> pipeline_cache_archived{false};
    std::vector<OverrideItem> GetOverrideableFields() const {
        return std::vector<OverrideItem>{
            make_override<VulkanSettings>("gpu_id", &VulkanSettings::gpu_id),
            make_override<VulkanSettings>("renderdoc_enabled", &VulkanSettings::renderdoc_enabled),
            make_override<VulkanSettings>("vkvalidation_enabled",
                                          &VulkanSettings::vkvalidation_enabled),
            make_override<VulkanSettings>("vkvalidation_core_enabled",
                                          &VulkanSettings::vkvalidation_core_enabled),
            make_override<VulkanSettings>("vkvalidation_sync_enabled",
                                          &VulkanSettings::vkvalidation_sync_enabled),
            make_override<VulkanSettings>("vkvalidation_gpu_enabled",
                                          &VulkanSettings::vkvalidation_gpu_enabled),
            make_override<VulkanSettings>("vkcrash_diagnostic_enabled",
                                          &VulkanSettings::vkcrash_diagnostic_enabled),
            make_override<VulkanSettings>("vkhost_markers", &VulkanSettings::vkhost_markers),
            make_override<VulkanSettings>("vkguest_markers", &VulkanSettings::vkguest_markers),
            make_override<VulkanSettings>("pipeline_cache_enabled",
                                          &VulkanSettings::pipeline_cache_enabled),
            make_override<VulkanSettings>("pipeline_cache_archived",
                                          &VulkanSettings::pipeline_cache_archived),
        };
    }
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(VulkanSettings, gpu_id, renderdoc_enabled, vkvalidation_enabled,
                                   vkvalidation_core_enabled, vkvalidation_sync_enabled,
                                   vkvalidation_gpu_enabled, vkcrash_diagnostic_enabled,
                                   vkhost_markers, vkguest_markers, pipeline_cache_enabled,
                                   pipeline_cache_archived)
// -------------------------------
// User settings
// -------------------------------
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(User, user_id, user_color, user_name, controller_port)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Users, default_user_id, user)

// -------------------------------
// Main manager
// -------------------------------
class EmulatorSettings {
public:
    EmulatorSettings();
    ~EmulatorSettings();

    static std::shared_ptr<EmulatorSettings> GetInstance();
    static void SetInstance(std::shared_ptr<EmulatorSettings> instance);

    bool Save(const std::string& serial = "") const;
    bool Load(const std::string& serial = "");
    void SetDefaultValues();

    // general accessors
    bool AddGameInstallDir(const std::filesystem::path& dir, bool enabled = true);
    std::vector<std::filesystem::path> GetGameInstallDirs() const;
    void SetAllGameInstallDirs(const std::vector<GameInstallDir>& dirs);
    void RemoveGameInstallDir(const std::filesystem::path& dir);
    void SetGameInstallDirEnabled(const std::filesystem::path& dir, bool enabled);
    void SetGameInstallDirs(const std::vector<std::filesystem::path>& dirs_config);
    const std::vector<bool> GetGameInstallDirsEnabled();
    const std::vector<GameInstallDir>& GetAllGameInstallDirs() const;

    std::filesystem::path GetHomeDir();
    void SetHomeDir(const std::filesystem::path& dir);
    std::filesystem::path GetSysModulesDir();
    void SetSysModulesDir(const std::filesystem::path& dir);

    // user helpers
    UserManager& GetUserManager() {
        return m_userManager;
    }

private:
    GeneralSettings m_general{};
    DebugSettings m_debug{};
    InputSettings m_input{};
    AudioSettings m_audio{};
    GPUSettings m_gpu{};
    VulkanSettings m_vulkan{};
    UserManager m_userManager;

    static std::shared_ptr<EmulatorSettings> s_instance;
    static std::mutex s_mutex;

    // Generic helper that applies override descriptors for a specific group
    template <typename Group>
    void ApplyGroupOverrides(Group& group, const nlohmann::json& groupJson,
                             std::vector<std::string>& changed) {
        for (auto& item : group.GetOverrideableFields()) {
            if (!groupJson.contains(item.key))
                continue;
            item.apply(&group, groupJson.at(item.key), changed);
        }
    }

    static void PrintChangedSummary(const std::vector<std::string>& changed);

public:
#define SETTING_FORWARD(group, Name, field)                                                        \
    auto Get##Name() const {                                                                       \
        return group.field.value;                                                                  \
    }                                                                                              \
    void Set##Name(const decltype(group.field.value)& v) {                                         \
        group.field.value = v;                                                                     \
    }
#define SETTING_FORWARD_BOOL(group, Name, field)                                                   \
    auto Is##Name() const {                                                                        \
        return group.field.value;                                                                  \
    }                                                                                              \
    void Set##Name(const decltype(group.field.value)& v) {                                         \
        group.field.value = v;                                                                     \
    }
#define SETTING_FORWARD_BOOL_READONLY(group, Name, field)                                          \
    auto Is##Name() const {                                                                        \
        return group.field.value;                                                                  \
    }

    // General settings
    SETTING_FORWARD(m_general, VolumeSlider, volume_slider)
    SETTING_FORWARD_BOOL(m_general, Neo, neo_mode)
    SETTING_FORWARD_BOOL(m_general, DevKit, dev_kit_mode)
    SETTING_FORWARD(m_general, ExtraDmemInMBytes, extra_dmem_in_mbytes)
    SETTING_FORWARD_BOOL(m_general, PSNSignedIn, psn_signed_in)
    SETTING_FORWARD_BOOL(m_general, TrophyPopupDisabled, trophy_popup_disabled)
    SETTING_FORWARD(m_general, TrophyNotificationDuration, trophy_notification_duration)
    SETTING_FORWARD(m_general, TrophyNotificationSide, trophy_notification_side)
    SETTING_FORWARD_BOOL(m_general, ShowSplash, show_splash)
    SETTING_FORWARD(m_general, AddonInstallDir, addon_install_dir)
    SETTING_FORWARD(m_general, LogFilter, log_filter)
    SETTING_FORWARD(m_general, LogType, log_type)
    SETTING_FORWARD_BOOL(m_general, ConnectedToNetwork, connected_to_network)
    SETTING_FORWARD_BOOL(m_general, DiscordRPCEnabled, discord_rpc_enabled)

    // Audio settings
    SETTING_FORWARD(m_audio, MicDevice, mic_device)
    SETTING_FORWARD(m_audio, MainOutputDevice, main_output_device)
    SETTING_FORWARD(m_audio, PadSpkOutputDevice, padSpk_output_device)

    // Debug settings
    SETTING_FORWARD_BOOL(m_debug, SeparateLoggingEnabled, separate_logging_enabled)
    SETTING_FORWARD_BOOL(m_debug, DebugDump, debug_dump)
    SETTING_FORWARD_BOOL(m_debug, ShaderCollect, shader_collect)
    SETTING_FORWARD_BOOL(m_debug, LogEnabled, log_enabled)

    // GPU Settings
    SETTING_FORWARD_BOOL(m_gpu, NullGPU, null_gpu)
    SETTING_FORWARD_BOOL(m_gpu, DumpShaders, dump_shaders)
    SETTING_FORWARD_BOOL(m_gpu, CopyGpuBuffers, copy_gpu_buffers)
    SETTING_FORWARD_BOOL(m_gpu, FullScreen, full_screen)
    SETTING_FORWARD(m_gpu, FullScreenMode, full_screen_mode)
    SETTING_FORWARD(m_gpu, PresentMode, present_mode)
    SETTING_FORWARD(m_gpu, WindowHeight, window_height)
    SETTING_FORWARD(m_gpu, WindowWidth, window_width)
    SETTING_FORWARD(m_gpu, InternalScreenHeight, internal_screen_height)
    SETTING_FORWARD(m_gpu, InternalScreenWidth, internal_screen_width)
    SETTING_FORWARD_BOOL(m_gpu, HdrAllowed, hdr_allowed)
    SETTING_FORWARD_BOOL(m_gpu, FsrEnabled, fsr_enabled)
    SETTING_FORWARD_BOOL(m_gpu, RcasEnabled, rcas_enabled)
    SETTING_FORWARD(m_gpu, RcasAttenuation, rcas_attenuation)
    SETTING_FORWARD_BOOL(m_gpu, ReadbacksEnabled, readbacks_enabled)
    SETTING_FORWARD_BOOL(m_gpu, ReadbackLinearImagesEnabled, readback_linear_images_enabled)
    SETTING_FORWARD_BOOL(m_gpu, DirectMemoryAccessEnabled, direct_memory_access_enabled)
    SETTING_FORWARD_BOOL_READONLY(m_gpu, PatchShaders, patch_shaders)

    u32 GetVblankFrequency() {
        if (m_gpu.vblank_frequency.value < 60) {
            m_gpu.vblank_frequency.value = 60;
        }
        return m_gpu.vblank_frequency.value;
    }
    void SetVblankFrequency(const u32& v) {
        if (v < 60) {
            m_gpu.vblank_frequency.value = 60;
        } else {
            m_gpu.vblank_frequency.value = v;
        }
    }

    // Input Settings
    SETTING_FORWARD(m_input, CursorState, cursor_state)
    SETTING_FORWARD(m_input, CursorHideTimeout, cursor_hide_timeout)
    SETTING_FORWARD(m_input, UsbDevice, usb_device)
    SETTING_FORWARD_BOOL(m_input, MotionControlsEnabled, motion_controls_enabled)
    SETTING_FORWARD_BOOL(m_input, BackgroundControllerInput, background_controller_input)

    // Vulkan settings
    SETTING_FORWARD(m_vulkan, GpuId, gpu_id)
    SETTING_FORWARD_BOOL(m_vulkan, RenderdocEnabled, renderdoc_enabled)
    SETTING_FORWARD_BOOL(m_vulkan, VkValidationEnabled, vkvalidation_enabled)
    SETTING_FORWARD_BOOL(m_vulkan, VkValidationCoreEnabled, vkvalidation_core_enabled)
    SETTING_FORWARD_BOOL(m_vulkan, VkValidationSyncEnabled, vkvalidation_sync_enabled)
    SETTING_FORWARD_BOOL(m_vulkan, VkValidationGpuEnabled, vkvalidation_gpu_enabled)
    SETTING_FORWARD_BOOL(m_vulkan, VkCrashDiagnosticEnabled, vkcrash_diagnostic_enabled)
    SETTING_FORWARD_BOOL(m_vulkan, VkHostMarkersEnabled, vkhost_markers)
    SETTING_FORWARD_BOOL(m_vulkan, VkGuestMarkersEnabled, vkguest_markers)
    SETTING_FORWARD_BOOL(m_vulkan, PipelineCacheEnabled, pipeline_cache_enabled)
    SETTING_FORWARD_BOOL(m_vulkan, PipelineCacheArchived, pipeline_cache_archived)

#undef SETTING_FORWARD
#undef SETTING_FORWARD_BOOL
};
