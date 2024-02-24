// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/debug.h"
#include "common/log.h"
#include "common/singleton.h"
#include "core/hle/kernel/Objects/physical_memory.h"
#include "core/hle/kernel/cpu_management.h"
#include "core/hle/kernel/event_queues.h"
#include "core/hle/kernel/memory_management.h"
#include "core/hle/libraries/libkernel/file_system.h"
#include "core/hle/libraries/libkernel/libkernel.h"
#include "core/hle/libraries/libkernel/thread_management.h"
#include "core/hle/libraries/libkernel/time_management.h"
#include "core/hle/libraries/libs.h"
#include "core/loader/elf.h"

#ifdef _WIN64
#include <io.h>
#include <windows.h>
#else
#include <sys/mman.h>
#endif

namespace Core::Libraries::LibKernel {

constexpr bool log_libkernel_file = true; // disable it to disable logging

static u64 g_stack_chk_guard = 0xDEADBEEF54321ABC; // dummy return

int32_t PS4_SYSV_ABI sceKernelReleaseDirectMemory(off_t start, size_t len) {
    BREAKPOINT();
    return 0;
}

static PS4_SYSV_ABI void stack_chk_fail() {
    BREAKPOINT();
}

int PS4_SYSV_ABI sceKernelMunmap(void* addr, size_t len) {
    BREAKPOINT();
}

void PS4_SYSV_ABI sceKernelUsleep(unsigned int microseconds) {
    std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
}

struct iovec {
    void* iov_base; /* Base	address. */
    size_t iov_len; /* Length. */
};

size_t PS4_SYSV_ABI _writev(int fd, const struct iovec* iov, int iovcn) {
    // weird it gives fd ==0 and writes to stdout , i am not sure if it that is valid (found in
    // openorbis)
    size_t total_written = 0;
    for (int i = 0; i < iovcn; i++) {
        total_written += ::fwrite(iov[i].iov_base, 1, iov[i].iov_len, stdout);
    }
    return total_written;
}

static thread_local int libc_error;
int* PS4_SYSV_ABI __Error() {
    return &libc_error;
}

#define PROT_READ 0x1
#define PROT_WRITE 0x2

int PS4_SYSV_ABI sceKernelMmap(void* addr, u64 len, int prot, int flags, int fd, off_t offset,
                               void** res) {
#ifdef _WIN64
    PRINT_FUNCTION_NAME();
    if (prot > 3) // READ,WRITE or bitwise READ | WRITE supported
    {
        LOG_ERROR_IF(log_libkernel_file, "sceKernelMmap prot ={} not supported\n", prot);
    }
    DWORD flProtect;
    if (prot & PROT_WRITE) {
        flProtect = PAGE_READWRITE;
    }
    off_t end = len + offset;
    HANDLE mmap_fd, h;
    if (fd == -1)
        mmap_fd = INVALID_HANDLE_VALUE;
    else
        mmap_fd = (HANDLE)_get_osfhandle(fd);
    h = CreateFileMapping(mmap_fd, NULL, flProtect, 0, end, NULL);
    int k = GetLastError();
    if (NULL == h)
        return -1;
    DWORD dwDesiredAccess;
    if (prot & PROT_WRITE)
        dwDesiredAccess = FILE_MAP_WRITE;
    else
        dwDesiredAccess = FILE_MAP_READ;
    void* ret = MapViewOfFile(h, dwDesiredAccess, 0, offset, len);
    if (ret == NULL) {
        CloseHandle(h);
        ret = nullptr;
    }
    *res = ret;
    return 0;
#else
    void* result = mmap(addr, len, prot, flags, fd, offset);
    if (result != MAP_FAILED) {
        *res = result;
        return 0;
    }
    std::abort();
#endif
}

PS4_SYSV_ABI void* posix_mmap(void* addr, u64 len, int prot, int flags, int fd, u64 offset) {
    void* ptr;
    LOG_INFO_IF(log_libkernel_file, "posix mmap redirect to sceKernelMmap\n");
    // posix call the difference is that there is a different behaviour when it doesn't return 0 or
    // SCE_OK
    int result = sceKernelMmap(addr, len, prot, flags, fd, offset, &ptr);
    if (result != 0) {
        BREAKPOINT();
    }
    return ptr;
}

void LibKernel_Register(Loader::SymbolsResolver* sym) {
    // obj
    LIB_OBJ("f7uOxY9mM1U", "libkernel", 1, "libkernel", 1, 1, &g_stack_chk_guard);
    // memory
    LIB_FUNCTION("rTXw65xmLIA", "libkernel", 1, "libkernel", 1, 1,
                 Kernel::sceKernelAllocateDirectMemory);
    LIB_FUNCTION("pO96TwzOm5E", "libkernel", 1, "libkernel", 1, 1,
                 Kernel::sceKernelGetDirectMemorySize);
    LIB_FUNCTION("L-Q3LEjIbgA", "libkernel", 1, "libkernel", 1, 1,
                 Kernel::sceKernelMapDirectMemory);
    LIB_FUNCTION("MBuItvba6z8", "libkernel", 1, "libkernel", 1, 1, sceKernelReleaseDirectMemory);
    LIB_FUNCTION("cQke9UuBQOk", "libkernel", 1, "libkernel", 1, 1, sceKernelMunmap);
    // equeue
    LIB_FUNCTION("D0OdFMjp46I", "libkernel", 1, "libkernel", 1, 1, Kernel::sceKernelCreateEqueue);
    LIB_FUNCTION("fzyMKs9kim0", "libkernel", 1, "libkernel", 1, 1, Kernel::sceKernelWaitEqueue);
    // misc
    LIB_FUNCTION("WslcK1FQcGI", "libkernel", 1, "libkernel", 1, 1, Kernel::sceKernelIsNeoMode);
    LIB_FUNCTION("Ou3iL1abvng", "libkernel", 1, "libkernel", 1, 1, stack_chk_fail);
    LIB_FUNCTION("9BcDykPmo1I", "libkernel", 1, "libkernel", 1, 1, __Error);
    LIB_FUNCTION("BPE9s9vQQXo", "libkernel", 1, "libkernel", 1, 1, posix_mmap);
    LIB_FUNCTION("1jfXLRVzisc", "libkernel", 1, "libkernel", 1, 1, sceKernelUsleep);
    LIB_FUNCTION("YSHRBRLn2pI", "libkernel", 1, "libkernel", 1, 1, _writev);

    Core::Libraries::LibKernel::fileSystemSymbolsRegister(sym);
    Core::Libraries::LibKernel::timeSymbolsRegister(sym);
    Core::Libraries::LibKernel::pthreadSymbolsRegister(sym);
}

} // namespace Core::Libraries::LibKernel
