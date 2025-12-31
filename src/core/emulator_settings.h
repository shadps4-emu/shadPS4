// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
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
    Setting<bool> shader_dump{false};              // specific
    Setting<bool> fps_color{true};
    Setting<bool> log_enabled{true}; // specific

    std::vector<OverrideItem> GetOverrideableFields() const {
        return std::vector<OverrideItem>{
            make_override<DebugSettings>("debug_dump", &DebugSettings::debug_dump),
            make_override<DebugSettings>("shader_dump", &DebugSettings::shader_dump),
            make_override<DebugSettings>("separate_logging_enabled",
                                         &DebugSettings::separate_logging_enabled),
            make_override<DebugSettings>("log_enabled", &DebugSettings::log_enabled)};
    }
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DebugSettings, separate_logging_enabled, debug_dump, shader_dump,
                                   fps_color, log_enabled)

// -------------------------------
// Input settings
// -------------------------------
enum HideCursorState : int { Never, Idle, Always };

struct InputSettings {
    Setting<int> cursor_state{HideCursorState::Idle}; // specific
    Setting<int> cursor_hide_timeout{5};              // specific
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
            make_override<InputSettings>("motion_controls_enabled",
                                         &InputSettings::motion_controls_enabled),
            make_override<InputSettings>("background_controller_input",
                                         &InputSettings::background_controller_input)};
    }
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(InputSettings, cursor_state, cursor_hide_timeout,
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
    Setting<bool> should_copy_gpu_buffers{false};
    Setting<bool> readbacks_enabled{false};
    Setting<bool> readback_linear_images_enabled{false};
    Setting<bool> direct_memory_access_enabled{false};
    Setting<bool> should_dump_shaders{false};
    Setting<bool> should_patch_shaders{false};
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
            make_override<GPUSettings>("full_screen", &GPUSettings::full_screen),
            make_override<GPUSettings>("present_mode", &GPUSettings::present_mode)};
    }
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GPUSettings, window_width, window_height, internal_screen_width,
                                   internal_screen_height, null_gpu, should_copy_gpu_buffers,
                                   readbacks_enabled, readback_linear_images_enabled,
                                   direct_memory_access_enabled, should_dump_shaders,
                                   should_patch_shaders, vblank_frequency, full_screen,
                                   full_screen_mode, present_mode, hdr_allowed, fsr_enabled,
                                   rcas_enabled, rcas_attenuation)
// -------------------------------
// Vulkan settings
// -------------------------------
struct VulkanSettings {
    Setting<s32> gpu_id{-1};
    Setting<bool> full_screen{false};
    // TODO
    std::vector<OverrideItem> GetOverrideableFields() const {
        return std::vector<OverrideItem>{
            make_override<VulkanSettings>("gpu_id", &VulkanSettings::gpu_id),
            make_override<VulkanSettings>("full_screen", &VulkanSettings::full_screen)};
    }
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(VulkanSettings, gpu_id, full_screen)
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
    SETTING_FORWARD(m_audio, MainOutputDevice, main_output_device)
    SETTING_FORWARD(m_audio, PadSpkOutputDevice, padSpk_output_device)

    // Debug settings
    SETTING_FORWARD_BOOL(m_debug, SeparateLoggingEnabled, separate_logging_enabled)
    SETTING_FORWARD_BOOL(m_debug, DebugDump, debug_dump)
    SETTING_FORWARD_BOOL(m_debug, ShaderDump, shader_dump)
    SETTING_FORWARD_BOOL(m_debug, LogEnabled, log_enabled)

    // GPU Settings
    SETTING_FORWARD_BOOL(m_gpu, NullGPU, null_gpu)
    SETTING_FORWARD(m_gpu, FullScreenMode, full_screen_mode)
    SETTING_FORWARD(m_gpu, PresentMode, present_mode)

    // Vulkan settings
    SETTING_FORWARD(m_vulkan, GpuId, gpu_id)
    SETTING_FORWARD_BOOL(m_vulkan, FullScreen, full_screen)

#undef SETTING_FORWARD
#undef SETTING_FORWARD_BOOL
};
