// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/logging/log.h"
#include "common/types.h"
#include "core/file_sys/quasifs/quasifs_inode_quasi_device.h"

namespace Core::Devices {

class NopDevice final : public QuasiFS::Device {

public:
    NopDevice() = default;
    ~NopDevice() = default;

    static QuasiFS::dev_ptr Create() {
        return std::make_shared<NopDevice>();
    }

    // clang-format off
    s64 read(void* buf, u64 count) override { return 0; };
    s64 write(const void* buf, u64 count) override { return 0; };
    s32 ioctl(u64 cmd, Common::VaCtx* args) override { return 0; };
    s32 fstat(Libraries::Kernel::OrbisKernelStat* sb) override { return 0; };
    s32 fsync() override { return 0; };
    s32 ftruncate(s64 length) override { return 0; };
    s64 getdents(void* buf, u64 nbytes, s64 offset, s64* basep) override { return 0; };
    // clang-format on
};

} // namespace Core::Devices
