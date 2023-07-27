#include "../Loader/Elf.h"
#include "LibKernel.h"
#include "Libs.h"
#include "../../../Debug.h"
#include "../../../Util/Log.h"
#include "Kernel/MemoryManagement.h"

namespace HLE::Libs::LibKernel {

    static u64 g_stack_chk_guard = 0xDEADBEEF54321ABC; //dummy return

    int32_t PS4_SYSV_ABI sceKernelMapDirectMemory(void** addr, size_t len, int prot, int flags, off_t directMemoryStart, size_t alignment) {
        BREAKPOINT();
        return 0;
    }
    int32_t PS4_SYSV_ABI sceKernelReleaseDirectMemory(off_t start, size_t len) { 
        BREAKPOINT();
        return 0;
    }

    int PS4_SYSV_ABI sceKernelCreateEqueue(/* SceKernelEqueue* eq*/ int eq, const char* name) 
    { 
        //BREAKPOINT();
        LOG_INFO_IF(true, "dummy sceKernelCreateEqueue\n");
        return 0;
    }
    int PS4_SYSV_ABI sceKernelWaitEqueue(/*SceKernelEqueue eq, SceKernelEvent* ev,*/ int num, int* out /*, SceKernelUseconds* timo*/) 
    { 
        BREAKPOINT();
        return 0;
    }
    int PS4_SYSV_ABI sceKernelIsNeoMode()
    { 
        //BREAKPOINT();
        return 0; //it isn't PS4VR TODO
    }

    static PS4_SYSV_ABI void stack_chk_fail() { BREAKPOINT();
    }
    void LibKernel_Register(SymbolsResolver* sym) { 
        //obj
        LIB_OBJ("f7uOxY9mM1U", "libkernel", 1, "libkernel", 1, 1, &HLE::Libs::LibKernel::g_stack_chk_guard);
        //memory
        LIB_FUNCTION("rTXw65xmLIA", "libkernel", 1, "libkernel", 1, 1, HLE::Libs::LibKernel::MemoryManagement::sceKernelAllocateDirectMemory); 
        LIB_FUNCTION("pO96TwzOm5E", "libkernel", 1, "libkernel", 1, 1, HLE::Libs::LibKernel::MemoryManagement::sceKernelGetDirectMemorySize);
        LIB_FUNCTION("L-Q3LEjIbgA", "libkernel", 1, "libkernel", 1, 1, sceKernelMapDirectMemory);
        LIB_FUNCTION("MBuItvba6z8", "libkernel", 1, "libkernel", 1, 1, sceKernelReleaseDirectMemory);
        //equeue
        LIB_FUNCTION("D0OdFMjp46I", "libkernel", 1, "libkernel", 1, 1, sceKernelCreateEqueue);
        LIB_FUNCTION("fzyMKs9kim0", "libkernel", 1, "libkernel", 1, 1, sceKernelWaitEqueue);
        //misc
        LIB_FUNCTION("WslcK1FQcGI", "libkernel", 1, "libkernel", 1, 1, sceKernelIsNeoMode);
        LIB_FUNCTION("Ou3iL1abvng", "libkernel", 1, "libkernel", 1, 1, stack_chk_fail);
    }

}; 