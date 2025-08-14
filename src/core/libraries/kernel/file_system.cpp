// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <map>
#include <ranges>

#include "common/assert.h"
#include "common/error.h"
#include "common/logging/log.h"
#include "common/scope_exit.h"
#include "common/singleton.h"
#include "core/devices/console_device.h"
#include "core/devices/deci_tty6_device.h"
#include "core/devices/logger.h"
#include "core/devices/nop_device.h"
#include "core/devices/random_device.h"
#include "core/devices/srandom_device.h"
#include "core/devices/urandom_device.h"
#include "core/file_sys/fs.h"
#include "core/libraries/kernel/file_system.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/kernel/posix_error.h"
#include "core/libraries/libs.h"
#include "core/libraries/network/sockets.h"
#include "core/memory.h"
#include "kernel.h"

#ifdef _WIN32
#include <io.h>
#include <winsock2.h>
#else
#include <sys/select.h>
#endif

namespace D = Core::Devices;
using FactoryDevice = std::function<std::shared_ptr<D::BaseDevice>(u32, const char*, int, u16)>;

#define GET_DEVICE_FD(fd)                                                                          \
    [](u32, const char*, int, u16) {                                                               \
        return Common::Singleton<Core::FileSys::HandleTable>::Instance()->GetFile(fd)->device;     \
    }

// prefix path, only dev devices
static std::map<std::string, FactoryDevice> available_device = {
    // clang-format off
    {"/dev/stdin", GET_DEVICE_FD(0)},
    {"/dev/stdout", GET_DEVICE_FD(1)},
    {"/dev/stderr", GET_DEVICE_FD(2)},

    {"/dev/fd/0", GET_DEVICE_FD(0)},
    {"/dev/fd/1", GET_DEVICE_FD(1)},
    {"/dev/fd/2", GET_DEVICE_FD(2)},

    {"/dev/deci_stdin", GET_DEVICE_FD(0)},
    {"/dev/deci_stdout", GET_DEVICE_FD(1)},
    {"/dev/deci_stderr", GET_DEVICE_FD(2)},

    {"/dev/null", GET_DEVICE_FD(0)}, // fd0 (stdin) is a nop device

    {"/dev/urandom",  &D::URandomDevice::Create },
    {"/dev/random",   &D::RandomDevice::Create },
    {"/dev/srandom",  &D::SRandomDevice::Create },
    {"/dev/console",  &D::ConsoleDevice::Create },
    {"/dev/deci_tty6",&D::DeciTty6Device::Create }
    // clang-format on
};

