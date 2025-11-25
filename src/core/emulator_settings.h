// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
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

    Setting<int> volumeSlider{100};
    Setting<bool> isNeo{false};
    Setting<bool> isDevKit{false};
    Setting<int> extraDmemInMbytes{0};
    Setting<bool> isPSNSignedIn{false};
    Setting<bool> isTrophyPopupDisabled{false};
    Setting<double> trophyNotificationDuration{6.0};
    Setting<std::string> logFilter{""};
    Setting<std::string> logType{"sync"};
    Setting<bool> isShowSplash{false};
    Setting<std::string> isSideTrophy{"right"};
    Setting<bool> isConnectedToNetwork{false};
    Setting<bool> isDiscordRPCEnabled{false};

    // return a vector of override descriptors (runtime, but tiny)
    std::vector<OverrideItem> GetOverrideableFields() const {
        return std::vector<OverrideItem>{
            make_override<GeneralSettings>("volumeSlider", &GeneralSettings::volumeSlider),
            make_override<GeneralSettings>("isNeo", &GeneralSettings::isNeo),
            make_override<GeneralSettings>("isDevKit", &GeneralSettings::isDevKit),
            make_override<GeneralSettings>("extraDmemInMbytes",
                                           &GeneralSettings::extraDmemInMbytes),
            make_override<GeneralSettings>("isPSNSignedIn", &GeneralSettings::isPSNSignedIn),
            make_override<GeneralSettings>("isTrophyPopupDisabled",
                                           &GeneralSettings::isTrophyPopupDisabled),
            make_override<GeneralSettings>("trophyNotificationDuration",
                                           &GeneralSettings::trophyNotificationDuration),
            make_override<GeneralSettings>("logFilter", &GeneralSettings::logFilter),
            make_override<GeneralSettings>("logType", &GeneralSettings::logType),
            make_override<GeneralSettings>("isShowSplash", &GeneralSettings::isShowSplash),
            make_override<GeneralSettings>("isSideTrophy", &GeneralSettings::isSideTrophy),
            make_override<GeneralSettings>("isConnectedToNetwork",
                                           &GeneralSettings::isConnectedToNetwork)};
    }
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GeneralSettings, install_dirs, addon_install_dir, home_dir,
                                   sys_modules_dir, volumeSlider, isNeo, isDevKit,
                                   extraDmemInMbytes, isPSNSignedIn, isTrophyPopupDisabled,
                                   trophyNotificationDuration, logFilter, logType, isShowSplash,
                                   isSideTrophy, isConnectedToNetwork, isDiscordRPCEnabled)

// -------------------------------
// Debug settings
// -------------------------------
struct DebugSettings {
    Setting<bool> separate_logging_enabled{false}; // specific
    Setting<bool> DebugDump{false};                // specific
    Setting<bool> ShaderDebug{false};              // specific
    Setting<bool> FpsColor{true};
    Setting<bool> logEnabled{true}; // specific

    std::vector<OverrideItem> GetOverrideableFields() const {
        return std::vector<OverrideItem>{
            make_override<DebugSettings>("DebugDump", &DebugSettings::DebugDump),
            make_override<DebugSettings>("ShaderDebug", &DebugSettings::ShaderDebug),
            make_override<DebugSettings>("separate_logging_enabled",
                                         &DebugSettings::separate_logging_enabled),
            make_override<DebugSettings>("logEnabled", &DebugSettings::logEnabled)};
    }
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DebugSettings, separate_logging_enabled, DebugDump, ShaderDebug,
                                   FpsColor, logEnabled)

// -------------------------------
// Input settings
// -------------------------------
enum HideCursorState : int { Never, Idle, Always };

struct InputSettings {
    Setting<int> cursorState{HideCursorState::Idle}; // specific
    Setting<int> cursorHideTimeout{5};               // specific
    Setting<bool> useSpecialPad{false};
    Setting<int> specialPadClass{1};
    Setting<bool> isMotionControlsEnabled{true}; // specific
    Setting<bool> useUnifiedInputConfig{true};
    Setting<std::string> defaultControllerID{""};
    Setting<bool> backgroundControllerInput{false}; // specific

