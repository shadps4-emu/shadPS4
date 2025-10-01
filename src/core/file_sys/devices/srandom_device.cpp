// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdlib>
#include <ctime>
#include "common/logging/log.h"
#include "core/file_sys/devices/srandom_device.h"

namespace Core::Devices {

std::shared_ptr<BaseDevice> SRandomDevice::Create(u32 handle, const char*, s32, u16) {
    std::srand(std::time(nullptr));
    return std::static_pointer_cast<BaseDevice>(std::make_shared<SRandomDevice>(handle));
}

s32 SRandomDevice::ioctl(u64 cmd, Common::VaCtx* args) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called, cmd = {:#x}", cmd);
    return 0;
}

s64 SRandomDevice::write(const void* buf, u64 nbytes) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 SRandomDevice::writev(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 SRandomDevice::readv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 SRandomDevice::preadv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt, s64 offset) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 SRandomDevice::lseek(s64 offset, s32 whence) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 SRandomDevice::read(void* buf, u64 nbytes) {
    auto rbuf = static_cast<s8*>(buf);
    for (u64 i = 0; i < nbytes; i++) {
        rbuf[i] = std::rand();
    }
    return nbytes;
}

s32 SRandomDevice::fstat(Libraries::Kernel::OrbisKernelStat* sb) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s32 SRandomDevice::fsync() {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s32 SRandomDevice::ftruncate(s64 length) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 SRandomDevice::getdents(void* buf, u32 nbytes, s64* basep) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 SRandomDevice::pwrite(const void* buf, u64 nbytes, s64 offset) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

} // namespace Core::Devices