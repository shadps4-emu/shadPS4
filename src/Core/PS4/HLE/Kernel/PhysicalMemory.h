#pragma once

namespace HLE::Libs::LibKernel::MemoryManagement {

class PysicalMemory {
    struct AllocatedBlock {
        u64 start_addr;
        u64 size;
    };

  private:
    std::vector<AllocatedBlock> m_allocatedBlocks;
};

}  // namespace HLE::Libs::LibKernel::MemoryManagement