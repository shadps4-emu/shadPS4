// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdlib>
#include <ctime>
#include "common/logging/log.h"
#include "srandom_device.h"

namespace Core::Devices {

std::shared_ptr<BaseDevice> SRandomDevice::Create(u32 handle, const char*, int, u16) {
    std::srand(std::time(nullptr));
    return std::shared_ptr<BaseDevice>(
        reinterpret_cast<Devices::BaseDevice*>(new SRandomDevice(handle)));
}

int SRandomDevice::ioctl(u64 cmd, Common::VaCtx* args) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

s64 SRandomDevice::write(const void* buf, size_t nbytes) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

size_t SRandomDevice::writev(const Libraries::Kernel::OrbisKernelIovec* iov, int iovcnt) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

size_t SRandomDevice::readv(const Libraries::Kernel::OrbisKernelIovec* iov, int iovcnt) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

s64 SRandomDevice::preadv(const Libraries::Kernel::OrbisKernelIovec* iov, int iovcnt, u64 offset) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

s64 SRandomDevice::lseek(s64 offset, int whence) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

s64 SRandomDevice::read(void* buf, size_t nbytes) {
    auto rbuf = static_cast<char*>(buf);
    for (size_t i = 0; i < nbytes; i++) {
        rbuf[i] = std::rand();
    }
    return nbytes;
}

int SRandomDevice::fstat(Libraries::Kernel::OrbisKernelStat* sb) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

s32 SRandomDevice::fsync() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return s32();
}

int SRandomDevice::ftruncate(s64 length) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

int SRandomDevice::getdents(void* buf, u32 nbytes, s64* basep) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

s64 SRandomDevice::pwrite(const void* buf, size_t nbytes, u64 offset) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

} // namespace Core::Devices