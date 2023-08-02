#include "PhysicalMemory.h"

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

        m_allocatedBlocks.push_back(block);

        *physAddrOut = find_free_pos;
        return true;
    }

    return false;
}
}
