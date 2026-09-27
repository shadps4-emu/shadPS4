// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "common/logging/log.h"
#include "common/types.h"

#define EmulatorSettings (*EmulatorSettingsImpl::GetInstance())

enum HideCursorState : int {
    Never,
    Idle,
    Always,
};

enum UsbBackendType : int {
    Real,
    SkylandersPortal,
    InfinityBase,
    DimensionsToypad,
};

enum GpuReadbacksMode : int {
    Disabled,
    Relaxed,
    Precise,
};

enum class ConfigMode {
    Default,
    Global,
    Clean,
};

enum AudioBackend : int {
    SDL,
    OpenAL,
    // Add more backends as needed
};

enum OpenALHrtfMode : int {
    HrtfAuto, // Let OpenAL Soft decide (on for headphone-like stereo outputs)
    HrtfOn,   // Force HRTF binaural rendering
    HrtfOff,  // Never use HRTF
};

enum OpenALOutputMode : int {
    OutputAuto,       // Let OpenAL Soft negotiate with the device
    OutputStereo,     // Force stereo output
    OutputQuad,       // Force quadraphonic output
    OutputSurround51, // Force 5.1 surround output
    OutputSurround71, // Force 7.1 surround output
};

template <typename T>
struct Setting {
    T default_value{};
    T value{};
    std::optional<T> game_specific_value{};

    Setting() = default;
    // Single-argument ctor: initialises both default_value and value so
    // that CleanMode can always recover the intended factory default.
    /*implicit*/ Setting(T init) : default_value(std::move(init)), value(default_value) {}

    /// Return the active value under the given mode.
    T get(ConfigMode mode = ConfigMode::Default) const {
        switch (mode) {
        case ConfigMode::Default:
            return game_specific_value.value_or(value);
        case ConfigMode::Global:
            return value;
        case ConfigMode::Clean:
            return default_value;
        }
        return value;
    }

    /// Write v to the base layer.
    /// Set proper value as base or game_specific
    void set(const T& v, bool game_specific = false) {
        if (game_specific) {
            game_specific_value = v;
        } else {
            value = v;
        }
    }

    /// Discard the game-specific override; subsequent get(Default) will
    /// fall back to the base value.
    void reset_game_specific() {
        game_specific_value = std::nullopt;
    }
};

template <typename T>
void to_json(nlohmann::json& j, const Setting<T>& s) {
    j = s.value;
}

template <typename T>
void from_json(const nlohmann::json& j, Setting<T>& s) {
    s.value = j.get<T>();
}

struct OverrideItem {
    const char* key;
    std::function<void(void* group_ptr, const nlohmann::json& entry,
                       std::vector<std::string>& changed)>
        apply;
    /// Return the value that should be written to the per-game config file.
    /// Falls back to base value if no game-specific override is set.
    std::function<nlohmann::json(const void* group_ptr)> get_for_save;

    /// Clear game_specific_value for this field.
    std::function<void(void* group_ptr)> reset_game_specific;
};

