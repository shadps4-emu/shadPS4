// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
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
            if (m_userManager.GetUsers().user.empty())
                m_userManager.GetUsers().user = m_userManager.CreateDefaultUser();
            Save();
            return false;
        }

        std::ifstream in(path);
        if (!in) {
            LOG_ERROR(EmuSettings, "Failed to open user settings: {}", path.string());
            return false;
        }

        json j;
        in >> j;

        if (j.contains("Users")) {
            m_userManager.GetUsers() = j.at("Users").get<Users>();
            LOG_DEBUG(EmuSettings, "User settings loaded successfully");
            return true;
        }

        return false;
    } catch (const std::exception& e) {
        LOG_ERROR(EmuSettings, "Error loading user settings: {}", e.what());
        return false;
    }
}