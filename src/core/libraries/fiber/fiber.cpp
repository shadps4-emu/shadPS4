// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "fiber.h"

#include "common/elf_info.h"
#include "common/logging/log.h"
#include "core/libraries/fiber/fiber_error.h"
#include "core/libraries/libs.h"
#include "core/tls.h"

namespace Libraries::Fiber {

static constexpr u32 kFiberSignature0 = 0xdef1649c;
static constexpr u32 kFiberSignature1 = 0xb37592a0;
static constexpr u32 kFiberOptSignature = 0xbb40e64d;
static constexpr u64 kFiberStackSignature = 0x7149f2ca7149f2ca;
static constexpr u64 kFiberStackSizeCheck = 0xdeadbeefdeadbeef;

static std::atomic<u32> context_size_check = false;

OrbisFiberContext* GetFiberContext() {
    return Core::GetTcbBase()->tcb_fiber;
}

extern "C" s32 PS4_SYSV_ABI _sceFiberSetJmp(OrbisFiberContext* ctx) asm("_sceFiberSetJmp");
extern "C" s32 PS4_SYSV_ABI _sceFiberLongJmp(OrbisFiberContext* ctx) asm("_sceFiberLongJmp");
extern "C" void PS4_SYSV_ABI _sceFiberSwitchEntry(OrbisFiberData* data,
                                                  bool set_fpu) asm("_sceFiberSwitchEntry");
extern "C" void PS4_SYSV_ABI _sceFiberForceQuit(u64 ret) asm("_sceFiberForceQuit");

extern "C" void PS4_SYSV_ABI _sceFiberForceQuit(u64 ret) {
    OrbisFiberContext* g_ctx = GetFiberContext();
    g_ctx->return_val = ret;
    _sceFiberLongJmp(g_ctx);
}

void PS4_SYSV_ABI _sceFiberCheckStackOverflow(OrbisFiberContext* ctx) {
    u64* stack_base = reinterpret_cast<u64*>(ctx->current_fiber->addr_context);
    u64 stack_size = ctx->current_fiber->size_context;
    if (stack_base && *stack_base != kFiberStackSignature) {
        UNREACHABLE_MSG("Stack overflow detected in fiber with size = 0x{:x}", stack_size);
    }
}

s32 PS4_SYSV_ABI _sceFiberAttachContext(OrbisFiber* fiber, void* addr_context, u64 size_context) {
    if (size_context && size_context < ORBIS_FIBER_CONTEXT_MINIMUM_SIZE) {
        return ORBIS_FIBER_ERROR_RANGE;
    }
    if (size_context & 15) {
        return ORBIS_FIBER_ERROR_INVALID;
    }
    if (!addr_context || !size_context) {
        return ORBIS_FIBER_ERROR_INVALID;
    }
    if (fiber->addr_context) {
        return ORBIS_FIBER_ERROR_INVALID;
    }

    fiber->addr_context = addr_context;
    fiber->size_context = size_context;
    fiber->context_start = addr_context;
    fiber->context_end = reinterpret_cast<u8*>(addr_context) + size_context;

    /* Apply signature to start of stack */
    *(u64*)addr_context = kFiberStackSignature;

    if (fiber->flags & FiberFlags::ContextSizeCheck) {
        u64* stack_start = reinterpret_cast<u64*>(fiber->context_start);
        u64* stack_end = reinterpret_cast<u64*>(fiber->context_end);

        u64* stack_ptr = stack_start + 1;
        while (stack_ptr < stack_end) {
            *stack_ptr++ = kFiberStackSizeCheck;
        }
    }

    return ORBIS_OK;
}

void PS4_SYSV_ABI _sceFiberSwitchToFiber(OrbisFiber* fiber, u64 arg_on_run_to,
                                         OrbisFiberContext* ctx) {
    OrbisFiberContext* fiber_ctx = fiber->context;
    if (fiber_ctx) {
        ctx->arg_on_run_to = arg_on_run_to;
        _sceFiberLongJmp(fiber_ctx);
        __builtin_trap();
    }

    OrbisFiberData data{};
    if (ctx->prev_fiber) {
        OrbisFiber* prev_fiber = ctx->prev_fiber;
        ctx->prev_fiber = nullptr;
        data.state = reinterpret_cast<u32*>(&prev_fiber->state);
    } else {
        data.state = nullptr;
    }

    data.entry = fiber->entry;
    data.arg_on_initialize = fiber->arg_on_initialize;
    data.arg_on_run_to = arg_on_run_to;
    data.stack_addr = reinterpret_cast<u8*>(fiber->addr_context) + fiber->size_context;
    if (fiber->flags & FiberFlags::SetFpuRegs) {
        data.fpucw = 0x037f;
        data.mxcsr = 0x9fc0;
        _sceFiberSwitchEntry(&data, true);
    } else {
        _sceFiberSwitchEntry(&data, false);
    }

    __builtin_trap();
}

void PS4_SYSV_ABI _sceFiberSwitch(OrbisFiber* cur_fiber, OrbisFiber* fiber, u64 arg_on_run_to,
                                  OrbisFiberContext* ctx) {
    ctx->prev_fiber = cur_fiber;
    ctx->current_fiber = fiber;

    if (fiber->addr_context == nullptr) {
        ctx->prev_fiber = nullptr;

        OrbisFiberData data{};
        data.entry = fiber->entry;
        data.arg_on_initialize = fiber->arg_on_initialize;
        data.arg_on_run_to = arg_on_run_to;
        data.stack_addr = reinterpret_cast<void*>(ctx->rsp & ~15);
        data.state = reinterpret_cast<u32*>(&cur_fiber->state);

        if (fiber->flags & FiberFlags::SetFpuRegs) {
            data.fpucw = 0x037f;
            data.mxcsr = 0x9fc0;
            _sceFiberSwitchEntry(&data, true);
        } else {
            _sceFiberSwitchEntry(&data, false);
        }

        __builtin_trap();
    }

    _sceFiberSwitchToFiber(fiber, arg_on_run_to, ctx);
    __builtin_trap();
}

void PS4_SYSV_ABI _sceFiberTerminate(OrbisFiber* fiber, u64 arg_on_return, OrbisFiberContext* ctx) {
    ctx->arg_on_return = arg_on_return;
    _sceFiberLongJmp(ctx);
    __builtin_trap();
}

s32 PS4_SYSV_ABI sceFiberInitializeImpl(OrbisFiber* fiber, const char* name, OrbisFiberEntry entry,
                                        u64 arg_on_initialize, void* addr_context, u64 size_context,
                                        const OrbisFiberOptParam* opt_param, u32 flags,
                                        u32 build_ver) {
    if (!fiber || !name || !entry) {
        return ORBIS_FIBER_ERROR_NULL;
    }
    if ((u64)fiber & 7 || (u64)addr_context & 15) {
        return ORBIS_FIBER_ERROR_ALIGNMENT;
    }
    if (opt_param && (u64)opt_param & 7) {
        return ORBIS_FIBER_ERROR_ALIGNMENT;
    }
    if (size_context && size_context < ORBIS_FIBER_CONTEXT_MINIMUM_SIZE) {
        return ORBIS_FIBER_ERROR_RANGE;
    }
    if (size_context & 15) {
        return ORBIS_FIBER_ERROR_INVALID;
    }
    if (!addr_context && size_context) {
        return ORBIS_FIBER_ERROR_INVALID;
    }
    if (addr_context && !size_context) {
        return ORBIS_FIBER_ERROR_INVALID;
    }
    if (opt_param && opt_param->magic != kFiberOptSignature) {
        return ORBIS_FIBER_ERROR_INVALID;
    }

    u32 user_flags = flags;
    if (build_ver >= Common::ElfInfo::FW_35) {
        user_flags |= FiberFlags::SetFpuRegs;
    }
    if (context_size_check) {
        user_flags |= FiberFlags::ContextSizeCheck;
    }

    strncpy(fiber->name, name, ORBIS_FIBER_MAX_NAME_LENGTH);

    fiber->entry = entry;
    fiber->arg_on_initialize = arg_on_initialize;
    fiber->addr_context = addr_context;
    fiber->size_context = size_context;
    fiber->context = nullptr;
    fiber->flags = user_flags;

    /*
        A low stack area is problematic, as we can easily
        cause a stack overflow with our HLE.
    */
    if (size_context && size_context <= 4096) {
        LOG_WARNING(Lib_Fiber, "Fiber initialized with small stack area.");
    }

    fiber->magic_start = kFiberSignature0;
    fiber->magic_end = kFiberSignature1;

    if (addr_context != nullptr) {
        fiber->context_start = addr_context;
        fiber->context_end = reinterpret_cast<u8*>(addr_context) + size_context;

        /* Apply signature to start of stack */
        *(u64*)addr_context = kFiberStackSignature;

        if (flags & FiberFlags::ContextSizeCheck) {
            u64* stack_start = reinterpret_cast<u64*>(fiber->context_start);
            u64* stack_end = reinterpret_cast<u64*>(fiber->context_end);

            u64* stack_ptr = stack_start + 1;
            while (stack_ptr < stack_end) {
                *stack_ptr++ = kFiberStackSizeCheck;
            }
        }
    }

    fiber->state = FiberState::Idle;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFiberOptParamInitialize(OrbisFiberOptParam* opt_param) {
    if (!opt_param) {
        return ORBIS_FIBER_ERROR_NULL;
    }
    if ((u64)opt_param & 7) {
        return ORBIS_FIBER_ERROR_ALIGNMENT;
    }

    opt_param->magic = kFiberOptSignature;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFiberFinalize(OrbisFiber* fiber) {
    if (!fiber) {
        return ORBIS_FIBER_ERROR_NULL;
    }
    if ((u64)fiber & 7) {
        return ORBIS_FIBER_ERROR_ALIGNMENT;
    }
    if (fiber->magic_start != kFiberSignature0 || fiber->magic_end != kFiberSignature1) {
        return ORBIS_FIBER_ERROR_INVALID;
    }

    FiberState expected = FiberState::Idle;
    if (!fiber->state.compare_exchange_strong(expected, FiberState::Terminated)) {
        return ORBIS_FIBER_ERROR_STATE;
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFiberRunImpl(OrbisFiber* fiber, void* addr_context, u64 size_context,
                                 u64 arg_on_run_to, u64* arg_on_return) {
    if (!fiber) {
        return ORBIS_FIBER_ERROR_NULL;
    }
    if ((u64)fiber & 7 || (u64)addr_context & 15) {
        return ORBIS_FIBER_ERROR_ALIGNMENT;
    }
    if (fiber->magic_start != kFiberSignature0 || fiber->magic_end != kFiberSignature1) {
        return ORBIS_FIBER_ERROR_INVALID;
    }

    Core::Tcb* tcb = Core::GetTcbBase();
    if (tcb->tcb_fiber) {
        return ORBIS_FIBER_ERROR_PERMISSION;
    }

    /* Caller wants to attach context and run. */
    if (addr_context != nullptr || size_context != 0) {
        s32 res = _sceFiberAttachContext(fiber, addr_context, size_context);
        if (res < 0) {
            return res;
        }
    }

    FiberState expected = FiberState::Idle;
    if (!fiber->state.compare_exchange_strong(expected, FiberState::Run)) {
        return ORBIS_FIBER_ERROR_STATE;
    }

    OrbisFiberContext ctx{};
    ctx.current_fiber = fiber;
    ctx.prev_fiber = nullptr;
    ctx.return_val = 0;

    tcb->tcb_fiber = &ctx;

    s32 jmp = _sceFiberSetJmp(&ctx);
    if (!jmp) {
        if (fiber->addr_context) {
            _sceFiberSwitchToFiber(fiber, arg_on_run_to, &ctx);
            __builtin_trap();
        }

        OrbisFiberData data{};
        data.entry = fiber->entry;
        data.arg_on_initialize = fiber->arg_on_initialize;
        data.arg_on_run_to = arg_on_run_to;
        data.stack_addr = reinterpret_cast<void*>(ctx.rsp & ~15);
        data.state = nullptr;
        if (fiber->flags & FiberFlags::SetFpuRegs) {
            data.fpucw = 0x037f;
            data.mxcsr = 0x9fc0;
            _sceFiberSwitchEntry(&data, true);
        } else {
            _sceFiberSwitchEntry(&data, false);
        }
    }

    OrbisFiber* cur_fiber = ctx.current_fiber;
    ctx.current_fiber = nullptr;
    cur_fiber->state = FiberState::Idle;

    if (ctx.return_val != 0) {
        /* Fiber entry returned! This should never happen. */
        UNREACHABLE_MSG("Fiber entry function returned.");
    }

    if (arg_on_return) {
        *arg_on_return = ctx.arg_on_return;
    }

    tcb->tcb_fiber = nullptr;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFiberSwitchImpl(OrbisFiber* fiber, void* addr_context, u64 size_context,
                                    u64 arg_on_run_to, u64* arg_on_run) {
    if (!fiber) {
        return ORBIS_FIBER_ERROR_NULL;
    }
    if ((u64)fiber & 7 || (u64)addr_context & 15) {
        return ORBIS_FIBER_ERROR_ALIGNMENT;
    }
    if (fiber->magic_start != kFiberSignature0 || fiber->magic_end != kFiberSignature1) {
        return ORBIS_FIBER_ERROR_INVALID;
    }

    OrbisFiberContext* g_ctx = GetFiberContext();
    if (!g_ctx) {
        return ORBIS_FIBER_ERROR_PERMISSION;
    }

    /* Caller wants to attach context and switch. */
    if (addr_context != nullptr || size_context != 0) {
        s32 res = _sceFiberAttachContext(fiber, addr_context, size_context);
        if (res < 0) {
            return res;
        }
    }

    FiberState expected = FiberState::Idle;
    if (!fiber->state.compare_exchange_strong(expected, FiberState::Run)) {
        return ORBIS_FIBER_ERROR_STATE;
    }

    OrbisFiber* cur_fiber = g_ctx->current_fiber;
    if (cur_fiber->addr_context == nullptr) {
        _sceFiberSwitch(cur_fiber, fiber, arg_on_run_to, g_ctx);
        __builtin_trap();
    }

    OrbisFiberContext ctx{};
    s32 jmp = _sceFiberSetJmp(&ctx);
    if (!jmp) {
        cur_fiber->context = &ctx;
        _sceFiberCheckStackOverflow(g_ctx);
        _sceFiberSwitch(cur_fiber, fiber, arg_on_run_to, g_ctx);
        __builtin_trap();
    }

    g_ctx = GetFiberContext();
    if (g_ctx->prev_fiber) {
        g_ctx->prev_fiber->state = FiberState::Idle;
        g_ctx->prev_fiber = nullptr;
    }

    if (arg_on_run) {
        *arg_on_run = g_ctx->arg_on_run_to;
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFiberGetSelf(OrbisFiber** fiber) {
    if (!fiber) {
        return ORBIS_FIBER_ERROR_NULL;
    }

    OrbisFiberContext* g_ctx = GetFiberContext();
    if (!g_ctx) {
        return ORBIS_FIBER_ERROR_PERMISSION;
    }

    *fiber = g_ctx->current_fiber;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFiberReturnToThread(u64 arg_on_return, u64* arg_on_run) {
    OrbisFiberContext* g_ctx = GetFiberContext();
    if (!g_ctx) {
        return ORBIS_FIBER_ERROR_PERMISSION;
    }

    OrbisFiber* cur_fiber = g_ctx->current_fiber;
    if (cur_fiber->addr_context) {
        OrbisFiberContext ctx{};
        s32 jmp = _sceFiberSetJmp(&ctx);
        if (jmp) {
            g_ctx = GetFiberContext();
            if (g_ctx->prev_fiber) {
                g_ctx->prev_fiber->state = FiberState::Idle;
                g_ctx->prev_fiber = nullptr;
            }
            if (arg_on_run) {
                *arg_on_run = g_ctx->arg_on_run_to;
            }
            return ORBIS_OK;
        }

        cur_fiber->context = &ctx;
        _sceFiberCheckStackOverflow(g_ctx);
    }

    _sceFiberTerminate(cur_fiber, arg_on_return, g_ctx);
    __builtin_trap();
}

s32 PS4_SYSV_ABI sceFiberGetInfo(OrbisFiber* fiber, OrbisFiberInfo* fiber_info) {
    if (!fiber || !fiber_info) {
        return ORBIS_FIBER_ERROR_NULL;
    }
    if ((u64)fiber & 7 || (u64)fiber_info & 7) {
        return ORBIS_FIBER_ERROR_ALIGNMENT;
    }
    if (fiber_info->size != sizeof(OrbisFiberInfo)) {
        return ORBIS_FIBER_ERROR_INVALID;
    }
    if (fiber->magic_start != kFiberSignature0 || fiber->magic_end != kFiberSignature1) {
        return ORBIS_FIBER_ERROR_INVALID;
    }

    fiber_info->entry = fiber->entry;
    fiber_info->arg_on_initialize = fiber->arg_on_initialize;
    fiber_info->addr_context = fiber->addr_context;
    fiber_info->size_context = fiber->size_context;
    strncpy(fiber_info->name, fiber->name, ORBIS_FIBER_MAX_NAME_LENGTH);

    fiber_info->size_context_margin = -1;
    if (fiber->flags & FiberFlags::ContextSizeCheck && fiber->addr_context != nullptr) {
        u64 stack_margin = 0;
        u64* stack_start = reinterpret_cast<u64*>(fiber->context_start);
        u64* stack_end = reinterpret_cast<u64*>(fiber->context_end);

        if (*stack_start == kFiberStackSignature) {
            u64* stack_ptr = stack_start + 1;
            while (stack_ptr < stack_end) {
                if (*stack_ptr == kFiberStackSizeCheck) {
                    stack_ptr++;
                }
            }

            stack_margin =
                reinterpret_cast<u64>(stack_ptr) - reinterpret_cast<u64>(stack_start + 1);
        }

        fiber_info->size_context_margin = stack_margin;
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFiberStartContextSizeCheck(u32 flags) {
    if (flags != 0) {
        return ORBIS_FIBER_ERROR_INVALID;
    }

    u32 expected = 0;
    if (!context_size_check.compare_exchange_strong(expected, 1u)) {
        return ORBIS_FIBER_ERROR_STATE;
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFiberStopContextSizeCheck() {
    u32 expected = 1;
    if (!context_size_check.compare_exchange_strong(expected, 0u)) {
        return ORBIS_FIBER_ERROR_STATE;
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFiberRename(OrbisFiber* fiber, const char* name) {
    if (!fiber || !name) {
        return ORBIS_FIBER_ERROR_NULL;
    }
    if ((u64)fiber & 7) {
        return ORBIS_FIBER_ERROR_ALIGNMENT;
    }
    if (fiber->magic_start != kFiberSignature0 || fiber->magic_end != kFiberSignature1) {
        return ORBIS_FIBER_ERROR_INVALID;
    }

    strncpy(fiber->name, name, ORBIS_FIBER_MAX_NAME_LENGTH);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFiberGetThreadFramePointerAddress(u64* addr_frame_pointer) {
    if (!addr_frame_pointer) {
        return ORBIS_FIBER_ERROR_NULL;
    }

    OrbisFiberContext* g_ctx = GetFiberContext();
    if (!g_ctx) {
        return ORBIS_FIBER_ERROR_PERMISSION;
    }

    *addr_frame_pointer = g_ctx->rbp;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFiberInitialize(OrbisFiber* fiber, const char* name, OrbisFiberEntry entry,
                                    u64 arg_on_initialize, void* addr_context, u64 size_context,
                                    const OrbisFiberOptParam* opt_param, u32 build_ver) {
    return sceFiberInitializeImpl(fiber, name, entry, arg_on_initialize, addr_context, size_context,
                                  opt_param, 0, build_ver);
}

s32 PS4_SYSV_ABI sceFiberRun(OrbisFiber* fiber, u64 arg_on_run_to, u64* arg_on_return) {
    return sceFiberRunImpl(fiber, nullptr, 0, arg_on_run_to, arg_on_return);
}

s32 PS4_SYSV_ABI sceFiberSwitch(OrbisFiber* fiber, u64 arg_on_run_to, u64* arg_on_run) {
    return sceFiberSwitchImpl(fiber, nullptr, 0, arg_on_run_to, arg_on_run);
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("hVYD7Ou2pCQ", "libSceFiber", 1, "libSceFiber", sceFiberInitialize);
    LIB_FUNCTION("7+OJIpko9RY", "libSceFiber", 1, "libSceFiber",
                 sceFiberInitializeImpl); // _sceFiberInitializeWithInternalOptionImpl
    LIB_FUNCTION("asjUJJ+aa8s", "libSceFiber", 1, "libSceFiber", sceFiberOptParamInitialize);
    LIB_FUNCTION("JeNX5F-NzQU", "libSceFiber", 1, "libSceFiber", sceFiberFinalize);

    LIB_FUNCTION("a0LLrZWac0M", "libSceFiber", 1, "libSceFiber", sceFiberRun);
    LIB_FUNCTION("PFT2S-tJ7Uk", "libSceFiber", 1, "libSceFiber", sceFiberSwitch);
    LIB_FUNCTION("p+zLIOg27zU", "libSceFiber", 1, "libSceFiber", sceFiberGetSelf);
    LIB_FUNCTION("B0ZX2hx9DMw", "libSceFiber", 1, "libSceFiber", sceFiberReturnToThread);

    LIB_FUNCTION("avfGJ94g36Q", "libSceFiber", 1, "libSceFiber",
                 sceFiberRunImpl); // _sceFiberAttachContextAndRun
    LIB_FUNCTION("ZqhZFuzKT6U", "libSceFiber", 1, "libSceFiber",
                 sceFiberSwitchImpl); // _sceFiberAttachContextAndSwitch

    LIB_FUNCTION("uq2Y5BFz0PE", "libSceFiber", 1, "libSceFiber", sceFiberGetInfo);
    LIB_FUNCTION("Lcqty+QNWFc", "libSceFiber", 1, "libSceFiber", sceFiberStartContextSizeCheck);
    LIB_FUNCTION("Kj4nXMpnM8Y", "libSceFiber", 1, "libSceFiber", sceFiberStopContextSizeCheck);
    LIB_FUNCTION("JzyT91ucGDc", "libSceFiber", 1, "libSceFiber", sceFiberRename);

    LIB_FUNCTION("0dy4JtMUcMQ", "libSceFiber", 1, "libSceFiber",
                 sceFiberGetThreadFramePointerAddress);
}

} // namespace Libraries::Fiber
