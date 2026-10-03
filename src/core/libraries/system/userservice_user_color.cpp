// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/system/userservice.h"
#include "core/libraries/system/userservice_error.h"
#include "core/user_settings.h"

namespace Libraries::UserService {

s32 PS4_SYSV_ABI sceUserServiceGetUserColor(int user_id, OrbisUserServiceUserColor* color) {
    LOG_DEBUG(Lib_UserService, "called user_id = {}", user_id);
    if (color == nullptr || user_id == ORBIS_USER_SERVICE_USER_ID_INVALID) {
        return ORBIS_USER_SERVICE_ERROR_INVALID_ARGUMENT;
    }
    const auto settings = UserSettingsImpl::GetInstance();
    const auto* user = settings->GetUserManager().GetUserByID(user_id);
    if (user == nullptr) {
        LOG_ERROR(Lib_UserService, "User color requested for unknown user_id = {}", user_id);
        return ORBIS_USER_SERVICE_ERROR_INVALID_ARGUMENT;
    }
    // Preserve the existing saved-colour mapping for valid users. A failed
    // lookup must leave the caller's output untouched, not dereference null.
    *color = static_cast<OrbisUserServiceUserColor>(user->user_color);
    return ORBIS_OK;
}

} // namespace Libraries::UserService
