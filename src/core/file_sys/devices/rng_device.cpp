// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdlib>
#include <ctime>
#include "common/logging/log.h"
#include "rng_device.h"

namespace Core::Devices {

std::shared_ptr<BaseDevice> RngDevice::Create(u32 handle, const char*, int, u16) {
    std::srand(std::time(nullptr));
    return std::shared_ptr<BaseDevice>(
        reinterpret_cast<Devices::BaseDevice*>(new RngDevice(handle)));
}

int RngDevice::ioctl(u64 cmd, Common::VaCtx* args) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    if (cmd == 0x40445301 || cmd == 0x40445302) {
        
    }
    return 0;
}

s64 RngDevice::write(const void* buf, size_t nbytes) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

size_t RngDevice::writev(const Libraries::Kernel::OrbisKernelIovec* iov, int iovcnt) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

size_t RngDevice::readv(const Libraries::Kernel::OrbisKernelIovec* iov, int iovcnt) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

s64 RngDevice::preadv(const Libraries::Kernel::OrbisKernelIovec* iov, int iovcnt, u64 offset) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

s64 RngDevice::lseek(s64 offset, int whence) {
    return 0;
}

s64 RngDevice::read(void* buf, size_t nbytes) {
    auto rbuf = static_cast<char*>(buf);
    for (size_t i = 0; i < nbytes; i++) {
        rbuf[i] = std::rand() & 0xFF;
    }
    return nbytes;
}

int RngDevice::fstat(Libraries::Kernel::OrbisKernelStat* sb) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

s32 RngDevice::fsync() {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

int RngDevice::ftruncate(s64 length) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

int RngDevice::getdents(void* buf, u32 nbytes, s64* basep) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

s64 RngDevice::pwrite(const void* buf, size_t nbytes, u64 offset) {
    LOG_ERROR(Kernel_Pthread, "(STUBBED) called");
    return 0;
}

} // namespace Core::Devices