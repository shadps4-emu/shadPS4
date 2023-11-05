#include "common/log.h"
#include "common/debug.h"
#include "common/singleton.h"
#include "core/loader/elf.h"
#include "core/hle/kernel/Objects/physical_memory.h"
#include "core/hle/kernel/cpu_management.h"
#include "core/hle/kernel/event_queues.h"
#include "core/hle/kernel/memory_management.h"
#include "core/hle/libraries/libkernel/libkernel.h"
#include "core/hle/libraries/libkernel/file_system.h"
#include "core/hle/libraries/libkernel/time_management.h"
#include "core/hle/libraries/libs.h"

#ifdef _WIN64
#include <windows.h>
#endif

namespace Core::Libraries::LibKernel {

static u64 g_stack_chk_guard = 0xDEADBEEF54321ABC;  // dummy return

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

void LibKernel_Register(Loader::SymbolsResolver* sym) {
    // obj
    LIB_OBJ("f7uOxY9mM1U", "libkernel", 1, "libkernel", 1, 1, &g_stack_chk_guard);
    // memory
    LIB_FUNCTION("rTXw65xmLIA", "libkernel", 1, "libkernel", 1, 1, Kernel::sceKernelAllocateDirectMemory);
    LIB_FUNCTION("pO96TwzOm5E", "libkernel", 1, "libkernel", 1, 1, Kernel::sceKernelGetDirectMemorySize);
    LIB_FUNCTION("L-Q3LEjIbgA", "libkernel", 1, "libkernel", 1, 1, Kernel::sceKernelMapDirectMemory);
    LIB_FUNCTION("MBuItvba6z8", "libkernel", 1, "libkernel", 1, 1, sceKernelReleaseDirectMemory);
    LIB_FUNCTION("cQke9UuBQOk", "libkernel", 1, "libkernel", 1, 1, sceKernelMunmap);
    // equeue
    LIB_FUNCTION("D0OdFMjp46I", "libkernel", 1, "libkernel", 1, 1, Kernel::sceKernelCreateEqueue);
    LIB_FUNCTION("fzyMKs9kim0", "libkernel", 1, "libkernel", 1, 1, Kernel::sceKernelWaitEqueue);
    // misc
    LIB_FUNCTION("WslcK1FQcGI", "libkernel", 1, "libkernel", 1, 1, Kernel::sceKernelIsNeoMode);
    LIB_FUNCTION("Ou3iL1abvng", "libkernel", 1, "libkernel", 1, 1, stack_chk_fail);
    
    Core::Libraries::LibKernel::fileSystemSymbolsRegister(sym);
    Core::Libraries::LibKernel::timeSymbolsRegister(sym);
}

} // namespace Core::Libraries::LibKernel
