// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <filesystem>
#include <iostream>
#include <common/path_util.h>
#include "emulator_settings.h"
#include "user_manager.h"

bool UserManager::AddUser(const User& user) {
    for (const auto& u : m_users.user) {
        if (u.user_id == user.user_id)
            return false; // already exists
    }

    m_users.user.push_back(user);

    // Create user home directory and subfolders
    const auto user_dir =
        EmulatorSettings::GetInstance()->GetHomeDir() / std::to_string(user.user_id);

    std::error_code ec;
    if (!std::filesystem::exists(user_dir)) {
        std::filesystem::create_directory(user_dir, ec);
        std::filesystem::create_directory(user_dir / "savedata", ec);
        std::filesystem::create_directory(user_dir / "trophy", ec);
        std::filesystem::create_directory(user_dir / "trophy/data", ec);
    }

    return true;
}

bool UserManager::RemoveUser(s32 user_id) {
    auto it = std::remove_if(m_users.user.begin(), m_users.user.end(),
                             [user_id](const User& u) { return u.user_id == user_id; });
    if (it == m_users.user.end())
        return false; // not found

    const auto user_dir = EmulatorSettings::GetInstance()->GetHomeDir() / std::to_string(user_id);

    if (std::filesystem::exists(user_dir)) {
        std::error_code ec;
        std::filesystem::remove_all(user_dir, ec);
    }

    m_users.user.erase(it, m_users.user.end());
    return true;
}

bool UserManager::RenameUser(s32 user_id, const std::string& new_name) {
    // Find user in the internal list
    for (auto& user : m_users.user) {
        if (user.user_id == user_id) {
            if (user.user_name == new_name)
                return true; // no change

            user.user_name = new_name;
            return true;
        }
    }
    return false;
}

User* UserManager::GetUserByID(s32 user_id) {
    for (auto& u : m_users.user) {
        if (u.user_id == user_id)
            return &u;
    }
    return nullptr;
}

const std::vector<User>& UserManager::GetAllUsers() const {
    return m_users.user;
}

std::vector<User> UserManager::CreateDefaultUser() {
    User default_user;
    default_user.user_id = 1;
    default_user.user_color = 0; // BLUE
    default_user.user_name = "shadPS4";
    default_user.controller_port = 1;

    const auto user_dir =
        EmulatorSettings::GetInstance()->GetHomeDir() / std::to_string(default_user.user_id);

    if (!std::filesystem::exists(user_dir)) {
        std::filesystem::create_directory(user_dir);
        std::filesystem::create_directory(user_dir / "savedata");
        std::filesystem::create_directory(user_dir / "trophy");
        std::filesystem::create_directory(user_dir / "trophy/data");
    }

    return {default_user};
}

bool UserManager::SetDefaultUser(u32 user_id) {
    auto it = std::find_if(m_users.user.begin(), m_users.user.end(),
                           [user_id](const User& u) { return u.user_id == user_id; });
    if (it == m_users.user.end())
        return false;

    m_users.default_user_id = user_id;
    SetControllerPort(user_id, 1); // Set default user to port 1
    return true;
}

void UserManager::SetControllerPort(u32 user_id, int port) {
    for (auto& u : m_users.user) {
        if (u.user_id != user_id && u.controller_port == port)
            u.controller_port = -1;
        if (u.user_id == user_id)
            u.controller_port = port;
    }
}
// Returns a list of users that have valid home directories
std::vector<User> UserManager::GetValidUsers() const {
    std::vector<User> result;
    result.reserve(m_users.user.size());

    const auto home_dir = EmulatorSettings::GetInstance()->GetHomeDir();

    for (const auto& user : m_users.user) {
        const auto user_dir = home_dir / std::to_string(user.user_id);
        if (std::filesystem::exists(user_dir)) {
            result.push_back(user);
        }
    }

    return result;
}