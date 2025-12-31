// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include "common/types.h"

struct User {
    s32 user_id;
    u32 user_color;
    std::string user_name;
    int controller_port; // 1�4
};

struct Users {
    int default_user_id = 1;
    std::vector<User> user;
};

class UserManager {
public:
    UserManager() = default;

    bool AddUser(const User& user);
    bool RemoveUser(s32 user_id);
    bool RenameUser(s32 user_id, const std::string& new_name);
    User* GetUserByID(s32 user_id);
    const std::vector<User>& GetAllUsers() const;
    std::vector<User> CreateDefaultUser();
    bool SetDefaultUser(u32 user_id);
    void SetControllerPort(u32 user_id, int port);
    std::vector<User> GetValidUsers() const;

    Users& GetUsers() {
        return m_users;
    }
    const Users& GetUsers() const {
        return m_users;
    }

private:
    Users m_users;
};