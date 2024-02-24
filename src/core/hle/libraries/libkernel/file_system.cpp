// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/debug.h"
#include "common/log.h"
#include "common/singleton.h"
#include "core/file_sys/fs.h"
#include "core/hle/error_codes.h"
#include "core/hle/libraries/libkernel/file_system.h"
#include "core/hle/libraries/libs.h"

namespace Core::Libraries::LibKernel {

constexpr bool log_file_fs = true; // disable it to disable logging

int PS4_SYSV_ABI sceKernelOpen(const char* path, int flags, u16 mode) {
    LOG_INFO_IF(log_file_fs, "sceKernelOpen path = {} flags = {:#x} mode = {:#x}\n", path, flags,
                mode);
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();

    // only open files support!
    u32 handle = h->createHandle();
    auto* file = h->getFile(handle);
    file->m_guest_name = path;
    file->m_host_name = mnt->getHostFile(file->m_guest_name);

    bool result = file->f.open(file->m_host_name);
    if (!result) {
        h->deleteHandle(handle);
        return SCE_KERNEL_ERROR_EACCES;
    }
    file->isOpened = true;
    return handle;
}

int PS4_SYSV_ABI posix_open(const char* path, int flags, /* SceKernelMode*/ u16 mode) {
    LOG_INFO_IF(log_file_fs, "posix open redirect to sceKernelOpen\n");
    int result = sceKernelOpen(path, flags, mode);
    if (result < 0) {
        BREAKPOINT(); // posix calls different only for their return values
    }
    return result;
}

size_t PS4_SYSV_ABI _readv(int d, const SceKernelIovec* iov, int iovcnt) {
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->getFile(d);
    size_t total_read = 0;
    file->m_mutex.lock();
    for (int i = 0; i < iovcnt; i++) {
        total_read += file->f.readBytes(iov[i].iov_base, iov[i].iov_len).second;
    }
    file->m_mutex.unlock();
    return total_read;
}

void fileSystemSymbolsRegister(Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("1G3lF1Gg1k8", "libkernel", 1, "libkernel", 1, 1, sceKernelOpen);
    LIB_FUNCTION("wuCroIGjt2g", "libScePosix", 1, "libkernel", 1, 1, posix_open);
    LIB_FUNCTION("+WRlkKjZvag", "libkernel", 1, "libkernel", 1, 1, _readv);

    // openOrbis (to check if it is valid out of OpenOrbis
    LIB_FUNCTION("6c3rCVE-fTU", "libkernel", 1, "libkernel", 1, 1,
                 posix_open); // _open shoudld be equal to open function
}

} // namespace Core::Libraries::LibKernel
