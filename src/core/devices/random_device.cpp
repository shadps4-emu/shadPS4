// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdlib>
#include <ctime>
#include "common/logging/log.h"
#include "random_device.h"

namespace Core::Devices {

std::shared_ptr<BaseDevice> RandomDevice::Create(u32 handle, const char*, int, u16) {
    std::srand(std::time(nullptr));
    return std::shared_ptr<BaseDevice>(
        reinterpret_cast<Devices::BaseDevice*>(new RandomDevice(handle)));
}

int RandomDevice::ioctl(u64 cmd, Common::VaCtx* args) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

s64 RandomDevice::write(const void* buf, size_t nbytes) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

size_t RandomDevice::writev(const Libraries::Kernel::SceKernelIovec* iov, int iovcnt) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

size_t RandomDevice::readv(const Libraries::Kernel::SceKernelIovec* iov, int iovcnt) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

s64 RandomDevice::preadv(const Libraries::Kernel::SceKernelIovec* iov, int iovcnt, u64 offset) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

s64 RandomDevice::lseek(s64 offset, int whence) {
    return 0;
}

s64 RandomDevice::read(void* buf, size_t nbytes) {
    auto rbuf = static_cast<char*>(buf);
    for (size_t i = 0; i < nbytes; i++) {
        rbuf[i] = std::rand() & 0xFF;
    }
    return nbytes;
}

int RandomDevice::fstat(Libraries::Kernel::OrbisKernelStat* sb) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

s32 RandomDevice::fsync() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

int RandomDevice::ftruncate(s64 length) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

int RandomDevice::getdents(void* buf, u32 nbytes, s64* basep) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

s64 RandomDevice::pwrite(const void* buf, size_t nbytes, u64 offset) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

} // namespace Core::Devices