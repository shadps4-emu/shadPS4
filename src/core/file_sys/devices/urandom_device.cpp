// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdlib>
#include <ctime>
#include "common/logging/log.h"
#include "urandom_device.h"

namespace Core::Devices {

std::shared_ptr<BaseDevice> URandomDevice::Create(u32 handle, const char*, int, u16) {
    std::srand(std::time(nullptr));
    return std::shared_ptr<BaseDevice>(
        reinterpret_cast<Devices::BaseDevice*>(new URandomDevice(handle)));
}

int URandomDevice::ioctl(u64 cmd, Common::VaCtx* args) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

s64 URandomDevice::write(const void* buf, size_t nbytes) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

size_t URandomDevice::writev(const Libraries::Kernel::OrbisKernelIovec* iov, int iovcnt) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

size_t URandomDevice::readv(const Libraries::Kernel::OrbisKernelIovec* iov, int iovcnt) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

s64 URandomDevice::preadv(const Libraries::Kernel::OrbisKernelIovec* iov, int iovcnt, u64 offset) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

s64 URandomDevice::lseek(s64 offset, int whence) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

s64 URandomDevice::read(void* buf, size_t nbytes) {
    auto rbuf = static_cast<char*>(buf);
    for (size_t i = 0; i < nbytes; i++) {
        rbuf[i] = std::rand() & 0xFF;
    }
    return nbytes;
}

int URandomDevice::fstat(Libraries::Kernel::OrbisKernelStat* sb) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

s32 URandomDevice::fsync() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

int URandomDevice::ftruncate(s64 length) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

int URandomDevice::getdents(void* buf, u32 nbytes, s64* basep) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

s64 URandomDevice::pwrite(const void* buf, size_t nbytes, u64 offset) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

} // namespace Core::Devices