// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "flexible_memory.h"

namespace Libraries::Kernel {
bool FlexibleMemory::Map(u64 virtual_addr, std::size_t len, int prot,
                         VirtualMemory::MemoryMode cpu_mode) {
    std::scoped_lock lock{mutex};

    AllocatedBlock block{};
    block.map_virtual_addr = virtual_addr;
    block.map_size = len;
    block.prot = prot;
    block.cpu_mode = cpu_mode;

    allocated_blocks.push_back(block);
    allocated_total += len;

    return true;
}
} // namespace Libraries::Kernel