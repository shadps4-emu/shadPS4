#include "memory_management.h"

#include <debug.h>

#include <bit>
#include <magic_enum.hpp>

#include "../../../../Util/Log.h"
#include "../../../../Util/Singleton.h"
#include "../ErrorCodes.h"
#include "../Libs.h"
#include "Objects/physical_memory.h"

namespace HLE::Libs::LibKernel::MemoryManagement {

constexpr bool log_file_memory = true;  // disable it to disable logging

bool isPowerOfTwo(u64 n) { return std::popcount(n) == 1; }

bool is16KBAligned(u64 n) { return ((n % (16ull * 1024) == 0)); }

u64 PS4_SYSV_ABI sceKernelGetDirectMemorySize() {
    PRINT_FUNCTION_NAME();
    return SCE_KERNEL_MAIN_DMEM_SIZE;
}

int PS4_SYSV_ABI sceKernelAllocateDirectMemory(s64 searchStart, s64 searchEnd, u64 len, u64 alignment, int memoryType, s64* physAddrOut) {
    PRINT_FUNCTION_NAME();

    if (searchStart < 0 || searchEnd <= searchStart) {
        LOG_TRACE_IF(log_file_memory, "sceKernelAllocateDirectMemory returned SCE_KERNEL_ERROR_EINVAL searchStart,searchEnd invalid\n");
        return SCE_KERNEL_ERROR_EINVAL;
    }
    bool isInRange = (searchStart < len && searchEnd > len);
    if (len <= 0 || !is16KBAligned(len) || !isInRange) {
        LOG_TRACE_IF(log_file_memory, "sceKernelAllocateDirectMemory returned SCE_KERNEL_ERROR_EINVAL memory range invalid\n");
        return SCE_KERNEL_ERROR_EINVAL;
    }
    if ((alignment != 0 || is16KBAligned(alignment)) && !isPowerOfTwo(alignment)) {
        LOG_TRACE_IF(log_file_memory, "sceKernelAllocateDirectMemory returned SCE_KERNEL_ERROR_EINVAL alignment invalid\n");
        return SCE_KERNEL_ERROR_EINVAL;
    }
    if (physAddrOut == nullptr) {
        LOG_TRACE_IF(log_file_memory, "sceKernelAllocateDirectMemory returned SCE_KERNEL_ERROR_EINVAL physAddrOut is null\n");
        return SCE_KERNEL_ERROR_EINVAL;
    }
    auto memtype = magic_enum::enum_cast<MemoryTypes>(memoryType);

    LOG_INFO_IF(log_file_memory, "search_start = {}\n", log_hex_full(searchStart));
    LOG_INFO_IF(log_file_memory, "search_end   = {}\n", log_hex_full(searchEnd));
    LOG_INFO_IF(log_file_memory, "len          = {}\n", log_hex_full(len));
    LOG_INFO_IF(log_file_memory, "alignment    = {}\n", log_hex_full(alignment));
    LOG_INFO_IF(log_file_memory, "memory_type  = {}\n", magic_enum::enum_name(memtype.value()));

    u64 physical_addr = 0;
    auto* physical_memory = Singleton<HLE::Kernel::Objects::PhysicalMemory>::Instance();
    if (!physical_memory->Alloc(searchStart, searchEnd, len, alignment, &physical_addr, memoryType)) {
        LOG_TRACE_IF(log_file_memory, "sceKernelAllocateDirectMemory returned SCE_KERNEL_ERROR_EAGAIN can't allocate physical memory\n");
        return SCE_KERNEL_ERROR_EAGAIN;
    }
    *physAddrOut = static_cast<s64>(physical_addr);
    LOG_INFO_IF(true, "physAddrOut  = {}\n", log_hex_full(physical_addr));
    return SCE_OK;
}

int PS4_SYSV_ABI sceKernelMapDirectMemory(void** addr, u64 len, int prot, int flags, s64 directMemoryStart, u64 alignment) {
    auto* physical_memory = Singleton<HLE::Kernel::Objects::PhysicalMemory>::Instance();
    BREAKPOINT();
    return SCE_OK;
}

}  // namespace HLE::Libs::LibKernel::MemoryManagement