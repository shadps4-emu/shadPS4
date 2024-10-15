// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "fiber.h"

#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/linker.h"

#ifdef _WIN64
#include <windows.h>
#endif

namespace Libraries::Fiber {

constexpr static u64 kFiberSignature = 0x054ad954;

thread_local SceFiber* gCurrentFiber = nullptr;
thread_local void* gFiberThread = nullptr;

void FiberEntry(void* param) {
    SceFiber* fiber = static_cast<SceFiber*>(param);
    u64 argRun = 0;
    u64 argRet = 0;

    gCurrentFiber = fiber;

    if (fiber->pArgRun != nullptr) {
        argRun = *fiber->pArgRun;
    }

    const auto* linker = Common::Singleton<Core::Linker>::Instance();
    linker->ExecuteGuest(fiber->entry, fiber->argOnInitialize, argRun);

    UNREACHABLE();
}

s32 PS4_SYSV_ABI sceFiberInitialize(SceFiber* fiber, const char* name, SceFiberEntry entry,
                                    u64 argOnInitialize, void* addrContext, u64 sizeContext,
                                    const SceFiberOptParam* optParam) {
    LOG_INFO(Lib_Fiber, "called: name = {}", name);

    if (!fiber || !name || !entry) {
        return ORBIS_FIBER_ERROR_NULL;
    }

    fiber->signature = kFiberSignature;

    fiber->entry = entry;
    fiber->argOnInitialize = argOnInitialize;

    fiber->argRun = 0;
    fiber->pArgRun = &fiber->argRun;
    fiber->argReturn = 0;
    fiber->pArgReturn = &fiber->argReturn;

    fiber->sizeContext = sizeContext;

    fiber->state = FiberState::Init;
#ifdef _WIN64
    fiber->handle = CreateFiber(sizeContext, FiberEntry, fiber);
#else
    UNREACHABLE_MSG("Missing implementation");
#endif
    strncpy(fiber->name, name, ORBIS_FIBER_MAX_NAME_LENGTH);

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFiberOptParamInitialize(SceFiberOptParam* optParam) {
    LOG_ERROR(Lib_Fiber, "called");

    if (!optParam) {
        return ORBIS_FIBER_ERROR_NULL;
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFiberFinalize(SceFiber* fiber) {
    LOG_TRACE(Lib_Fiber, "called");

    if (!fiber) {
        return ORBIS_FIBER_ERROR_NULL;
    }
    if ((u64)fiber % 8 != 0) {
        return ORBIS_FIBER_ERROR_ALIGNMENT;
    }
    if (fiber->signature != kFiberSignature) {
        return ORBIS_FIBER_ERROR_INVALID;
    }
    if (fiber->state != FiberState::Run) {
        return ORBIS_FIBER_ERROR_STATE;
    }

    fiber->signature = 0;
    fiber->state = FiberState::None;

#ifdef _WIN64
    DeleteFiber(fiber->handle);
#else
    UNREACHABLE_MSG("Missing implementation");
#endif
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFiberRun(SceFiber* fiber, u64 argOnRunTo, u64* argOnReturn) {
    LOG_TRACE(Lib_Fiber, "called");

    if (!fiber) {
        return ORBIS_FIBER_ERROR_NULL;
    }
    if ((u64)fiber % 8 != 0) {
        return ORBIS_FIBER_ERROR_ALIGNMENT;
    }
    if (fiber->signature != kFiberSignature) {
        return ORBIS_FIBER_ERROR_INVALID;
    }
    if (fiber->state == FiberState::Run) {
        return ORBIS_FIBER_ERROR_STATE;
    }

    if (gFiberThread == nullptr) {
#ifdef _WIN64
        gFiberThread = ConvertThreadToFiber(nullptr);
#else
        UNREACHABLE_MSG("Missing implementation");
#endif
    }

    gCurrentFiber = fiber;

    if (fiber->pArgRun != nullptr) {
        *fiber->pArgRun = argOnRunTo;
    }

    fiber->pArgReturn = argOnReturn;
    fiber->state = FiberState::Run;
#ifdef _WIN64
    SwitchToFiber(fiber->handle);
#else
    UNREACHABLE_MSG("Missing implementation");
#endif
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFiberSwitch(SceFiber* fiber, u64 argOnRunTo, u64* argOnRun) {
    LOG_TRACE(Lib_Fiber, "called");

    if (!fiber) {
        return ORBIS_FIBER_ERROR_NULL;
    }
    if ((u64)fiber % 8 != 0) {
        return ORBIS_FIBER_ERROR_ALIGNMENT;
    }
    if (fiber->signature != kFiberSignature) {
        return ORBIS_FIBER_ERROR_INVALID;
    }
    if (gCurrentFiber == nullptr) {
        return ORBIS_FIBER_ERROR_PERMISSION;
    }
    if (fiber->state == FiberState::Run) {
        return ORBIS_FIBER_ERROR_STATE;
    }

    gCurrentFiber->state = FiberState::Suspend;

    // TODO: argOnRun

    *fiber->pArgRun = argOnRunTo;
    fiber->state = FiberState::Run;

    gCurrentFiber = fiber;
#ifdef _WIN64
    SwitchToFiber(fiber->handle);
#else
    UNREACHABLE_MSG("Missing implementation");
#endif
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFiberGetSelf(SceFiber** fiber) {
    LOG_TRACE(Lib_Fiber, "called");

    if (!fiber || !gCurrentFiber) {
        return ORBIS_FIBER_ERROR_NULL;
    }
    if (gCurrentFiber->signature != kFiberSignature) {
        return ORBIS_FIBER_ERROR_PERMISSION;
    }

    *fiber = gCurrentFiber;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFiberReturnToThread(u64 argOnReturn, u64* argOnRun) {
    LOG_TRACE(Lib_Fiber, "called");

    if (gCurrentFiber->signature != kFiberSignature) {
        return ORBIS_FIBER_ERROR_PERMISSION;
    }

    if (gCurrentFiber->pArgReturn != nullptr) {
        *gCurrentFiber->pArgReturn = argOnReturn;
    }

    // TODO: argOnRun
    gCurrentFiber->state = FiberState::Suspend;
    gCurrentFiber = nullptr;
#ifdef _WIN64
    SwitchToFiber(gFiberThread);
#else
    UNREACHABLE_MSG("Missing implementation");
#endif
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFiberGetInfo(SceFiber* fiber, SceFiberInfo* fiberInfo) {
    LOG_INFO(Lib_Fiber, "called");

    if (!fiber || !fiberInfo) {
        return ORBIS_FIBER_ERROR_NULL;
    }

    fiberInfo->entry = fiber->entry;
    fiberInfo->argOnInitialize = fiber->argOnInitialize;
    fiberInfo->addrContext = nullptr;
    fiberInfo->sizeContext = fiber->sizeContext;
    fiberInfo->sizeContextMargin = 0;

    strncpy(fiberInfo->name, fiber->name, ORBIS_FIBER_MAX_NAME_LENGTH);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFiberStartContextSizeCheck(u32 flags) {
    LOG_ERROR(Lib_Fiber, "called");

    if (flags != 0) {
        return ORBIS_FIBER_ERROR_INVALID;
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFiberStopContextSizeCheck() {
    LOG_ERROR(Lib_Fiber, "called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFiberRename(SceFiber* fiber, const char* name) {
    LOG_INFO(Lib_Fiber, "called, name = {}", name);

    if (!fiber || !name) {
        return ORBIS_FIBER_ERROR_NULL;
    }
    if ((u64)fiber % 8 != 0) {
        return ORBIS_FIBER_ERROR_ALIGNMENT;
    }

    strncpy(fiber->name, name, ORBIS_FIBER_MAX_NAME_LENGTH);
    return ORBIS_OK;
}

void RegisterlibSceFiber(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("hVYD7Ou2pCQ", "libSceFiber", 1, "libSceFiber", 1, 1, sceFiberInitialize);
    LIB_FUNCTION("asjUJJ+aa8s", "libSceFiber", 1, "libSceFiber", 1, 1, sceFiberOptParamInitialize);
    LIB_FUNCTION("JeNX5F-NzQU", "libSceFiber", 1, "libSceFiber", 1, 1, sceFiberFinalize);

    LIB_FUNCTION("a0LLrZWac0M", "libSceFiber", 1, "libSceFiber", 1, 1, sceFiberRun);
    LIB_FUNCTION("PFT2S-tJ7Uk", "libSceFiber", 1, "libSceFiber", 1, 1, sceFiberSwitch);
    LIB_FUNCTION("p+zLIOg27zU", "libSceFiber", 1, "libSceFiber", 1, 1, sceFiberGetSelf);
    LIB_FUNCTION("B0ZX2hx9DMw", "libSceFiber", 1, "libSceFiber", 1, 1, sceFiberReturnToThread);

    LIB_FUNCTION("uq2Y5BFz0PE", "libSceFiber", 1, "libSceFiber", 1, 1, sceFiberGetInfo);
    LIB_FUNCTION("Lcqty+QNWFc", "libSceFiber", 1, "libSceFiber", 1, 1,
                 sceFiberStartContextSizeCheck);
    LIB_FUNCTION("Kj4nXMpnM8Y", "libSceFiber", 1, "libSceFiber", 1, 1,
                 sceFiberStopContextSizeCheck);
    LIB_FUNCTION("JzyT91ucGDc", "libSceFiber", 1, "libSceFiber", 1, 1, sceFiberRename);
}

} // namespace Libraries::Fiber