// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::CommonDialog {

enum class Status : u32 {
    NONE = 0,
    INITIALIZED = 1,
    RUNNING = 2,
    FINISHED = 3,
};

enum class Result : u32 {
    OK = 0,
    USER_CANCELED = 1,
};

enum class Error : u32 {
    OK = 0,
    NOT_SYSTEM_INITIALIZED = 0x80B80001,
    ALREADY_SYSTEM_INITIALIZED = 0x80B80002,
    NOT_INITIALIZED = 0x80B80003,
    ALREADY_INITIALIZED = 0x80B80004,
    NOT_FINISHED = 0x80B80005,
    INVALID_STATE = 0x80B80006,
    RESULT_NONE = 0x80B80007,
    BUSY = 0x80B80008,
    OUT_OF_MEMORY = 0x80B80009,
    PARAM_INVALID = 0x80B8000A,
    NOT_RUNNING = 0x80B8000B,
    ALREADY_CLOSE = 0x80B8000C,
    ARG_NULL = 0x80B8000D,
    UNEXPECTED_FATAL = 0x80B8000E,
    NOT_SUPPORTED = 0x80B8000F,
    INHIBIT_SHAREPLAY_CLIENT = 0x80B80010,
};

extern bool g_isInitialized;
extern bool g_isUsed;

struct BaseParam {
    std::size_t size;
    std::array<u8, 36> reserved;
    u32 magic;
};

int PS4_SYSV_ABI _ZN3sce16CommonDialogUtil12getSelfAppIdEv();
int PS4_SYSV_ABI _ZN3sce16CommonDialogUtil6Client11closeModuleEv();
int PS4_SYSV_ABI _ZN3sce16CommonDialogUtil6Client11updateStateEv();
int PS4_SYSV_ABI _ZN3sce16CommonDialogUtil6Client15launchCmnDialogEv();
int PS4_SYSV_ABI _ZN3sce16CommonDialogUtil6ClientD0Ev();
int PS4_SYSV_ABI _ZN3sce16CommonDialogUtil6ClientD1Ev();
int PS4_SYSV_ABI _ZN3sce16CommonDialogUtil6ClientD2Ev();
int PS4_SYSV_ABI _ZNK3sce16CommonDialogUtil6Client10isCloseReqEv();
int PS4_SYSV_ABI _ZNK3sce16CommonDialogUtil6Client13getFinishDataEPvm();
int PS4_SYSV_ABI _ZNK3sce16CommonDialogUtil6Client14getClientStateEv();
int PS4_SYSV_ABI _ZNK3sce16CommonDialogUtil6Client19isInitializedStatusEv();
int PS4_SYSV_ABI _ZNK3sce16CommonDialogUtil6Client8getAppIdEv();
int PS4_SYSV_ABI _ZNK3sce16CommonDialogUtil6Client8isFinishEv();
int PS4_SYSV_ABI _ZNK3sce16CommonDialogUtil6Client9getResultEv();
int PS4_SYSV_ABI _ZTVN3sce16CommonDialogUtil6ClientE();
Error PS4_SYSV_ABI sceCommonDialogInitialize();
bool PS4_SYSV_ABI sceCommonDialogIsUsed();
int PS4_SYSV_ABI Func_0FF577E4E8457883();
int PS4_SYSV_ABI Func_41716C2CE379416C();
int PS4_SYSV_ABI Func_483A427D8F6E0748();
int PS4_SYSV_ABI Func_6944B83E02727BDF();
int PS4_SYSV_ABI Func_69F2DD23A8B4950C();
int PS4_SYSV_ABI Func_9954673DEAC170AD();
int PS4_SYSV_ABI Func_A7D4D3AB86CB7455();
int PS4_SYSV_ABI Func_ADE4C51256B8350C();
int PS4_SYSV_ABI Func_B71349CF15FACAB0();
int PS4_SYSV_ABI Func_CB18E00EFA946C64();
int PS4_SYSV_ABI Func_F2AEE270605622B0();

void RegisterlibSceCommonDialog(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::CommonDialog
