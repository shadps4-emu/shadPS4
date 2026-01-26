// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "fiber.h"

#include "common/elf_info.h"
#include "common/logging/log.h"
#include "core/libraries/fiber/fiber_error.h"
#include "core/libraries/kernel/memory.h"
#include "core/libraries/kernel/threads/pthread.h"
#include "core/libraries/libs.h"
#include "core/libraries/razor_cpu/razor_cpu.h"
#include "core/libraries/system/sysmodule.h"
#include "core/libraries/ulobjmgr/ulobjmgr.h"
#include "core/tls.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace Libraries::Fiber {

static constexpr u32 kFiberSignature0 = 0xdef1649c;
static constexpr u32 kFiberSignature1 = 0xb37592a0;
static constexpr u32 kFiberOptSignature = 0xbb40e64d;
static constexpr u64 kFiberEntryXor = 0xca953a6953c56aa5;
static constexpr u64 kFiberArgInitXor = 0xa356a3569c95ca5a;
static constexpr u32 kFiberRazorIdXor = 0x5a4c69a5;
static constexpr u64 kFiberSwitchCookieInit = 0xa5a569695c5c5a5a;
static constexpr u64 kFiberStackSignature = 0x7149f2ca7149f2ca;
static constexpr u64 kFiberStackSizeCheck = 0xdeadbeefdeadbeef;
static constexpr u64 kFiberNameSeedInit = 0x1234567812345678;
static constexpr u64 kFiberNameSeedMul = 0xfedcba89fedcba89;
static constexpr u64 kFiberNameSeedAdd = 0x9182736591827365;

static constexpr u8 kFiberNameXorTable[ORBIS_FIBER_MAX_NAME_LENGTH + 1] = {
    0x5a, 0x5a, 0x66, 0x66, 0x99, 0x99, 0x66, 0x96, 0x99, 0x66, 0x99, 0x96, 0x33, 0x33, 0xcc, 0xcc,
    0x33, 0xc3, 0xcc, 0x33, 0xcc, 0xc3, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0xa5, 0xaa, 0x55, 0xaa, 0xa5,
};

static std::atomic<u32> context_size_check = false;
static std::atomic<u64> name_seed = kFiberNameSeedInit;
static std::atomic<u32> fiber_globals_init = false;
static std::atomic<u32> razor_enabled = false;
static std::atomic<u32> asan_enabled = false;
static std::atomic<u64> switch_cookie_counter = 0;

static u64 PthreadSelf() {
    auto* thread = ::Libraries::Kernel::posix_pthread_self();
    return reinterpret_cast<u64>(thread);
}

static void GetThreadStack(void** stack_addr, size_t* stack_size) {
    if (!stack_addr || !stack_size) {
        return;
    }

    auto* thread = ::Libraries::Kernel::g_curthread;
    if (!thread) {
        *stack_addr = nullptr;
        *stack_size = 0;
        return;
    }

    *stack_addr = thread->attr.stackaddr_attr;
    *stack_size = thread->attr.stacksize_attr;
}

static void RazorCpuFiberSwitch(u32 from_id, u32 to_id, u32 reason) {
    (void)from_id;
    (void)to_id;
    (void)reason;
    ::Libraries::RazorCpu::sceRazorCpuFiberSwitch();
}

static void RazorCpuFiberLogNameChange(OrbisFiber* fiber, const char* name) {
    (void)fiber;
    (void)name;
    ::Libraries::RazorCpu::sceRazorCpuFiberLogNameChange();
}

static s32 UlobjmgrRegister(u64 arg0, s32 arg1, u32* arg2) {
    return ::Libraries::Ulobjmgr::Func_046DBA8411A2365C(arg0, arg1, arg2);
}

static s32 UlobjmgrUnregister(u32 arg0) {
    return ::Libraries::Ulobjmgr::Func_4A67FE7D435B94F7(arg0);
}

extern "C" {
void PS4_SYSV_ABI __sanitizer_start_switch_fiber(void** fake_stack_save, const void* stack_addr,
                                                 size_t stack_size);
void PS4_SYSV_ABI __sanitizer_finish_switch_fiber(void* fake_stack_save,
                                                  const void** old_stack_addr,
                                                  size_t* old_stack_size);
void PS4_SYSV_ABI __asan_destroy_fake_stack();
} // extern "C"

