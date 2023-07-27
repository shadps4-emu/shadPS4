#include "MemoryManagement.h"
#include "../../../../Debug.h"
#include "MemMngCodes.h"
#include <bit>

namespace HLE::Libs::LibKernel::MemoryManagement {

bool isPowerOfTwo(u64 n) { return std::popcount(n) == 1; }

bool is16KBAligned(u64 n) { return ((n % (static_cast<u64>(16) * 1024) == 0)); }

u64 PS4_SYSV_ABI sceKernelGetDirectMemorySize() { return SCE_KERNEL_MAIN_DMEM_SIZE; }

int PS4_SYSV_ABI sceKernelAllocateDirectMemory(s64 searchStart, s64 searchEnd, u64 len, u64 alignment, int memoryType,s64* physAddrOut){
    BREAKPOINT();
    return 0;
}

}  // namespace HLE::Libs::LibKernel::MemoryManagement