// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdlib>
#include <ctime>
#include "common/logging/log.h"
#include "core/file_sys/devices/rng_device.h"

namespace Core::Devices {

std::shared_ptr<BaseDevice> RngDevice::Create(u32 handle, const char*, s32, u16) {
    std::srand(std::time(nullptr));
    return std::static_pointer_cast<BaseDevice>(std::make_shared<RngDevice>(handle));
}

s32 RngDevice::ioctl(u64 cmd, Common::VaCtx* args) {
    LOG_INFO(Kernel_Pthread, "called, cmd = {:#x}", cmd);
    // Both commands are for generating a random number
    if (cmd == 0x40445301 || cmd == 0x40445302) {
        auto& data = *vaArgPtr<GetRandomArgs>(&args->va_list);
        data.result = 0;
        for (u64 i = 0; i < 64; i++) {
            data.buf[i] = std::rand();
        }
    } else {
        // ENOIOCTL
        return -3;
    }
    return 0;
}

s64 RngDevice::write(const void* buf, u64 nbytes) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 RngDevice::writev(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 RngDevice::readv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 RngDevice::preadv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt, s64 offset) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 RngDevice::lseek(s64 offset, s32 whence) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 RngDevice::read(void* buf, u64 nbytes) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s32 RngDevice::fstat(Libraries::Kernel::OrbisKernelStat* sb) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s32 RngDevice::fsync() {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s32 RngDevice::ftruncate(s64 length) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 RngDevice::getdents(void* buf, u32 nbytes, s64* basep) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 RngDevice::pwrite(const void* buf, u64 nbytes, s64 offset) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

} // namespace Core::Devices