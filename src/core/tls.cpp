// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <mutex>
#include "common/arch.h"
#include "common/assert.h"
#include "common/types.h"
#include "core/cpu_patches.h"
#include "core/libraries/kernel/threads/threads.h"
#include "core/tls.h"

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__) && defined(ARCH_X86_64)
#include <architecture/i386/table.h>
#include <boost/icl/interval_set.hpp>
#include <i386/user_ldt.h>
#include <sys/mman.h>
#elif !defined(ARCH_X86_64)
#include <pthread.h>
#endif

namespace Core {

#ifdef _WIN32

// Windows

static DWORD slot = 0;
static std::once_flag slot_alloc_flag;

static void AllocTcbKey() {
    slot = TlsAlloc();
}

u32 GetTcbKey() {
    std::call_once(slot_alloc_flag, &AllocTcbKey);
    return slot;
}

void SetTcbBase(void* image_address) {
    const BOOL result = TlsSetValue(GetTcbKey(), image_address);
    ASSERT(result != 0);
}

Tcb* GetTcbBase() {
    return reinterpret_cast<Tcb*>(TlsGetValue(GetTcbKey()));
}

#elif defined(__APPLE__) && defined(ARCH_X86_64)

// Apple x86_64

// Reserve space in the 32-bit address range for allocating TCB pages.
asm(".zerofill TCB_SPACE,TCB_SPACE,__guest_system,0x3FC000");

static constexpr u64 ldt_region_base = 0x4000;
static constexpr u64 ldt_region_size = 0x3FC000;
static constexpr u16 ldt_block_size = 0x1000;
static constexpr u16 ldt_index_base = 8;
static constexpr u16 ldt_index_total = (ldt_region_size - ldt_region_base) / ldt_block_size;

static boost::icl::interval_set<u16> free_ldts{};
static std::mutex free_ldts_lock;
static std::once_flag ldt_region_init_flag;

static u16 GetLdtIndex() {
    sel_t selector;
    asm volatile("mov %%fs, %0" : "=r"(selector));
    return selector.index;
}

static void InitLdtRegion() {
    const void* result =
        mmap(reinterpret_cast<void*>(ldt_region_base), ldt_region_size, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    ASSERT_MSG(result != MAP_FAILED, "Failed to map memory region for LDT entries.");

    free_ldts +=
        boost::icl::interval<u16>::right_open(ldt_index_base, ldt_index_base + ldt_index_total);
}

static void** SetupThreadLdt() {
    std::call_once(ldt_region_init_flag, InitLdtRegion);

    // Allocate a new LDT index for the current thread.
    u16 ldt_index;
    {
        std::unique_lock lock{free_ldts_lock};
        ASSERT_MSG(!free_ldts.empty(), "Out of LDT space.");
        ldt_index = first(*free_ldts.begin());
        free_ldts -= ldt_index;
    }
    const u64 addr = ldt_region_base + (ldt_index - ldt_index_base) * ldt_block_size;

    // Create an LDT entry for the TCB.
    const ldt_entry ldt{.data{
        .base00 = static_cast<u16>(addr),
        .base16 = static_cast<u8>(addr >> 16),
        .base24 = static_cast<u8>(addr >> 24),
        .limit00 = static_cast<u16>(ldt_block_size - 1),
        .limit16 = 0,
        .type = DESC_DATA_WRITE,
        .dpl = 3,     // User accessible
        .present = 1, // Segment present
        .stksz = DESC_DATA_32B,
        .granular = DESC_GRAN_BYTE,
    }};
    int ret = i386_set_ldt(ldt_index, &ldt, 1);
    ASSERT_MSG(ret == ldt_index,
               "Failed to set LDT for TLS area: expected {}, but syscall returned {}", ldt_index,
               ret);

    // Set the FS segment to the created LDT.
    const sel_t sel{
        .rpl = USER_PRIV,
        .ti = SEL_LDT,
        .index = ldt_index,
    };
    asm volatile("mov %0, %%fs" ::"r"(sel));

    return reinterpret_cast<void**>(addr);
}

static void FreeThreadLdt() {
    std::unique_lock lock{free_ldts_lock};
    free_ldts += GetLdtIndex();
}

void SetTcbBase(void* image_address) {
    if (image_address != nullptr) {
        *SetupThreadLdt() = image_address;
    } else {
        FreeThreadLdt();
    }
}

Tcb* GetTcbBase() {
    Tcb* tcb;
    asm volatile("mov %%fs:0x0, %0" : "=r"(tcb));
    return tcb;
}

#elif defined(ARCH_X86_64)

// Other POSIX x86_64

void SetTcbBase(void* image_address) {
    asm volatile("wrgsbase %0" ::"r"(image_address) : "memory");
}

Tcb* GetTcbBase() {
    Tcb* tcb;
    asm volatile("rdgsbase %0" : "=r"(tcb)::"memory");
    return tcb;
}

#else

// POSIX non-x86_64
// Just sets up a simple thread-local variable to store it, then instruction translation can point
// code to it.

static pthread_key_t slot = 0;
static std::once_flag slot_alloc_flag;

static void AllocTcbKey() {
    ASSERT(pthread_key_create(&slot, nullptr) == 0);
}

pthread_key_t GetTcbKey() {
    std::call_once(slot_alloc_flag, &AllocTcbKey);
    return slot;
}

void SetTcbBase(void* image_address) {
    ASSERT(pthread_setspecific(GetTcbKey(), image_address) == 0);
}

Tcb* GetTcbBase() {
    return static_cast<Tcb*>(pthread_getspecific(GetTcbKey()));
}

#endif

thread_local std::once_flag init_tls_flag;

void EnsureThreadInitialized() {
    std::call_once(init_tls_flag, [] {
#ifdef ARCH_X86_64
        InitializeThreadPatchStack();
#endif
        SetTcbBase(Libraries::Kernel::g_curthread->tcb);
    });
}

} // namespace Core
