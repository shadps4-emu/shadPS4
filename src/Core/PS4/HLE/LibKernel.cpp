#include "../Loader/Elf.h"
#include "LibKernel.h"
#include "Libs.h"
#include <debug.h>
#include <Util/log.h>
#include "Kernel/memory_management.h"
#include "../../../Util/Singleton.h"
#include "Kernel/Objects/physical_memory.h"
#include "Kernel/cpu_management.h"
#include "Kernel/event_queues.h"

namespace HLE::Libs::LibKernel {

    static u64 g_stack_chk_guard = 0xDEADBEEF54321ABC; //dummy return

    int32_t PS4_SYSV_ABI sceKernelReleaseDirectMemory(off_t start, size_t len) { 
        BREAKPOINT();
        return 0;
    }

    int PS4_SYSV_ABI sceKernelWaitEqueue(/*SceKernelEqueue eq, SceKernelEvent* ev,*/ int num, int* out /*, SceKernelUseconds* timo*/) 
    { 
        BREAKPOINT();
        return 0;
    }

    static PS4_SYSV_ABI void stack_chk_fail() { BREAKPOINT();
    }

    void sched_yield() { BREAKPOINT();
    }
    void sysconf() { BREAKPOINT();
    }
    void dummy() { BREAKPOINT(); }
    void LibKernel_Register(SymbolsResolver* sym) { 
        //obj
        LIB_OBJ("f7uOxY9mM1U", "libkernel", 1, "libkernel", 1, 1, &HLE::Libs::LibKernel::g_stack_chk_guard);
        //memory
        LIB_FUNCTION("rTXw65xmLIA", "libkernel", 1, "libkernel", 1, 1, MemoryManagement::sceKernelAllocateDirectMemory); 
        LIB_FUNCTION("pO96TwzOm5E", "libkernel", 1, "libkernel", 1, 1, MemoryManagement::sceKernelGetDirectMemorySize);
        LIB_FUNCTION("L-Q3LEjIbgA", "libkernel", 1, "libkernel", 1, 1, MemoryManagement::sceKernelMapDirectMemory);
        LIB_FUNCTION("MBuItvba6z8", "libkernel", 1, "libkernel", 1, 1, sceKernelReleaseDirectMemory);
        //equeue
        LIB_FUNCTION("D0OdFMjp46I", "libkernel", 1, "libkernel", 1, 1, EventQueues::sceKernelCreateEqueue);
        LIB_FUNCTION("fzyMKs9kim0", "libkernel", 1, "libkernel", 1, 1, sceKernelWaitEqueue);
        //misc
        LIB_FUNCTION("WslcK1FQcGI", "libkernel", 1, "libkernel", 1, 1, CPUManagement::sceKernelIsNeoMode);
        LIB_FUNCTION("Ou3iL1abvng", "libkernel", 1, "libkernel", 1, 1, stack_chk_fail);
        LIB_FUNCTION("6XG4B33N09g", "libkernel", 1, "libkernel", 1, 1, sched_yield);
        LIB_FUNCTION("mkawd0NA9ts", "libkernel", 1, "libkernel", 1, 1, sysconf);

        LIB_FUNCTION("wuCroIGjt2g", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("PfccT7qURYE", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("aPcyptbOiZs", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("mqQMh1zPPT8", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("yS8U2TGCe1A", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("FxVZqBAA7ks", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("KiJEPEWRyUY", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("6Z83sYWFlA8", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("mkx2fVhNMsg", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("sIlRvQqsN2Y", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("RXXqi4CtF8w", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("iGjsr1WAtI0", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("WrOLvHU0yQM", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("7Xl257M4VNI", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("2Z+PpY6CaJg", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("27bAgiJmOh0", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("ku7D4q1Y9PI", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("EgmLo6EWgso", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("+U1R4WtXvoc", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("dQHWEsJtoE4", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("7H0iTOciTLo", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("UqDGjXA5yUM", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("ltCfaGr2JGE", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("1jfXLRVzisc", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("K-jXhbt2gn4", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("BPE9s9vQQXo", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("h9CcP3J0oVM", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("0-KXaS70xy4", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("HF7lK46xzjY", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("EotR8a3ASf4", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("bY-PO6JhzhQ", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("mDmgMOGVUqg", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("mqULNdimTn0", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("ttHNfU+qDBU", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("Jahsnh4KKkg", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("YSHRBRLn2pI", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("Oy6IpwgtYOk", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("9BcDykPmo1I", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("Z4QosVuAsA0", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("Op8TBGY5KHg", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("0t0-MxQNwK4", "libkernel", 1, "libkernel", 1, 1, dummy);
        LIB_FUNCTION("2MOy+rUfuhQ", "libkernel", 1, "libkernel", 1, 1, dummy);
    }

}; 