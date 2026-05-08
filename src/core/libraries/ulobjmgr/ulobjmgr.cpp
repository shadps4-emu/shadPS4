// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/posix_error.h"
#include "core/libraries/libs.h"
#include "core/libraries/ulobjmgr/ulobjmgr.h"

namespace Libraries::Ulobjmgr {

s32 PS4_SYSV_ABI _sceUlobjmgrRegisterObject(u64 arg0, s32 arg1, u32* arg2) {
    if (arg0 == 0 || arg1 == 0 || arg2 == nullptr) {
        return POSIX_EINVAL;
    }
    *arg2 = 0;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _sceUlobjmgrUnregisterObject(u32 arg0) {
    if (arg0 >= 0x4000) {
        return POSIX_EINVAL;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_1D9F50D9CFB8054E() {
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_4B07893BBB77A649(u64 arg0) {
    if (arg0 == 0) {
        return POSIX_EINVAL;
    }
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("BG26hBGiNlw", "ulobjmgr", 1, "ulobjmgr", _sceUlobjmgrRegisterObject);
    LIB_FUNCTION("Smf+fUNblPc", "ulobjmgr", 1, "ulobjmgr", _sceUlobjmgrUnregisterObject);
    LIB_FUNCTION("HZ9Q2c+4BU4", "ulobjmgr", 1, "ulobjmgr", Func_1D9F50D9CFB8054E);
    LIB_FUNCTION("SweJO7t3pkk", "ulobjmgr", 1, "ulobjmgr", Func_4B07893BBB77A649);
};

} // namespace Libraries::Ulobjmgr
