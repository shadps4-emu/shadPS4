#include "physical_memory.h"

namespace HLE::Kernel::Objects {

static u64 AlignUp(u64 pos, u64 align) { return (align != 0 ? (pos + (align - 1)) & ~(align - 1) : pos); }

bool PhysicalMemory::Alloc(u64 searchStart, u64 searchEnd, u64 len, u64 alignment, u64* physAddrOut, int memoryType) {
    u64 find_free_pos = 0;

    // iterate through allocated blocked and find the next free position
    for (const auto& block : m_allocatedBlocks) {
        u64 n = block.start_addr + block.size;
        if (n > find_free_pos) {
            find_free_pos = n;
        }
    }

    // align free position
    find_free_pos = AlignUp(find_free_pos, alignment);

    // if the new position is between searchStart - searchEnd , allocate a new block
    if (find_free_pos >= searchStart && find_free_pos + len <= searchEnd) {
        AllocatedBlock block{};
        block.size = len;
        block.start_addr = find_free_pos;
        block.memoryType = memoryType;
        block.gpu_mode = GPU::GPUMemoryMode::NoAccess;
        block.map_size = 0;
        block.map_virtual_addr = 0;
        block.prot = 0;
        block.cpu_mode = VirtualMemory::MemoryMode::NoAccess;

        m_allocatedBlocks.push_back(block);

        *physAddrOut = find_free_pos;
        return true;
    }

    return false;
}
bool PhysicalMemory::Map(u64 virtual_addr, u64 phys_addr, u64 len, int prot, VirtualMemory::MemoryMode cpu_mode, GPU::GPUMemoryMode gpu_mode) {
    for (auto& b : m_allocatedBlocks) {
        if (phys_addr >= b.start_addr && phys_addr < b.start_addr + b.size) {
            if (b.map_virtual_addr != 0 || b.map_size != 0) {
                return false;
            }

            b.map_virtual_addr = virtual_addr;
            b.map_size = len;
            b.prot = prot;
            b.cpu_mode = cpu_mode;
            b.gpu_mode = gpu_mode;

            return true;
        }
    }

    return false;
}
}  // namespace HLE::Kernel::Objects
