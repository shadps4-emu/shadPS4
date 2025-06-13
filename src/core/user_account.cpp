// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <filesystem>
#include <common/io_file.h>
#include <common/path_util.h>
#include <nlohmann/json.hpp>
#include "common/config.h"
#include "user_account.h"

using json = nlohmann::json;

user_account::user_account(const std::string& user_id) {
    // Setting userId.
    m_user_id = user_id;

    m_user_dir = Common::FS::GetUserPathString(Common::FS::PathType::HomeDir) + "/" +
                 Config::getDefaultUserId() + "/";

    Common::FS::IOFile userfile(m_user_dir + "localuser.json", Common::FS::FileAccessMode::Read);
    if (userfile.IsOpen()) {
        nlohmann::json jsonfile;
        try {
            jsonfile = nlohmann::json::parse(userfile.ReadString(userfile.GetSize()));
        } catch (const nlohmann::json::parse_error& e) {
            // TODO error code
        }
        userfile.Close();
        m_username = jsonfile.value("username", "shadps4");
        if (m_username.length() > 16) // max of 16 chars allowed
        {
            m_username = m_username.substr(0, 16); // substring 16 only characters to display
        }
    }
}

std::map<u32, user_account> user_account::GetUserAccounts(const std::string& base_dir) {
    std::map<u32, user_account> user_list;
    // TODO

    return user_list;
}
