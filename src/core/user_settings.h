// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "common/logging/log.h"
#include "common/types.h"
#include "core/user_manager.h"

#define UserSettings (*UserSettingsImpl::GetInstance())

// -------------------------------
// User settings
// -------------------------------
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(User, user_id, user_color, user_name, controller_port)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Users, default_user_id, user)

class UserSettingsImpl {
public:
    UserSettingsImpl();
    ~UserSettingsImpl();

    UserManager& GetUserManager() {
        return m_userManager;
    }
    bool Save() const;
    bool Load();

private:
    UserManager m_userManager;

    static std::shared_ptr<UserSettingsImpl> s_instance;
    static std::mutex s_mutex;

};