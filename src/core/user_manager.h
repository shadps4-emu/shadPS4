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
    // ShadNet settings
    std::string shadnet_npid = "";     // account identifier
    std::string shadnet_password = ""; // account password
    std::string shadnet_token = "";    // 2FA/validation token (future use)
    std::string shadnet_email = "";    // email address (furute use)
    bool shadnet_enabled = false;      // enable shadnet for user
    // these are used to populate NP profile fields
    std::string np_country = "us";               // ISO 3166-1 alpha-2, e.g. "us", "jp", "gb"
    std::string np_language = "en";              // ISO 639-1, e.g. "en", "ja", "fr"
    u8 np_age = 30;                              // 0..127
    std::string np_date_of_birth = "1994-01-01"; // ISO 8601 date "YYYY-MM-DD"
};

struct Users {
    std::vector<User> user{};
    std::string commit_hash{};
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(User, user_id, user_color, user_name, player_index,
                                                shadnet_npid, shadnet_password, shadnet_token,
                                                shadnet_email, shadnet_enabled, np_country,
                                                np_language, np_age, np_date_of_birth)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Users, user, commit_hash)

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
    void LoginUser(User* u, s32 player_index);
    void LogoutUser(User* u);

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
