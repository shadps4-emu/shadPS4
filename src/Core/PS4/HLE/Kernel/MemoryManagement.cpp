#include "MemoryManagement.h"
#include "../../../../Debug.h"
#include "../../../../Util/Log.h"
#include "../Libs.h"
#include "../ErrorCodes.h"
#include "MemMngCodes.h"
#include <bit>

namespace HLE::Libs::LibKernel::MemoryManagement {

bool isPowerOfTwo(u64 n) { return std::popcount(n) == 1; }

bool is16KBAligned(u64 n) { return ((n % (16ull * 1024) == 0)); }

u64 PS4_SYSV_ABI sceKernelGetDirectMemorySize() {
    PRINT_FUNCTION_NAME();
    return SCE_KERNEL_MAIN_DMEM_SIZE;
}

int PS4_SYSV_ABI sceKernelAllocateDirectMemory(s64 searchStart, s64 searchEnd, u64 len, u64 alignment, int memoryType,s64* physAddrOut){
   
    PRINT_FUNCTION_NAME();
    
    if (searchStart < 0 || searchEnd <= searchStart) { 
        //TODO debug logging
        return SCE_KERNEL_ERROR_EINVAL;
    }
    bool isInRange = (searchStart < len && searchEnd > len);
    if (len <= 0 || !is16KBAligned(len) || !isInRange){
        // TODO debug logging
        return SCE_KERNEL_ERROR_EINVAL;
    }
    if ((alignment != 0 || is16KBAligned(alignment)) && !isPowerOfTwo(alignment)){
        // TODO debug logging
        return SCE_KERNEL_ERROR_EINVAL;
    }
    if (physAddrOut == nullptr) {
        // TODO debug logging
        return SCE_KERNEL_ERROR_EINVAL;
    }
    LOG_INFO_IF(true, "search_start = {:#018x}\n", searchStart);
    LOG_INFO_IF(true, "search_end   = {:#018x}\n", searchEnd);
    LOG_INFO_IF(true, "len          = {:#018x}\n", len);
    LOG_INFO_IF(true, "alignment    = {:#018x}\n", alignment);
    LOG_INFO_IF(true, "memory_type  = {}\n", memoryType);

    BREAKPOINT();
    return 0;
}

}  // namespace HLE::Libs::LibKernel::MemoryManagement