template <typename Struct, typename T>
inline OverrideItem make_override(const char* key, Setting<T> Struct::* member) {
    return OverrideItem{
        key,
        [member, key](void* base, const nlohmann::json& entry, std::vector<std::string>& changed) {
            LOG_DEBUG(Config, "[make_override] Processing key: {}", key);
            LOG_DEBUG(Config, "[make_override] Entry JSON: {}", entry.dump());
            Struct* obj = reinterpret_cast<Struct*>(base);
            Setting<T>& dst = obj->*member;
            try {
                T newValue = entry.get<T>();
                if constexpr (std::is_same_v<T, std::vector<int>>) {
                    std::ostringstream newValueStr;
                    newValueStr << "[";
                    for (size_t i = 0; i < newValue.size(); ++i) {
                        if (i > 0)
                            newValueStr << ", ";
                        newValueStr << newValue[i];
                    }
                    newValueStr << "]";
                    LOG_DEBUG(Config, "[make_override] Parsed value: {}", newValueStr.str());
                } else if constexpr (std::is_same_v<T, std::array<std::string, 4>>) {
                    std::ostringstream newValueStr;
                    newValueStr << "[";
                    for (size_t i = 0; i < newValue.size(); ++i) {
                        if (i > 0)
                            newValueStr << ", ";
                        newValueStr << newValue[i];
                    }
                    newValueStr << "]";
                    LOG_DEBUG(Config, "[make_override] Parsed value: {}", newValueStr.str());
                } else if constexpr (std::is_same_v<T, std::array<bool, 4>>) {
                    std::ostringstream newValueStr;
                    newValueStr << "[";
                    for (size_t i = 0; i < newValue.size(); ++i) {
                        if (i > 0)
                            newValueStr << ", ";
                        newValueStr << (newValue[i] ? "true" : "false");
                    }
                    newValueStr << "]";
                    LOG_DEBUG(Config, "[make_override] Parsed value: {}", newValueStr.str());
                } else {
                    LOG_DEBUG(Config, "[make_override] Parsed value: {}", newValue);
                }

                if constexpr (std::is_same_v<T, std::vector<int>>) {
                    std::ostringstream currentValueStr;
                    currentValueStr << "[";
                    for (size_t i = 0; i < dst.value.size(); ++i) {
                        if (i > 0)
                            currentValueStr << ", ";
                        currentValueStr << dst.value[i];
                    }
                    currentValueStr << "]";
                    LOG_DEBUG(Config, "[make_override] Current value: {}", currentValueStr.str());
                } else if constexpr (std::is_same_v<T, std::array<std::string, 4>>) {
                    std::ostringstream currentValueStr;
                    currentValueStr << "[";
                    for (size_t i = 0; i < dst.value.size(); ++i) {
                        if (i > 0)
                            currentValueStr << ", ";
                        currentValueStr << dst.value[i];
                    }
                    currentValueStr << "]";
                    LOG_DEBUG(Config, "[make_override] Current value: {}", currentValueStr.str());
                } else if constexpr (std::is_same_v<T, std::array<bool, 4>>) {
                    std::ostringstream currentValueStr;
                    currentValueStr << "[";
                    for (size_t i = 0; i < dst.value.size(); ++i) {
                        if (i > 0)
                            currentValueStr << ", ";
                        currentValueStr << (dst.value[i] ? "true" : "false");
                    }
                    currentValueStr << "]";
                    LOG_DEBUG(Config, "[make_override] Current value: {}", currentValueStr.str());
                } else {
                    LOG_DEBUG(Config, "[make_override] Current value: {}", dst.value);
                }
                if (dst.value != newValue) {
                    std::ostringstream oss;
                    if constexpr (std::is_same_v<T, std::vector<int>>) {
                        oss << key << " ( [";
                        for (size_t i = 0; i < dst.value.size(); ++i) {
                            if (i > 0)
                                oss << ", ";
                            oss << dst.value[i];
                        }
                        oss << "] → [";
                        for (size_t i = 0; i < newValue.size(); ++i) {
                            if (i > 0)
                                oss << ", ";
                            oss << newValue[i];
                        }
                        oss << "] )";
                    } else if constexpr (std::is_same_v<T, std::array<std::string, 4>>) {
                        oss << key << " ( [";
                        for (size_t i = 0; i < dst.value.size(); ++i) {
                            if (i > 0)
                                oss << ", ";
                            oss << dst.value[i];
                        }
                        oss << "] → [";
                        for (size_t i = 0; i < newValue.size(); ++i) {
                            if (i > 0)
                                oss << ", ";
                            oss << newValue[i];
                        }
                        oss << "] )";
                    } else if constexpr (std::is_same_v<T, std::array<bool, 4>>) {
                        oss << key << " ( [";
                        for (size_t i = 0; i < dst.value.size(); ++i) {
                            if (i > 0)
                                oss << ", ";
                            oss << (dst.value[i] ? "true" : "false");
                        }
                        oss << "] → [";
                        for (size_t i = 0; i < newValue.size(); ++i) {
                            if (i > 0)
                                oss << ", ";
                            oss << (newValue[i] ? "true" : "false");
                        }
                        oss << "] )";
                    } else {
                        oss << key << " ( " << dst.value << " → " << newValue << " )";
                    }
                    changed.push_back(oss.str());
                    LOG_DEBUG(Config, "[make_override] Recorded change: {}", oss.str());
                }
                dst.game_specific_value = newValue;
                LOG_DEBUG(Config, "[make_override] Successfully updated {}", key);
            } catch (const std::exception& e) {
                LOG_ERROR(Config, "[make_override] error parsing {}: {}", key, e.what());
                LOG_ERROR(Config, "[make_override] Entry was: {}", entry.dump());
                LOG_ERROR(Config, "[make_override] Type name: {}", entry.type_name());
            }
        },

        // --- get_for_save -------------------------------------------
        // Returns game_specific_value when present, otherwise base value.
        // This means a freshly-opened game-specific dialog still shows
        // useful (current-global) values rather than empty entries.
        [member](const void* base) -> nlohmann::json {
            const Struct* obj = reinterpret_cast<const Struct*>(base);
            const Setting<T>& src = obj->*member;
            return nlohmann::json(src.game_specific_value.value_or(src.value));
        },

        // --- reset_game_specific ------------------------------------
        [member](void* base) {
            Struct* obj = reinterpret_cast<Struct*>(base);
            (obj->*member).reset_game_specific();
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
    Setting<std::filesystem::path> font_dir;

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
    Setting<bool> identical_log_grouped{true};
    Setting<bool> connected_to_network{false};
    Setting<bool> discord_rpc_enabled{false};
    Setting<bool> show_fps_counter{false};
    Setting<int> console_language{1};

    // Additional missing settings from config.cpp
    Setting<bool> enable_auto_backup{false};
    Setting<bool> restart_with_base_game{false};
    Setting<bool> separate_update_enabled{false};
    Setting<bool> screen_tip_disable{false};
    Setting<bool> mute_enabled{false};
    Setting<bool> play_bgm{false};
    Setting<int> bgm_volume{50};
    Setting<std::array<std::string, 4>> user_names{
        {"shadPS4", "shadps4-2", "shadPS4-3", "shadPS4-4"}};
    Setting<std::array<bool, 4>> player_enabled_states{{true, true, true, true}};
    Setting<std::string> update_channel{"Shadlix"};
    Setting<bool> auto_update{false};
    Setting<bool> pause_on_unfocus{false};
    Setting<bool> show_welcome_dialog{true};
    Setting<bool> always_show_changelog{false};
    Setting<bool> disable_hardcoded_hotkeys{false};
    Setting<bool> use_home_button_for_hotkeys{false};
    Setting<bool> enable_mods{true};
    Setting<bool> enable_updates{true};
    Setting<bool> compatibility_enabled{false};
    Setting<bool> check_compatibility_on_startup{false};
    Setting<std::string> http_host_override{"localhost"};
    Setting<bool> first_boot_handled{false};
    Setting<std::string> choose_home_tab{"General"};
    Setting<std::string> default_controller_id{""};
    Setting<std::string> active_controller_id{""};
    Setting<int> cpu_core_mode{0}; // 0 = All cores
    Setting<std::vector<int>> custom_cpu_cores{};
    Setting<int> big_picture_scale{1000};
    Setting<std::string> shadnet_server{""};

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
            make_override<GeneralSettings>("identical_log_grouped",
                                           &GeneralSettings::identical_log_grouped),
            make_override<GeneralSettings>("show_splash", &GeneralSettings::show_splash),
            make_override<GeneralSettings>("trophy_notification_side",
                                           &GeneralSettings::trophy_notification_side),
            make_override<GeneralSettings>("connected_to_network",
                                           &GeneralSettings::connected_to_network),
            // Additional overrideable fields
            make_override<GeneralSettings>("enable_auto_backup",
                                           &GeneralSettings::enable_auto_backup),
            make_override<GeneralSettings>("restart_with_base_game",
                                           &GeneralSettings::restart_with_base_game),
            make_override<GeneralSettings>("separate_update_enabled",
                                           &GeneralSettings::separate_update_enabled),
            make_override<GeneralSettings>("screen_tip_disable",
                                           &GeneralSettings::screen_tip_disable),
            make_override<GeneralSettings>("mute_enabled", &GeneralSettings::mute_enabled),
            make_override<GeneralSettings>("play_bgm", &GeneralSettings::play_bgm),
            make_override<GeneralSettings>("bgm_volume", &GeneralSettings::bgm_volume),
            make_override<GeneralSettings>("pause_on_unfocus", &GeneralSettings::pause_on_unfocus),
            make_override<GeneralSettings>("disable_hardcoded_hotkeys",
                                           &GeneralSettings::disable_hardcoded_hotkeys),
            make_override<GeneralSettings>("use_home_button_for_hotkeys",
                                           &GeneralSettings::use_home_button_for_hotkeys),
            make_override<GeneralSettings>("enable_mods", &GeneralSettings::enable_mods),
            make_override<GeneralSettings>("enable_updates", &GeneralSettings::enable_updates),
            make_override<GeneralSettings>("http_host_override",
                                           &GeneralSettings::http_host_override),
            make_override<GeneralSettings>("cpu_core_mode", &GeneralSettings::cpu_core_mode),
            make_override<GeneralSettings>("custom_cpu_cores", &GeneralSettings::custom_cpu_cores),
            make_override<GeneralSettings>("big_picture_scale",
                                           &GeneralSettings::big_picture_scale)};
    }
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    GeneralSettings, install_dirs, addon_install_dir, home_dir, sys_modules_dir, font_dir,
    volume_slider, neo_mode, dev_kit_mode, extra_dmem_in_mbytes, psn_signed_in,
    trophy_popup_disabled, trophy_notification_duration, log_filter, log_type, show_splash,
    identical_log_grouped, trophy_notification_side, connected_to_network, discord_rpc_enabled,
    show_fps_counter, console_language, enable_auto_backup, restart_with_base_game,
    separate_update_enabled, screen_tip_disable, mute_enabled, play_bgm, bgm_volume, user_names,
    player_enabled_states, update_channel, auto_update, pause_on_unfocus, show_welcome_dialog,
    always_show_changelog, disable_hardcoded_hotkeys, use_home_button_for_hotkeys, enable_mods,
    enable_updates, compatibility_enabled, check_compatibility_on_startup, http_host_override,
    first_boot_handled, choose_home_tab, default_controller_id, active_controller_id, cpu_core_mode,
    custom_cpu_cores, big_picture_scale)

