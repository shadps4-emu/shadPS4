#include "memory_management.h"

#include <core/PS4/GPU/gpu_memory.h>
#include <core/virtual_memory.h>
#include "common/log.h"
#include "common/debug.h"

#include <bit>
#include <magic_enum.hpp>

#include "common/singleton.h"
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
    auto* physical_memory = singleton<HLE::Kernel::Objects::PhysicalMemory>::instance();
    if (!physical_memory->Alloc(searchStart, searchEnd, len, alignment, &physical_addr, memoryType)) {
        LOG_TRACE_IF(log_file_memory, "sceKernelAllocateDirectMemory returned SCE_KERNEL_ERROR_EAGAIN can't allocate physical memory\n");
        return SCE_KERNEL_ERROR_EAGAIN;
    }
    *physAddrOut = static_cast<s64>(physical_addr);
    LOG_INFO_IF(true, "physAddrOut  = {}\n", log_hex_full(physical_addr));
    return SCE_OK;
}

int PS4_SYSV_ABI sceKernelMapDirectMemory(void** addr, u64 len, int prot, int flags, s64 directMemoryStart, u64 alignment) {
    PRINT_FUNCTION_NAME();
    if (len == 0 || !is16KBAligned(len)) {
        LOG_TRACE_IF(log_file_memory, "sceKernelMapDirectMemory returned SCE_KERNEL_ERROR_EINVAL len invalid\n");
        return SCE_KERNEL_ERROR_EINVAL;
    }
    if (!is16KBAligned(directMemoryStart)) {
        LOG_TRACE_IF(log_file_memory, "sceKernelMapDirectMemory returned SCE_KERNEL_ERROR_EINVAL directMemoryStart invalid\n");
        return SCE_KERNEL_ERROR_EINVAL;
    }
    if (alignment != 0) {
        if ((!isPowerOfTwo(alignment) && !is16KBAligned(alignment))) {
            LOG_TRACE_IF(log_file_memory, "sceKernelMapDirectMemory returned SCE_KERNEL_ERROR_EINVAL alignment invalid\n");
            return SCE_KERNEL_ERROR_EINVAL;
        }
    }

    LOG_INFO_IF(log_file_memory, "len               = {}\n", log_hex_full(len));
    LOG_INFO_IF(log_file_memory, "prot              = {}\n", log_hex_full(prot));
    LOG_INFO_IF(log_file_memory, "flags             = {}\n", log_hex_full(flags));
    LOG_INFO_IF(log_file_memory, "directMemoryStart = {}\n", log_hex_full(directMemoryStart));
    LOG_INFO_IF(log_file_memory, "alignment         = {}\n", log_hex_full(alignment));

    VirtualMemory::MemoryMode cpu_mode = VirtualMemory::MemoryMode::NoAccess;
    GPU::MemoryMode gpu_mode = GPU::MemoryMode::NoAccess;

    switch (prot) {
        case 0x32:
        case 0x33:  // SCE_KERNEL_PROT_CPU_READ|SCE_KERNEL_PROT_CPU_WRITE|SCE_KERNEL_PROT_GPU_READ|SCE_KERNEL_PROT_GPU_ALL
            cpu_mode = VirtualMemory::MemoryMode::ReadWrite;
            gpu_mode = GPU::MemoryMode::ReadWrite;
            break;
        default: BREAKPOINT();
    }

    auto in_addr = reinterpret_cast<u64>(*addr);
    u64 out_addr = 0;

    if (flags == 0) {
        out_addr = VirtualMemory::memory_alloc_aligned(in_addr, len, cpu_mode, alignment);
    }
    LOG_INFO_IF(log_file_memory, "in_addr           = {}\n", log_hex_full(in_addr));
    LOG_INFO_IF(log_file_memory, "out_addr          = {}\n", log_hex_full(out_addr));

    *addr = reinterpret_cast<void*>(out_addr);  // return out_addr to first functions parameter

    if (out_addr == 0) {
        return SCE_KERNEL_ERROR_ENOMEM;
    }

    auto* physical_memory = singleton<HLE::Kernel::Objects::PhysicalMemory>::instance();
    if (!physical_memory->Map(out_addr, directMemoryStart, len, prot, cpu_mode, gpu_mode)) {
        BREAKPOINT();
    }

    if (gpu_mode != GPU::MemoryMode::NoAccess) {
        GPU::memorySetAllocArea(out_addr, len);
    }
    return SCE_OK;
}

}  // namespace HLE::Libs::LibKernel::MemoryManagement