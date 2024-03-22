// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/file_sys/fs.h"
#include "core/hle/error_codes.h"
#include "core/hle/libraries/libkernel/file_system.h"
#include "core/hle/libraries/libs.h"

namespace Core::Libraries::LibKernel {

int PS4_SYSV_ABI sceKernelOpen(const char* path, int flags, u16 mode) {
    LOG_INFO(Kernel_Fs, "path = {} flags = {:#x} mode = {:#x}", path, flags, mode);
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();

    // only open files support!
    u32 handle = h->CreateHandle();
    auto* file = h->GetFile(handle);
    file->m_guest_name = path;
    file->m_host_name = mnt->GetHostFile(file->m_guest_name);

    file->f.Open(file->m_host_name, Common::FS::FileAccessMode::Read);
    if (!file->f.IsOpen()) {
        h->DeleteHandle(handle);
        return SCE_KERNEL_ERROR_EACCES;
    }
    file->is_opened = true;
    return handle;
}

int PS4_SYSV_ABI posix_open(const char* path, int flags, /* SceKernelMode*/ u16 mode) {
    LOG_INFO(Kernel_Fs, "posix open redirect to sceKernelOpen\n");
    int result = sceKernelOpen(path, flags, mode);
    // Posix calls different only for their return values
    ASSERT(result >= 0);
    return result;
}

size_t PS4_SYSV_ABI _readv(int d, const SceKernelIovec* iov, int iovcnt) {
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->GetFile(d);
    size_t total_read = 0;
    file->m_mutex.lock();
    for (int i = 0; i < iovcnt; i++) {
        total_read += file->f.ReadRaw<u8>(iov[i].iov_base, iov[i].iov_len);
    }
    file->m_mutex.unlock();
    return total_read;
}

s64 PS4_SYSV_ABI lseek(int d, s64 offset, int whence) {
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->GetFile(d);

    file->m_mutex.lock();

    if (whence == 1) {
        offset = static_cast<int64_t>(file->f.Tell()) + offset;
        whence = 0;
    }

    if (whence == 2) {
        offset = static_cast<int64_t>(file->f.GetSize()) + offset;
        whence = 0;
    }

    file->f.Seek(offset);
    auto pos = static_cast<int64_t>(file->f.Tell());

    file->m_mutex.unlock();
    return pos;
}

void fileSystemSymbolsRegister(Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("1G3lF1Gg1k8", "libkernel", 1, "libkernel", 1, 1, sceKernelOpen);
    LIB_FUNCTION("wuCroIGjt2g", "libScePosix", 1, "libkernel", 1, 1, posix_open);
    LIB_FUNCTION("+WRlkKjZvag", "libkernel", 1, "libkernel", 1, 1, _readv);
    LIB_FUNCTION("Oy6IpwgtYOk", "libkernel", 1, "libkernel", 1, 1, lseek);

    // openOrbis (to check if it is valid out of OpenOrbis
    LIB_FUNCTION("6c3rCVE-fTU", "libkernel", 1, "libkernel", 1, 1,
                 posix_open); // _open shoudld be equal to open function
}

} // namespace Core::Libraries::LibKernel
