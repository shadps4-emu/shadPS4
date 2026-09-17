// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/file_sys/devices/deci_tty_device.h"

namespace Core::Devices {

std::shared_ptr<BaseDevice> DeciTtyDevice::Create(u32 handle, const char*, s32, u16) {
    return std::static_pointer_cast<BaseDevice>(std::make_shared<DeciTtyDevice>(handle));
}

s32 DeciTtyDevice::ioctl(u64 cmd, Common::VaCtx* args) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called, cmd = {:#x}", cmd);
    return 0;
}

s64 DeciTtyDevice::write(const void* buf, u64 nbytes) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 DeciTtyDevice::writev(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 DeciTtyDevice::readv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 DeciTtyDevice::preadv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt, s64 offset) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 DeciTtyDevice::lseek(s64 offset, s32 whence) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 DeciTtyDevice::read(void* buf, u64 nbytes) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s32 DeciTtyDevice::fstat(Libraries::Kernel::OrbisKernelStat* sb) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s32 DeciTtyDevice::fsync() {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s32 DeciTtyDevice::ftruncate(s64 length) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 DeciTtyDevice::getdents(void* buf, u32 nbytes, s64* basep) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 DeciTtyDevice::pwrite(const void* buf, u64 nbytes, s64 offset) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

} // namespace Core::Devices