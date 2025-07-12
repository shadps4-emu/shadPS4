// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "move.h"

namespace Libraries::Move {

int PS4_SYSV_ABI sceMoveOpen() {
    LOG_TRACE(Lib_Move, "(STUBBED) called");
    return ORBIS_FAIL;
}

int PS4_SYSV_ABI sceMoveGetDeviceInfo() {
    LOG_ERROR(Lib_Move, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceMoveReadStateLatest() {
    LOG_TRACE(Lib_Move, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceMoveReadStateRecent() {
    LOG_TRACE(Lib_Move, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceMoveTerm() {
    LOG_ERROR(Lib_Move, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceMoveInit() {
    LOG_ERROR(Lib_Move, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("HzC60MfjJxU", "libSceMove", 1, "libSceMove", 1, 1, sceMoveOpen);
    LIB_FUNCTION("GWXTyxs4QbE", "libSceMove", 1, "libSceMove", 1, 1, sceMoveGetDeviceInfo);
    LIB_FUNCTION("ttU+JOhShl4", "libSceMove", 1, "libSceMove", 1, 1, sceMoveReadStateLatest);
    LIB_FUNCTION("f2bcpK6kJfg", "libSceMove", 1, "libSceMove", 1, 1, sceMoveReadStateRecent);
    LIB_FUNCTION("tsZi60H4ypY", "libSceMove", 1, "libSceMove", 1, 1, sceMoveTerm);
    LIB_FUNCTION("j1ITE-EoJmE", "libSceMove", 1, "libSceMove", 1, 1, sceMoveInit);
};

} // namespace Libraries::Move