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
// General settings
// -------------------------------
struct GeneralSettings {
    Setting<std::filesystem::path> sys_fonts_dir;
    std::vector<OverrideItem> GetOverrideableFields() const {
        return std::vector<OverrideItem>{};
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GeneralSettings, sys_fonts_dir)

// -------------------------------
// Debug settings
// -------------------------------
struct DebugSettings {
    std::vector<OverrideItem> GetOverrideableFields() const {
        return std::vector<OverrideItem>{};
    }
};
//NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DebugSettings)

// -------------------------------
// Input settings
// -------------------------------

struct InputSettings {
    std::vector<OverrideItem> GetOverrideableFields() const {
        return std::vector<OverrideItem>{};
    }
};
//NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(InputSettings)

// -------------------------------
// Audio settings
// -------------------------------
struct AudioSettings {
    std::vector<OverrideItem> GetOverrideableFields() const {
        return std::vector<OverrideItem>{};
    }
};

//NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AudioSettings)

// -------------------------------
// GPU settings
// -------------------------------
struct GPUSettings {
    std::vector<OverrideItem> GetOverrideableFields() const {
        return std::vector<OverrideItem>{};
    }
};
//NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GPUSettings)
// -------------------------------
// Vulkan settings
// -------------------------------
struct VulkanSettings {
    std::vector<OverrideItem> GetOverrideableFields() const {
        return std::vector<OverrideItem>{};
    }
};
//NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(VulkanSettings)

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
    std::filesystem::path GetSysFontsDir();
    void SetSysFontsDir(const std::filesystem::path& dir);

private:
    GeneralSettings m_general{};
    DebugSettings m_debug{};
    InputSettings m_input{};
    AudioSettings m_audio{};
    GPUSettings m_gpu{};
    VulkanSettings m_vulkan{};
    // UserManager m_userManager;

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

#undef SETTING_FORWARD
#undef SETTING_FORWARD_BOOL
};
