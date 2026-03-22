// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "common/types.h"

struct User {
    s32 user_id = -1;
    std::string user_name = "";
    u32 user_color;
    int player_index = 0; // 1-4

    bool logged_in = false;
};

struct Users {
    std::vector<User> user{};
    std::string commit_hash{};
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(User, user_id, user_color, user_name, player_index)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Users, user, commit_hash)

using LoggedInUsers = std::array<User*, 4>;

class UserManager {
public:
    UserManager() = default;

    bool AddUser(const User& user);
    bool RemoveUser(s32 user_id);
    bool RenameUser(s32 user_id, const std::string& new_name);
    User* GetUserByID(s32 user_id);
    User* GetUserByPlayerIndex(s32 index);
    const std::vector<User>& GetAllUsers() const;
    Users CreateDefaultUsers();
    bool SetDefaultUser(u32 user_id);
    User GetDefaultUser();
    void SetControllerPort(u32 user_id, int port);
    std::vector<User> GetValidUsers() const;
    LoggedInUsers GetLoggedInUsers() const;

    Users& GetUsers() {
        return m_users;
    }
    const Users& GetUsers() const {
        return m_users;
    }

    bool Save() const;

private:
    Users m_users;
    LoggedInUsers logged_in_users{};
};
