#pragma once
#include <types.h>

#include <vector>

namespace HLE::Kernel::Objects {

class PhysicalMemory {
  public:
    struct AllocatedBlock {
        u64 start_addr;
        u64 size;
        int memoryType;
    };
    PhysicalMemory() {}
    virtual ~PhysicalMemory() {}

  public:
    bool Alloc(u64 searchStart, u64 searchEnd, u64 len, u64 alignment, u64* physAddrOut, int memoryType);

  private:
    std::vector<AllocatedBlock> m_allocatedBlocks;
};

}  // namespace HLE::Kernel::Objects