namespace Libraries::Kernel {

s32 PS4_SYSV_ABI open(const char* raw_path, s32 flags, u16 mode) {
    LOG_INFO(Kernel_Fs, "path = {} flags = {:#x} mode = {}", raw_path, flags, mode);
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();

    bool read = (flags & 0x3) == ORBIS_KERNEL_O_RDONLY;
    bool write = (flags & 0x3) == ORBIS_KERNEL_O_WRONLY;
    bool rdwr = (flags & 0x3) == ORBIS_KERNEL_O_RDWR;

    if (!read && !write && !rdwr) {
        // Start by checking for invalid flags.
        *__Error() = POSIX_EINVAL;
        return -1;
    }

    bool nonblock = (flags & ORBIS_KERNEL_O_NONBLOCK) != 0;
    bool append = (flags & ORBIS_KERNEL_O_APPEND) != 0;
    // Flags fsync and sync behave the same
    bool sync = (flags & ORBIS_KERNEL_O_SYNC) != 0 || (flags & ORBIS_KERNEL_O_FSYNC) != 0;
    bool create = (flags & ORBIS_KERNEL_O_CREAT) != 0;
    bool truncate = (flags & ORBIS_KERNEL_O_TRUNC) != 0;
    bool excl = (flags & ORBIS_KERNEL_O_EXCL) != 0;
    bool dsync = (flags & ORBIS_KERNEL_O_DSYNC) != 0;
    bool direct = (flags & ORBIS_KERNEL_O_DIRECT) != 0;
    bool directory = (flags & ORBIS_KERNEL_O_DIRECTORY) != 0;

    if (sync || direct || dsync || nonblock) {
        LOG_WARNING(Kernel_Fs, "flags {:#x} not fully handled", flags);
    }

    std::string_view path{raw_path};
    u32 handle = h->CreateHandle();
    auto* file = h->GetFile(handle);

    if (path.starts_with("/dev/")) {
        for (const auto& [prefix, factory] : available_device) {
            if (path.starts_with(prefix)) {
                file->is_opened = true;
                file->type = Core::FileSys::FileType::Device;
                file->m_guest_name = path;
                file->device = factory(handle, path.data(), flags, mode);
                return handle;
            }
        }
    }

    bool read_only = false;
    file->m_guest_name = path;
    file->m_host_name = mnt->GetHostPath(file->m_guest_name, &read_only);
    bool exists = std::filesystem::exists(file->m_host_name);
    s32 e = 0;

    if (create) {
        if (excl && exists) {
            // Error if file exists
            h->DeleteHandle(handle);
            *__Error() = POSIX_EEXIST;
            return -1;
        }

        if (!exists) {
            if (read_only) {
                // Can't create files in a read only directory
                h->DeleteHandle(handle);
                *__Error() = POSIX_EROFS;
                return -1;
            }
            // Create a file if it doesn't exist
            Common::FS::IOFile out(file->m_host_name, Common::FS::FileAccessMode::Write);
        }
    } else if (!exists) {
        // If we're not creating a file, and it doesn't exist, return ENOENT
        h->DeleteHandle(handle);
        *__Error() = POSIX_ENOENT;
        return -1;
    }

    if (std::filesystem::is_directory(file->m_host_name) || directory) {
        // Directories can be opened even if the directory flag isn't set.
        // In these cases, error behavior is identical to the directory code path.
        directory = true;
    }

    if (directory) {
        if (!std::filesystem::is_directory(file->m_host_name)) {
            // If the opened file is not a directory, return ENOTDIR.
            // This will trigger when create & directory is specified, this is expected.
            h->DeleteHandle(handle);
            *__Error() = POSIX_ENOTDIR;
            return -1;
        }

        file->type = Core::FileSys::FileType::Directory;

        // Populate directory contents
        mnt->IterateDirectory(file->m_guest_name,
                              [&file](const auto& ent_path, const auto ent_is_file) {
                                  auto& dir_entry = file->dirents.emplace_back();
                                  dir_entry.name = ent_path.filename().string();
                                  dir_entry.isFile = ent_is_file;
                              });
        file->dirents_index = 0;

        if (read) {
            e = file->f.Open(file->m_host_name, Common::FS::FileAccessMode::Read);
        } else if (write || rdwr) {
            // Cannot open directories with any type of write access
            h->DeleteHandle(handle);
            *__Error() = POSIX_EISDIR;
            return -1;
        }

        if (e == EACCES) {
            // Hack to bypass some platform limitations, ignore the error and continue as normal.
            LOG_WARNING(Kernel_Fs, "Opening directories is not fully supported on this platform");
            e = 0;
        }

        if (truncate) {
            // Cannot open directories with truncate
            h->DeleteHandle(handle);
            *__Error() = POSIX_EISDIR;
            return -1;
        }
    } else {
        file->type = Core::FileSys::FileType::Regular;

        if (truncate && read_only) {
            // Can't open files with truncate flag in a read only directory
            h->DeleteHandle(handle);
            *__Error() = POSIX_EROFS;
            return -1;
        } else if (truncate) {
            // Open the file as read-write so we can truncate regardless of flags.
            // Since open starts by closing the file, this won't interfere with later open calls.
            e = file->f.Open(file->m_host_name, Common::FS::FileAccessMode::ReadWrite);
            if (e == 0) {
                // If the file was opened successfully, reduce size to 0
                file->f.SetSize(0);
            }
        }

        if (read) {
            // Read only
            e = file->f.Open(file->m_host_name, Common::FS::FileAccessMode::Read);
        } else if (read_only) {
            // Can't open files with write/read-write access in a read only directory
            h->DeleteHandle(handle);
            *__Error() = POSIX_EROFS;
            return -1;
        } else if (append) {
            // Append can be specified with rdwr or write, but we treat it as a separate mode.
            e = file->f.Open(file->m_host_name, Common::FS::FileAccessMode::Append);
        } else if (write) {
            // Write only
            e = file->f.Open(file->m_host_name, Common::FS::FileAccessMode::Write);
        } else if (rdwr) {
            // Read and write
            e = file->f.Open(file->m_host_name, Common::FS::FileAccessMode::ReadWrite);
        }
    }

    if (e != 0) {
        // Open failed in platform-specific code, errno needs to be converted.
        h->DeleteHandle(handle);
        SetPosixErrno(e);
        return -1;
    }

    file->is_opened = true;
    return handle;
}

s32 PS4_SYSV_ABI posix_open(const char* filename, s32 flags, u16 mode) {
    return open(filename, flags, mode);
}

s32 PS4_SYSV_ABI sceKernelOpen(const char* path, s32 flags, /* SceKernelMode*/ u16 mode) {
    s32 result = open(path, flags, mode);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s32 PS4_SYSV_ABI close(s32 fd) {
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->GetFile(fd);
    if (file == nullptr) {
        *__Error() = POSIX_EBADF;
        return -1;
    }
    if (fd < 3) {
        *__Error() = POSIX_EPERM;
        return -1;
    }
    if (file->type == Core::FileSys::FileType::Regular) {
        file->f.Close();
    } else if (file->type == Core::FileSys::FileType::Socket) {
        file->socket->Close();
    }
    file->is_opened = false;
    LOG_INFO(Kernel_Fs, "Closing {}", file->m_guest_name);
    // FIXME: Lock file mutex before deleting it?
    h->DeleteHandle(fd);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_close(s32 fd) {
    return close(fd);
}

s32 PS4_SYSV_ABI sceKernelClose(s32 fd) {
    s32 result = close(fd);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s64 PS4_SYSV_ABI write(s32 fd, const void* buf, size_t nbytes) {
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->GetFile(fd);
    if (file == nullptr) {
        *__Error() = POSIX_EBADF;
        return -1;
    }

    std::scoped_lock lk{file->m_mutex};
    if (file->type == Core::FileSys::FileType::Device) {
        s64 result = file->device->write(buf, nbytes);
        if (result < 0) {
            ErrSceToPosix(result);
            return -1;
        }
        return result;
    } else if (file->type == Core::FileSys::FileType::Socket) {
        // Socket functions handle errnos internally.
        return file->socket->SendPacket(buf, nbytes, 0, nullptr, 0);
    }

    return file->f.WriteRaw<u8>(buf, nbytes);
}

s64 PS4_SYSV_ABI posix_write(s32 fd, const void* buf, size_t nbytes) {
    return write(fd, buf, nbytes);
}

s64 PS4_SYSV_ABI sceKernelWrite(s32 fd, const void* buf, size_t nbytes) {
    s64 result = write(fd, buf, nbytes);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

size_t ReadFile(Common::FS::IOFile& file, void* buf, size_t nbytes) {
    const auto* memory = Core::Memory::Instance();
    // Invalidate up to the actual number of bytes that could be read.
    const auto remaining = file.GetSize() - file.Tell();
    memory->InvalidateMemory(reinterpret_cast<VAddr>(buf), std::min<u64>(nbytes, remaining));

    return file.ReadRaw<u8>(buf, nbytes);
}

size_t PS4_SYSV_ABI readv(s32 fd, const SceKernelIovec* iov, s32 iovcnt) {
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->GetFile(fd);
    if (file == nullptr) {
        *__Error() = POSIX_EBADF;
        return -1;
    }

    std::scoped_lock lk{file->m_mutex};
    if (file->type == Core::FileSys::FileType::Device) {
        size_t result = file->device->readv(iov, iovcnt);
        if (result < 0) {
            ErrSceToPosix(result);
            return -1;
        }
        return result;
    }
    size_t total_read = 0;
    for (s32 i = 0; i < iovcnt; i++) {
        total_read += ReadFile(file->f, iov[i].iov_base, iov[i].iov_len);
    }
    return total_read;
}

size_t PS4_SYSV_ABI posix_readv(s32 fd, const SceKernelIovec* iov, s32 iovcnt) {
    return readv(fd, iov, iovcnt);
}

size_t PS4_SYSV_ABI sceKernelReadv(s32 fd, const SceKernelIovec* iov, s32 iovcnt) {
    size_t result = readv(fd, iov, iovcnt);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

size_t PS4_SYSV_ABI writev(s32 fd, const SceKernelIovec* iov, s32 iovcnt) {
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->GetFile(fd);
    if (file == nullptr) {
        *__Error() = POSIX_EBADF;
        return -1;
    }

    std::scoped_lock lk{file->m_mutex};

    if (file->type == Core::FileSys::FileType::Device) {
        size_t result = file->device->writev(iov, iovcnt);
        if (result < 0) {
            ErrSceToPosix(result);
            return -1;
        }
        return result;
    }
    size_t total_written = 0;
    for (s32 i = 0; i < iovcnt; i++) {
        total_written += file->f.WriteRaw<u8>(iov[i].iov_base, iov[i].iov_len);
    }
    return total_written;
}

size_t PS4_SYSV_ABI posix_writev(s32 fd, const SceKernelIovec* iov, s32 iovcnt) {
    return writev(fd, iov, iovcnt);
}

size_t PS4_SYSV_ABI sceKernelWritev(s32 fd, const SceKernelIovec* iov, s32 iovcnt) {
    size_t result = writev(fd, iov, iovcnt);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s64 PS4_SYSV_ABI posix_lseek(s32 fd, s64 offset, s32 whence) {
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->GetFile(fd);
    if (file == nullptr) {
        *__Error() = POSIX_EBADF;
        return -1;
    }

    std::scoped_lock lk{file->m_mutex};
    if (file->type == Core::FileSys::FileType::Device) {
        s64 result = file->device->lseek(offset, whence);
        if (result < 0) {
            ErrSceToPosix(result);
            return -1;
        }
        return result;
    }

    Common::FS::SeekOrigin origin{};
    if (whence == 0) {
        origin = Common::FS::SeekOrigin::SetOrigin;
    } else if (whence == 1) {
        origin = Common::FS::SeekOrigin::CurrentPosition;
    } else if (whence == 2) {
        origin = Common::FS::SeekOrigin::End;
    } else if (whence == 3 || whence == 4) {
        // whence parameter belongs to an unsupported POSIX extension
        *__Error() = POSIX_ENOTTY;
        return -1;
    } else {
        // whence parameter is invalid
        *__Error() = POSIX_EINVAL;
        return -1;
    }

    if (!file->f.Seek(offset, origin)) {
        if (errno != 0) {
            // Seek failed in platform-specific code, errno needs to be converted.
            SetPosixErrno(errno);
            return -1;
        }
        // Shouldn't be possible, but just in case.
        return -1;
    }

    s64 result = file->f.Tell();
    if (result < 0) {
        // Tell failed in platform-specific code, errno needs to be converted.
        SetPosixErrno(errno);
        return -1;
    }
    return result;
}

s64 PS4_SYSV_ABI sceKernelLseek(s32 fd, s64 offset, s32 whence) {
    s64 result = posix_lseek(fd, offset, whence);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s64 PS4_SYSV_ABI read(s32 fd, void* buf, size_t nbytes) {
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->GetFile(fd);
    if (file == nullptr) {
        *__Error() = POSIX_EBADF;
        return -1;
    }

    std::scoped_lock lk{file->m_mutex};
    if (file->type == Core::FileSys::FileType::Device) {
        s64 result = file->device->read(buf, nbytes);
        if (result < 0) {
            ErrSceToPosix(result);
            return -1;
        }
        return result;
    } else if (file->type == Core::FileSys::FileType::Socket) {
        // Socket functions handle errnos internally.
        return file->socket->ReceivePacket(buf, nbytes, 0, nullptr, 0);
    }
    return ReadFile(file->f, buf, nbytes);
}

s64 PS4_SYSV_ABI posix_read(s32 fd, void* buf, size_t nbytes) {
    return read(fd, buf, nbytes);
}

s64 PS4_SYSV_ABI sceKernelRead(s32 fd, void* buf, size_t nbytes) {
    s64 result = read(fd, buf, nbytes);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s32 PS4_SYSV_ABI posix_mkdir(const char* path, u16 mode) {
    LOG_INFO(Kernel_Fs, "path = {} mode = {}", path, mode);
    if (path == nullptr) {
        *__Error() = POSIX_ENOTDIR;
        return -1;
    }
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();

    bool ro = false;
    const auto dir_name = mnt->GetHostPath(path, &ro);

    if (std::filesystem::exists(dir_name)) {
        *__Error() = POSIX_EEXIST;
        return -1;
    }

    if (ro) {
        *__Error() = POSIX_EROFS;
        return -1;
    }

    // CUSA02456: path = /aotl after sceSaveDataMount(mode = 1)
    std::error_code ec;
    if (dir_name.empty() || !std::filesystem::create_directory(dir_name, ec)) {
        *__Error() = POSIX_EIO;
        return -1;
    }

    if (!std::filesystem::exists(dir_name)) {
        *__Error() = POSIX_ENOENT;
        return -1;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelMkdir(const char* path, u16 mode) {
    s32 result = posix_mkdir(path, mode);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s32 PS4_SYSV_ABI posix_rmdir(const char* path) {
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    bool ro = false;

    const std::filesystem::path dir_name = mnt->GetHostPath(path, &ro);

    if (ro) {
        *__Error() = POSIX_EROFS;
        return -1;
    }

    if (dir_name.empty() || !std::filesystem::is_directory(dir_name)) {
        *__Error() = POSIX_ENOTDIR;
        return -1;
    }

    if (!std::filesystem::exists(dir_name)) {
        *__Error() = POSIX_ENOENT;
        return -1;
    }

    std::error_code ec;
    s32 result = std::filesystem::remove_all(dir_name, ec);

    if (ec) {
        *__Error() = POSIX_EIO;
        return -1;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelRmdir(const char* path) {
    s32 result = posix_rmdir(path);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s32 PS4_SYSV_ABI posix_stat(const char* path, OrbisKernelStat* sb) {
    LOG_INFO(Kernel_Fs, "(PARTIAL) path = {}", path);
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    const auto path_name = mnt->GetHostPath(path);
    std::memset(sb, 0, sizeof(OrbisKernelStat));
    const bool is_dir = std::filesystem::is_directory(path_name);
    const bool is_file = std::filesystem::is_regular_file(path_name);
    if (!is_dir && !is_file) {
        *__Error() = POSIX_ENOENT;
        return -1;
    }
    if (std::filesystem::is_directory(path_name)) {
        sb->st_mode = 0000777u | 0040000u;
        sb->st_size = 65536;
        sb->st_blksize = 65536;
        sb->st_blocks = 128;
        // TODO incomplete
    } else {
        sb->st_mode = 0000777u | 0100000u;
        sb->st_size = static_cast<s64>(std::filesystem::file_size(path_name));
        sb->st_blksize = 512;
        sb->st_blocks = (sb->st_size + 511) / 512;
        // TODO incomplete
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelStat(const char* path, OrbisKernelStat* sb) {
    s32 result = posix_stat(path, sb);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s32 PS4_SYSV_ABI sceKernelCheckReachability(const char* path) {
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    std::string_view guest_path{path};
    for (const auto& prefix : available_device | std::views::keys) {
        if (guest_path.starts_with(prefix)) {
            return ORBIS_OK;
        }
    }
    const auto path_name = mnt->GetHostPath(guest_path);
    if (!std::filesystem::exists(path_name)) {
        return ORBIS_KERNEL_ERROR_ENOENT;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fstat(s32 fd, OrbisKernelStat* sb) {
    LOG_INFO(Kernel_Fs, "(PARTIAL) fd = {}", fd);
    if (sb == nullptr) {
        *__Error() = POSIX_EFAULT;
        return -1;
    }
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->GetFile(fd);
    if (file == nullptr) {
        *__Error() = POSIX_EBADF;
        return -1;
    }
    std::memset(sb, 0, sizeof(OrbisKernelStat));

    switch (file->type) {
    case Core::FileSys::FileType::Device: {
        s32 result = file->device->fstat(sb);
        if (result < 0) {
            ErrSceToPosix(result);
            return -1;
        }
        return result;
    }
    case Core::FileSys::FileType::Regular: {
        sb->st_mode = 0000777u | 0100000u;
        sb->st_size = file->f.GetSize();
        sb->st_blksize = 512;
        sb->st_blocks = (sb->st_size + 511) / 512;
        // TODO incomplete
        break;
    }
    case Core::FileSys::FileType::Directory: {
        sb->st_mode = 0000777u | 0040000u;
        sb->st_size = 65536;
        sb->st_blksize = 65536;
        sb->st_blocks = 128;
        // TODO incomplete
        break;
    }
    case Core::FileSys::FileType::Socket: {
        // Socket functions handle errnos internally
        return file->socket->fstat(sb);
    }
    default:
        UNREACHABLE();
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_fstat(s32 fd, OrbisKernelStat* sb) {
    return fstat(fd, sb);
}

s32 PS4_SYSV_ABI sceKernelFstat(s32 fd, OrbisKernelStat* sb) {
    s32 result = fstat(fd, sb);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s32 PS4_SYSV_ABI posix_ftruncate(s32 fd, s64 length) {
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->GetFile(fd);

    if (file == nullptr) {
        *__Error() = POSIX_EBADF;
        return -1;
    }

    if (file->type == Core::FileSys::FileType::Device) {
        s32 result = file->device->ftruncate(length);
        if (result < 0) {
            ErrSceToPosix(result);
            return -1;
        }
        return result;
    }

    if (file->m_host_name.empty()) {
        *__Error() = POSIX_EACCES;
        return -1;
    }

    file->f.SetSize(length);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelFtruncate(s32 fd, s64 length) {
    s32 result = posix_ftruncate(fd, length);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s32 PS4_SYSV_ABI posix_rename(const char* from, const char* to) {
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    bool ro = false;
    const auto src_path = mnt->GetHostPath(from, &ro);
    if (!std::filesystem::exists(src_path)) {
        *__Error() = POSIX_ENOENT;
        return -1;
    }
    if (ro) {
        *__Error() = POSIX_EROFS;
        return -1;
    }
    const auto dst_path = mnt->GetHostPath(to, &ro);
    if (ro) {
        *__Error() = POSIX_EROFS;
        return -1;
    }
    const bool src_is_dir = std::filesystem::is_directory(src_path);
    const bool dst_is_dir = std::filesystem::is_directory(dst_path);
    if (src_is_dir && !dst_is_dir) {
        *__Error() = POSIX_ENOTDIR;
        return -1;
    }
    if (!src_is_dir && dst_is_dir) {
        *__Error() = POSIX_EISDIR;
        return -1;
    }
    if (dst_is_dir && !std::filesystem::is_empty(dst_path)) {
        *__Error() = POSIX_ENOTEMPTY;
        return -1;
    }

    // On Windows, std::filesystem::rename will error if the file has been opened before.
    std::filesystem::copy(src_path, dst_path, std::filesystem::copy_options::overwrite_existing);
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto file = h->GetFile(src_path);
    if (file) {
        // We need to force ReadWrite if the file had Write access before
        // Otherwise f.Open will clear the file contents.
        auto access_mode = file->f.GetAccessMode() == Common::FS::FileAccessMode::Write
                               ? Common::FS::FileAccessMode::ReadWrite
                               : file->f.GetAccessMode();
        file->f.Close();
        std::filesystem::remove(src_path);
        file->f.Open(dst_path, access_mode);
    } else {
        std::filesystem::remove(src_path);
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelRename(const char* from, const char* to) {
    s32 result = posix_rename(from, to);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s64 PS4_SYSV_ABI posix_preadv(s32 fd, SceKernelIovec* iov, s32 iovcnt, s64 offset) {
    if (offset < 0) {
        *__Error() = POSIX_EINVAL;
        return -1;
    }

    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->GetFile(fd);
    if (file == nullptr) {
        *__Error() = POSIX_EBADF;
        return -1;
    }

    std::scoped_lock lk{file->m_mutex};
    if (file->type == Core::FileSys::FileType::Device) {
        s64 result = file->device->preadv(iov, iovcnt, offset);
        if (result < 0) {
            ErrSceToPosix(result);
            return -1;
        }
        return result;
    }

    const s64 pos = file->f.Tell();
    SCOPE_EXIT {
        file->f.Seek(pos);
    };
    if (!file->f.Seek(offset)) {
        *__Error() = POSIX_EIO;
        return -1;
    }
    size_t total_read = 0;
    for (int i = 0; i < iovcnt; i++) {
        total_read += ReadFile(file->f, iov[i].iov_base, iov[i].iov_len);
    }
    return total_read;
}

s64 PS4_SYSV_ABI sceKernelPreadv(s32 fd, SceKernelIovec* iov, s32 iovcnt, s64 offset) {
    s64 result = posix_preadv(fd, iov, iovcnt, offset);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s64 PS4_SYSV_ABI posix_pread(s32 fd, void* buf, size_t nbytes, s64 offset) {
    SceKernelIovec iovec{buf, nbytes};
    return posix_preadv(fd, &iovec, 1, offset);
}

s64 PS4_SYSV_ABI sceKernelPread(s32 fd, void* buf, size_t nbytes, s64 offset) {
    SceKernelIovec iovec{buf, nbytes};
    return sceKernelPreadv(fd, &iovec, 1, offset);
}

s32 PS4_SYSV_ABI posix_fsync(s32 fd) {
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->GetFile(fd);
    if (file == nullptr) {
        *__Error() = POSIX_EBADF;
        return -1;
    }

    if (file->type == Core::FileSys::FileType::Device) {
        s32 result = file->device->fsync();
        if (result < 0) {
            ErrSceToPosix(result);
            return -1;
        }
        return result;
    }
    file->f.Flush();
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelFsync(s32 fd) {
    s32 result = posix_fsync(fd);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

static s64 GetDents(s32 fd, char* buf, u64 nbytes, s64* basep) {
    if (buf == nullptr) {
        *__Error() = POSIX_EFAULT;
        return -1;
    }
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->GetFile(fd);
    if (file == nullptr) {
        *__Error() = POSIX_EBADF;
        return -1;
    }

    if (file->type == Core::FileSys::FileType::Device) {
        s32 result = file->device->getdents(buf, nbytes, basep);
        if (result < 0) {
            ErrSceToPosix(result);
            return -1;
        }
        return result;
    }

    if (file->type != Core::FileSys::FileType::Directory || nbytes < 512 ||
        file->dirents_index > file->dirents.size()) {
        *__Error() = POSIX_EINVAL;
        return -1;
    }

    s64 bytes_to_write = nbytes;
    char* buf_to_write = buf;
    u64 bytes_written = 0;
    while (bytes_to_write >= sizeof(OrbisKernelDirent)) {
        if (file->dirents_index == file->dirents.size()) {
            break;
        }
        const auto& entry = file->dirents.at(file->dirents_index++);
        auto str = entry.name;
        static int fileno = 1000; // random
        OrbisKernelDirent* sce_ent = (OrbisKernelDirent*)buf_to_write;
        // TODO this should be unique, maybe switch to a hash or something?
        sce_ent->d_fileno = fileno++;
        sce_ent->d_reclen = sizeof(OrbisKernelDirent);
        sce_ent->d_type = (entry.isFile ? 8 : 4);
        sce_ent->d_namlen = str.size();
        strncpy(sce_ent->d_name, str.c_str(), ORBIS_MAX_PATH);
        sce_ent->d_name[ORBIS_MAX_PATH] = '\0';

        buf_to_write += sizeof(OrbisKernelDirent);
        bytes_to_write -= sizeof(OrbisKernelDirent);
        bytes_written += sizeof(OrbisKernelDirent);
    }

    if (basep != nullptr) {
        *basep = file->dirents_index;
    }

    return bytes_written;
}

s64 PS4_SYSV_ABI posix_getdents(s32 fd, char* buf, u64 nbytes) {
    return GetDents(fd, buf, nbytes, nullptr);
}

s64 PS4_SYSV_ABI sceKernelGetdents(s32 fd, char* buf, u64 nbytes) {
    s64 result = posix_getdents(fd, buf, nbytes);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s64 PS4_SYSV_ABI getdirentries(s32 fd, char* buf, u64 nbytes, s64* basep) {
    return GetDents(fd, buf, nbytes, basep);
}

s64 PS4_SYSV_ABI posix_getdirentries(s32 fd, char* buf, u64 nbytes, s64* basep) {
    return GetDents(fd, buf, nbytes, basep);
}

s64 PS4_SYSV_ABI sceKernelGetdirentries(s32 fd, char* buf, u64 nbytes, s64* basep) {
    s64 result = GetDents(fd, buf, nbytes, basep);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s64 PS4_SYSV_ABI posix_pwritev(s32 fd, const SceKernelIovec* iov, s32 iovcnt, s64 offset) {
    if (offset < 0) {
        *__Error() = POSIX_EINVAL;
        return -1;
    }

    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->GetFile(fd);
    if (file == nullptr) {
        *__Error() = POSIX_EBADF;
        return -1;
    }

    std::scoped_lock lk{file->m_mutex};

    if (file->type == Core::FileSys::FileType::Device) {
        s64 result = file->device->pwritev(iov, iovcnt, offset);
        if (result < 0) {
            ErrSceToPosix(result);
            return -1;
        }
        return result;
    }
    const s64 pos = file->f.Tell();
    SCOPE_EXIT {
        file->f.Seek(pos);
    };
    if (!file->f.Seek(offset)) {
        *__Error() = POSIX_EIO;
        return -1;
    }
    size_t total_written = 0;
    for (int i = 0; i < iovcnt; i++) {
        total_written += file->f.WriteRaw<u8>(iov[i].iov_base, iov[i].iov_len);
    }
    return total_written;
}

s64 PS4_SYSV_ABI posix_pwrite(s32 fd, void* buf, size_t nbytes, s64 offset) {
    SceKernelIovec iovec{buf, nbytes};
    return posix_pwritev(fd, &iovec, 1, offset);
}

s64 PS4_SYSV_ABI sceKernelPwrite(s32 fd, void* buf, size_t nbytes, s64 offset) {
    s64 result = posix_pwrite(fd, buf, nbytes, offset);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s64 PS4_SYSV_ABI sceKernelPwritev(s32 fd, const SceKernelIovec* iov, s32 iovcnt, s64 offset) {
    s64 result = posix_pwritev(fd, iov, iovcnt, offset);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s32 PS4_SYSV_ABI posix_unlink(const char* path) {
    if (path == nullptr) {
        *__Error() = POSIX_EINVAL;
        return -1;
    }

    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();

    bool ro = false;
    const auto host_path = mnt->GetHostPath(path, &ro);
    if (host_path.empty()) {
        *__Error() = POSIX_ENOENT;
        return -1;
    }

    if (ro) {
        *__Error() = POSIX_EROFS;
        return -1;
    }

    if (std::filesystem::is_directory(host_path)) {
        *__Error() = POSIX_EPERM;
        return -1;
    }

    auto* file = h->GetFile(host_path);
    if (file == nullptr) {
        // File to unlink hasn't been opened, manually open and unlink it.
        Common::FS::IOFile file(host_path, Common::FS::FileAccessMode::ReadWrite);
        file.Unlink();
    } else {
        file->f.Unlink();
    }

    LOG_INFO(Kernel_Fs, "Unlinked {}", path);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelUnlink(const char* path) {
    s32 result = posix_unlink(path);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

#ifdef _WIN32
#define __FD_SETSIZE 1024

typedef struct {
    unsigned long fds_bits[__FD_SETSIZE / (8 * sizeof(unsigned long))];
} fd_set_posix;

#define FD_SET_POSIX(fd, set)                                                                      \
    ((set)->fds_bits[(fd) / (8 * sizeof(unsigned long))] |=                                        \
     (1UL << ((fd) % (8 * sizeof(unsigned long)))))

#define FD_CLR_POSIX(fd, set)                                                                      \
    ((set)->fds_bits[(fd) / (8 * sizeof(unsigned long))] &=                                        \
     ~(1UL << ((fd) % (8 * sizeof(unsigned long)))))

#define FD_ISSET_POSIX(fd, set)                                                                    \
    (((set)->fds_bits[(fd) / (8 * sizeof(unsigned long))] &                                        \
      (1UL << ((fd) % (8 * sizeof(unsigned long))))) != 0)

#define FD_ZERO_POSIX(set) memset((set), 0, sizeof(fd_set_posix))

s32 PS4_SYSV_ABI posix_select(s32 nfds, fd_set_posix* readfds, fd_set_posix* writefds,
                              fd_set_posix* exceptfds, OrbisKernelTimeval* timeout) {
    LOG_DEBUG(Kernel_Fs, "nfds = {}, readfds = {}, writefds = {}, exceptfds = {}, timeout = {}",
              nfds, fmt::ptr(readfds), fmt::ptr(writefds), fmt::ptr(exceptfds), fmt::ptr(timeout));

    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();

    fd_set read_host = {}, write_host = {}, except_host = {};
    FD_ZERO(&read_host);
    FD_ZERO(&write_host);
    FD_ZERO(&except_host);

    fd_set_posix read_ready, write_ready, except_ready;
    FD_ZERO_POSIX(&read_ready);
    FD_ZERO_POSIX(&write_ready);
    FD_ZERO_POSIX(&except_ready);

    std::map<s32, s32> host_to_guest;
    s32 socket_max_fd = -1;

    for (s32 i = 0; i < nfds; ++i) {
        bool want_read = readfds && FD_ISSET_POSIX(i, readfds);
        bool want_write = writefds && FD_ISSET_POSIX(i, writefds);
        bool want_except = exceptfds && FD_ISSET_POSIX(i, exceptfds);
        if (!(want_read || want_write || want_except)) {
            continue;
        }

        auto* file = h->GetFile(i);
        if (!file || ((file->type == Core::FileSys::FileType::Regular && !file->f.IsOpen()) ||
                      (file->type == Core::FileSys::FileType::Socket && !file->is_opened))) {
            LOG_ERROR(Kernel_Fs, "fd {} is null or not opened", i);
            *__Error() = POSIX_EBADF;
            return -1;
        }

        s32 native_fd = -1;
        switch (file->type) {
        case Core::FileSys::FileType::Regular:
            native_fd = static_cast<s32>(file->f.GetFileMapping());
            break;
        case Core::FileSys::FileType::Socket: {
            auto sock = file->socket->Native();
            native_fd = sock ? static_cast<s32>(*sock) : -1;
            break;
        }
        case Core::FileSys::FileType::Device:
            native_fd = -1;
            break;
        default:
            UNREACHABLE();
            break;
        }

        if (file->type == Core::FileSys::FileType::Regular ||
            file->type == Core::FileSys::FileType::Device) {
            // Disk files always ready
            if (want_read) {
                FD_SET_POSIX(i, &read_ready);
            }
            if (want_write) {
                FD_SET_POSIX(i, &write_ready);
            }
            // exceptfds not supported on regular files
        } else if (file->type == Core::FileSys::FileType::Socket) {
            if (want_read) {
                FD_SET(native_fd, &read_host);
            }
            if (want_write) {
                FD_SET(native_fd, &write_host);
            }
            if (want_except) {
                FD_SET(native_fd, &except_host);
            }
            socket_max_fd = std::max(socket_max_fd, native_fd);
        }

        if (native_fd == -1) {
            continue;
        }

        host_to_guest[native_fd] = i;
    }

    LOG_DEBUG(Kernel_Fs,
              "Before select(): read_host.fd_count = {}, write_host.fd_count = {}, "
              "except_host.fd_count = {}",
              read_host.fd_count, write_host.fd_count, except_host.fd_count);

    if (read_host.fd_count == 0 && write_host.fd_count == 0 && except_host.fd_count == 0) {
        LOG_WARNING(Kernel_Fs, "No sockets in fd_sets, select() will return immediately");
    }

    if (readfds) {
        FD_ZERO_POSIX(readfds);
    }
    if (writefds) {
        FD_ZERO_POSIX(writefds);
    }
    if (exceptfds) {
        FD_ZERO_POSIX(exceptfds);
    }

    s32 result = 0;
    if (socket_max_fd != -1) {
        timeval tv = {};
        timeval* tv_ptr = nullptr;
        if (timeout) {
            tv.tv_sec = timeout->tv_sec;
            tv.tv_usec = timeout->tv_usec;
            tv_ptr = &tv;
        }
        result = select(0, read_host.fd_count > 0 ? &read_host : nullptr,
                        write_host.fd_count > 0 ? &write_host : nullptr,
                        except_host.fd_count > 0 ? &except_host : nullptr, tv_ptr);
        if (result == SOCKET_ERROR) {
            s32 err = WSAGetLastError();
            LOG_ERROR(Kernel_Fs, "select() failed with error {}", err);
            switch (err) {
            case WSAEFAULT:
                *__Error() = POSIX_EFAULT;
                break;
            case WSAEINVAL:
                *__Error() = POSIX_EINVAL;
                break;
            case WSAENOBUFS:
                *__Error() = POSIX_ENOBUFS;
                break;
            default:
                LOG_ERROR(Kernel_Fs, "Unhandled error case {}", err);
                break;
            }
            return -1;
        }

        for (s32 i = 0; i < read_host.fd_count; ++i) {
            s32 fd = static_cast<s32>(read_host.fd_array[i]);
            FD_SET_POSIX(host_to_guest[fd], readfds);
        }
        for (s32 i = 0; i < write_host.fd_count; ++i) {
            s32 fd = static_cast<s32>(write_host.fd_array[i]);
            FD_SET_POSIX(host_to_guest[fd], writefds);
        }
        for (s32 i = 0; i < except_host.fd_count; ++i) {
            s32 fd = static_cast<s32>(except_host.fd_array[i]);
            FD_SET_POSIX(host_to_guest[fd], exceptfds);
        }
    }

    // Add regular/device files ready count
    s32 disk_ready = 0;
    for (s32 i = 0; i < nfds; ++i) {
        if (FD_ISSET_POSIX(i, &read_ready)) {
            FD_SET_POSIX(i, readfds);
            disk_ready++;
        }
        if (FD_ISSET_POSIX(i, &write_ready)) {
            FD_SET_POSIX(i, writefds);
            disk_ready++;
        }
    }

    return result + disk_ready;
}
#else
s32 PS4_SYSV_ABI posix_select(s32 nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds,
                              OrbisKernelTimeval* timeout) {
    LOG_DEBUG(Kernel_Fs, "nfds = {}, readfds = {}, writefds = {}, exceptfds = {}, timeout = {}",
              nfds, fmt::ptr(readfds), fmt::ptr(writefds), fmt::ptr(exceptfds), fmt::ptr(timeout));

    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    fd_set read_host, write_host, except_host;
    FD_ZERO(&read_host);
    FD_ZERO(&write_host);
    FD_ZERO(&except_host);

    std::map<s32, s32> host_to_guest;
    s32 max_fd = -1;

    for (s32 i = 0; i < nfds; ++i) {
        auto read = readfds && FD_ISSET(i, readfds);
        auto write = writefds && FD_ISSET(i, writefds);
        auto except = exceptfds && FD_ISSET(i, exceptfds);
        if (read || write || except) {
            auto* file = h->GetFile(i);
            if (file == nullptr ||
                ((file->type == Core::FileSys::FileType::Regular && !file->f.IsOpen()) ||
                 (file->type == Core::FileSys::FileType::Socket && !file->is_opened))) {
                LOG_ERROR(Kernel_Fs, "fd {} is null or not opened", i);
                *__Error() = POSIX_EBADF;
                return -1;
            }

            s32 native_fd = [&] {
                switch (file->type) {
                case Core::FileSys::FileType::Regular:
                    return static_cast<s32>(file->f.GetFileMapping());
                case Core::FileSys::FileType::Device:
                    return -1;
                case Core::FileSys::FileType::Socket: {
                    auto sock = file->socket->Native();
                    // until P2P sockets contain a proper socket
                    return sock ? static_cast<s32>(*sock) : -1;
                }
                default:
                    UNREACHABLE();
                }
            }();
            if (native_fd == -1) {
                continue;
            }
            host_to_guest.emplace(native_fd, i);

            max_fd = std::max(max_fd, native_fd);

            if (read) {
                FD_SET(native_fd, &read_host);
            }
            if (write) {
                FD_SET(native_fd, &write_host);
            }
            if (except) {
                FD_SET(native_fd, &except_host);
            }
        }
    }

    if (max_fd == -1) {
        LOG_WARNING(Kernel_Fs, "all requested file descriptors are unsupported");
        return 0;
    }

    s32 ret = select(max_fd + 1, &read_host, &write_host, &except_host, (timeval*)timeout);

    if (ret > 0) {
        if (readfds) {
            FD_ZERO(readfds);
        }
        if (writefds) {
            FD_ZERO(writefds);
        }
        if (exceptfds) {
            FD_ZERO(exceptfds);
        }

        for (s32 i = 0; i < max_fd + 1; ++i) {
            if (readfds && FD_ISSET(i, &read_host)) {
                FD_SET(host_to_guest[i], readfds);
            }
            if (writefds && FD_ISSET(i, &write_host)) {
                FD_SET(host_to_guest[i], writefds);
            }
            if (exceptfds && FD_ISSET(i, &except_host)) {
                FD_SET(host_to_guest[i], exceptfds);
            }
        }
    }
    if (ret < 0) {
        s32 error = errno;
        LOG_ERROR(Kernel_Fs, "native select call failed with {} ({})", error,
                  Common::NativeErrorToString(error));
        SetPosixErrno(error);
    }

    return ret;
}
#endif

void RegisterFileSystem(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("6c3rCVE-fTU", "libkernel", 1, "libkernel", 1, 1, open);
    LIB_FUNCTION("wuCroIGjt2g", "libScePosix", 1, "libkernel", 1, 1, posix_open);
    LIB_FUNCTION("wuCroIGjt2g", "libkernel", 1, "libkernel", 1, 1, posix_open);
    LIB_FUNCTION("1G3lF1Gg1k8", "libkernel", 1, "libkernel", 1, 1, sceKernelOpen);
    LIB_FUNCTION("NNtFaKJbPt0", "libkernel", 1, "libkernel", 1, 1, close);
    LIB_FUNCTION("bY-PO6JhzhQ", "libScePosix", 1, "libkernel", 1, 1, posix_close);
    LIB_FUNCTION("bY-PO6JhzhQ", "libkernel", 1, "libkernel", 1, 1, posix_close);
    LIB_FUNCTION("UK2Tl2DWUns", "libkernel", 1, "libkernel", 1, 1, sceKernelClose);
    LIB_FUNCTION("FxVZqBAA7ks", "libkernel", 1, "libkernel", 1, 1, write);
    LIB_FUNCTION("FN4gaPmuFV8", "libScePosix", 1, "libkernel", 1, 1, posix_write);
    LIB_FUNCTION("FN4gaPmuFV8", "libkernel", 1, "libkernel", 1, 1, posix_write);
    LIB_FUNCTION("4wSze92BhLI", "libkernel", 1, "libkernel", 1, 1, sceKernelWrite);
    LIB_FUNCTION("+WRlkKjZvag", "libkernel", 1, "libkernel", 1, 1, readv);
    LIB_FUNCTION("YSHRBRLn2pI", "libkernel", 1, "libkernel", 1, 1, writev);
    LIB_FUNCTION("kAt6VDbHmro", "libkernel", 1, "libkernel", 1, 1, sceKernelWritev);
    LIB_FUNCTION("Oy6IpwgtYOk", "libScePosix", 1, "libkernel", 1, 1, posix_lseek);
    LIB_FUNCTION("Oy6IpwgtYOk", "libkernel", 1, "libkernel", 1, 1, posix_lseek);
    LIB_FUNCTION("oib76F-12fk", "libkernel", 1, "libkernel", 1, 1, sceKernelLseek);
    LIB_FUNCTION("DRuBt2pvICk", "libkernel", 1, "libkernel", 1, 1, read);
    LIB_FUNCTION("AqBioC2vF3I", "libScePosix", 1, "libkernel", 1, 1, posix_read);
    LIB_FUNCTION("AqBioC2vF3I", "libkernel", 1, "libkernel", 1, 1, posix_read);
    LIB_FUNCTION("Cg4srZ6TKbU", "libkernel", 1, "libkernel", 1, 1, sceKernelRead);
    LIB_FUNCTION("JGMio+21L4c", "libScePosix", 1, "libkernel", 1, 1, posix_mkdir);
    LIB_FUNCTION("JGMio+21L4c", "libkernel", 1, "libkernel", 1, 1, posix_mkdir);
    LIB_FUNCTION("1-LFLmRFxxM", "libkernel", 1, "libkernel", 1, 1, sceKernelMkdir);
    LIB_FUNCTION("c7ZnT7V1B98", "libScePosix", 1, "libkernel", 1, 1, posix_rmdir);
    LIB_FUNCTION("c7ZnT7V1B98", "libkernel", 1, "libkernel", 1, 1, posix_rmdir);
    LIB_FUNCTION("naInUjYt3so", "libkernel", 1, "libkernel", 1, 1, sceKernelRmdir);
    LIB_FUNCTION("E6ao34wPw+U", "libScePosix", 1, "libkernel", 1, 1, posix_stat);
    LIB_FUNCTION("E6ao34wPw+U", "libkernel", 1, "libkernel", 1, 1, posix_stat);
    LIB_FUNCTION("eV9wAD2riIA", "libkernel", 1, "libkernel", 1, 1, sceKernelStat);
    LIB_FUNCTION("uWyW3v98sU4", "libkernel", 1, "libkernel", 1, 1, sceKernelCheckReachability);
    LIB_FUNCTION("mqQMh1zPPT8", "libScePosix", 1, "libkernel", 1, 1, posix_fstat);
    LIB_FUNCTION("mqQMh1zPPT8", "libkernel", 1, "libkernel", 1, 1, posix_fstat);
    LIB_FUNCTION("kBwCPsYX-m4", "libkernel", 1, "libkernel", 1, 1, sceKernelFstat);
    LIB_FUNCTION("ih4CD9-gghM", "libkernel", 1, "libkernel", 1, 1, posix_ftruncate);
    LIB_FUNCTION("VW3TVZiM4-E", "libkernel", 1, "libkernel", 1, 1, sceKernelFtruncate);
    LIB_FUNCTION("NN01qLRhiqU", "libScePosix", 1, "libkernel", 1, 1, posix_rename);
    LIB_FUNCTION("NN01qLRhiqU", "libkernel", 1, "libkernel", 1, 1, posix_rename);
    LIB_FUNCTION("52NcYU9+lEo", "libkernel", 1, "libkernel", 1, 1, sceKernelRename);
    LIB_FUNCTION("yTj62I7kw4s", "libkernel", 1, "libkernel", 1, 1, sceKernelPreadv);
    LIB_FUNCTION("ezv-RSBNKqI", "libScePosix", 1, "libkernel", 1, 1, posix_pread);
    LIB_FUNCTION("ezv-RSBNKqI", "libkernel", 1, "libkernel", 1, 1, posix_pread);
    LIB_FUNCTION("+r3rMFwItV4", "libkernel", 1, "libkernel", 1, 1, sceKernelPread);
    LIB_FUNCTION("juWbTNM+8hw", "libScePosix", 1, "libkernel", 1, 1, posix_fsync);
    LIB_FUNCTION("juWbTNM+8hw", "libkernel", 1, "libkernel", 1, 1, posix_fsync);
    LIB_FUNCTION("fTx66l5iWIA", "libkernel", 1, "libkernel", 1, 1, sceKernelFsync);
    LIB_FUNCTION("j2AIqSqJP0w", "libkernel", 1, "libkernel", 1, 1, sceKernelGetdents);
    LIB_FUNCTION("sfKygSjIbI8", "libkernel", 1, "libkernel", 1, 1, getdirentries);
    LIB_FUNCTION("taRWhTJFTgE", "libkernel", 1, "libkernel", 1, 1, sceKernelGetdirentries);
    LIB_FUNCTION("C2kJ-byS5rM", "libkernel", 1, "libkernel", 1, 1, posix_pwrite);
    LIB_FUNCTION("FCcmRZhWtOk", "libScePosix", 1, "libkernel", 1, 1, posix_pwritev);
    LIB_FUNCTION("FCcmRZhWtOk", "libkernel", 1, "libkernel", 1, 1, posix_pwritev);
    LIB_FUNCTION("nKWi-N2HBV4", "libkernel", 1, "libkernel", 1, 1, sceKernelPwrite);
    LIB_FUNCTION("mBd4AfLP+u8", "libkernel", 1, "libkernel", 1, 1, sceKernelPwritev);
    LIB_FUNCTION("AUXVxWeJU-A", "libkernel", 1, "libkernel", 1, 1, sceKernelUnlink);
    LIB_FUNCTION("T8fER+tIGgk", "libScePosix", 1, "libkernel", 1, 1, posix_select);
    LIB_FUNCTION("T8fER+tIGgk", "libkernel", 1, "libkernel", 1, 1, posix_select);
}

} // namespace Libraries::Kernel
