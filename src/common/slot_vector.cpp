// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstddef>
#include "common/assert.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

namespace Common {

void* ReserveMemoryPages(std::size_t size) noexcept {
#ifdef _WIN32
    void* const base = VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
    ASSERT(base);
#else
    void* const base =
        mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    ASSERT(base != MAP_FAILED);
#endif
    return base;
}

void CommitMemoryPages(void* address, std::size_t size) noexcept {
#ifdef _WIN32
    void* const result = VirtualAlloc(address, size, MEM_COMMIT, PAGE_READWRITE);
#else
    void* const result =
        mmap(address, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
#endif
    ASSERT(result == address);
}

void ReleaseMemoryPages(void* base, [[maybe_unused]] std::size_t size) noexcept {
    if (!base) {
        return;
    }
#ifdef _WIN32
    ASSERT(VirtualFree(base, 0, MEM_RELEASE));
#else
    ASSERT(munmap(base, size) == 0);
#endif
}

} // namespace Common
