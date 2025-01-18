//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "console.h"
#include "core/libraries/kernel/file_system.h"

namespace Core::Devices {
int ConsoleDevice::ioctl(u64 cmd, Common::VaCtx* args) {
    return 0;
}
s64 ConsoleDevice::write(const void* buf, size_t nbytes) {
    return s64();
}
size_t ConsoleDevice::writev(const Libraries::Kernel::SceKernelIovec* iov, int iovcnt) {
    return size_t();
}
size_t ConsoleDevice::readv(const Libraries::Kernel::SceKernelIovec* iov, int iovcnt) {
    return size_t();
}
s64 ConsoleDevice::preadv(const Libraries::Kernel::SceKernelIovec* iov, int iovcnt, u64 offset) {
    return s64();
}
s64 ConsoleDevice::lseek(s64 offset, int whence) {
    return s64();
}

s64 ConsoleDevice::read(void* buf, size_t nbytes) {
    return s64();
}

int ConsoleDevice::fstat(Libraries::Kernel::OrbisKernelStat* sb) {
    return 0;
}

s32 ConsoleDevice::fsync() {
    return s32();
}

int ConsoleDevice::ftruncate(s64 length) {
    return 0;
}

int ConsoleDevice::getdents(void* buf, u32 nbytes, s64* basep) {
    return 0;
}

s64 ConsoleDevice::pwrite(const void* buf, size_t nbytes, u64 offset) {
    return s64();
}

} // namespace Core::Devices