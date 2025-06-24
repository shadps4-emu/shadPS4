// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <charconv>
#include <map>
#include <string>
#include "common/types.h"

class user_account {
public:
    explicit user_account(const std::string& user_id = "00000001");

    const std::string& GetUserId() const {
        return m_user_id;
    }
    const std::string& GetUserDir() const {
        return m_user_dir;
    }
    const std::string& GetUsername() const {
        return m_username;
    }

    static std::map<u32, user_account> GetUserAccounts(const std::string& base_dir);
    static void createdDefaultUser();
    static u32 check_user(const std::string& user) {
        u32 id = 0;

        if (user.size() == 8) {
            std::from_chars(&user.front(), &user.back() + 1, id);
        }

        return id;
    }

private:
    std::string m_user_id;
    std::string m_user_dir;
    std::string m_username;
};