extern "C" void PS4_SYSV_ABI __sanitizer_start_switch_fiber(void** fake_stack_save,
                                                            const void* stack_addr,
                                                            size_t stack_size) {
    (void)fake_stack_save;
    (void)stack_addr;
    (void)stack_size;
}

extern "C" void PS4_SYSV_ABI __sanitizer_finish_switch_fiber(void* fake_stack_save,
                                                             const void** old_stack_addr,
                                                             size_t* old_stack_size) {
    (void)fake_stack_save;
    (void)old_stack_addr;
    (void)old_stack_size;
}

extern "C" void PS4_SYSV_ABI __asan_destroy_fake_stack() {}

static void EnsureFiberGlobalsInitialized() {
    u32 expected = 0;
    if (fiber_globals_init.compare_exchange_strong(expected, 1u, std::memory_order_relaxed)) {
        const auto razor_loaded = ::Libraries::SysModule::sceSysmoduleIsLoadedInternal(
            ::Libraries::SysModule::OrbisSysModuleInternal::ORBIS_SYSMODULE_INTERNAL_RAZOR_CPU);
        razor_enabled.store(razor_loaded == 0 ? 1u : 0u, std::memory_order_relaxed);
        asan_enabled.store(::Libraries::Kernel::sceKernelIsAddressSanitizerEnabled() != 0 ? 1u : 0u,
                           std::memory_order_relaxed);
    }
}

static bool RazorEnabled() {
    EnsureFiberGlobalsInitialized();
    return razor_enabled.load(std::memory_order_relaxed) != 0;
}

static bool AsanEnabled() {
    EnsureFiberGlobalsInitialized();
    return asan_enabled.load(std::memory_order_relaxed) != 0;
}

static void UpdateSwitchCookie(OrbisFiber* fiber) {
    const u64 value = switch_cookie_counter.fetch_add(1, std::memory_order_relaxed) + 1;
    fiber->switch_cookie = value ^ kFiberSwitchCookieInit;
}

static u64 ComputeContextSizeMargin(const OrbisFiber* fiber) {
    if (!fiber || !fiber->context_start || !fiber->context_end) {
        return 0;
    }

    u64* stack_start = reinterpret_cast<u64*>(fiber->context_start);
    u64* stack_end = reinterpret_cast<u64*>(fiber->context_end);
    if (stack_start >= stack_end) {
        return 0;
    }

    u64* stack_ptr = stack_start + 1;
    while (stack_ptr < stack_end && *stack_ptr == kFiberStackSizeCheck) {
        ++stack_ptr;
    }

    return reinterpret_cast<u64>(stack_ptr) - reinterpret_cast<u64>(stack_start + 1);
}

static u64 NextNameSeed() {
    u64 seed = name_seed.load(std::memory_order_relaxed);
    u64 next = 0;
    do {
        next = seed * kFiberNameSeedMul + kFiberNameSeedAdd;
    } while (!name_seed.compare_exchange_weak(seed, next, std::memory_order_relaxed));
    return next;
}

static void FillFiberRandomPad(OrbisFiber* fiber) {
    if (!fiber) {
        return;
    }

    constexpr size_t kRandomPadWords = 0x78 / sizeof(u64);
    u64 values[kRandomPadWords]{};
    for (size_t i = 0; i < kRandomPadWords; ++i) {
        values[i] = NextNameSeed();
    }
    std::memcpy(fiber->random_pad, values, sizeof(values));
}

static void EncodeFiberName(OrbisFiber* fiber, const char* name) {
    if (!fiber) {
        return;
    }

    u64 seed_values[4] = {NextNameSeed(), NextNameSeed(), NextNameSeed(), NextNameSeed()};
    std::memcpy(fiber->name_xor, seed_values, sizeof(seed_values));

    if (!name) {
        fiber->name_xor[0] = kFiberNameXorTable[0];
        return;
    }

    for (u32 i = 0; i < ORBIS_FIBER_MAX_NAME_LENGTH; ++i) {
        const u8 value = static_cast<u8>(name[i]);
        if (value == 0) {
            fiber->name_xor[i] = kFiberNameXorTable[i];
            return;
        }
        fiber->name_xor[i] = value ^ kFiberNameXorTable[i];
    }

    fiber->name_xor[ORBIS_FIBER_MAX_NAME_LENGTH] = 0xa5;
}

