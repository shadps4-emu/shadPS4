// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <vector>
#include "common/types.h"
#include "core/PS4/GPU/gpu_memory.h"
#include "core/virtual_memory.h"

namespace Core::Kernel {

class PhysicalMemory {
public:
    struct AllocatedBlock {
        u64 start_addr;
        u64 size;
        int memoryType;
        u64 map_virtual_addr;
        u64 map_size;
        int prot;
        VirtualMemory::MemoryMode cpu_mode;
        GPU::MemoryMode gpu_mode;
    };
    PhysicalMemory() {}
    virtual ~PhysicalMemory() {}

public:
    bool Alloc(u64 searchStart, u64 searchEnd, u64 len, u64 alignment, u64* physAddrOut,
               int memoryType);
    bool Map(u64 virtual_addr, u64 phys_addr, u64 len, int prot, VirtualMemory::MemoryMode cpu_mode,
             GPU::MemoryMode gpu_mode);

private:
    std::vector<AllocatedBlock> m_allocatedBlocks;
    std::mutex m_mutex;
};

} // namespace Core::Kernel
