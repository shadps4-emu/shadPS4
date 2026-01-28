// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Ulobjmgr {
// NID: BG26hBGiNlw
s32 PS4_SYSV_ABI Func_046DBA8411A2365C(u64 arg0, s32 arg1, u32* arg2);
// NID: HZ9Q2c+4BU4
s32 PS4_SYSV_ABI Func_1D9F50D9CFB8054E();
// NID: Smf+fUNblPc
s32 PS4_SYSV_ABI Func_4A67FE7D435B94F7(u32 arg0);
// NID: SweJO7t3pkk
s32 PS4_SYSV_ABI Func_4B07893BBB77A649(u64 arg0);

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Ulobjmgr
