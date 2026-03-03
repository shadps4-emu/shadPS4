// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <map>
#include <common/path_util.h>
#include <common/scm_rev.h>
#include "common/logging/log.h"
#include "user_settings.h"

using json = nlohmann::json;

// Singleton storage
std::shared_ptr<UserSettingsImpl> UserSettingsImpl::s_instance = nullptr;
std::mutex UserSettingsImpl::s_mutex;

// Singleton
UserSettingsImpl::UserSettingsImpl() = default;

UserSettingsImpl::~UserSettingsImpl() {
    Save();
}

std::shared_ptr<UserSettingsImpl> UserSettingsImpl::GetInstance() {
    std::lock_guard lock(s_mutex);
    if (!s_instance)
        s_instance = std::make_shared<UserSettingsImpl>();
    return s_instance;
}

void UserSettingsImpl::SetInstance(std::shared_ptr<UserSettingsImpl> instance) {
    std::lock_guard lock(s_mutex);
    s_instance = std::move(instance);
}

bool UserSettingsImpl::Save() const {
    const auto path = Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "users.json";
    try {
        json j;
        j["Users"] = m_userManager.GetUsers();

        std::ofstream out(path);
        if (!out) {
            LOG_ERROR(EmuSettings, "Failed to open user settings for writing: {}", path.string());
            return false;
        }
        out << std::setw(4) << j;
        return !out.fail();
    } catch (const std::exception& e) {
        LOG_ERROR(EmuSettings, "Error saving user settings: {}", e.what());
        return false;
    }
}

bool UserSettingsImpl::Load() {
    const auto path = Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "users.json";
    try {
        if (!std::filesystem::exists(path)) {
            LOG_DEBUG(EmuSettings, "User settings file not found: {}", path.string());
            // Create default user if no file exists
            if (m_userManager.GetUsers().user.empty()) {
                m_userManager.GetUsers().user = m_userManager.CreateDefaultUser();
                m_userManager.GetUsers().default_user_id = 1;
            }
            Save(); // Save default users
            return false;
        }

        std::ifstream in(path);
        if (!in) {
            LOG_ERROR(EmuSettings, "Failed to open user settings: {}", path.string());
            return false;
        }

        json j;
        in >> j;

        // Create a default Users object
        Users default_users;
        default_users.default_user_id = 1;
        if (default_users.user.empty()) {
            User default_user;
            default_user.user_id = 1;
            default_user.user_color = 0;
            default_user.user_name = "shadPS4";
            default_user.controller_port = 1;
            default_users.user.push_back(default_user);
        }

        // Convert default_users to json for merging
        json default_json;
        default_json["Users"] = default_users;

        // Merge the loaded json with defaults (preserves existing data, adds missing fields)
        if (j.contains("Users")) {
            json current = default_json["Users"];
            current.update(j["Users"]);
            m_userManager.GetUsers() = current.get<Users>();
        } else {
            m_userManager.GetUsers() = default_users;
        }

        LOG_DEBUG(EmuSettings, "User settings loaded successfully");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR(EmuSettings, "Error loading user settings: {}", e.what());
        // Fall back to defaults
        if (m_userManager.GetUsers().user.empty()) {
            m_userManager.GetUsers().user = m_userManager.CreateDefaultUser();
            m_userManager.GetUsers().default_user_id = 1;
        }
        return false;
    }
}