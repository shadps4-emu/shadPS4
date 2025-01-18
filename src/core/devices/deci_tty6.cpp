//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/kernel/file_system.h"
#include "deci_tty6.h"
#include "logger.h"

namespace Core::Devices {
int DeciTty6Device::ioctl(u64 cmd, Common::VaCtx* args) {
    return 0;
}
s64 DeciTty6Device::write(const void* buf, size_t nbytes) {
    return s64();
}
size_t DeciTty6Device::writev(const Libraries::Kernel::SceKernelIovec* iov, int iovcnt) {
    return size_t();
}
size_t DeciTty6Device::readv(const Libraries::Kernel::SceKernelIovec* iov, int iovcnt) {
    return size_t();
}
s64 DeciTty6Device::preadv(const Libraries::Kernel::SceKernelIovec* iov, int iovcnt, u64 offset) {
    return s64();
}
s64 DeciTty6Device::lseek(s64 offset, int whence) {
    return s64();
}

s64 DeciTty6Device::read(void* buf, size_t nbytes) {
    return s64();
}

int DeciTty6Device::fstat(Libraries::Kernel::OrbisKernelStat* sb) {
    return 0;
}

s32 DeciTty6Device::fsync() {
    return s32();
}

int DeciTty6Device::ftruncate(s64 length) {
    return 0;
}

int DeciTty6Device::getdents(void* buf, u32 nbytes, s64* basep) {
    return 0;
}

s64 DeciTty6Device::pwrite(const void* buf, size_t nbytes, u64 offset) {
    return s64();
}
} // namespace Core::Devices