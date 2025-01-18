//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/kernel/file_system.h"
#include "logger.h"
#include "random.h"

namespace Core::Devices {
int RandomDevice::ioctl(u64 cmd, Common::VaCtx* args) {
    return 0;
}
s64 RandomDevice::write(const void* buf, size_t nbytes) {
    return s64();
}
size_t RandomDevice::writev(const Libraries::Kernel::SceKernelIovec* iov, int iovcnt) {
    return size_t();
}
size_t RandomDevice::readv(const Libraries::Kernel::SceKernelIovec* iov, int iovcnt) {
    return size_t();
}
s64 RandomDevice::preadv(const Libraries::Kernel::SceKernelIovec* iov, int iovcnt, u64 offset) {
    return s64();
}
s64 RandomDevice::lseek(s64 offset, int whence) {
    return s64();
}

s64 RandomDevice::read(void* buf, size_t nbytes) {
    return s64();
}

int RandomDevice::fstat(Libraries::Kernel::OrbisKernelStat* sb) {
    return 0;
}

s32 RandomDevice::fsync() {
    return s32();
}

int RandomDevice::ftruncate(s64 length) {
    return 0;
}

int RandomDevice::getdents(void* buf, u32 nbytes, s64* basep) {
    return 0;
}

s64 RandomDevice::pwrite(const void* buf, size_t nbytes, u64 offset) {
    return s64();
}
} // namespace Core::Devices