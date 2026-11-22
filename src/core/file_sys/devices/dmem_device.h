// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include "core/file_sys/devices/base_device.h"

namespace Core::Devices {

class DmemDevice final : public BaseDevice {
    u32 handle;

public:
    static std::shared_ptr<BaseDevice> Create(u32 handle, const char*, s32, u16);
    explicit DmemDevice(u32 handle) : handle(handle) {}

    enum DmemDevIoctls : u32 {
        GetMemorySize = 0x4008800a,
        GetMemoryType = 0xc0208004,
        AllocateMemory = 0xc0288001,
        AllocateMainMemory = 0xc0288011,
        ReleaseMemory = 0x80108002,
        CheckedReleaseMemory = 0x80108015,
        AvailableMemorySize = 0xc0208016,
        MemoryQuery = 0x80288012,
    };

    struct GetMemoryTypeArgs {
        VAddr addr;
        VAddr out_start;
        VAddr out_end;
        s32 mtype;
    };

    struct AllocateMemoryArgs {
        PAddr search_start;
        PAddr search_end;
        u64 size;
        u64 alignment;
        s32 memory_type;
    };

    struct ReleaseMemoryArgs {
        VAddr addr;
        u64 size;
    };

    struct AvailableMemoryArgs {
        PAddr search_start;
        PAddr search_end;
        u64 alignment;
        u64 out_size;
    };

    struct MemoryQueryArgs {
        u32 pool_id;
        s32 flags;
        u64 pad;
        PAddr addr;
        void* query_info;
        u64 info_size;
    };

    ~DmemDevice() override = default;

    s32 ioctl(u32 cmd, void* args) override;

    s32 mmap(u64 offset, u64 size, Core::MemoryProt prot, Core::MemoryProt* max_prot,
             Core::MemoryMapFlags flags, std::shared_ptr<VmObject>* out_object) override;

    s64 read(void* buf, u64 nbytes) override {
        return ORBIS_KERNEL_ERROR_ENODEV;
    }

    s64 write(const void* buf, u64 nbytes) override {
        return ORBIS_KERNEL_ERROR_ENODEV;
    }

    s64 readv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) override {
        return ORBIS_KERNEL_ERROR_ENODEV;
    }

    s64 writev(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) override {
        return ORBIS_KERNEL_ERROR_ENODEV;
    }

    s64 preadv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt, s64 offset) override {
        return ORBIS_KERNEL_ERROR_ENODEV;
    }

    s64 pwritev(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt, s64 offset) override {
        return ORBIS_KERNEL_ERROR_ENODEV;
    }

    s64 pwrite(const void* buf, u64 nbytes, s64 offset) override {
        return ORBIS_KERNEL_ERROR_ENODEV;
    }
};

} // namespace Core::Devices
