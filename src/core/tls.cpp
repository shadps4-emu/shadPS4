// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <mutex>
#include "common/arch.h"
#include "common/assert.h"
#include "common/types.h"
#include "core/libraries/kernel/threads/pthread.h"
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
#if defined(__linux__) && defined(ARCH_X86_64)
#include <asm/prctl.h>
#include <sys/prctl.h>
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

#elif defined(__APPLE__) && defined(ARCH_X86_64)

// Apple x86_64

// Reserve space in the 32-bit address range for allocating TCB pages.
asm(".zerofill TCB_SPACE,TCB_SPACE,__tcb_space,0x3FC000");

struct LdtPage {
    void* tcb;
    u16 index;
};

static constexpr uintptr_t ldt_region_base = 0x4000;
static constexpr size_t ldt_region_size = 0x3FC000;
static constexpr u16 ldt_block_size = 0x1000;
static constexpr u16 ldt_index_base = 8;
static constexpr u16 ldt_index_total = (ldt_region_size - ldt_region_base) / ldt_block_size;

static boost::icl::interval_set<u16> free_ldts{};
static std::mutex free_ldts_lock;
static std::once_flag ldt_region_init_flag;
static pthread_key_t ldt_page_slot = 0;

static void FreeLdtPage(void* raw) {
    const auto* ldt_page = static_cast<LdtPage*>(raw);

    std::unique_lock lock{free_ldts_lock};
    free_ldts += ldt_page->index;
}

static void InitLdtRegion() {
    const void* result =
        mmap(reinterpret_cast<void*>(ldt_region_base), ldt_region_size, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    ASSERT_MSG(result != MAP_FAILED, "Failed to map memory region for LDT entries.");

    free_ldts +=
        boost::icl::interval<u16>::right_open(ldt_index_base, ldt_index_base + ldt_index_total);
    ASSERT_MSG(pthread_key_create(&ldt_page_slot, FreeLdtPage) == 0,
               "Failed to create thread LDT page key: {}", errno);
}

void SetTcbBase(void* image_address) {
    std::call_once(ldt_region_init_flag, InitLdtRegion);

    auto* ldt_page = static_cast<LdtPage*>(pthread_getspecific(ldt_page_slot));
    if (ldt_page != nullptr) {
        // Update TCB pointer in existing page.
        ldt_page->tcb = image_address;
        return;
    }

    // Allocate a new LDT index for the current thread.
    u16 ldt_index;
    {
        std::unique_lock lock{free_ldts_lock};
        ASSERT_MSG(!free_ldts.empty(), "Out of LDT space.");
        ldt_index = first(*free_ldts.begin());
        free_ldts -= ldt_index;
    }

    const uintptr_t addr = ldt_region_base + (ldt_index - ldt_index_base) * ldt_block_size;

    // Create an LDT entry for the TCB.
    ldt_entry ldt{};
    ldt.data = {
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
    };
    int ret = i386_set_ldt(ldt_index, &ldt, 1);
    ASSERT_MSG(ret == ldt_index,
               "Failed to set LDT {} at {:#x} for TLS area: syscall returned {}, errno {}",
               ldt_index, addr, ret, errno);

    // Set the FS segment to the created LDT.
    const sel_t new_selector{
        .rpl = USER_PRIV,
        .ti = SEL_LDT,
        .index = ldt_index,
    };
    asm volatile("mov %0, %%fs" ::"r"(new_selector));

    // Store the TCB base pointer and index in the created LDT area.
    ldt_page = reinterpret_cast<LdtPage*>(addr);
    ldt_page->tcb = image_address;
    ldt_page->index = ldt_index;

    ASSERT_MSG(pthread_setspecific(ldt_page_slot, ldt_page) == 0,
               "Failed to store thread LDT page pointer: {}", errno);
}

#elif defined(ARCH_X86_64)

// Other POSIX x86_64

void SetTcbBase(void* image_address) {
    const int ret = syscall(SYS_arch_prctl, ARCH_SET_GS, (unsigned long)image_address);
    ASSERT_MSG(ret == 0, "Failed to set GS base: errno {}", errno);
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

#endif

thread_local std::once_flag init_tls_flag;

void EnsureThreadInitialized() {
    std::call_once(init_tls_flag, [] { SetTcbBase(Libraries::Kernel::g_curthread->tcb); });
}

} // namespace Core