static void DecodeFiberName(const OrbisFiber* fiber, char* out, size_t out_size) {
    if (!fiber || !out || out_size < ORBIS_FIBER_MAX_NAME_LENGTH + 1) {
        return;
    }

    for (u32 i = 0; i < ORBIS_FIBER_MAX_NAME_LENGTH; ++i) {
        const u8 value = fiber->name_xor[i] ^ kFiberNameXorTable[i];
        out[i] = static_cast<char>(value);
        if (value == 0) {
            std::memset(out + i + 1, 0, ORBIS_FIBER_MAX_NAME_LENGTH - i);
            return;
        }
    }

    out[ORBIS_FIBER_MAX_NAME_LENGTH] = '\0';
}

static OrbisFiberEntry DecodeEntry(const OrbisFiber* fiber) {
    return reinterpret_cast<OrbisFiberEntry>(fiber->entry_xor ^ kFiberEntryXor);
}

static void EncodeEntry(OrbisFiber* fiber, OrbisFiberEntry entry) {
    fiber->entry_xor = reinterpret_cast<u64>(entry) ^ kFiberEntryXor;
}

static u64 DecodeArgOnInitialize(const OrbisFiber* fiber) {
    return fiber->arg_on_initialize_xor ^ kFiberArgInitXor;
}

static void EncodeArgOnInitialize(OrbisFiber* fiber, u64 arg_on_initialize) {
    fiber->arg_on_initialize_xor = arg_on_initialize ^ kFiberArgInitXor;
}

static bool TryTransitionFiberState(OrbisFiber* fiber, FiberState expected, FiberState desired) {
    std::atomic_ref<u32> state_ref(fiber->state);
    u32 expected_value = static_cast<u32>(expected);
    return state_ref.compare_exchange_strong(expected_value, static_cast<u32>(desired),
                                             std::memory_order_seq_cst);
}

static void StoreFiberState(OrbisFiber* fiber, FiberState value) {
    std::atomic_ref<u32> state_ref(fiber->state);
    state_ref.store(static_cast<u32>(value), std::memory_order_seq_cst);
}

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
    OrbisFiber* fiber = ctx->current_fiber;
    u64* stack_base = reinterpret_cast<u64*>(fiber->addr_context);
    u64 stack_size = fiber->size_context;
    if (stack_base && *stack_base != kFiberStackSignature) {
        char name[ORBIS_FIBER_MAX_NAME_LENGTH + 1]{};
        DecodeFiberName(fiber, name, sizeof(name));

        const uintptr_t stack_base_addr = reinterpret_cast<uintptr_t>(stack_base);
        const uintptr_t stack_top_addr = stack_base_addr + static_cast<uintptr_t>(stack_size);
        LOG_CRITICAL(
            Lib_Fiber,
            "Fiber stack overflow: name='{}' fiber={:#x} ctx={:#x} stack_base={:#x} "
            "stack_top={:#x} size=0x{:x} sig={:#x} expected={:#x} context_start={:#x} "
            "context_end={:#x} flags=0x{:x} state=0x{:x} switch_cookie={:#x} magic_start=0x{:x} "
            "magic_end=0x{:x}",
            name, reinterpret_cast<uintptr_t>(fiber), reinterpret_cast<uintptr_t>(ctx),
            stack_base_addr, stack_top_addr, stack_size, *stack_base, kFiberStackSignature,
            reinterpret_cast<uintptr_t>(fiber->context_start),
            reinterpret_cast<uintptr_t>(fiber->context_end), fiber->flags, fiber->state,
            fiber->switch_cookie, fiber->magic_start, fiber->magic_end);
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
    LOG_INFO(
        Lib_Fiber,
        "Fiber attach context: fiber={:#x} addr_context={:#x} size=0x{:x} sig={:#x} flags=0x{:x}",
        reinterpret_cast<uintptr_t>(fiber), reinterpret_cast<uintptr_t>(addr_context), size_context,
        *(u64*)addr_context, fiber->flags);

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
                                         OrbisFiberContext* ctx, u64 asan_cookie) {
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

    data.entry = DecodeEntry(fiber);
    data.arg_on_initialize = DecodeArgOnInitialize(fiber);
    data.arg_on_run_to = arg_on_run_to;
    data.stack_addr = reinterpret_cast<u8*>(fiber->addr_context) + fiber->size_context;
    data.asan_fake_stack = reinterpret_cast<void*>(asan_cookie);
    _sceFiberSwitchEntry(&data, (fiber->flags & FiberFlags::SetFpuRegs) != 0);

    __builtin_trap();
}

