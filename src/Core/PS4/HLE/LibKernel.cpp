#include "../Loader/Elf.h"
#include "LibKernel.h"
#include "Libs.h"
#include <debug.h>
#include <Util/log.h>
#include "Kernel/memory_management.h"
#include "../../../Util/Singleton.h"
#include "Kernel/Objects/physical_memory.h"
#include "Util/config.h"

namespace HLE::Libs::LibKernel {

    static u64 g_stack_chk_guard = 0xDEADBEEF54321ABC; //dummy return

    int32_t PS4_SYSV_ABI sceKernelReleaseDirectMemory(off_t start, size_t len) { 
        BREAKPOINT();
        return 0;
    }

    int PS4_SYSV_ABI sceKernelCreateEqueue(/* SceKernelEqueue* eq*/ int eq, const char* name) 
    { 
        //BREAKPOINT();
        PRINT_DUMMY_FUNCTION_NAME();
        return 0;
    }
    int PS4_SYSV_ABI sceKernelWaitEqueue(/*SceKernelEqueue eq, SceKernelEvent* ev,*/ int num, int* out /*, SceKernelUseconds* timo*/) 
    { 
        BREAKPOINT();
        return 0;
    }
    int PS4_SYSV_ABI sceKernelIsNeoMode()
    { 
        PRINT_FUNCTION_NAME();
        bool isNeo = Config::isNeoMode();
        return isNeo ? 1 : 0;
    }

    static PS4_SYSV_ABI void stack_chk_fail() { BREAKPOINT();
    }
    void LibKernel_Register(SymbolsResolver* sym) { 
        //obj
        LIB_OBJ("f7uOxY9mM1U", "libkernel", 1, "libkernel", 1, 1, &HLE::Libs::LibKernel::g_stack_chk_guard);
        //memory
        LIB_FUNCTION("rTXw65xmLIA", "libkernel", 1, "libkernel", 1, 1, MemoryManagement::sceKernelAllocateDirectMemory); 
        LIB_FUNCTION("pO96TwzOm5E", "libkernel", 1, "libkernel", 1, 1, MemoryManagement::sceKernelGetDirectMemorySize);
        LIB_FUNCTION("L-Q3LEjIbgA", "libkernel", 1, "libkernel", 1, 1, MemoryManagement::sceKernelMapDirectMemory);
        LIB_FUNCTION("MBuItvba6z8", "libkernel", 1, "libkernel", 1, 1, sceKernelReleaseDirectMemory);
        //equeue
        LIB_FUNCTION("D0OdFMjp46I", "libkernel", 1, "libkernel", 1, 1, sceKernelCreateEqueue);
        LIB_FUNCTION("fzyMKs9kim0", "libkernel", 1, "libkernel", 1, 1, sceKernelWaitEqueue);
        //misc
        LIB_FUNCTION("WslcK1FQcGI", "libkernel", 1, "libkernel", 1, 1, sceKernelIsNeoMode);
        LIB_FUNCTION("Ou3iL1abvng", "libkernel", 1, "libkernel", 1, 1, stack_chk_fail);
    }

}; 