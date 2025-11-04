// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/file_sys/devices/deci_tty6_device.h"

namespace Core::Devices {

std::shared_ptr<BaseDevice> DeciTty6Device::Create(u32 handle, const char*, s32, u16) {
    return std::static_pointer_cast<BaseDevice>(std::make_shared<DeciTty6Device>(handle));
}

s32 DeciTty6Device::ioctl(u64 cmd, Common::VaCtx* args) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called, cmd = {:#x}", cmd);
    return 0;
}

s64 DeciTty6Device::write(const void* buf, u64 nbytes) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 DeciTty6Device::writev(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 DeciTty6Device::readv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 DeciTty6Device::preadv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt, s64 offset) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 DeciTty6Device::lseek(s64 offset, s32 whence) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 DeciTty6Device::read(void* buf, u64 nbytes) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s32 DeciTty6Device::fstat(Libraries::Kernel::OrbisKernelStat* sb) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s32 DeciTty6Device::fsync() {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s32 DeciTty6Device::ftruncate(s64 length) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 DeciTty6Device::getdents(void* buf, u32 nbytes, s64* basep) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

s64 DeciTty6Device::pwrite(const void* buf, u64 nbytes, s64 offset) {
    LOG_ERROR(Kernel_Fs, "(STUBBED) called");
    return 0;
}

} // namespace Core::Devices