    std::vector<OverrideItem> GetOverrideableFields() const {
        return std::vector<OverrideItem>{
            make_override<InputSettings>("cursorState", &InputSettings::cursorState),
            make_override<InputSettings>("cursorHideTimeout", &InputSettings::cursorHideTimeout),
            make_override<InputSettings>("isMotionControlsEnabled",
                                         &InputSettings::isMotionControlsEnabled),
            make_override<InputSettings>("backgroundControllerInput",
                                         &InputSettings::backgroundControllerInput)};
    }
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(InputSettings, cursorState, cursorHideTimeout, useSpecialPad,
                                   specialPadClass, isMotionControlsEnabled, useUnifiedInputConfig,
                                   defaultControllerID, backgroundControllerInput)
// -------------------------------
// Audio settings
// -------------------------------
struct AudioSettings {
    Setting<std::string> micDevice{"Default Device"};
    Setting<std::string> mainOutputDevice{"Default Device"};
    Setting<std::string> padSpkOutputDevice{"Default Device"};

    // TODO add overrides
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AudioSettings, micDevice, mainOutputDevice, padSpkOutputDevice)

// -------------------------------
// GPU settings
// -------------------------------
struct GPUSettings {
    Setting<u32> windowWidth{1280};
    Setting<u32> windowHeight{720};
    Setting<u32> internalScreenWidth{1280};
    Setting<u32> internalScreenHeight{720};
    Setting<bool> isNullGpu{false};
    Setting<bool> shouldCopyGPUBuffers{false};
    Setting<bool> readbacksEnabled{false};
    Setting<bool> readbackLinearImagesEnabled{false};
    Setting<bool> directMemoryAccessEnabled{false};
    Setting<bool> shouldDumpShaders{false};
    Setting<bool> shouldPatchShaders{false};
    Setting<u32> vblankFrequency{60};
    Setting<bool> isFullscreen{false};
    Setting<std::string> fullscreenMode{"Windowed"};
    Setting<std::string> presentMode{"Mailbox"};
    Setting<bool> isHDRAllowed{false};
    Setting<bool> fsrEnabled{false};
    Setting<bool> rcasEnabled{true};
    Setting<int> rcasAttenuation{250};
    // TODO add overrides
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GPUSettings, windowWidth, windowHeight, internalScreenWidth,
                                   internalScreenHeight, isNullGpu, shouldCopyGPUBuffers,
                                   readbacksEnabled, readbackLinearImagesEnabled,
                                   directMemoryAccessEnabled, shouldDumpShaders, shouldPatchShaders,
                                   vblankFrequency, isFullscreen, fullscreenMode, presentMode,
                                   isHDRAllowed, fsrEnabled, rcasEnabled, rcasAttenuation)
// -------------------------------
// Vulkan settings
// -------------------------------
struct VulkanSettings {};

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

    // general accessors
    bool AddGameInstallDir(const std::filesystem::path& dir, bool enabled = true);
    std::vector<std::filesystem::path> GetGameInstallDirs() const;
    void SetAllGameInstallDirs(const std::vector<GameInstallDir>& dirs);
    std::filesystem::path GetHomeDir();
    void SetHomeDir(const std::filesystem::path& dir);
    std::filesystem::path GetSysModulesDir();
    void SetSysModulesDir(const std::filesystem::path& dir);

    // user helpers
    UserManager& GetUserManager() {
        return m_userManager;
    }
    const UserManager& GetUserManager() const {
        return m_userManager;
    }

private:
    GeneralSettings m_general{};
    DebugSettings m_debug{};
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
    SETTING_FORWARD(m_general, VolumeSlider, volumeSlider)
    SETTING_FORWARD_BOOL(m_general, Neo, isNeo)
    SETTING_FORWARD(m_general, AddonInstallDir, addon_install_dir)

    // Debug settings
    SETTING_FORWARD_BOOL(m_debug, SeparateLoggingEnabled, separate_logging_enabled)

#undef SETTING_FORWARD
#undef SETTING_FORWARD_BOOL
};