// -------------------------------
// Debug settings
// -------------------------------
struct DebugSettings {
    Setting<bool> separate_logging_enabled{false}; // specific
    Setting<bool> debug_dump{false};               // specific
    Setting<bool> shader_collect{false};           // specific
    Setting<bool> shader_debug{false};             // specific
    Setting<bool> shader_skips_enabled{false};     // specific
    Setting<bool> fps_color_state{false};          // specific
    Setting<bool> log_enabled{true};               // specific
    Setting<std::string> config_version{""};       // specific

    std::vector<OverrideItem> GetOverrideableFields() const {
        return std::vector<OverrideItem>{
            make_override<DebugSettings>("debug_dump", &DebugSettings::debug_dump),
            make_override<DebugSettings>("shader_collect", &DebugSettings::shader_collect),
            make_override<DebugSettings>("shader_debug", &DebugSettings::shader_debug),
            make_override<DebugSettings>("shader_skips_enabled",
                                         &DebugSettings::shader_skips_enabled),
            make_override<DebugSettings>("separate_logging_enabled",
                                         &DebugSettings::separate_logging_enabled),
            make_override<DebugSettings>("log_enabled", &DebugSettings::log_enabled)};
    }
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DebugSettings, separate_logging_enabled, debug_dump,
                                   shader_collect, shader_debug, shader_skips_enabled,
                                   fps_color_state, log_enabled, config_version)

