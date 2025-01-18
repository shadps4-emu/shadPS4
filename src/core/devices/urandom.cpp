//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/kernel/file_system.h"
#include "logger.h"
#include "urandom.h"

namespace Core::Devices {

std::shared_ptr<BaseDevice> URandomDevice::Create(u32 handle, const char*, int, u16) {
    return std::shared_ptr<BaseDevice>(
        reinterpret_cast<Devices::BaseDevice*>(new URandomDevice(handle)));
}

int URandomDevice::ioctl(u64 cmd, Common::VaCtx* args) {
    return 0;
}
s64 URandomDevice::write(const void* buf, size_t nbytes) {
    return s64();
}
size_t URandomDevice::writev(const Libraries::Kernel::SceKernelIovec* iov, int iovcnt) {
    return size_t();
}
size_t URandomDevice::readv(const Libraries::Kernel::SceKernelIovec* iov, int iovcnt) {
    return size_t();
}
s64 URandomDevice::preadv(const Libraries::Kernel::SceKernelIovec* iov, int iovcnt, u64 offset) {
    return s64();
}
s64 URandomDevice::lseek(s64 offset, int whence) {
    return s64();
}

s64 URandomDevice::read(void* buf, size_t nbytes) {
    return s64();
}

int URandomDevice::fstat(Libraries::Kernel::OrbisKernelStat* sb) {
    return 0;
}

s32 URandomDevice::fsync() {
    return s32();
}

int URandomDevice::ftruncate(s64 length) {
    return 0;
}

int URandomDevice::getdents(void* buf, u32 nbytes, s64* basep) {
    return 0;
}

s64 URandomDevice::pwrite(const void* buf, size_t nbytes, u64 offset) {
    return s64();
}
} // namespace Core::Devices