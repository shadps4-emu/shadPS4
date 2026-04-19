// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <filesystem>
#include <iostream>
#include <common/path_util.h>
#include "emulator_settings.h"
#include "libraries/system/userservice.h"
#include "user_manager.h"
#include "user_settings.h"

bool UserManager::AddUser(const User& user) {
    for (const auto& u : m_users.user) {
        if (u.user_id == user.user_id)
            return false; // already exists
    }

    m_users.user.push_back(user);

    // Create user home directory and subfolders
    const auto user_dir = EmulatorSettings.GetHomeDir() / std::to_string(user.user_id);

    std::error_code ec;
    if (!std::filesystem::exists(user_dir)) {
        std::filesystem::create_directory(user_dir, ec);
        std::filesystem::create_directory(user_dir / "savedata", ec);
        std::filesystem::create_directory(user_dir / "trophy", ec);
        std::filesystem::create_directory(user_dir / "inputs", ec);
    }

    Save();
    return true;
}

bool UserManager::RemoveUser(s32 user_id) {
    auto it = std::remove_if(m_users.user.begin(), m_users.user.end(),
                             [user_id](const User& u) { return u.user_id == user_id; });
    if (it == m_users.user.end())
        return false; // not found

    const auto user_dir = EmulatorSettings.GetHomeDir() / std::to_string(user_id);

    if (std::filesystem::exists(user_dir)) {
        std::error_code ec;
        std::filesystem::remove_all(user_dir, ec);
    }

    m_users.user.erase(it, m_users.user.end());
    Save();
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
    Save();
    return false;
}

User* UserManager::GetUserByID(s32 user_id) {
    for (auto& u : m_users.user) {
        if (u.user_id == user_id)
            return &u;
    }
    return nullptr;
}

User* UserManager::GetUserByPlayerIndex(s32 index) {
    for (auto& u : m_users.user) {
        if (u.player_index == index)
            return &u;
    }
    return nullptr;
}

const std::vector<User>& UserManager::GetAllUsers() const {
    return m_users.user;
}

Users UserManager::CreateDefaultUsers() {
    Users default_users;
    default_users.user = {
        {
            .user_id = 1000,
            .user_name = "shadPS4",
            .user_color = 1,
            .player_index = 1,
            .shadnet_npid = "",
            .shadnet_password = "",
            .shadnet_token = "",
            .shadnet_email = "",
            .shadnet_enabled = false,
        },
        {
            .user_id = 1001,
            .user_name = "shadPS4-2",
            .user_color = 2,
            .player_index = 2,
            .shadnet_npid = "",
            .shadnet_password = "",
            .shadnet_token = "",
            .shadnet_email = "",
            .shadnet_enabled = false,
        },
        {
            .user_id = 1002,
            .user_name = "shadPS4-3",
            .user_color = 3,
            .player_index = 3,
            .shadnet_npid = "",
            .shadnet_password = "",
            .shadnet_token = "",
            .shadnet_email = "",
            .shadnet_enabled = false,
        },
        {
            .user_id = 1003,
            .user_name = "shadPS4-4",
            .user_color = 4,
            .player_index = 4,
            .shadnet_npid = "",
            .shadnet_password = "",
            .shadnet_token = "",
            .shadnet_email = "",
            .shadnet_enabled = false,
        },
    };

    for (auto& u : default_users.user) {
        const auto user_dir = EmulatorSettings.GetHomeDir() / std::to_string(u.user_id);

        if (!std::filesystem::exists(user_dir)) {
            std::filesystem::create_directory(user_dir);
            std::filesystem::create_directory(user_dir / "savedata");
            std::filesystem::create_directory(user_dir / "trophy");
            std::filesystem::create_directory(user_dir / "inputs");
        }
    }

    return default_users;
}

bool UserManager::SetDefaultUser(u32 user_id) {
    auto it = std::find_if(m_users.user.begin(), m_users.user.end(),
                           [user_id](const User& u) { return u.user_id == user_id; });
    if (it == m_users.user.end())
        return false;

    SetControllerPort(user_id, 1); // Set default user to port 1
    return Save();
}

User UserManager::GetDefaultUser() {
    return *GetUserByPlayerIndex(1);
}

void UserManager::SetControllerPort(u32 user_id, int port) {
    for (auto& u : m_users.user) {
        if (u.user_id != user_id && u.player_index == port)
            u.player_index = -1;
        if (u.user_id == user_id)
            u.player_index = port;
    }
    Save();
}
// Returns a list of users that have valid home directories
std::vector<User> UserManager::GetValidUsers() const {
    std::vector<User> result;
    result.reserve(m_users.user.size());

    const auto home_dir = EmulatorSettings.GetHomeDir();

    for (const auto& user : m_users.user) {
        const auto user_dir = home_dir / std::to_string(user.user_id);
        if (std::filesystem::exists(user_dir)) {
            result.push_back(user);
        }
    }

    return result;
}

LoggedInUsers UserManager::GetLoggedInUsers() const {
    return logged_in_users;
}

using namespace Libraries::UserService;

void UserManager::LoginUser(User* u, s32 player_index) {
    if (!u) {
        return;
    }
    u->logged_in = true;
    // u->player_index = player_index;
    AddUserServiceEvent({OrbisUserServiceEventType::Login, u->user_id});
    logged_in_users[player_index - 1] = u;
}

void UserManager::LogoutUser(User* u) {
    if (!u) {
        return;
    }
    u->logged_in = false;
    AddUserServiceEvent({OrbisUserServiceEventType::Logout, u->user_id});
    logged_in_users[u->player_index - 1] = {};
}

bool UserManager::Save() const {
    return UserSettings.Save();
}