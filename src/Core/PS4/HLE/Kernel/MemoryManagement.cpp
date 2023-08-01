#include "MemoryManagement.h"
#include "../../../../Debug.h"
#include "../../../../Util/Log.h"
#include "../Libs.h"
#include "../ErrorCodes.h"
#include "MemMngCodes.h"
#include <bit>
#include "PhysicalMemory.h"

namespace HLE::Libs::LibKernel::MemoryManagement {
  
static PhysicalMemory* g_physical_memory = nullptr;


void PhysicalMemoryInit() { g_physical_memory = new PhysicalMemory; }

static  u64 align_pos(u64 pos, u64 align) { return (align != 0 ? (pos + (align - 1)) & ~(align - 1) : pos); }

bool PhysicalMemory::Alloc(u64 searchStart, u64 searchEnd, u64 len, u64 alignment, u64* physAddrOut, int memoryType) {
    
    u64 find_free_pos = 0;

    //iterate through allocated blocked and find the next free position
    if (!m_allocatedBlocks.empty()) {
        for (const auto& block : m_allocatedBlocks) {
            u64 n = block.start_addr + block.size;
            if (n > find_free_pos) {
                find_free_pos = n;
            }
        }
    }
    
    //align free position
    find_free_pos = align_pos(find_free_pos, alignment);

    //if the new position is between searchStart - searchEnd , allocate a new block
    if (find_free_pos >= searchStart && find_free_pos + len <= searchEnd) {
        AllocatedBlock block{};
        block.size = len;
        block.start_addr = find_free_pos;
        block.memoryType = memoryType;

        m_allocatedBlocks.push_back(block);

        *physAddrOut = find_free_pos;
        return true;
    }

    return false;
}
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

    u64 physical_addr = 0;
    if (!g_physical_memory->Alloc(searchStart, searchEnd, len, alignment, &physical_addr, memoryType)) {
        //TODO debug logging
        return SCE_KERNEL_ERROR_EAGAIN;
    }
    *physAddrOut = static_cast<s64>(physical_addr);
    LOG_INFO_IF(true, "physAddrOut    = {:#018x}\n", physical_addr);
    return SCE_OK;
}

}  // namespace HLE::Libs::LibKernel::MemoryManagement