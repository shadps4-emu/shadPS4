// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
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

#define UserSettings ShadPs4App::GetInstance()->m_user_settings

#define UserManagement UserSettings.GetUserManager()

// -------------------------------
// User settings
// -------------------------------

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
    bool m_loaded{false};
};