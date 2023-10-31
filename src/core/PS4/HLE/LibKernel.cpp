#include "LibKernel.h"

#include <Util/log.h>
#include <debug.h>
#include <windows.h>
#include <io.h>

#include "Emulator/Util/singleton.h"
#include "../Loader/Elf.h"
#include "Kernel/Objects/physical_memory.h"
#include "Kernel/cpu_management.h"
#include "Kernel/event_queues.h"
#include "Kernel/memory_management.h"
#include "Libs.h"
#include "core/hle/libraries/libkernel/file_system.h"
#include "core/hle/libraries/libkernel/time_management.h"

namespace HLE::Libs::LibKernel {

static u64 g_stack_chk_guard = 0xDEADBEEF54321ABC;  // dummy return

int32_t PS4_SYSV_ABI sceKernelReleaseDirectMemory(off_t start, size_t len) {
    BREAKPOINT();
    return 0;
}

static PS4_SYSV_ABI void stack_chk_fail() { BREAKPOINT(); }

int PS4_SYSV_ABI sceKernelMunmap(void* addr, size_t len) { BREAKPOINT(); }

static thread_local int libc_error;
PS4_SYSV_ABI int* __Error() { return &libc_error; }

#define PROT_READ 0x1
#define PROT_WRITE 0x2

int PS4_SYSV_ABI sceKernelMmap(void* addr, u64 len, int prot, int flags, int fd, off_t offset, void** res) {
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
    if (NULL == h) return -1;
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
}

PS4_SYSV_ABI void* mmap(void* addr, u64 len, int prot, int flags, int fd, u64 offset) {
    void* ptr;
    // posix call the difference is that there is a different behaviour when it doesn't return 0 or SCE_OK
    int result = sceKernelMmap(addr, len, prot, flags, fd, offset, &ptr);
    if (result != 0) {
        BREAKPOINT();
    }
    return ptr;
}

void LibKernel_Register(SymbolsResolver* sym) {
    // obj
    LIB_OBJ("f7uOxY9mM1U", "libkernel", 1, "libkernel", 1, 1, &HLE::Libs::LibKernel::g_stack_chk_guard);
    // memory
    LIB_FUNCTION("rTXw65xmLIA", "libkernel", 1, "libkernel", 1, 1, MemoryManagement::sceKernelAllocateDirectMemory);
    LIB_FUNCTION("pO96TwzOm5E", "libkernel", 1, "libkernel", 1, 1, MemoryManagement::sceKernelGetDirectMemorySize);
    LIB_FUNCTION("L-Q3LEjIbgA", "libkernel", 1, "libkernel", 1, 1, MemoryManagement::sceKernelMapDirectMemory);
    LIB_FUNCTION("MBuItvba6z8", "libkernel", 1, "libkernel", 1, 1, sceKernelReleaseDirectMemory);
    LIB_FUNCTION("cQke9UuBQOk", "libkernel", 1, "libkernel", 1, 1, sceKernelMunmap);
    // equeue
    LIB_FUNCTION("D0OdFMjp46I", "libkernel", 1, "libkernel", 1, 1, EventQueues::sceKernelCreateEqueue);
    LIB_FUNCTION("fzyMKs9kim0", "libkernel", 1, "libkernel", 1, 1, EventQueues::sceKernelWaitEqueue);
    // misc
    LIB_FUNCTION("WslcK1FQcGI", "libkernel", 1, "libkernel", 1, 1, CPUManagement::sceKernelIsNeoMode);
    LIB_FUNCTION("Ou3iL1abvng", "libkernel", 1, "libkernel", 1, 1, stack_chk_fail);

    LIB_FUNCTION("9BcDykPmo1I", "libkernel", 1, "libkernel", 1, 1, __Error);

    LIB_FUNCTION("BPE9s9vQQXo", "libkernel", 1, "libkernel", 1, 1, mmap);
    
    Core::Libraries::LibKernel::fileSystemSymbolsRegister(sym);
    Core::Libraries::LibKernel::timeSymbolsRegister(sym);
}

};  // namespace HLE::Libs::LibKernel