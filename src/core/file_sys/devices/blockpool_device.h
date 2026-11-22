// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include "core/file_sys/devices/base_device.h"

namespace Core::Devices {

class BlockpoolDevice final : public BaseDevice {
    u32 handle;

public:
    static std::shared_ptr<BaseDevice> Create(u32 handle, const char*, s32, u16);
    explicit BlockpoolDevice(u32 handle) : handle(handle) {}

    ~BlockpoolDevice() override = default;

    enum BlockpoolDevIoctls : u32 {
        GetBlockStats = 0x4010a802,
        PoolExpand = 0xc020a801,
    };

    struct PoolExpandArgs {
        u64 size;
        u64 start;
        u64 end;
        u32 flags;
    };

    s32 ioctl(u32 cmd, void* args) override;
    s32 fstat(Libraries::Kernel::OrbisKernelStat* sb) override;

    s64 write(const void* buf, u64 nbytes) override {
        return ORBIS_KERNEL_ERROR_ENXIO;
    }

    s64 readv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) override {
        return ORBIS_KERNEL_ERROR_ENXIO;
    }

    s64 writev(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) override {
        return ORBIS_KERNEL_ERROR_ENXIO;
    }

    s64 preadv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt, s64 offset) override {
        return ORBIS_KERNEL_ERROR_ENXIO;
    }

    s64 pwritev(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt, s64 offset) override {
        return ORBIS_KERNEL_ERROR_ENXIO;
    }

    s64 pwrite(const void* buf, u64 nbytes, s64 offset) override {
        return ORBIS_KERNEL_ERROR_ENXIO;
    }

    s64 read(void* buf, u64 nbytes) override {
        return ORBIS_KERNEL_ERROR_ENXIO;
    }

private:
};

} // namespace Core::Devices