// -------------------------------
// Input settings
// -------------------------------

struct InputSettings {
    Setting<int> cursor_state{HideCursorState::Idle};      // specific
    Setting<int> cursor_hide_timeout{5};                   // specific
    Setting<int> usb_device_backend{UsbBackendType::Real}; // specific
    Setting<bool> use_special_pad{false};
    Setting<int> special_pad_class{1};
    Setting<bool> motion_controls_enabled{true}; // specific
    Setting<bool> use_unified_input_config{true};
    Setting<std::string> default_controller_id{""};
    Setting<bool> background_controller_input{false}; // specific
    Setting<bool> ime_accessibility_enabled{false};   // specific
    Setting<bool> ime_url_mail_short_panel{false};    // specific
    Setting<bool> is_circle_enter{false};             // specific
    Setting<s32> camera_id{-1};
    Setting<bool> use_mice_as_mice{false};

    std::vector<OverrideItem> GetOverrideableFields() const {
        return std::vector<OverrideItem>{
            make_override<InputSettings>("cursor_state", &InputSettings::cursor_state),
            make_override<InputSettings>("cursor_hide_timeout",
                                         &InputSettings::cursor_hide_timeout),
            make_override<InputSettings>("usb_device_backend", &InputSettings::usb_device_backend),
            make_override<InputSettings>("motion_controls_enabled",
                                         &InputSettings::motion_controls_enabled),
            make_override<InputSettings>("background_controller_input",
                                         &InputSettings::background_controller_input),
            make_override<InputSettings>("ime_accessibility_enabled",
                                         &InputSettings::ime_accessibility_enabled),
            make_override<InputSettings>("ime_url_mail_short_panel",
                                         &InputSettings::ime_url_mail_short_panel),
            make_override<InputSettings>("is_circle_enter", &InputSettings::is_circle_enter),
            make_override<InputSettings>("camera_id", &InputSettings::camera_id),
            make_override<InputSettings>("use_mice_as_mice", &InputSettings::use_mice_as_mice)};
    }
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(InputSettings, cursor_state, cursor_hide_timeout,
                                   usb_device_backend, use_special_pad, special_pad_class,
                                   motion_controls_enabled, use_unified_input_config,
                                   default_controller_id, background_controller_input,
                                   ime_accessibility_enabled, ime_url_mail_short_panel, camera_id,
                                   is_circle_enter, use_mice_as_mice)
// -------------------------------
// Audio settings
// -------------------------------
struct AudioSettings {
    Setting<u32> audio_backend{AudioBackend::SDL};
    Setting<std::string> sdl_mic_device{"Default Device"};
    Setting<std::string> sdl_main_output_device{"Default Device"};
    Setting<std::string> sdl_padSpk_output_device{"Default Device"};
    Setting<std::string> openal_mic_device{"Default Device"};
    Setting<std::string> openal_main_output_device{"Default Device"};
    Setting<std::string> openal_padSpk_output_device{"Default Device"};
    Setting<u32> openal_hrtf{OpenALHrtfMode::HrtfAuto};
    Setting<u32> openal_output_mode{OpenALOutputMode::OutputAuto};

