// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/file_sys/fs.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/file_system.h"
#include "core/libraries/libs.h"

namespace Libraries::Kernel {

int PS4_SYSV_ABI sceKernelOpen(const char* path, int flags, u16 mode) {
    LOG_INFO(Kernel_Fs, "path = {} flags = {:#x} mode = {}", path, flags, mode);
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();

    bool read = (flags & 0x3) == ORBIS_KERNEL_O_RDONLY;
    bool write = (flags & 0x3) == ORBIS_KERNEL_O_WRONLY;
    bool rdwr = (flags & 0x3) == ORBIS_KERNEL_O_RDWR;

    bool nonblock = (flags & ORBIS_KERNEL_O_NONBLOCK) != 0;
    bool append = (flags & ORBIS_KERNEL_O_APPEND) != 0;
    bool fsync = (flags & ORBIS_KERNEL_O_FSYNC) != 0;
    bool sync = (flags & ORBIS_KERNEL_O_SYNC) != 0;
    bool create = (flags & ORBIS_KERNEL_O_CREAT) != 0;
    bool truncate = (flags & ORBIS_KERNEL_O_TRUNC) != 0;
    bool excl = (flags & ORBIS_KERNEL_O_EXCL) != 0;
    bool dsync = (flags & ORBIS_KERNEL_O_DSYNC) != 0;
    bool direct = (flags & ORBIS_KERNEL_O_DIRECT) != 0;
    bool directory = (flags & ORBIS_KERNEL_O_DIRECTORY) != 0;

    if (directory) {
        UNREACHABLE(); // not supported yet
    } else {
        u32 handle = h->CreateHandle();
        auto* file = h->GetFile(handle);
        file->m_guest_name = path;
        file->m_host_name = mnt->GetHostFile(file->m_guest_name);
        if (read) {
            file->f.Open(file->m_host_name, Common::FS::FileAccessMode::Read);
        } else if (write && create && truncate) {
            file->f.Open(file->m_host_name, Common::FS::FileAccessMode::Write);
        } else {
            UNREACHABLE();
        }
        if (!file->f.IsOpen()) {
            h->DeleteHandle(handle);
            return SCE_KERNEL_ERROR_EACCES;
        }
        file->is_opened = true;
        return handle;
    }
    return -1; // dummy
}

int PS4_SYSV_ABI posix_open(const char* path, int flags, /* SceKernelMode*/ u16 mode) {
    LOG_INFO(Kernel_Fs, "posix open redirect to sceKernelOpen\n");
    int result = sceKernelOpen(path, flags, mode);
    // Posix calls different only for their return values
    ASSERT(result >= 0);
    return result;
}

int PS4_SYSV_ABI sceKernelClose(int d) {
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->GetFile(d);
    if (file == nullptr) {
        return SCE_KERNEL_ERROR_EBADF;
    }
    if (!file->is_directory) {
        file->f.Close();
    }
    file->is_opened = false;
    LOG_INFO(Kernel_Fs, "Closing {}", file->m_guest_name);
    h->DeleteHandle(d);
    return SCE_OK;
}

size_t PS4_SYSV_ABI sceKernelWrite(int d, void* buf, size_t nbytes) {
    if (buf == nullptr) {
        return SCE_KERNEL_ERROR_EFAULT;
    }
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->GetFile(d);
    if (file == nullptr) {
        return SCE_KERNEL_ERROR_EBADF;
    }
    file->m_mutex.lock();
    u32 bytes_write = file->f.WriteRaw<u8>(buf, static_cast<u32>(nbytes));
    file->m_mutex.unlock();
    return bytes_write;
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

s64 PS4_SYSV_ABI sceKernelLseek(int d, s64 offset, int whence) {
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->GetFile(d);

    file->m_mutex.lock();
    Common::FS::SeekOrigin origin;
    if (whence == 0) {
        origin = Common::FS::SeekOrigin::SetOrigin;
    }

    if (whence == 1) {
        origin = Common::FS::SeekOrigin::CurrentPosition;
    }
    if (whence == 2) {
        origin = Common::FS::SeekOrigin::End;
    }

    file->f.Seek(offset, origin);
    auto pos = static_cast<int64_t>(file->f.Tell());

    file->m_mutex.unlock();
    return pos;
}

s64 PS4_SYSV_ABI lseek(int d, s64 offset, int whence) {
    return sceKernelLseek(d, offset, whence);
}

s64 PS4_SYSV_ABI sceKernelRead(int d, void* buf, size_t nbytes) {
    if (buf == nullptr) {
        return SCE_KERNEL_ERROR_EFAULT;
    }

    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->GetFile(d);
    if (file == nullptr) {
        return SCE_KERNEL_ERROR_EBADF;
    }
    file->m_mutex.lock();
    u32 bytes_read = file->f.ReadRaw<u8>(buf, static_cast<u32>(nbytes));
    file->m_mutex.unlock();
    return bytes_read;
}

int PS4_SYSV_ABI sceKernelMkdir(const char* path, u16 mode) {
    LOG_INFO(Kernel_Fs, "path = {} mode = {}", path, mode);
    if (path == nullptr) {
        return SCE_KERNEL_ERROR_EINVAL;
    }
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    std::string dir_name = mnt->GetHostFile(path);
    if (std::filesystem::exists(dir_name)) {
        return SCE_KERNEL_ERROR_EEXIST;
    }

    if (!std::filesystem::create_directory(dir_name)) {
        return SCE_KERNEL_ERROR_EIO;
    }

    if (!std::filesystem::exists(dir_name)) {
        return SCE_KERNEL_ERROR_ENOENT;
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceKernelStat(const char* path, OrbisKernelStat* sb) {
    LOG_INFO(Kernel_Fs, "(PARTIAL) path = {}", path);
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    std::string path_name = mnt->GetHostFile(path);
    memset(sb, 0, sizeof(OrbisKernelStat));
    bool is_dir = std::filesystem::is_directory(path_name);
    bool is_file = std::filesystem::is_regular_file(path_name);
    if (!is_dir && !is_file) {
        return ORBIS_KERNEL_ERROR_ENOENT;
    }
    if (std::filesystem::is_directory(path_name)) {
        sb->st_mode = 0000777u | 0040000u;
        sb->st_size = 0;
        sb->st_blksize = 512;
        sb->st_blocks = 0;
        // TODO incomplete
    } else {
        sb->st_mode = 0000777u | 0100000u;
        sb->st_size = static_cast<int64_t>(std::filesystem::file_size(path_name));
        sb->st_blksize = 512;
        sb->st_blocks = (sb->st_size + 511) / 512;
        // TODO incomplete
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI posix_stat(const char* path, OrbisKernelStat* sb) {
    int result = sceKernelStat(path, sb);
    if (result < 0) {
        UNREACHABLE(); // TODO
    }
    return ORBIS_OK;
}

s64 PS4_SYSV_ABI sceKernelPread(int d, void* buf, size_t nbytes, s64 offset) {
    if (d < 3) {
        return ORBIS_KERNEL_ERROR_EPERM;
    }

    if (buf == nullptr) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }

    if (offset < 0) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->GetFile(d);

    if (file == nullptr) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }
    file->m_mutex.lock();
    if (file->f.Tell() != offset) {
        file->f.Seek(offset);
    }
    u32 bytes_read = file->f.ReadRaw<u8>(buf, static_cast<u32>(nbytes));
    file->m_mutex.unlock();
    return bytes_read;
}

int PS4_SYSV_ABI sceKernelFStat(int fd, OrbisKernelStat* sb) {
    LOG_INFO(Kernel_Fs, "(PARTIAL) fd = {}", fd);
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->GetFile(fd);
    memset(sb, 0, sizeof(OrbisKernelStat));

    if (file->is_directory) {
        sb->st_mode = 0000777u | 0040000u;
        sb->st_size = 0;
        sb->st_blksize = 512;
        sb->st_blocks = 0;
        // TODO incomplete
    } else {
        sb->st_mode = 0000777u | 0100000u;
        sb->st_size = file->f.GetSize();
        sb->st_blksize = 512;
        sb->st_blocks = (sb->st_size + 511) / 512;
        // TODO incomplete
    }
    return ORBIS_OK;
}

void fileSystemSymbolsRegister(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("1G3lF1Gg1k8", "libkernel", 1, "libkernel", 1, 1, sceKernelOpen);
    LIB_FUNCTION("wuCroIGjt2g", "libScePosix", 1, "libkernel", 1, 1, posix_open);
    LIB_FUNCTION("UK2Tl2DWUns", "libkernel", 1, "libkernel", 1, 1, sceKernelClose);
    LIB_FUNCTION("4wSze92BhLI", "libkernel", 1, "libkernel", 1, 1, sceKernelWrite);

    LIB_FUNCTION("+WRlkKjZvag", "libkernel", 1, "libkernel", 1, 1, _readv);
    LIB_FUNCTION("Oy6IpwgtYOk", "libkernel", 1, "libkernel", 1, 1, lseek);
    LIB_FUNCTION("oib76F-12fk", "libkernel", 1, "libkernel", 1, 1, sceKernelLseek);
    LIB_FUNCTION("Cg4srZ6TKbU", "libkernel", 1, "libkernel", 1, 1, sceKernelRead);
    LIB_FUNCTION("1-LFLmRFxxM", "libkernel", 1, "libkernel", 1, 1, sceKernelMkdir);
    LIB_FUNCTION("eV9wAD2riIA", "libkernel", 1, "libkernel", 1, 1, sceKernelStat);
    LIB_FUNCTION("kBwCPsYX-m4", "libkernel", 1, "libkernel", 1, 1, sceKernelFStat);

    LIB_FUNCTION("E6ao34wPw+U", "libScePosix", 1, "libkernel", 1, 1, posix_stat);
    LIB_FUNCTION("+r3rMFwItV4", "libkernel", 1, "libkernel", 1, 1, sceKernelPread);

    // openOrbis (to check if it is valid out of OpenOrbis
    LIB_FUNCTION("6c3rCVE-fTU", "libkernel", 1, "libkernel", 1, 1,
                 posix_open); // _open shoudld be equal to open function
}

} // namespace Libraries::Kernel
