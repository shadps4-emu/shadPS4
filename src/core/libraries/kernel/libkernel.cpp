// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <thread>
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/cpu_management.h"
#include "core/libraries/kernel/event_flag/event_flag.h"
#include "core/libraries/kernel/event_queues.h"
#include "core/libraries/kernel/file_system.h"
#include "core/libraries/kernel/libkernel.h"
#include "core/libraries/kernel/memory_management.h"
#include "core/libraries/kernel/thread_management.h"
#include "core/libraries/kernel/time_management.h"
#include "core/libraries/libs.h"
#include "core/linker.h"
#include "core/memory.h"
#ifdef _WIN64
#include <io.h>
#include <windows.h>
#else
#include <sys/mman.h>
#endif
#include <core/file_format/psf.h>

namespace Libraries::Kernel {

static u64 g_stack_chk_guard = 0xDEADBEEF54321ABC; // dummy return

static void* PS4_SYSV_ABI sceKernelGetProcParam() {
    auto* linker = Common::Singleton<Core::Linker>::Instance();
    return reinterpret_cast<void*>(linker->GetProcParam());
}

int32_t PS4_SYSV_ABI sceKernelReleaseDirectMemory(off_t start, size_t len) {
    auto* memory = Core::Memory::Instance();
    memory->Free(start, len);
    return 0;
}

static PS4_SYSV_ABI void stack_chk_fail() {
    UNREACHABLE();
}

int PS4_SYSV_ABI sceKernelMunmap(void* addr, size_t len) {
    LOG_INFO(Kernel_Vmm, "addr = {}, len = {:#x}", fmt::ptr(addr), len);
    auto* memory = Core::Memory::Instance();
    memory->UnmapMemory(std::bit_cast<VAddr>(addr), len);
    return SCE_OK;
}

void PS4_SYSV_ABI sceKernelUsleep(u32 microseconds) {
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
    LOG_INFO(Kernel_Vmm, "called");
    if (prot > 3) {
        LOG_ERROR(Kernel_Vmm, "prot = {} not supported", prot);
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
    LOG_INFO(Kernel_Vmm, "posix mmap redirect to sceKernelMmap\n");
    // posix call the difference is that there is a different behaviour when it doesn't return 0 or
    // SCE_OK
    int result = sceKernelMmap(addr, len, prot, flags, fd, offset, &ptr);
    ASSERT(result == 0);
    return ptr;
}

static uint64_t g_mspace_atomic_id_mask = 0;
static uint64_t g_mstate_table[64] = {0};

struct HeapInfoInfo {
    uint64_t size = sizeof(HeapInfoInfo);
    uint32_t flag;
    uint32_t getSegmentInfo;
    uint64_t* mspace_atomic_id_mask;
    uint64_t* mstate_table;
};

void PS4_SYSV_ABI sceLibcHeapGetTraceInfo(HeapInfoInfo* info) {
    info->mspace_atomic_id_mask = &g_mspace_atomic_id_mask;
    info->mstate_table = g_mstate_table;
    info->getSegmentInfo = 0;
}

s64 PS4_SYSV_ABI ps4__write(int d, const void* buf, std::size_t nbytes) {
    if (d <= 2) { // stdin,stdout,stderr
        char* str = strdup((const char*)buf);
        if (str[nbytes - 1] == '\n')
            str[nbytes - 1] = 0;
        LOG_INFO(Tty, "{}", str);
        free(str);
        return nbytes;
    }
    LOG_ERROR(Kernel, "(STUBBED) called d = {} nbytes = {} ", d, nbytes);
    UNREACHABLE(); // normal write , is it a posix call??
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceKernelConvertUtcToLocaltime(time_t time, time_t* local_time,
                                                struct OrbisTimesec* st, unsigned long* dst_sec) {
    LOG_TRACE(Kernel, "Called");
    const auto* time_zone = std::chrono::current_zone();
    auto info = time_zone->get_info(std::chrono::system_clock::now());

    *local_time = info.offset.count() + info.save.count() * 60 + time;

    if (st != nullptr) {
        st->t = time;
        st->west_sec = info.offset.count() * 60;
        st->dst_sec = info.save.count() * 60;
    }

    if (dst_sec != nullptr) {
        *dst_sec = info.save.count() * 60;
    }

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceKernelGetCompiledSdkVersion(int* ver) {
    auto* param_sfo = Common::Singleton<PSF>::Instance();
    int version = param_sfo->GetInteger("SYSTEM_VER");
    LOG_INFO(Kernel, "returned system version = {:#x}", version);
    *ver = version;
    return ORBIS_OK;
}

s64 PS4_SYSV_ABI ps4__read(int d, void* buf, u64 nbytes) {
    ASSERT_MSG(d == 0, "d is not 0!");

    return static_cast<s64>(
        strlen(std::fgets(static_cast<char*>(buf), static_cast<int>(nbytes), stdin)));
}

void LibKernel_Register(Core::Loader::SymbolsResolver* sym) {
    // obj
    LIB_OBJ("f7uOxY9mM1U", "libkernel", 1, "libkernel", 1, 1, &g_stack_chk_guard);
    // memory
    LIB_FUNCTION("rTXw65xmLIA", "libkernel", 1, "libkernel", 1, 1, sceKernelAllocateDirectMemory);
    LIB_FUNCTION("pO96TwzOm5E", "libkernel", 1, "libkernel", 1, 1, sceKernelGetDirectMemorySize);
    LIB_FUNCTION("L-Q3LEjIbgA", "libkernel", 1, "libkernel", 1, 1, sceKernelMapDirectMemory);
    LIB_FUNCTION("WFcfL2lzido", "libkernel", 1, "libkernel", 1, 1, sceKernelQueryMemoryProtection);
    LIB_FUNCTION("BHouLQzh0X0", "libkernel", 1, "libkernel", 1, 1, sceKernelDirectMemoryQuery);
    LIB_FUNCTION("MBuItvba6z8", "libkernel", 1, "libkernel", 1, 1, sceKernelReleaseDirectMemory);
    LIB_FUNCTION("cQke9UuBQOk", "libkernel", 1, "libkernel", 1, 1, sceKernelMunmap);
    LIB_FUNCTION("mL8NDH86iQI", "libkernel", 1, "libkernel", 1, 1, sceKernelMapNamedFlexibleMemory);
    LIB_FUNCTION("IWIBBdTHit4", "libkernel", 1, "libkernel", 1, 1, sceKernelMapFlexibleMemory);
    // equeue
    LIB_FUNCTION("D0OdFMjp46I", "libkernel", 1, "libkernel", 1, 1, sceKernelCreateEqueue);
    LIB_FUNCTION("jpFjmgAC5AE", "libkernel", 1, "libkernel", 1, 1, sceKernelDeleteEqueue);
    LIB_FUNCTION("fzyMKs9kim0", "libkernel", 1, "libkernel", 1, 1, sceKernelWaitEqueue);
    LIB_FUNCTION("vz+pg2zdopI", "libkernel", 1, "libkernel", 1, 1, sceKernelGetEventUserData);
    LIB_FUNCTION("4R6-OvI2cEA", "libkernel", 1, "libkernel", 1, 1, sceKernelAddUserEvent);
    // misc
    LIB_FUNCTION("WslcK1FQcGI", "libkernel", 1, "libkernel", 1, 1, sceKernelIsNeoMode);
    LIB_FUNCTION("Ou3iL1abvng", "libkernel", 1, "libkernel", 1, 1, stack_chk_fail);
    LIB_FUNCTION("9BcDykPmo1I", "libkernel", 1, "libkernel", 1, 1, __Error);
    LIB_FUNCTION("BPE9s9vQQXo", "libkernel", 1, "libkernel", 1, 1, posix_mmap);
    LIB_FUNCTION("1jfXLRVzisc", "libkernel", 1, "libkernel", 1, 1, sceKernelUsleep);
    LIB_FUNCTION("YSHRBRLn2pI", "libkernel", 1, "libkernel", 1, 1, _writev);
    LIB_FUNCTION("959qrazPIrg", "libkernel", 1, "libkernel", 1, 1, sceKernelGetProcParam);
    LIB_FUNCTION("-o5uEDpN+oY", "libkernel", 1, "libkernel", 1, 1, sceKernelConvertUtcToLocaltime);
    LIB_FUNCTION("WB66evu8bsU", "libkernel", 1, "libkernel", 1, 1, sceKernelGetCompiledSdkVersion);
    LIB_FUNCTION("DRuBt2pvICk", "libkernel", 1, "libkernel", 1, 1, ps4__read);

    Libraries::Kernel::fileSystemSymbolsRegister(sym);
    Libraries::Kernel::timeSymbolsRegister(sym);
    Libraries::Kernel::pthreadSymbolsRegister(sym);
    Libraries::Kernel::RegisterKernelEventFlag(sym);

    // temp
    LIB_FUNCTION("NWtTN10cJzE", "libSceLibcInternalExt", 1, "libSceLibcInternal", 1, 1,
                 sceLibcHeapGetTraceInfo);
    LIB_FUNCTION("FxVZqBAA7ks", "libkernel", 1, "libkernel", 1, 1, ps4__write);
}

} // namespace Libraries::Kernel
