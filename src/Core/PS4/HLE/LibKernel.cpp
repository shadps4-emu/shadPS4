#include "../Loader/Elf.h"
#include "LibKernel.h"
#include "Libs.h"

namespace HLE::Libs::LibKernel {

    int sceKernelAllocateDirectMemory(off_t searchStart, off_t searchEnd, size_t len, size_t alignment, int memoryType, off_t* physAddrOut) { return 0;//OK
    }
    size_t sceKernelGetDirectMemorySize() { return 0;
    }
    int32_t sceKernelMapDirectMemory(void** addr, size_t len, int prot, int flags, off_t directMemoryStart, size_t alignment) { return 0;
    }
    int32_t sceKernelReleaseDirectMemory(off_t start, size_t len) { return 0;
    }

    int sceKernelCreateEqueue(/* SceKernelEqueue* eq*/int eq,const char* name) 
    { return 0;
    }
    int sceKernelWaitEqueue(/*SceKernelEqueue eq, SceKernelEvent* ev,*/ int num, int* out /*, SceKernelUseconds* timo*/) { return 0;
    }
    int sceKernelIsNeoMode()
    { return 0;
    }

    static void stack_chk_fail() {

    }
    void LibKernel_Register(SymbolsResolver* sym) { 
        //memory
        LIB_FUNCTION("rTXw65xmLIA", "libkernel", 1, "libkernel", 1, 1, sceKernelAllocateDirectMemory); 
        LIB_FUNCTION("pO96TwzOm5E", "libkernel", 1, "libkernel", 1, 1, sceKernelGetDirectMemorySize);
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