    std::vector<OverrideItem> GetOverrideableFields() const {
        return std::vector<OverrideItem>{
            make_override<AudioSettings>("audio_backend", &AudioSettings::audio_backend),
            make_override<AudioSettings>("sdl_mic_device", &AudioSettings::sdl_mic_device),
            make_override<AudioSettings>("sdl_main_output_device",
                                         &AudioSettings::sdl_main_output_device),
            make_override<AudioSettings>("sdl_padSpk_output_device",
                                         &AudioSettings::sdl_padSpk_output_device),
            make_override<AudioSettings>("openal_mic_device", &AudioSettings::openal_mic_device),
            make_override<AudioSettings>("openal_main_output_device",
                                         &AudioSettings::openal_main_output_device),
            make_override<AudioSettings>("openal_padSpk_output_device",
                                         &AudioSettings::openal_padSpk_output_device),
            make_override<AudioSettings>("openal_hrtf", &AudioSettings::openal_hrtf),
            make_override<AudioSettings>("openal_output_mode", &AudioSettings::openal_output_mode)};
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AudioSettings, audio_backend, sdl_mic_device,
                                   sdl_main_output_device, sdl_padSpk_output_device,
                                   openal_mic_device, openal_main_output_device,
                                   openal_padSpk_output_device, openal_hrtf, openal_output_mode)

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
    Setting<u32> readbacks_mode{GpuReadbacksMode::Disabled};
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
    Setting<bool> userfaultfd{false};
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
            make_override<GPUSettings>("readbacks_mode", &GPUSettings::readbacks_mode),
            make_override<GPUSettings>("readback_linear_images_enabled",
                                       &GPUSettings::readback_linear_images_enabled),
            make_override<GPUSettings>("direct_memory_access_enabled",
                                       &GPUSettings::direct_memory_access_enabled),
            make_override<GPUSettings>("vblank_frequency", &GPUSettings::vblank_frequency),
        };
    }
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GPUSettings, window_width, window_height, internal_screen_width,
                                   internal_screen_height, null_gpu, copy_gpu_buffers,
                                   readbacks_mode, readback_linear_images_enabled,
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
// Main manager
// -------------------------------
class EmulatorSettingsImpl {
public:
    EmulatorSettingsImpl();
    ~EmulatorSettingsImpl();

