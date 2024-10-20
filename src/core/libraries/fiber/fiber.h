// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/assert.h"
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}
namespace Libraries::Fiber {

#define ORBIS_FIBER_MAX_NAME_LENGTH (31)

typedef void PS4_SYSV_ABI (*SceFiberEntry)(u64 argOnInitialize, u64 argOnRun);

enum FiberState : u32 {
    None = 0u,
    Init = 1u,
    Run = 2u,
    Suspend = 3u,
};

struct SceFiber {
    u64 signature;
    FiberState state;
    SceFiberEntry entry;

    u64 argOnInitialize;

    u64 argRun;
    u64* pArgRun;

    u64 argReturn;
    u64* pArgReturn;

    u64 sizeContext;

    char name[ORBIS_FIBER_MAX_NAME_LENGTH];
    void* handle;
};
static_assert(sizeof(SceFiber) <= 256);

struct SceFiberInfo {
    u64 size;
    SceFiberEntry entry;
    u64 argOnInitialize;
    void* addrContext;
    u64 sizeContext;
    char name[ORBIS_FIBER_MAX_NAME_LENGTH + 1];
    u64 sizeContextMargin;
};
static_assert(sizeof(SceFiberInfo) <= 128);

typedef void* SceFiberOptParam;

s32 PS4_SYSV_ABI sceFiberInitialize(SceFiber* fiber, const char* name, SceFiberEntry entry,
                                    u64 argOnInitialize, void* addrContext, u64 sizeContext,
                                    const SceFiberOptParam* optParam);

s32 PS4_SYSV_ABI sceFiberOptParamInitialize(SceFiberOptParam* optParam);

s32 PS4_SYSV_ABI sceFiberFinalize(SceFiber* fiber);

s32 PS4_SYSV_ABI sceFiberRun(SceFiber* fiber, u64 argOnRunTo, u64* argOnReturn);

s32 PS4_SYSV_ABI sceFiberSwitch(SceFiber* fiber, u64 argOnRunTo, u64* argOnRun);

s32 PS4_SYSV_ABI sceFiberGetSelf(SceFiber** fiber);

s32 PS4_SYSV_ABI sceFiberReturnToThread(u64 argOnReturn, u64* argOnRun);

s32 PS4_SYSV_ABI sceFiberGetInfo(SceFiber* fiber, SceFiberInfo* fiberInfo);

s32 PS4_SYSV_ABI sceFiberStartContextSizeCheck(u32 flags);

s32 PS4_SYSV_ABI sceFiberStopContextSizeCheck(void);

s32 PS4_SYSV_ABI sceFiberRename(SceFiber* fiber, const char* name);

void RegisterlibSceFiber(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Fiber