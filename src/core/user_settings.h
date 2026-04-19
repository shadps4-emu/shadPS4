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

#define UserSettings (*UserSettingsImpl::GetInstance())

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

    static std::shared_ptr<UserSettingsImpl> GetInstance();
    static void SetInstance(std::shared_ptr<UserSettingsImpl> instance);

private:
    UserManager m_userManager;
    bool m_loaded{false};
    static std::shared_ptr<UserSettingsImpl> s_instance;
    static std::mutex s_mutex;
};