    static std::shared_ptr<EmulatorSettingsImpl> GetInstance();
    static void SetInstance(std::shared_ptr<EmulatorSettingsImpl> instance);

    bool Save(const std::string& serial = "");
    bool Load(const std::string& serial = "");
    void SetDefaultValues();
    bool TransferSettings();

    // Config mode
    ConfigMode GetConfigMode() const {
        return m_configMode;
    }
    void SetConfigMode(ConfigMode mode) {
        m_configMode = mode;
    }

    //
    // Game-specific override management
    /// Clears all per-game overrides.  Call this when a game exits so
    /// the emulator reverts to global settings.
    void ClearGameSpecificOverrides();

    /// Reset a single field's game-specific override by its JSON ke
    void ResetGameSpecificValue(const std::string& key);

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
    std::filesystem::path GetFontsDir();
    void SetFontsDir(const std::filesystem::path& dir);
    std::filesystem::path GetAddonInstallDir();
    void SetAddonInstallDir(const std::filesystem::path& dir);

private:
    GeneralSettings m_general{};
    DebugSettings m_debug{};
    InputSettings m_input{};
    AudioSettings m_audio{};
    GPUSettings m_gpu{};
    VulkanSettings m_vulkan{};
    ConfigMode m_configMode{ConfigMode::Default};

    bool m_loaded{false};

    static std::shared_ptr<EmulatorSettingsImpl> s_instance;
    static std::mutex s_mutex;

    /// Apply overrideable fields from groupJson into group.game_specific_value.
    template <typename Group>
    void ApplyGroupOverrides(Group& group, const nlohmann::json& groupJson,
                             std::vector<std::string>& changed) {
        for (auto& item : group.GetOverrideableFields()) {
            if (!groupJson.contains(item.key))
                continue;
            item.apply(&group, groupJson.at(item.key), changed);
        }
    }

    // Write all overrideable fields from group into out (for game-specific save).
    template <typename Group>
    static void SaveGroupGameSpecific(const Group& group, nlohmann::json& out) {
        for (auto& item : group.GetOverrideableFields())
            out[item.key] = item.get_for_save(&group);
    }

    // Discard every game-specific override in group.
    template <typename Group>
    static void ClearGroupOverrides(Group& group) {
        for (auto& item : group.GetOverrideableFields())
            item.reset_game_specific(&group);
    }

    static void PrintChangedSummary(const std::vector<std::string>& changed);

public:
    // Add these getters to access overrideable fields
    std::vector<OverrideItem> GetGeneralOverrideableFields() const {
        return m_general.GetOverrideableFields();
    }
    std::vector<OverrideItem> GetDebugOverrideableFields() const {
        return m_debug.GetOverrideableFields();
    }
    std::vector<OverrideItem> GetInputOverrideableFields() const {
        return m_input.GetOverrideableFields();
    }
    std::vector<OverrideItem> GetAudioOverrideableFields() const {
        return m_audio.GetOverrideableFields();
    }
    std::vector<OverrideItem> GetGPUOverrideableFields() const {
        return m_gpu.GetOverrideableFields();
    }
    std::vector<OverrideItem> GetVulkanOverrideableFields() const {
        return m_vulkan.GetOverrideableFields();
    }
    std::vector<std::string> GetAllOverrideableKeys() const;

#define SETTING_FORWARD(group, Name, field)                                                        \
    auto Get##Name() const {                                                                       \
        return (group).field.get(m_configMode);                                                    \
    }                                                                                              \
    void Set##Name(const decltype((group).field.value)& v, bool specific = false) {                \
        (group).field.set(v, specific);                                                            \
    }
