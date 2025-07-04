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

    m_user_dir =
        Common::FS::GetUserPathString(Common::FS::PathType::HomeDir) + "/" + m_user_id + "/";

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
    for (const auto& entry : std::filesystem::directory_iterator(base_dir)) {
        if (entry.is_directory()) {
            std::string folder_name = entry.path().filename().string();
            const u32 key = check_user(folder_name);
            if (key == 0) {
                continue;
            }
            const std::filesystem::path account_file = entry.path() / "localuser.json";
            if (std::filesystem::exists(account_file)) {
                user_list.emplace(key, user_account(folder_name));
            }
        }
    }

    return user_list;
}

void user_account::createdDefaultUser() {
    const auto& default_user_dir =
        Common::FS::GetUserPath(Common::FS::PathType::HomeDir) / Config::getDefaultUserId();
    if (!std::filesystem::exists(default_user_dir)) {
        std::filesystem::create_directory(default_user_dir);
        Common::FS::IOFile userfile(default_user_dir / "localuser.json",
                                    Common::FS::FileAccessMode::Write);
        nlohmann::json jsonfile;

        // Assign values
        jsonfile["username"] = "shadps4";

        std::string jsonStr = jsonfile.dump(4);
        userfile.WriteString(jsonStr);
        userfile.Close();
    }
}
