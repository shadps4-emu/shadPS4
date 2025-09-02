// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "deci_tty6_device.h"

namespace Core::Devices {

std::shared_ptr<BaseDevice> DeciTty6Device::Create(u32 handle, const char*, int, u16) {
    return std::shared_ptr<BaseDevice>(
        reinterpret_cast<Devices::BaseDevice*>(new DeciTty6Device(handle)));
}

int DeciTty6Device::ioctl(u64 cmd, Common::VaCtx* args) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

s64 DeciTty6Device::write(const void* buf, size_t nbytes) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

size_t DeciTty6Device::writev(const Libraries::Kernel::OrbisKernelIovec* iov, int iovcnt) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

size_t DeciTty6Device::readv(const Libraries::Kernel::OrbisKernelIovec* iov, int iovcnt) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

s64 DeciTty6Device::preadv(const Libraries::Kernel::OrbisKernelIovec* iov, int iovcnt, u64 offset) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

s64 DeciTty6Device::lseek(s64 offset, int whence) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

s64 DeciTty6Device::read(void* buf, size_t nbytes) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

int DeciTty6Device::fstat(Libraries::Kernel::OrbisKernelStat* sb) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

s32 DeciTty6Device::fsync() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

int DeciTty6Device::ftruncate(s64 length) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

int DeciTty6Device::getdents(void* buf, u32 nbytes, s64* basep) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

s64 DeciTty6Device::pwrite(const void* buf, size_t nbytes, u64 offset) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

} // namespace Core::Devices