#define SETTING_FORWARD_BOOL(group, Name, field)                                                   \
    bool Is##Name() const {                                                                        \
        return (group).field.get(m_configMode);                                                    \
    }                                                                                              \
    void Set##Name(bool v, bool specific = false) {                                                \
        (group).field.set(v, specific);                                                            \
    }
#define SETTING_FORWARD_BOOL_READONLY(group, Name, field)                                          \
    bool Is##Name() const {                                                                        \
        return (group).field.get(m_configMode);                                                    \
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
    SETTING_FORWARD_BOOL(m_general, IdenticalLogGrouped, identical_log_grouped)
    SETTING_FORWARD(m_general, LogFilter, log_filter)
    SETTING_FORWARD(m_general, LogType, log_type)
    SETTING_FORWARD_BOOL(m_general, ConnectedToNetwork, connected_to_network)
    SETTING_FORWARD_BOOL(m_general, DiscordRPCEnabled, discord_rpc_enabled)
    SETTING_FORWARD_BOOL(m_general, ShowFpsCounter, show_fps_counter)
    SETTING_FORWARD(m_general, ConsoleLanguage, console_language)

    // Additional general settings
    SETTING_FORWARD_BOOL(m_general, EnableAutoBackup, enable_auto_backup)
    SETTING_FORWARD_BOOL(m_general, RestartWithBaseGame, restart_with_base_game)
    SETTING_FORWARD_BOOL(m_general, SeparateUpdateEnabled, separate_update_enabled)
    SETTING_FORWARD_BOOL(m_general, ScreenTipDisable, screen_tip_disable)
    SETTING_FORWARD_BOOL(m_general, MuteEnabled, mute_enabled)
    SETTING_FORWARD_BOOL(m_general, PlayBGM, play_bgm)
    SETTING_FORWARD(m_general, BGMVolume, bgm_volume)
    SETTING_FORWARD(m_general, UserNames, user_names)
    SETTING_FORWARD(m_general, PlayerEnabledStates, player_enabled_states)
    SETTING_FORWARD(m_general, UpdateChannel, update_channel)
    SETTING_FORWARD_BOOL(m_general, AutoUpdate, auto_update)
    SETTING_FORWARD_BOOL(m_general, PauseOnUnfocus, pause_on_unfocus)
    SETTING_FORWARD_BOOL(m_general, ShowWelcomeDialog, show_welcome_dialog)
    SETTING_FORWARD_BOOL(m_general, AlwaysShowChangelog, always_show_changelog)
    SETTING_FORWARD_BOOL(m_general, DisableHardcodedHotkeys, disable_hardcoded_hotkeys)
    SETTING_FORWARD_BOOL(m_general, UseHomeButtonForHotkeys, use_home_button_for_hotkeys)
    SETTING_FORWARD_BOOL(m_general, EnableMods, enable_mods)
    SETTING_FORWARD_BOOL(m_general, EnableUpdates, enable_updates)
    SETTING_FORWARD_BOOL(m_general, CompatibilityEnabled, compatibility_enabled)
    SETTING_FORWARD_BOOL(m_general, CheckCompatibilityOnStartup, check_compatibility_on_startup)
    SETTING_FORWARD(m_general, HttpHostOverride, http_host_override)
    SETTING_FORWARD_BOOL(m_general, FirstBootHandled, first_boot_handled)
    SETTING_FORWARD(m_general, ChooseHomeTab, choose_home_tab)
    SETTING_FORWARD(m_general, ActiveControllerId, active_controller_id)
    SETTING_FORWARD(m_general, CpuCoreMode, cpu_core_mode)
    SETTING_FORWARD(m_general, CustomCpuCores, custom_cpu_cores)
    SETTING_FORWARD(m_general, BigPictureScale, big_picture_scale)
    SETTING_FORWARD(m_general, ShadNetServer, shadnet_server)

    // Audio settings
    SETTING_FORWARD(m_audio, AudioBackend, audio_backend)
    SETTING_FORWARD(m_audio, SDLMicDevice, sdl_mic_device)
    SETTING_FORWARD(m_audio, SDLMainOutputDevice, sdl_main_output_device)
    SETTING_FORWARD(m_audio, SDLPadSpkOutputDevice, sdl_padSpk_output_device)
    SETTING_FORWARD(m_audio, OpenALMicDevice, openal_mic_device)
    SETTING_FORWARD(m_audio, OpenALMainOutputDevice, openal_main_output_device)
    SETTING_FORWARD(m_audio, OpenALPadSpkOutputDevice, openal_padSpk_output_device)
    SETTING_FORWARD(m_audio, OpenALHrtf, openal_hrtf)
    SETTING_FORWARD(m_audio, OpenALOutputMode, openal_output_mode)

    // Debug settings
    SETTING_FORWARD_BOOL(m_debug, SeparateLoggingEnabled, separate_logging_enabled)
    SETTING_FORWARD_BOOL(m_debug, DebugDump, debug_dump)
    SETTING_FORWARD_BOOL(m_debug, ShaderCollect, shader_collect)
    SETTING_FORWARD_BOOL(m_debug, ShaderDebug, shader_debug)
    SETTING_FORWARD_BOOL(m_debug, ShaderSkipsEnabled, shader_skips_enabled)
    SETTING_FORWARD_BOOL(m_debug, FpsColorState, fps_color_state)
    SETTING_FORWARD_BOOL(m_debug, LogEnabled, log_enabled)

    std::string GetConfigVersion() const {
        return m_debug.config_version.get();
    }
    void SetConfigVersion(const std::string& version) {
        m_debug.config_version.value = version;
    }
    void SetConfigVersion(const char* version) {
        m_debug.config_version.value = std::string(version);
    }

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
    SETTING_FORWARD(m_gpu, ReadbacksMode, readbacks_mode)
    SETTING_FORWARD_BOOL(m_gpu, ReadbackLinearImagesEnabled, readback_linear_images_enabled)
    SETTING_FORWARD_BOOL(m_gpu, DirectMemoryAccessEnabled, direct_memory_access_enabled)
    SETTING_FORWARD_BOOL_READONLY(m_gpu, PatchShaders, patch_shaders)
    SETTING_FORWARD_BOOL(m_gpu, UserfaultfdTracking, userfaultfd)

    u32 GetVblankFrequency() {
        if (m_gpu.vblank_frequency.value < 30) {
            return 30;
        }
        return m_gpu.vblank_frequency.get();
    }
    void SetVblankFrequency(const u32& v, bool is_specific = false) {
        u32 val = v < 30 ? 30 : v;
        if (is_specific) {
            m_gpu.vblank_frequency.game_specific_value = val;
        } else {
            m_gpu.vblank_frequency.value = val;
        }
    }

    // Input Settings
    SETTING_FORWARD(m_input, CursorState, cursor_state)
    SETTING_FORWARD(m_input, CursorHideTimeout, cursor_hide_timeout)
    SETTING_FORWARD(m_input, UsbDeviceBackend, usb_device_backend)
    SETTING_FORWARD_BOOL(m_input, MotionControlsEnabled, motion_controls_enabled)
    SETTING_FORWARD_BOOL(m_input, BackgroundControllerInput, background_controller_input)
    SETTING_FORWARD_BOOL(m_input, ImeAccessibilityEnabled, ime_accessibility_enabled)
    SETTING_FORWARD_BOOL(m_input, ImeUrlMailShortPanel, ime_url_mail_short_panel)
    SETTING_FORWARD(m_input, DefaultControllerId, default_controller_id)
    SETTING_FORWARD(m_input, CameraId, camera_id)
    SETTING_FORWARD_BOOL(m_input, UsingSpecialPad, use_special_pad)
    SETTING_FORWARD(m_input, SpecialPadClass, special_pad_class)
    SETTING_FORWARD_BOOL(m_input, UseUnifiedInputConfig, use_unified_input_config)
    SETTING_FORWARD_BOOL(m_input, CircleEnter, is_circle_enter)
    SETTING_FORWARD_BOOL(m_input, MiceUsedAsMice, use_mice_as_mice)

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
#undef SETTING_FORWARD_BOOL_READONLY
};
