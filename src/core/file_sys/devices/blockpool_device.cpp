// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/file_sys/devices/blockpool_device.h"
#include "core/libraries/kernel/file_system.h"
#include "core/memory/kernel.h"

namespace Core::Devices {

std::shared_ptr<BaseDevice> BlockpoolDevice::Create(u32 handle, const char*, s32, u16) {
    return std::make_shared<BlockpoolDevice>(handle);
}

s32 BlockpoolDevice::ioctl(u32 cmd, void* args) {
    LOG_ERROR(Kernel_Fs, "called, cmd = {:#x}", cmd);
    if (cmd == GetBlockStats) {
        auto* memory = Core::Memory::Instance();
        auto& blockpool = memory->GetBlockpool();
        auto* data = reinterpret_cast<BlockStats*>(args);
        *data = blockpool.GetBlockStats();
        return ORBIS_OK;
    }
    if (cmd != PoolExpand) {
        return ORBIS_KERNEL_ERROR_ENOTTY;
    }
    auto* data = reinterpret_cast<PoolExpandArgs*>(args);
    if ((data->size & 0xffffu) != 0 || (data->flags & 0x1f000000u) != data->flags) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    u32 alignment{};
    if (data->flags == 0) {
        alignment = 64_KB;
    } else if (data->flags < 0x10000000) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    } else {
        alignment = 1u << (data->flags >> 24);
    }
    if (data->size == 0) {
        return ORBIS_OK;
    } else if (data->size >> 63) {
        return ORBIS_KERNEL_ERROR_ENOMEM;
    }
    const u64 start = std::max<s64>(data->start, 0);
    const u64 end = std::min<s64>(data->end, 0x1000000000);
    auto* memory = Core::Memory::Instance();
    auto& blockpool = memory->GetBlockpool();
    auto& dmem = memory->GetDmemManager();
    PAddr dmem_addr;
    if (dmem.Allocate(start, end, data->size, alignment, 3, &dmem_addr) != ORBIS_OK) {
        return ORBIS_KERNEL_ERROR_ENOMEM;
    }
    blockpool.Expand(dmem_addr, data->size);
    data->start = dmem_addr;
    return 0;
}

s32 BlockpoolDevice::fstat(Libraries::Kernel::OrbisKernelStat* sb) {
    std::memset(sb, 0, sizeof(*sb));
    sb->st_blksize = 0x10000;
    sb->st_mode = 0xb000;
    auto* memory = Core::Memory::Instance();
    auto& blockpool = memory->GetBlockpool();
    sb->st_blocks = blockpool.GetBlockStats().available_cached_blocks * 2;
    return 0;
}

} // namespace Core::Devices
