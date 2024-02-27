// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/hle/error_codes.h"
#include "core/hle/libraries/libs.h"
#include "core/hle/libraries/libuserservice/libuserservice.h"

namespace Core::Libraries::LibUserService {

s32 PS4_SYSV_ABI sceUserServiceInitialize(const SceUserServiceInitializeParams* initParams) {
    LOG_WARNING(Lib_UserService, "(STUBBED) called");
    return SCE_OK;
}

s32 PS4_SYSV_ABI sceUserServiceGetLoginUserIdList(SceUserServiceLoginUserIdList* userIdList) {
    LOG_WARNING(Lib_UserService, "(STUBBED) called");
    userIdList->user_id[0] = 1;
    userIdList->user_id[1] = -1;
    userIdList->user_id[2] = -1;
    userIdList->user_id[3] = -1;

    return SCE_OK;
}

void userServiceSymbolsRegister(Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("j3YMu1MVNNo", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceInitialize);
    LIB_FUNCTION("fPhymKNvK-A", "libSceUserService", 1, "libSceUserService", 1, 1,
                 sceUserServiceGetLoginUserIdList);
}

} // namespace Core::Libraries::LibUserService
