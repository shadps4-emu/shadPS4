// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <filesystem>
#include "user_account.h"

user_account::user_account(const std::string& user_id) {
    // Setting userId.
    m_user_id = user_id;
    // TODO
}

std::map<u32, user_account> user_account::GetUserAccounts(const std::string& base_dir) {
    std::map<u32, user_account> user_list;
    // TODO

    return user_list;
}
