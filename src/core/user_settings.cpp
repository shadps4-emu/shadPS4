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
    if (m_loaded)
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
        j["Users"]["commit_hash"] = std::string(Common::g_scm_rev);

        json existing = json::object();
        if (std::ifstream existingIn{path}; existingIn.good()) {
            try {
                existingIn >> existing;
            } catch (...) {
                existing = json::object();
            }
        }

        if (existing.contains("Users") && existing["Users"].is_object())
            existing["Users"].update(j["Users"]);
        else
            existing["Users"] = j["Users"];

        std::ofstream out(path);
        if (!out) {
            LOG_ERROR(Config, "Failed to open user settings for writing: {}", path.string());
            return false;
        }
        out << std::setw(2) << existing;
        return !out.fail();
    } catch (const std::exception& e) {
        LOG_ERROR(Config, "Error saving user settings: {}", e.what());
        return false;
    }
}

bool UserSettingsImpl::Load() {
    const auto path = Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "users.json";
    try {
        if (!std::filesystem::exists(path)) {
            LOG_DEBUG(Config, "User settings file not found: {}", path.string());
            if (m_userManager.GetUsers().user.empty())
                m_userManager.GetUsers() = m_userManager.CreateDefaultUsers();
            m_loaded = true;
            Save();
            return false;
        }

        std::ifstream in(path);
        if (!in) {
            LOG_ERROR(Config, "Failed to open user settings: {}", path.string());
            return false;
        }

        json j;
        in >> j;

        auto default_users = m_userManager.CreateDefaultUsers();
        json default_json;
        default_json["Users"] = default_users;

        if (j.contains("Users")) {
            json current = default_json["Users"];
            current.update(j["Users"]);
            m_userManager.GetUsers() = current.get<Users>();
        } else {
            m_userManager.GetUsers() = default_users;
        }

        LOG_DEBUG(Config, "User settings loaded successfully");

        m_loaded = true;
        if (m_userManager.GetUsers().commit_hash != Common::g_scm_rev)
            Save();

        return true;
    } catch (const std::exception& e) {
        LOG_ERROR(Config, "Error loading user settings: {}", e.what());
        if (m_userManager.GetUsers().user.empty())
            m_userManager.GetUsers() = m_userManager.CreateDefaultUsers();
        return false;
    }
}