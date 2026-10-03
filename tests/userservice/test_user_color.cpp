// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdlib>
#include <iostream>
#include "common/logging/log.h"
#include "core/libraries/system/userservice.h"
#include "core/libraries/system/userservice_error.h"
#include "core/user_settings.h"

// Replace the user store and logger only. The public HLE entry point is the
// production translation unit, including its argument and lookup validation.
namespace {
User* lookup_result{};
int lookup_calls{};
int requested_id{};

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}
} // namespace

namespace Common::Log {
std::array<Level, NUM_LOG_CLASSES> g_class_levels{};
void VLog(Class, Level, const char*, int, const char*, fmt::string_view, fmt::format_args) {}
} // namespace Common::Log

UserSettingsImpl::UserSettingsImpl() = default;
UserSettingsImpl::~UserSettingsImpl() = default;
std::shared_ptr<UserSettingsImpl> UserSettingsImpl::GetInstance() {
    static auto instance = std::make_shared<UserSettingsImpl>();
    return instance;
}

User* UserManager::GetUserByID(s32 user_id) {
    ++lookup_calls;
    requested_id = user_id;
    return lookup_result;
}

int main() {
    using namespace Libraries::UserService;
    constexpr auto sentinel = static_cast<OrbisUserServiceUserColor>(99);
    auto color = sentinel;
    Check(sceUserServiceGetUserColor(9999, &color) == ORBIS_USER_SERVICE_ERROR_INVALID_ARGUMENT,
          "Unknown user must return an error instead of dereferencing null");
    Check(color == sentinel, "Failed lookup modified the output");
    Check(lookup_calls == 1 && requested_id == 9999, "Wrong user lookup");

    Check(sceUserServiceGetUserColor(1000, nullptr) == ORBIS_USER_SERVICE_ERROR_INVALID_ARGUMENT,
          "Null output must be rejected");
    Check(sceUserServiceGetUserColor(ORBIS_USER_SERVICE_USER_ID_INVALID, &color) ==
              ORBIS_USER_SERVICE_ERROR_INVALID_ARGUMENT,
          "Invalid user sentinel must be rejected");
    Check(lookup_calls == 1 && color == sentinel, "Invalid arguments reached the user store");

    User user;
    lookup_result = &user;
    for (u32 saved_color = 0; saved_color <= 4; ++saved_color) {
        user.user_color = saved_color;
        Check(sceUserServiceGetUserColor(1000, &color) == ORBIS_OK, "Valid user rejected");
        Check(static_cast<u32>(color) == saved_color, "Existing colour mapping changed");
    }
    lookup_result = nullptr;
    color = sentinel;
    Check(sceUserServiceGetUserColor(1000, &color) == ORBIS_USER_SERVICE_ERROR_INVALID_ARGUMENT &&
              color == sentinel,
          "A user removed since a successful query must fail safely");
    std::cout << "PASS: missing/removed users, invalid arguments and existing colours\n";
}
