// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdlib>
#include <ctime>
#include "common/logging/log.h"
#include "core/file_sys/devices/random_device.h"

namespace Core::Devices {

std::shared_ptr<BaseDevice> RandomDevice::Create(u32 handle, const char*, s32, u16) {
    std::srand(std::time(nullptr));
    return std::static_pointer_cast<BaseDevice>(std::make_shared<RandomDevice>(handle));
}

s32 RandomDevice::ioctl(u64 cmd, Common::VaCtx* args) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called, cmd = {:#x}", cmd);
    return 0;
}

s64 RandomDevice::write(const void* buf, u64 nbytes) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 RandomDevice::writev(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 RandomDevice::readv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 RandomDevice::preadv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt, s64 offset) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 RandomDevice::lseek(s64 offset, s32 whence) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 RandomDevice::read(void* buf, u64 nbytes) {
    auto rbuf = static_cast<s8*>(buf);
    for (u64 i = 0; i < nbytes; i++) {
        rbuf[i] = std::rand() & 0xFF;
    }
    return nbytes;
}

s32 RandomDevice::fstat(Libraries::Kernel::OrbisKernelStat* sb) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s32 RandomDevice::fsync() {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s32 RandomDevice::ftruncate(s64 length) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 RandomDevice::getdents(void* buf, u32 nbytes, s64* basep) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 RandomDevice::pwrite(const void* buf, u64 nbytes, s64 offset) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

} // namespace Core::Devices