void PS4_SYSV_ABI _sceFiberSwitch(OrbisFiber* cur_fiber, OrbisFiber* fiber, u64 arg_on_run_to,
                                  OrbisFiberContext* ctx, u64 asan_cookie) {
    UpdateSwitchCookie(cur_fiber);
    ctx->prev_fiber = cur_fiber;
    ctx->current_fiber = fiber;

    if (RazorEnabled()) {
        RazorCpuFiberSwitch(static_cast<u32>(reinterpret_cast<uintptr_t>(cur_fiber)),
                            static_cast<u32>(reinterpret_cast<uintptr_t>(fiber)), 1);
    }

    if (fiber->addr_context == nullptr) {
        ctx->prev_fiber = nullptr;

        OrbisFiberData data{};
        data.entry = DecodeEntry(fiber);
        data.arg_on_initialize = DecodeArgOnInitialize(fiber);
        data.arg_on_run_to = arg_on_run_to;
        data.stack_addr = reinterpret_cast<void*>(ctx->jmp.rsp & ~15);
        data.state = reinterpret_cast<u32*>(&cur_fiber->state);
        if (asan_cookie != 0) {
            data.asan_fake_stack =
                reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(fiber->asan_fake_stack) + 1);
        } else {
            data.asan_fake_stack = nullptr;
        }
        _sceFiberSwitchEntry(&data, (fiber->flags & FiberFlags::SetFpuRegs) != 0);

        __builtin_trap();
    }

    _sceFiberSwitchToFiber(fiber, arg_on_run_to, ctx, asan_cookie);
    __builtin_trap();
}

void PS4_SYSV_ABI _sceFiberTerminate(OrbisFiber* fiber, u64 arg_on_return, OrbisFiberContext* ctx) {
    UpdateSwitchCookie(fiber);
    ctx->arg_on_return = arg_on_return;
    _sceFiberLongJmp(ctx);
    __builtin_trap();
}

