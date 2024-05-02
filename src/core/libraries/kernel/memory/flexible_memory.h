// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <vector>
#include "common/types.h"
#include "core/virtual_memory.h"

namespace Libraries::Kernel {

class FlexibleMemory {
public:
    struct AllocatedBlock {
        u64 map_virtual_addr;
        u64 map_size;
        int prot;
        VirtualMemory::MemoryMode cpu_mode;
    };

    FlexibleMemory(){};
    virtual ~FlexibleMemory(){};

public:
    bool Map(u64 virtual_addr, std::size_t len, int prot, VirtualMemory::MemoryMode cpu_mode);

private:
    std::vector<AllocatedBlock> allocated_blocks;
    u64 allocated_total = 0;
    std::mutex mutex;
};
} // namespace Libraries::Kernel