s32 PS4_SYSV_ABI sceFiberInitializeImpl(OrbisFiber* fiber, const char* name, OrbisFiberEntry entry,
                                        u64 arg_on_initialize, void* addr_context, u64 size_context,
                                        const OrbisFiberOptParam* opt_param, u32 flags,
                                        u32 build_ver) {
    EnsureFiberGlobalsInitialized();
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
    if (size_context && size_context <= 4096) {
        LOG_WARNING(Lib_Fiber, "Fiber initialized with small stack area.");
    }
    if (opt_param && opt_param->magic != kFiberOptSignature) {
        return ORBIS_FIBER_ERROR_INVALID;
    }

    u32 user_flags = flags;
    if (build_ver >= Common::ElfInfo::FW_35) {
        user_flags |= FiberFlags::SetFpuRegs;
    }
    if (context_size_check.load(std::memory_order_relaxed) != 0) {
        user_flags |= FiberFlags::ContextSizeCheck;
    }

    FillFiberRandomPad(fiber);
    EncodeFiberName(fiber, name);
    EncodeEntry(fiber, entry);
    EncodeArgOnInitialize(fiber, arg_on_initialize);
    fiber->addr_context = addr_context;
    fiber->size_context = size_context;
    fiber->context = nullptr;
    fiber->owner_thread = 0;
    fiber->flags = user_flags;
    fiber->razor_id_xor = 0;
    fiber->switch_cookie = kFiberSwitchCookieInit;
    fiber->asan_fake_stack = nullptr;
    fiber->context_start = nullptr;
    fiber->context_end = nullptr;
    fiber->reserved = 0;

    fiber->magic_start = kFiberSignature0;
    fiber->magic_end = kFiberSignature1;

    if (addr_context != nullptr) {
        fiber->context_start = addr_context;
        fiber->context_end = reinterpret_cast<u8*>(addr_context) + size_context;

        /* Apply signature to start of stack */
        *(u64*)addr_context = kFiberStackSignature;
        LOG_INFO(Lib_Fiber,
                 "Fiber init context: fiber={:#x} name='{}' addr_context={:#x} size=0x{:x} "
                 "sig={:#x} flags=0x{:x}",
                 reinterpret_cast<uintptr_t>(fiber), name,
                 reinterpret_cast<uintptr_t>(addr_context), size_context, *(u64*)addr_context,
                 fiber->flags);

        if (fiber->flags & FiberFlags::ContextSizeCheck) {
            u64* stack_start = reinterpret_cast<u64*>(fiber->context_start);
            u64* stack_end = reinterpret_cast<u64*>(fiber->context_end);

            u64* stack_ptr = stack_start + 1;
            while (stack_ptr < stack_end) {
                *stack_ptr++ = kFiberStackSizeCheck;
            }
        }
    }

    StoreFiberState(fiber, FiberState::Idle);
    if ((fiber->flags & FiberFlags::NoUlobjmgr) == 0) {
        LOG_DEBUG(Lib_Fiber, "Ulobjmgr register: fiber={:#x} flags=0x{:x}",
                  reinterpret_cast<uintptr_t>(fiber), fiber->flags);
        u32 razor_id = 0;
        UlobjmgrRegister(reinterpret_cast<u64>(fiber), 1, &razor_id);
        fiber->razor_id_xor = razor_id ^ kFiberRazorIdXor;
    } else {
        LOG_DEBUG(Lib_Fiber, "Ulobjmgr register skipped (NoUlobjmgr): fiber={:#x} flags=0x{:x}",
                  reinterpret_cast<uintptr_t>(fiber), fiber->flags);
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFiberOptParamInitialize(OrbisFiberOptParam* opt_param) {
    if (!opt_param) {
        return ORBIS_FIBER_ERROR_NULL;
    }
    if ((u64)opt_param & 7) {
        return ORBIS_FIBER_ERROR_ALIGNMENT;
    }

    std::memset(opt_param, 0, sizeof(*opt_param));
    opt_param->magic = kFiberOptSignature;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFiberFinalize(OrbisFiber* fiber) {
    EnsureFiberGlobalsInitialized();
    if (!fiber) {
        return ORBIS_FIBER_ERROR_NULL;
    }
    if ((u64)fiber & 7) {
        return ORBIS_FIBER_ERROR_ALIGNMENT;
    }
    if (fiber->magic_start != kFiberSignature0 || fiber->magic_end != kFiberSignature1) {
        return ORBIS_FIBER_ERROR_INVALID;
    }

    if (!TryTransitionFiberState(fiber, FiberState::Idle, FiberState::Terminated)) {
        return ORBIS_FIBER_ERROR_STATE;
    }

    if (RazorEnabled()) {
        char name[ORBIS_FIBER_MAX_NAME_LENGTH + 1]{};
        DecodeFiberName(fiber, name, sizeof(name));
        RazorCpuFiberLogNameChange(fiber, name);
    }
    if ((fiber->flags & FiberFlags::NoUlobjmgr) == 0) {
        LOG_DEBUG(Lib_Fiber, "Ulobjmgr unregister: fiber={:#x} flags=0x{:x}",
                  reinterpret_cast<uintptr_t>(fiber), fiber->flags);
        UlobjmgrUnregister(fiber->razor_id_xor ^ kFiberRazorIdXor);
    } else {
        LOG_DEBUG(Lib_Fiber, "Ulobjmgr unregister skipped (NoUlobjmgr): fiber={:#x} flags=0x{:x}",
                  reinterpret_cast<uintptr_t>(fiber), fiber->flags);
    }
    if (AsanEnabled() && fiber->asan_fake_stack != nullptr && fiber->addr_context != nullptr) {
        __asan_destroy_fake_stack();
        fiber->asan_fake_stack = nullptr;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFiberRunImpl(OrbisFiber* fiber, void* addr_context, u64 size_context,
                                 u64 arg_on_run_to, u64* arg_on_return) {
    EnsureFiberGlobalsInitialized();
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

    const bool attach_context = (addr_context != nullptr || size_context != 0);
    if (attach_context) {
        if (size_context != 0 && size_context < ORBIS_FIBER_CONTEXT_MINIMUM_SIZE) {
            return ORBIS_FIBER_ERROR_RANGE;
        }
        if (!addr_context || size_context == 0 || (size_context & 15)) {
            return ORBIS_FIBER_ERROR_INVALID;
        }
        if (fiber->addr_context != nullptr) {
            return ORBIS_FIBER_ERROR_INVALID;
        }
    }

    if (!TryTransitionFiberState(fiber, FiberState::Idle, FiberState::Run)) {
        return ORBIS_FIBER_ERROR_STATE;
    }

    /* Caller wants to attach context and run. */
    if (attach_context) {
        s32 res = _sceFiberAttachContext(fiber, addr_context, size_context);
        if (res < 0) {
            return res;
        }
    }

    fiber->owner_thread = PthreadSelf();

    OrbisFiberContext ctx{};
    ctx.current_fiber = fiber;
    ctx.prev_fiber = nullptr;
    ctx.arg_on_run_to = 0;
    ctx.arg_on_return = 0;
    ctx.return_val = 0;
    ctx.owner_thread = fiber->owner_thread;
    ctx.asan_fake_stack = nullptr;
    ctx.reserved0 = 0;
    ctx.reserved1 = 0;
    ctx.reserved2 = 0xffffffffu;
    ctx.reserved3 = 0xffffffffu;

    tcb->tcb_fiber = &ctx;

    if (RazorEnabled()) {
        RazorCpuFiberSwitch(0, static_cast<u32>(reinterpret_cast<uintptr_t>(fiber)), 2);
    }

    bool asan_switch = false;
    if (AsanEnabled() && fiber->addr_context != nullptr) {
        __sanitizer_start_switch_fiber(&ctx.asan_fake_stack, fiber->addr_context,
                                       fiber->size_context);
        asan_switch = true;
    }

    s32 jmp = _sceFiberSetJmp(&ctx);
    if (!jmp) {
        if (fiber->addr_context) {
            _sceFiberSwitchToFiber(fiber, arg_on_run_to, &ctx, asan_switch ? 1 : 0);
            __builtin_trap();
        }

        OrbisFiberData data{};
        data.entry = DecodeEntry(fiber);
        data.arg_on_initialize = DecodeArgOnInitialize(fiber);
        data.arg_on_run_to = arg_on_run_to;
        data.stack_addr = reinterpret_cast<void*>(ctx.jmp.rsp & ~15);
        data.state = nullptr;
        data.asan_fake_stack = nullptr;
        _sceFiberSwitchEntry(&data, (fiber->flags & FiberFlags::SetFpuRegs) != 0);
    }

    if (asan_switch) {
        __sanitizer_finish_switch_fiber(ctx.asan_fake_stack, nullptr, nullptr);
    }

    OrbisFiber* cur_fiber = ctx.current_fiber;
    ctx.current_fiber = nullptr;

    if (RazorEnabled()) {
        RazorCpuFiberSwitch(static_cast<u32>(reinterpret_cast<uintptr_t>(cur_fiber)), 0, 3);
    }
    StoreFiberState(cur_fiber, FiberState::Idle);

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
    EnsureFiberGlobalsInitialized();
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

    const bool attach_context = (addr_context != nullptr || size_context != 0);
    if (attach_context) {
        if (size_context != 0 && size_context < ORBIS_FIBER_CONTEXT_MINIMUM_SIZE) {
            return ORBIS_FIBER_ERROR_RANGE;
        }
        if (!addr_context || size_context == 0 || (size_context & 15)) {
            return ORBIS_FIBER_ERROR_INVALID;
        }
        if (fiber->addr_context != nullptr) {
            return ORBIS_FIBER_ERROR_INVALID;
        }
    }

    if (!TryTransitionFiberState(fiber, FiberState::Idle, FiberState::Run)) {
        return ORBIS_FIBER_ERROR_STATE;
    }

    /* Caller wants to attach context and switch. */
    if (attach_context) {
        s32 res = _sceFiberAttachContext(fiber, addr_context, size_context);
        if (res < 0) {
            return res;
        }
    }

    fiber->owner_thread = g_ctx->owner_thread;

    OrbisFiber* cur_fiber = g_ctx->current_fiber;
    if (cur_fiber->addr_context == nullptr) {
        u64 asan_cookie = 0;
        if (AsanEnabled() && fiber->addr_context != nullptr) {
            __sanitizer_start_switch_fiber(&cur_fiber->asan_fake_stack, fiber->addr_context,
                                           fiber->size_context);
            asan_cookie = 1;
        }
        _sceFiberSwitch(cur_fiber, fiber, arg_on_run_to, g_ctx, asan_cookie);
        __builtin_trap();
    }

    if (AsanEnabled()) {
        void* stack_addr = nullptr;
        size_t stack_size = 0;
        if (fiber->addr_context == nullptr) {
            GetThreadStack(&stack_addr, &stack_size);
        } else {
            stack_addr = fiber->addr_context;
            stack_size = fiber->size_context;
        }
        __sanitizer_start_switch_fiber(&cur_fiber->asan_fake_stack, stack_addr, stack_size);
    }

    OrbisFiberContext ctx{};
    s32 jmp = _sceFiberSetJmp(&ctx);
    if (!jmp) {
        cur_fiber->context = &ctx;
        _sceFiberCheckStackOverflow(g_ctx);
        _sceFiberSwitch(cur_fiber, fiber, arg_on_run_to, g_ctx, AsanEnabled() ? 1 : 0);
        __builtin_trap();
    }

    if (AsanEnabled()) {
        __sanitizer_finish_switch_fiber(cur_fiber->asan_fake_stack, nullptr, nullptr);
    }

    g_ctx = GetFiberContext();
    if (g_ctx->prev_fiber) {
        StoreFiberState(g_ctx->prev_fiber, FiberState::Idle);
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
        if (AsanEnabled()) {
            void* stack_addr = nullptr;
            size_t stack_size = 0;
            GetThreadStack(&stack_addr, &stack_size);
            __sanitizer_start_switch_fiber(&cur_fiber->asan_fake_stack, stack_addr, stack_size);
        }

        OrbisFiberContext ctx{};
        s32 jmp = _sceFiberSetJmp(&ctx);
        if (jmp) {
            if (AsanEnabled()) {
                __sanitizer_finish_switch_fiber(cur_fiber->asan_fake_stack, nullptr, nullptr);
            }
            g_ctx = GetFiberContext();
            if (g_ctx->prev_fiber) {
                StoreFiberState(g_ctx->prev_fiber, FiberState::Idle);
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

    fiber_info->entry = DecodeEntry(fiber);
    fiber_info->arg_on_initialize = DecodeArgOnInitialize(fiber);
    fiber_info->addr_context = fiber->addr_context;
    fiber_info->size_context = fiber->size_context;
    DecodeFiberName(fiber, fiber_info->name, sizeof(fiber_info->name));

    fiber_info->size_context_margin = -1;
    if (fiber->flags & FiberFlags::ContextSizeCheck && fiber->addr_context != nullptr) {
        u64 stack_margin = 0;
        u64* stack_start = reinterpret_cast<u64*>(fiber->context_start);
        u64* stack_end = reinterpret_cast<u64*>(fiber->context_end);

        if (*stack_start == kFiberStackSignature) {
            u64* stack_ptr = stack_start + 1;
            while (stack_ptr < stack_end && *stack_ptr == kFiberStackSizeCheck) {
                stack_ptr++;
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
    EnsureFiberGlobalsInitialized();
    if (!fiber || !name) {
        return ORBIS_FIBER_ERROR_NULL;
    }
    if ((u64)fiber & 7) {
        return ORBIS_FIBER_ERROR_ALIGNMENT;
    }
    if (fiber->magic_start != kFiberSignature0 || fiber->magic_end != kFiberSignature1) {
        return ORBIS_FIBER_ERROR_INVALID;
    }

    if (RazorEnabled()) {
        char old_name[ORBIS_FIBER_MAX_NAME_LENGTH + 1]{};
        DecodeFiberName(fiber, old_name, sizeof(old_name));
        RazorCpuFiberLogNameChange(fiber, old_name);
    }

    EncodeFiberName(fiber, name);
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

    *addr_frame_pointer = g_ctx->jmp.rbp;
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
