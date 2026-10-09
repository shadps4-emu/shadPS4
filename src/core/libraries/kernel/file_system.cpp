// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <climits>
#include <cstdint>
#include <map>
#ifdef _WIN32
#include <sys/utime.h>
#else
#include <sys/time.h>
#endif
#include <ranges>
#include <thread>
#include <vector>
#include <magic_enum/magic_enum.hpp>

#include "common/assert.h"
#include "common/error.h"
#include "common/logging/log.h"
#include "common/scope_exit.h"
#include "common/singleton.h"
#include "core/file_sys/devices/console_device.h"
#include "core/file_sys/devices/deci_tty_device.h"
#include "core/file_sys/devices/logger.h"
#include "core/file_sys/devices/nop_device.h"
#include "core/file_sys/devices/random_device.h"
#include "core/file_sys/devices/rng_device.h"
#include "core/file_sys/devices/srandom_device.h"
#include "core/file_sys/devices/urandom_device.h"
#include "core/file_sys/devices/zero_device.h"
#include "core/file_sys/directories/normal_directory.h"
#include "core/file_sys/directories/pfs_directory.h"
#include "core/file_sys/fs.h"
#include "core/libraries/kernel/file_system.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/kernel/posix_error.h"
#include "core/libraries/libs.h"
#include "core/libraries/net/net_kernel.h"
#include "core/memory.h"
#include "kernel.h"

#ifdef _WIN32
#include <io.h>
#include <winsock2.h>
#else
#include <sys/select.h>
#include <sys/stat.h>
#endif

namespace D = Core::Devices;
namespace fs = std::filesystem;
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
    {"/dev/deci_tty6",&D::DeciTtyDevice::Create },
    {"/dev/deci_tty7",&D::DeciTtyDevice::Create },
    {"/dev/rng",      &D::RngDevice::Create },
    {"/dev/zero",  &D::ZeroDevice::Create },
    // clang-format on
};

namespace Libraries::Kernel {

s32 PS4_SYSV_ABI open(const char* raw_path, s32 flags, u16 mode) {
    LOG_INFO(Kernel_Fs, "path = {} flags = {:#x} mode = {:#o}", raw_path, flags, mode);

    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();

    bool read = (flags & 0x3) == ORBIS_KERNEL_O_RDONLY;
    bool write = (flags & 0x3) == ORBIS_KERNEL_O_WRONLY;
    bool rdwr = (flags & 0x3) == ORBIS_KERNEL_O_RDWR;

    if (!read && !write && !rdwr) {
        // Start by checking for invalid flags.
        *__Error() = POSIX_EINVAL;
        LOG_ERROR(Kernel_Fs, "Opening path {} failed, invalid flags {:#x}", raw_path, flags);
        return -1;
    }

    if (strlen(raw_path) > 255) {
        *__Error() = POSIX_ENAMETOOLONG;
        LOG_ERROR(Kernel_Fs, "Opening path {} failed, path is too long", raw_path);
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

    const auto sanitized_path = Core::FileSys::MntPoints::SanitizeGuestPath(raw_path);
    if (!sanitized_path) {
        *__Error() = POSIX_EINVAL;
        LOG_ERROR(Kernel_Fs, "Opening path {} failed, invalid path", raw_path);
        return -1;
    }
    const std::string& path = *sanitized_path;
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
    bool exists = mnt->Exists(file->m_guest_name);

    if (create) {
        if (excl && exists) {
            // Error if file exists
            h->DeleteHandle(handle);
            *__Error() = POSIX_EEXIST;
            LOG_ERROR(Kernel_Fs, "Creating {} failed, file already exists", raw_path);
            return -1;
        }

        if (!exists) {
            if (read_only) {
                // Can't create files in a read only directory
                h->DeleteHandle(handle);
                *__Error() = POSIX_EROFS;
                LOG_ERROR(Kernel_Fs, "Creating {} failed, path is read-only", raw_path);
                return -1;
            }
            // Create a file if it doesn't exist
            Common::FS::IOFile out(file->m_host_name, Common::FS::FileAccessMode::Create);
        }
    } else if (!exists) {
        // If we're not creating a file, and it doesn't exist, return ENOENT
        h->DeleteHandle(handle);
        *__Error() = POSIX_ENOENT;
        LOG_ERROR(Kernel_Fs, "Opening path {} failed, file does not exist", raw_path);
        return -1;
    }

    if (mnt->IsDirectory(file->m_guest_name) || directory) {
        // Directories can be opened even if the directory flag isn't set.
        // In these cases, error behavior is identical to the directory code path.
        directory = true;
    }

    if (directory) {
        if (!mnt->IsDirectory(file->m_guest_name)) {
            // If the opened file is not a directory, return ENOTDIR.
            // This will trigger when create & directory is specified, this is expected.
            h->DeleteHandle(handle);
            *__Error() = POSIX_ENOTDIR;
            LOG_ERROR(Kernel_Fs, "Opening directory {} failed, file is not a directory", raw_path);
            return -1;
        }

        if (write || rdwr) {
            // Cannot open directories with any type of write access
            h->DeleteHandle(handle);
            *__Error() = POSIX_EISDIR;
            LOG_ERROR(Kernel_Fs, "Opening directory {} failed, cannot open directories for writing",
                      raw_path);
            return -1;
        }

        if (truncate) {
            // Cannot open directories with truncate
            h->DeleteHandle(handle);
            *__Error() = POSIX_EISDIR;
            LOG_ERROR(Kernel_Fs, "Opening directory {} failed, cannot truncate directories",
                      raw_path);
            return -1;
        }

        file->type = Core::FileSys::FileType::Directory;
        file->is_opened = true;
        if (file->m_guest_name.starts_with("/app0")) {
            // TODO: Properly identify type for paths like "/app0/.."
            file->directory = Core::Directories::PfsDirectory::Create(file->m_guest_name);
        } else {
            file->directory = Core::Directories::NormalDirectory::Create(file->m_guest_name);
        }
    } else {
        file->type = Core::FileSys::FileType::Regular;

        // Reject writes to read-only mounts up front, so we can report
        // the right errno instead of a generic open failure.
        if ((write || rdwr || truncate) && read_only) {
            h->DeleteHandle(handle);
            *__Error() = POSIX_EROFS;
            LOG_ERROR(Kernel_Fs, "Opening {} for writing failed, path is read-only", raw_path);
            return -1;
        }

        // Map the guest open flags onto a single host access mode.
        Common::FS::FileAccessMode access_mode;
        if (truncate) {
            access_mode = Common::FS::FileAccessMode::ReadWrite;
        } else if (read) {
            access_mode = Common::FS::FileAccessMode::Read;
        } else if (write) {
            access_mode =
                append ? Common::FS::FileAccessMode::Append : Common::FS::FileAccessMode::Write;
        } else if (rdwr) {
            access_mode = append ? Common::FS::FileAccessMode::ReadAppend
                                 : Common::FS::FileAccessMode::ReadWrite;
        } else {
            access_mode = Common::FS::FileAccessMode::Read;
        }

        file->handle = mnt->Open(file->m_guest_name, access_mode);
        if (!file->handle || !file->handle->IsOpen()) {
            h->DeleteHandle(handle);
            auto mount = mnt->GetMount(raw_path);
            *__Error() =
                mount ? (write || rdwr || truncate) && mount->read_only ? POSIX_EROFS : POSIX_EIO
                      : POSIX_EINVAL;
            LOG_ERROR(Kernel_Fs, "Opening {} failed, backend did not serve the file", raw_path);
            return -1;
        }

        if (truncate) {
            if (auto* host = file->handle->GetHostFile()) {
                host->SetSize(0);
            }
        }
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
    if (file->type == Core::FileSys::FileType::Socket) {
        return Libraries::Net::KernelClose(fd);
    }
    if (file->type == Core::FileSys::FileType::Regular) {
        file->handle.reset();
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

s64 PS4_SYSV_ABI write(s32 fd, const void* buf, u64 nbytes) {
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->GetFile(fd);
    if (file == nullptr) {
        *__Error() = POSIX_EBADF;
        return -1;
    }

    if (file->type == Core::FileSys::FileType::Socket) {
        return Libraries::Net::KernelWrite(fd, buf, nbytes);
    }
    std::scoped_lock lk{file->m_mutex};
    if (file->type == Core::FileSys::FileType::Device) {
        s64 result = file->device->write(buf, nbytes);
        if (result < 0) {
            ErrSceToPosix(result);
            return -1;
        }
        return result;
    } else if (file->type == Core::FileSys::FileType::Directory) {
        *__Error() = POSIX_EBADF;
        return -1;
    }

    return file->Write(buf, nbytes);
}

s64 PS4_SYSV_ABI posix_write(s32 fd, const void* buf, u64 nbytes) {
    return write(fd, buf, nbytes);
}

s64 PS4_SYSV_ABI sceKernelWrite(s32 fd, const void* buf, u64 nbytes) {
    s64 result = write(fd, buf, nbytes);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

static thread_local std::vector<u8> file_buf{};

s64 ReadFile(Core::FileSys::File* file, void* buf, u64 nbytes) {
    const auto* memory = Core::Memory::Instance();
    // Invalidate up to the actual number of bytes that could be read.
    const auto remaining = file->GetSize() - file->Tell();
    memory->InvalidateMemory(reinterpret_cast<VAddr>(buf), std::min<u64>(nbytes, remaining));
    if (file_buf.capacity() < nbytes) {
        file_buf.reserve(nbytes);
    }
    s64 bytes = file->Read(file_buf.data(), nbytes);
    if (bytes < 0) {
        return bytes;
    }
    std::memcpy(buf, file_buf.data(), bytes);
    return bytes;
}

s64 PS4_SYSV_ABI readv(s32 fd, const OrbisKernelIovec* iov, s32 iovcnt) {
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->GetFile(fd);
    if (file == nullptr) {
        *__Error() = POSIX_EBADF;
        return -1;
    }

    std::scoped_lock lk{file->m_mutex};
    if (file->type == Core::FileSys::FileType::Device) {
        s64 result = file->device->readv(iov, iovcnt);
        if (result < 0) {
            ErrSceToPosix(result);
            return -1;
        }
        return result;
    } else if (file->type == Core::FileSys::FileType::Directory) {
        s64 result = file->directory->readv(iov, iovcnt);
        if (result < 0) {
            ErrSceToPosix(result);
            return -1;
        }
        return result;
    }

    if (file->IsWriteOnly()) {
        *__Error() = POSIX_EBADF;
        return -1;
    }

    s64 total_read = 0;
    for (s32 i = 0; i < iovcnt; i++) {
        total_read += ReadFile(file, iov[i].iov_base, iov[i].iov_len);
    }
    return total_read;
}

s64 PS4_SYSV_ABI posix_readv(s32 fd, const OrbisKernelIovec* iov, s32 iovcnt) {
    return readv(fd, iov, iovcnt);
}

s64 PS4_SYSV_ABI sceKernelReadv(s32 fd, const OrbisKernelIovec* iov, s32 iovcnt) {
    s64 result = readv(fd, iov, iovcnt);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s64 PS4_SYSV_ABI writev(s32 fd, const OrbisKernelIovec* iov, s32 iovcnt) {
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->GetFile(fd);
    if (file == nullptr) {
        *__Error() = POSIX_EBADF;
        return -1;
    }

    std::scoped_lock lk{file->m_mutex};

    if (file->type == Core::FileSys::FileType::Device) {
        s64 result = file->device->writev(iov, iovcnt);
        if (result < 0) {
            ErrSceToPosix(result);
            return -1;
        }
        return result;
    } else if (file->type == Core::FileSys::FileType::Directory) {
        *__Error() = POSIX_EBADF;
        return -1;
    }

    s64 total_written = 0;
    for (s32 i = 0; i < iovcnt; i++) {
        total_written += file->Write(iov[i].iov_base, iov[i].iov_len);
    }
    return total_written;
}

s64 PS4_SYSV_ABI posix_writev(s32 fd, const OrbisKernelIovec* iov, s32 iovcnt) {
    return writev(fd, iov, iovcnt);
}

s64 PS4_SYSV_ABI sceKernelWritev(s32 fd, const OrbisKernelIovec* iov, s32 iovcnt) {
    s64 result = writev(fd, iov, iovcnt);
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
    } else if (file->type == Core::FileSys::FileType::Directory) {
        s64 result = file->directory->lseek(offset, whence);
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

    if (!file->Seek(offset, origin)) {
        if (errno != 0) {
            // Seek failed in platform-specific code, errno needs to be converted.
            SetPosixErrno(errno);
            return -1;
        }
        // Shouldn't be possible, but just in case.
        return -1;
    }

    s64 result = file->Tell();
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

s64 PS4_SYSV_ABI read(s32 fd, void* buf, u64 nbytes) {
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->GetFile(fd);
    if (file == nullptr) {
        *__Error() = POSIX_EBADF;
        return -1;
    }

    if (file->type == Core::FileSys::FileType::Socket) {
        return Libraries::Net::KernelRead(fd, buf, nbytes);
    }
    std::scoped_lock lk{file->m_mutex};
    if (file->type == Core::FileSys::FileType::Device) {
        s64 result = file->device->read(buf, nbytes);
        if (result < 0) {
            ErrSceToPosix(result);
            return -1;
        }
        return result;
    } else if (file->type == Core::FileSys::FileType::Directory) {
        s64 result = file->directory->read(buf, nbytes);
        if (result < 0) {
            ErrSceToPosix(result);
            return -1;
        }
        return result;
    }

    if (file->IsWriteOnly()) {
        *__Error() = POSIX_EBADF;
        return -1;
    }

    return ReadFile(file, buf, nbytes);
}

s64 PS4_SYSV_ABI posix_read(s32 fd, void* buf, u64 nbytes) {
    return read(fd, buf, nbytes);
}

s64 PS4_SYSV_ABI sceKernelRead(s32 fd, void* buf, u64 nbytes) {
    s64 result = read(fd, buf, nbytes);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s32 PS4_SYSV_ABI posix_mkdir(const char* path, u16 mode) {
    LOG_INFO(Kernel_Fs, "path = {} mode = {:#o}", path, mode);
    if (strlen(path) > 255) {
        *__Error() = POSIX_ENAMETOOLONG;
        return -1;
    }
    if (path == nullptr) {
        *__Error() = POSIX_ENOTDIR;
        return -1;
    }
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();

    bool ro = false;
    const auto dir_name = mnt->GetHostPath(path, &ro);

    if (mnt->Exists(path)) {
        *__Error() = POSIX_EEXIST;
        return -1;
    }

    if (ro) {
        *__Error() = POSIX_EROFS;
        return -1;
    }

    // CUSA02456: path = /aotl after sceSaveDataMount(mode = 1)
    std::error_code ec;
    if (dir_name.empty() || !fs::create_directory(dir_name, ec)) {
        *__Error() = POSIX_EIO;
        return -1;
    }

    if (!fs::exists(dir_name)) {
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
    if (strlen(path) > 255) {
        *__Error() = POSIX_ENAMETOOLONG;
        return -1;
    }
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    bool ro = false;

    const fs::path dir_name = mnt->GetHostPath(path, &ro);

    if (ro) {
        *__Error() = POSIX_EROFS;
        return -1;
    }

    if (dir_name.empty() || !fs::is_directory(dir_name)) {
        *__Error() = POSIX_ENOTDIR;
        return -1;
    }

    if (!fs::exists(dir_name)) {
        *__Error() = POSIX_ENOENT;
        return -1;
    }

    std::error_code ec;
    s32 result = fs::remove_all(dir_name, ec);

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

s32 PS4_SYSV_ABI posix_access(const char* path, s32 mode) {
    LOG_INFO(Kernel_Fs, "(PARTIAL) path = {}, mode = {}", path, mode);
    if (strlen(path) > 255) {
        *__Error() = POSIX_ENAMETOOLONG;
        return -1;
    }

    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    const bool is_dir = mnt->IsDirectory(path);
    const bool is_file = !is_dir && mnt->Exists(path);
    const bool is_root = strncmp(path, "/", 2) == 0;
    if (!is_dir && !is_file && !is_root) {
        *__Error() = POSIX_ENOENT;
        return -1;
    }
    if (is_root) {
        LOG_WARNING(Kernel_Fs, "Checking accessibility of filesystem root");
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_stat(const char* path, OrbisKernelStat* sb) {
    LOG_DEBUG(Kernel_Fs, "(PARTIAL) path = {}", path);
    if (strlen(path) > 255) {
        *__Error() = POSIX_ENAMETOOLONG;
        return -1;
    }
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    const auto path_name = mnt->GetHostPath(path);
    std::memset(sb, 0, sizeof(OrbisKernelStat));

    const bool is_dir = mnt->IsDirectory(path);
    const bool is_file = !is_dir && mnt->Exists(path);
    const bool is_root = strncmp(path, "/", 2) == 0;
    if (!is_dir && !is_file && !is_root) {
        *__Error() = POSIX_ENOENT;
        return -1;
    }
    if (is_root) {
        LOG_WARNING(Kernel_Fs, "Attempting to access filesystem root");
        sb->st_mode = 0000777u | 0040000u;
        sb->st_size = 65536;
        sb->st_blksize = 65536;
        sb->st_blocks = 128;
        return ORBIS_OK;
    }

    // get the difference between file clock and system clock
    const auto now_sys = std::chrono::system_clock::now();
    const auto now_file = fs::file_time_type::clock::now();
    // calculate the file modified time
    std::error_code ec;
    const auto mtime =
        fs::exists(path_name, ec) ? fs::last_write_time(path_name, ec) : fs::file_time_type{};
    const auto mtimestamp = now_sys + (mtime - now_file);

    if (is_dir) {
        sb->st_mode = 0000777u | 0040000u;
        sb->st_size = 65536;
        sb->st_blksize = 65536;
        sb->st_blocks = 128;
        sb->st_mtim.tv_sec =
            std::chrono::duration_cast<std::chrono::seconds>(mtimestamp.time_since_epoch()).count();
        // TODO incomplete
    } else {
        sb->st_mode = 0000777u | 0100000u;
        if (auto handle = mnt->Open(path, /*writable=*/false)) {
            Core::FileSys::FileStat fst{};
            handle->Stat(fst);
            sb->st_size = static_cast<s64>(fst.size);
            if (fst.mtime_sec != 0 || fst.mtime_nsec != 0) {
                sb->st_mtim.tv_sec = fst.mtime_sec;
                sb->st_mtim.tv_nsec = fst.mtime_nsec;
                sb->st_atim.tv_sec = fst.atime_sec;
                sb->st_atim.tv_nsec = fst.atime_nsec;
                sb->st_ctim.tv_sec = fst.ctime_sec;
                sb->st_ctim.tv_nsec = fst.ctime_nsec;
            } else {
                sb->st_mtim.tv_sec =
                    std::chrono::duration_cast<std::chrono::seconds>(mtimestamp.time_since_epoch())
                        .count();
            }
        } else {
            sb->st_size =
                fs::exists(path_name, ec) ? static_cast<s64>(fs::file_size(path_name, ec)) : 0;
            sb->st_mtim.tv_sec =
                std::chrono::duration_cast<std::chrono::seconds>(mtimestamp.time_since_epoch())
                    .count();
        }
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
    if (strlen(path) > 255) {
        return ORBIS_KERNEL_ERROR_ENAMETOOLONG;
    }

    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    std::string_view guest_path{path};
    if (guest_path == "/") {
        return ORBIS_OK;
    }
    for (const auto& prefix : available_device | std::views::keys) {
        if (guest_path.starts_with(prefix)) {
            return ORBIS_OK;
        }
    }
    const auto path_name = mnt->GetHostPath(guest_path);
    if (!mnt->Exists(guest_path)) {
        return ORBIS_KERNEL_ERROR_ENOENT;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fstat(s32 fd, OrbisKernelStat* sb) {
    LOG_DEBUG(Kernel_Fs, "(PARTIAL) fd = {}", fd);
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
        sb->st_blksize = 512;
        Core::FileSys::FileStat fst{};
        if (file->handle) {
            file->handle->Stat(fst);
        }
        sb->st_size = static_cast<s64>(fst.size);
        sb->st_blocks = (sb->st_size + 511) / 512;
        sb->st_mtim.tv_sec = fst.mtime_sec;
        sb->st_mtim.tv_nsec = fst.mtime_nsec;
        sb->st_atim.tv_sec = fst.atime_sec;
        sb->st_atim.tv_nsec = fst.atime_nsec;
        sb->st_ctim.tv_sec = fst.ctime_sec;
        sb->st_ctim.tv_nsec = fst.ctime_nsec;
        // TODO incomplete
        break;
    }
    case Core::FileSys::FileType::Directory: {
        s32 result = file->directory->fstat(sb);
        if (result < 0) {
            ErrSceToPosix(result);
            return -1;
        }
        return result;
    }
    case Core::FileSys::FileType::Socket: {
        return Libraries::Net::KernelFstat(fd, sb);
    }
    case Core::FileSys::FileType::Equeue: {
        LOG_ERROR(Kernel_Fs, "(STUBBED) file type {}", magic_enum::enum_name(file->type.load()));
        break;
    }
    default:
        UNREACHABLE_MSG("{}", u32(file->type.load()));
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
    auto* host = file->GetHostFile();
    if (host == nullptr) {
        *__Error() = POSIX_EROFS;
        return -1;
    }
    host->SetSize(length);
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

s32 PS4_SYSV_ABI posix_truncate(const char* path, s64 length) {
    if (!path) {
        *__Error() = POSIX_EFAULT;
        return -1;
    }
    if (strlen(path) > 255) {
        *__Error() = POSIX_ENAMETOOLONG;
        return -1;
    }
    if (length < 0) {
        *__Error() = POSIX_EINVAL;
        return -1;
    }

    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    if (!mnt->Exists(path)) {
        *__Error() = POSIX_ENOENT;
        return -1;
    }
    if (mnt->IsDirectory(path)) {
        *__Error() = POSIX_EISDIR;
        return -1;
    }

    bool ro = false;
    const auto host_path = mnt->GetHostPath(path, &ro);
    if (ro) {
        *__Error() = POSIX_EROFS;
        return -1;
    }
    if (host_path.empty()) {
        *__Error() = POSIX_ENOENT;
        return -1;
    }

    std::error_code ec;
    fs::resize_file(host_path, static_cast<u64>(length), ec);
    if (ec) {
        *__Error() = NativeToPosixErrno(ec.value());
        return -1;
    }
    return 0;
}

s32 PS4_SYSV_ABI sceKernelTruncate(const char* path, s64 length) {
    s32 result = posix_truncate(path, length);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "path = {}, length = {}, error = {}", path ? path : "(null)", length,
                  *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s32 PS4_SYSV_ABI posix_chmod(const char* path, u32 mode) {
    if (!path) {
        *__Error() = POSIX_EFAULT;
        return -1;
    }
    if (strlen(path) > 255) {
        *__Error() = POSIX_ENAMETOOLONG;
        return -1;
    }

    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    if (!mnt->Exists(path)) {
        *__Error() = POSIX_ENOENT;
        return -1;
    }

    bool ro = false;
    const auto host_path = mnt->GetHostPath(path, &ro);
    if (ro) {
        *__Error() = POSIX_EROFS;
        return -1;
    }
    if (host_path.empty()) {
        *__Error() = POSIX_ENOENT;
        return -1;
    }

    std::error_code ec;
    fs::permissions(host_path, static_cast<fs::perms>(mode & 07777), ec);
    if (ec) {
        *__Error() = NativeToPosixErrno(ec.value());
        return -1;
    }
    return 0;
}

s32 PS4_SYSV_ABI sceKernelChmod(const char* path, u32 mode) {
    s32 result = posix_chmod(path, mode);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "path = {}, mode = {:o}, error = {}", path ? path : "(null)", mode,
                  *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s32 PS4_SYSV_ABI posix_fchmod(s32 fd, u32 mode) {
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->GetFile(fd);
    if (file == nullptr) {
        *__Error() = POSIX_EBADF;
        return -1;
    }

    if (file->type != Core::FileSys::FileType::Regular &&
        file->type != Core::FileSys::FileType::Directory) {
        *__Error() = POSIX_EINVAL;
        return -1;
    }

    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    const auto* mount = mnt->GetMount(file->m_guest_name);
    if ((file->handle && file->handle->IsReadOnly()) || (mount && mount->read_only)) {
        *__Error() = POSIX_EROFS;
        return -1;
    }

    if (file->m_host_name.empty()) {
        *__Error() = POSIX_EBADF;
        return -1;
    }

    std::error_code ec;
    fs::permissions(file->m_host_name, static_cast<fs::perms>(mode & 07777), ec);
    if (ec) {
        *__Error() = NativeToPosixErrno(ec.value());
        return -1;
    }

    return 0;
}

s32 PS4_SYSV_ABI sceKernelFchmod(s32 fd, u32 mode) {
    s32 result = posix_fchmod(fd, mode);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "fd = {}, mode = {:o}, error = {}", fd, mode, *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s32 PS4_SYSV_ABI posix_utimes(const char* path, const OrbisKernelTimeval* times) {
    if (!path) {
        *__Error() = POSIX_EFAULT;
        return -1;
    }
    if (strlen(path) > 255) {
        *__Error() = POSIX_ENAMETOOLONG;
        return -1;
    }

    if (times) {
        if (times[0].tv_usec < 0 || times[0].tv_usec >= 1'000'000 || times[1].tv_usec < 0 ||
            times[1].tv_usec >= 1'000'000) {
            *__Error() = POSIX_EINVAL;
            return -1;
        }
    }

    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    if (!mnt->Exists(path)) {
        *__Error() = POSIX_ENOENT;
        return -1;
    }

    bool ro = false;
    const auto host_path = mnt->GetHostPath(path, &ro);
    if (ro) {
        *__Error() = POSIX_EROFS;
        return -1;
    }
    if (host_path.empty()) {
        *__Error() = POSIX_ENOENT;
        return -1;
    }

#ifdef _WIN32
    if (times) {
        struct __utimbuf64 ubuf;
        ubuf.actime = times[0].tv_sec;
        ubuf.modtime = times[1].tv_sec;
        if (_wutime64(host_path.c_str(), &ubuf) != 0) {
            *__Error() = NativeToPosixErrno(errno);
            return -1;
        }
    } else {
        if (_wutime64(host_path.c_str(), nullptr) != 0) {
            *__Error() = NativeToPosixErrno(errno);
            return -1;
        }
    }
#else
    if (times) {
        struct timeval tv[2];
        tv[0].tv_sec = static_cast<time_t>(times[0].tv_sec);
        tv[0].tv_usec = static_cast<suseconds_t>(times[0].tv_usec);
        tv[1].tv_sec = static_cast<time_t>(times[1].tv_sec);
        tv[1].tv_usec = static_cast<suseconds_t>(times[1].tv_usec);
        if (::utimes(host_path.c_str(), tv) != 0) {
            *__Error() = NativeToPosixErrno(errno);
            return -1;
        }
    } else {
        if (::utimes(host_path.c_str(), nullptr) != 0) {
            *__Error() = NativeToPosixErrno(errno);
            return -1;
        }
    }
#endif

    return 0;
}

s32 PS4_SYSV_ABI sceKernelUtimes(const char* path, const OrbisKernelTimeval* times) {
    s32 result = posix_utimes(path, times);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "path = {}, error = {}", path ? path : "(null)", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s32 PS4_SYSV_ABI posix_futimes(s32 fd, const OrbisKernelTimeval* times) {
    if (times) {
        if (times[0].tv_usec < 0 || times[0].tv_usec >= 1'000'000 || times[1].tv_usec < 0 ||
            times[1].tv_usec >= 1'000'000) {
            *__Error() = POSIX_EINVAL;
            return -1;
        }
    }

    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->GetFile(fd);
    if (file == nullptr) {
        *__Error() = POSIX_EBADF;
        return -1;
    }

    if (file->type != Core::FileSys::FileType::Regular &&
        file->type != Core::FileSys::FileType::Directory) {
        *__Error() = POSIX_EINVAL;
        return -1;
    }

    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    const auto* mount = mnt->GetMount(file->m_guest_name);
    if ((file->handle && file->handle->IsReadOnly()) || (mount && mount->read_only)) {
        *__Error() = POSIX_EROFS;
        return -1;
    }

    if (file->m_host_name.empty()) {
        *__Error() = POSIX_EBADF;
        return -1;
    }

#ifdef _WIN32
    if (times) {
        struct __utimbuf64 ubuf;
        ubuf.actime = times[0].tv_sec;
        ubuf.modtime = times[1].tv_sec;
        if (_wutime64(file->m_host_name.c_str(), &ubuf) != 0) {
            *__Error() = NativeToPosixErrno(errno);
            return -1;
        }
    } else {
        if (_wutime64(file->m_host_name.c_str(), nullptr) != 0) {
            *__Error() = NativeToPosixErrno(errno);
            return -1;
        }
    }
#else
    if (times) {
        struct timeval tv[2];
        tv[0].tv_sec = static_cast<time_t>(times[0].tv_sec);
        tv[0].tv_usec = static_cast<suseconds_t>(times[0].tv_usec);
        tv[1].tv_sec = static_cast<time_t>(times[1].tv_sec);
        tv[1].tv_usec = static_cast<suseconds_t>(times[1].tv_usec);
        if (::utimes(file->m_host_name.c_str(), tv) != 0) {
            *__Error() = NativeToPosixErrno(errno);
            return -1;
        }
    } else {
        if (::utimes(file->m_host_name.c_str(), nullptr) != 0) {
            *__Error() = NativeToPosixErrno(errno);
            return -1;
        }
    }
#endif

    return 0;
}

s32 PS4_SYSV_ABI sceKernelFutimes(s32 fd, const OrbisKernelTimeval* times) {
    s32 result = posix_futimes(fd, times);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "fd = {}, error = {}", fd, *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s32 PS4_SYSV_ABI posix_rename(const char* from, const char* to) {
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    bool ro = false;
    const auto src_path = mnt->GetHostPath(from, &ro);
    if (strlen(from) > 255) {
        *__Error() = POSIX_ENAMETOOLONG;
        return -1;
    }
    if (strlen(to) > 255) {
        *__Error() = POSIX_ENAMETOOLONG;
        return -1;
    }
    if (!fs::exists(src_path)) {
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
    const bool src_is_dir = fs::is_directory(src_path);
    const bool dst_is_dir = fs::is_directory(dst_path);

    if (fs::exists(dst_path)) {
        if (src_is_dir && !dst_is_dir) {
            *__Error() = POSIX_ENOTDIR;
            return -1;
        }
        if (!src_is_dir && dst_is_dir) {
            *__Error() = POSIX_EISDIR;
            return -1;
        }
        if (dst_is_dir && !fs::is_empty(dst_path)) {
            *__Error() = POSIX_ENOTEMPTY;
            return -1;
        }
    }

    // On Windows, fs::rename will error if the file has been opened before.
    fs::copy(src_path, dst_path,
             fs::copy_options::overwrite_existing | fs::copy_options::recursive);
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto file = h->GetFile(src_path);
    if (file) {
        Common::FS::FileAccessMode access_mode = Common::FS::FileAccessMode::ReadWrite;
        if (auto* host = file->GetHostFile()) {
            access_mode = host->GetAccessMode();
        }
        file->handle.reset();
        fs::remove(src_path);
        // Reopen through the mount stack at the destination guest path.
        file->handle = mnt->Open(std::string_view(to), access_mode);
        file->m_guest_name = to;
    } else {
        fs::remove_all(src_path);
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

s64 PS4_SYSV_ABI posix_preadv(s32 fd, OrbisKernelIovec* iov, s32 iovcnt, s64 offset) {
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
    } else if (file->type == Core::FileSys::FileType::Directory) {
        s64 result = file->directory->preadv(iov, iovcnt, offset);
        if (result < 0) {
            ErrSceToPosix(result);
            return -1;
        }
        return result;
    }

    if (file->IsWriteOnly()) {
        *__Error() = POSIX_EBADF;
        return -1;
    }

    const s64 pos = file->Tell();
    SCOPE_EXIT {
        file->Seek(pos);
    };
    if (!file->Seek(offset)) {
        *__Error() = POSIX_EIO;
        return -1;
    }
    s64 total_read = 0;
    for (s32 i = 0; i < iovcnt; i++) {
        total_read += ReadFile(file, iov[i].iov_base, iov[i].iov_len);
    }
    return total_read;
}

s64 PS4_SYSV_ABI sceKernelPreadv(s32 fd, OrbisKernelIovec* iov, s32 iovcnt, s64 offset) {
    s64 result = posix_preadv(fd, iov, iovcnt, offset);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s64 PS4_SYSV_ABI posix_pread(s32 fd, void* buf, u64 nbytes, s64 offset) {
    OrbisKernelIovec iovec{buf, nbytes};
    return posix_preadv(fd, &iovec, 1, offset);
}

s64 PS4_SYSV_ABI sceKernelPread(s32 fd, void* buf, u64 nbytes, s64 offset) {
    OrbisKernelIovec iovec{buf, nbytes};
    return sceKernelPreadv(fd, &iovec, 1, offset);
}

s32 PS4_SYSV_ABI posix_fsync(s32 fd) {
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* file = h->GetFile(fd);
    if (file == nullptr) {
        *__Error() = POSIX_EBADF;
        return -1;
    }

    std::scoped_lock lk{file->m_mutex};
    if (file->type == Core::FileSys::FileType::Device) {
        s32 result = file->device->fsync();
        if (result < 0) {
            ErrSceToPosix(result);
            return -1;
        }
        return result;
    }
    file->Flush();
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

s32 PS4_SYSV_ABI posix_fdatasync(s32 fd) {
    return posix_fsync(fd);
}

s32 PS4_SYSV_ABI sceKernelFdatasync(s32 fd) {
    s32 result = posix_fdatasync(fd);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

void PS4_SYSV_ABI posix_sync() {
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    h->FlushAll();
}

void PS4_SYSV_ABI sceKernelSync() {
    posix_sync();
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

    if (nbytes < 512) {
        *__Error() = POSIX_EINVAL;
        return -1;
    }

    switch (file->type) {
    case Core::FileSys::FileType::Directory: {
        s64 result = file->directory->getdents(buf, nbytes, basep);
        if (result < 0) {
            ErrSceToPosix(result);
            return -1;
        }
        return result;
    }
    case Core::FileSys::FileType::Device: {
        s64 result = file->device->getdents(buf, nbytes, basep);
        if (result < 0) {
            ErrSceToPosix(result);
            return -1;
        }
        return result;
    }
    default: {
        // Not directory or device
        *__Error() = POSIX_EINVAL;
        return -1;
    }
    }

    return ORBIS_OK;
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

s64 PS4_SYSV_ABI posix_pwritev(s32 fd, const OrbisKernelIovec* iov, s32 iovcnt, s64 offset) {
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
    } else if (file->type == Core::FileSys::FileType::Directory) {
        *__Error() = POSIX_EBADF;
        return -1;
    }

    const s64 pos = file->Tell();
    SCOPE_EXIT {
        file->Seek(pos);
    };
    if (!file->Seek(offset)) {
        *__Error() = POSIX_EIO;
        return -1;
    }
    s64 total_written = 0;
    for (s32 i = 0; i < iovcnt; i++) {
        total_written += file->Write(iov[i].iov_base, iov[i].iov_len);
    }
    return total_written;
}

s64 PS4_SYSV_ABI posix_pwrite(s32 fd, void* buf, u64 nbytes, s64 offset) {
    OrbisKernelIovec iovec{buf, nbytes};
    return posix_pwritev(fd, &iovec, 1, offset);
}

s64 PS4_SYSV_ABI sceKernelPwrite(s32 fd, void* buf, u64 nbytes, s64 offset) {
    s64 result = posix_pwrite(fd, buf, nbytes, offset);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s64 PS4_SYSV_ABI sceKernelPwritev(s32 fd, const OrbisKernelIovec* iov, s32 iovcnt, s64 offset) {
    s64 result = posix_pwritev(fd, iov, iovcnt, offset);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s32 PS4_SYSV_ABI posix_unlink(const char* path) {
    if (strlen(path) > 255) {
        *__Error() = POSIX_ENAMETOOLONG;
        return -1;
    }
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

    if (fs::is_directory(host_path)) {
        *__Error() = POSIX_EPERM;
        return -1;
    }

    auto* file = h->GetFile(host_path);
    if (file == nullptr) {
        // File to unlink hasn't been opened, manually open and unlink it.
        Common::FS::IOFile file(host_path, Common::FS::FileAccessMode::ReadWrite);
        file.Unlink();
    } else if (auto* host = file->GetHostFile()) {
        host->Unlink();
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

namespace {

struct GuestFdSet {
    u64 bits[1024 / 64];
};
constexpr s32 kGuestFdSetSize = 1024;

bool GuestFdIsSet(const GuestFdSet* set, s32 fd) {
    return set != nullptr && (set->bits[fd / 64] >> (fd % 64) & 1) != 0;
}

void GuestFdSetBit(GuestFdSet* set, s32 fd) {
    set->bits[fd / 64] |= 1ull << (fd % 64);
}

} // namespace

s32 PS4_SYSV_ABI posix_select(s32 nfds, GuestFdSet* readfds, GuestFdSet* writefds,
                              GuestFdSet* exceptfds, OrbisKernelTimeval* timeout) {
    LOG_DEBUG(Kernel_Fs, "nfds = {}, readfds = {}, writefds = {}, exceptfds = {}, timeout = {}",
              nfds, fmt::ptr(readfds), fmt::ptr(writefds), fmt::ptr(exceptfds), fmt::ptr(timeout));
    if (nfds < 0 || nfds > kGuestFdSetSize) {
        *__Error() = POSIX_EINVAL;
        return -1;
    }
    s64 timeout_us = -1; // no limit
    if (timeout != nullptr) {
        if (timeout->tv_sec < 0 || timeout->tv_usec < 0 || timeout->tv_usec >= 1'000'000) {
            *__Error() = POSIX_EINVAL;
            return -1;
        }
        timeout_us = timeout->tv_sec > (INT64_MAX / 1'000'000 - 1)
                         ? INT64_MAX
                         : timeout->tv_sec * 1'000'000 + timeout->tv_usec;
    }

    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    GuestFdSet read_ready{}, write_ready{};
    s32 ready = 0;
    std::vector<Core::Net::SelectEntry> sockets;
    for (s32 i = 0; i < nfds; ++i) {
        const bool want_read = GuestFdIsSet(readfds, i);
        const bool want_write = GuestFdIsSet(writefds, i);
        if (!want_read && !want_write && !GuestFdIsSet(exceptfds, i)) {
            continue;
        }
        auto* file = h->GetFile(i);
        if (!file || ((file->type == Core::FileSys::FileType::Regular && !file->IsBackendOpen()) ||
                      (file->type == Core::FileSys::FileType::Socket && !file->is_opened))) {
            LOG_ERROR(Kernel_Fs, "fd {} is null or not opened", i);
            *__Error() = POSIX_EBADF;
            return -1;
        }
        if (file->type == Core::FileSys::FileType::Socket) {
            const u32 interest = (want_read ? Core::Net::Host::EvIn : 0u) |
                                 (want_write ? Core::Net::Host::EvOut : 0u);
            if (interest != 0) {
                sockets.push_back({i, interest, 0});
            }
            continue;
        }
        if (want_read && i != 0) {
            GuestFdSetBit(&read_ready, i);
            ++ready;
        }
        if (want_write) {
            GuestFdSetBit(&write_ready, i);
            ++ready;
        }
    }

    if (ready > 0) {
        timeout_us = 0;
    }
    if (!sockets.empty()) {
        const s64 n = Libraries::Net::KernelSelect(sockets, timeout_us);
        if (n < 0) {
            return -1; // errno set
        }
        ready += static_cast<s32>(n);
    } else if (ready == 0 && timeout_us != 0) {
        // Nothing that can become ready: select() is a sleep (forever without a timeout).
        do {
            std::this_thread::sleep_for(
                std::chrono::microseconds{timeout_us < 0 ? 1'000'000 : timeout_us});
        } while (timeout_us < 0);
    }

    for (const auto& e : sockets) {
        if (e.ready & Core::Net::Host::EvIn) {
            GuestFdSetBit(&read_ready, e.id);
        }
        if (e.ready & Core::Net::Host::EvOut) {
            GuestFdSetBit(&write_ready, e.id);
        }
    }
    if (readfds) {
        *readfds = read_ready;
    }
    if (writefds) {
        *writefds = write_ready;
    }
    if (exceptfds) {
        *exceptfds = {};
    }
    return ready;
}

void RegisterFileSystem(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("6c3rCVE-fTU", "libkernel", 1, "libkernel", open);
    LIB_FUNCTION("wuCroIGjt2g", "libScePosix", 1, "libkernel", posix_open);
    LIB_FUNCTION("wuCroIGjt2g", "libkernel", 1, "libkernel", posix_open);
    LIB_FUNCTION("1G3lF1Gg1k8", "libkernel", 1, "libkernel", sceKernelOpen);
    LIB_FUNCTION("NNtFaKJbPt0", "libkernel", 1, "libkernel", close);
    LIB_FUNCTION("bY-PO6JhzhQ", "libScePosix", 1, "libkernel", posix_close);
    LIB_FUNCTION("bY-PO6JhzhQ", "libkernel", 1, "libkernel", posix_close);
    LIB_FUNCTION("UK2Tl2DWUns", "libkernel", 1, "libkernel", sceKernelClose);
    LIB_FUNCTION("FxVZqBAA7ks", "libkernel", 1, "libkernel", write);
    LIB_FUNCTION("FN4gaPmuFV8", "libScePosix", 1, "libkernel", posix_write);
    LIB_FUNCTION("FN4gaPmuFV8", "libkernel", 1, "libkernel", posix_write);
    LIB_FUNCTION("4wSze92BhLI", "libkernel", 1, "libkernel", sceKernelWrite);
    LIB_FUNCTION("+WRlkKjZvag", "libkernel", 1, "libkernel", readv);
    LIB_FUNCTION("YSHRBRLn2pI", "libkernel", 1, "libkernel", writev);
    LIB_FUNCTION("kAt6VDbHmro", "libkernel", 1, "libkernel", sceKernelWritev);
    LIB_FUNCTION("Oy6IpwgtYOk", "libScePosix", 1, "libkernel", posix_lseek);
    LIB_FUNCTION("Oy6IpwgtYOk", "libkernel", 1, "libkernel", posix_lseek);
    LIB_FUNCTION("oib76F-12fk", "libkernel", 1, "libkernel", sceKernelLseek);
    LIB_FUNCTION("DRuBt2pvICk", "libkernel", 1, "libkernel", read);
    LIB_FUNCTION("AqBioC2vF3I", "libScePosix", 1, "libkernel", posix_read);
    LIB_FUNCTION("AqBioC2vF3I", "libkernel", 1, "libkernel", posix_read);
    LIB_FUNCTION("Cg4srZ6TKbU", "libkernel", 1, "libkernel", sceKernelRead);
    LIB_FUNCTION("JGMio+21L4c", "libScePosix", 1, "libkernel", posix_mkdir);
    LIB_FUNCTION("JGMio+21L4c", "libkernel", 1, "libkernel", posix_mkdir);
    LIB_FUNCTION("1-LFLmRFxxM", "libkernel", 1, "libkernel", sceKernelMkdir);
    LIB_FUNCTION("c7ZnT7V1B98", "libScePosix", 1, "libkernel", posix_rmdir);
    LIB_FUNCTION("c7ZnT7V1B98", "libkernel", 1, "libkernel", posix_rmdir);
    LIB_FUNCTION("naInUjYt3so", "libkernel", 1, "libkernel", sceKernelRmdir);
    LIB_FUNCTION("8vE6Z6VEYyk", "libkernel_psmkit", 1, "libkernel", posix_access);
    LIB_FUNCTION("E6ao34wPw+U", "libScePosix", 1, "libkernel", posix_stat);
    LIB_FUNCTION("E6ao34wPw+U", "libkernel", 1, "libkernel", posix_stat);
    LIB_FUNCTION("eV9wAD2riIA", "libkernel", 1, "libkernel", sceKernelStat);
    LIB_FUNCTION("uWyW3v98sU4", "libkernel", 1, "libkernel", sceKernelCheckReachability);
    LIB_FUNCTION("mqQMh1zPPT8", "libScePosix", 1, "libkernel", posix_fstat);
    LIB_FUNCTION("mqQMh1zPPT8", "libkernel", 1, "libkernel", posix_fstat);
    LIB_FUNCTION("kBwCPsYX-m4", "libkernel", 1, "libkernel", sceKernelFstat);
    LIB_FUNCTION("ih4CD9-gghM", "libkernel", 1, "libkernel", posix_ftruncate);
    LIB_FUNCTION("ih4CD9-gghM", "libScePosix", 1, "libkernel", posix_ftruncate);
    LIB_FUNCTION("VW3TVZiM4-E", "libkernel", 1, "libkernel", sceKernelFtruncate);
    LIB_FUNCTION("ayrtszI7GBg", "libkernel", 1, "libkernel", posix_truncate);
    LIB_FUNCTION("ayrtszI7GBg", "libScePosix", 1, "libkernel", posix_truncate);
    LIB_FUNCTION("WlyEA-sLDf0", "libkernel", 1, "libkernel", sceKernelTruncate);
    LIB_FUNCTION("z0dtnPxYgtg", "libkernel", 1, "libkernel", posix_chmod);
    LIB_FUNCTION("z0dtnPxYgtg", "libScePosix", 1, "libkernel", posix_chmod);
    LIB_FUNCTION("fgIsQ10xYVA", "libkernel", 1, "libkernel", sceKernelChmod);
    LIB_FUNCTION("n01yNbQO5W4", "libkernel", 1, "libkernel", posix_fchmod);
    LIB_FUNCTION("n01yNbQO5W4", "libScePosix", 1, "libkernel", posix_fchmod);
    LIB_FUNCTION("UtszJWHrDcA", "libkernel", 1, "libkernel", sceKernelFchmod);
    LIB_FUNCTION("GDuV00CHrUg", "libkernel", 1, "libkernel", posix_utimes);
    LIB_FUNCTION("GDuV00CHrUg", "libScePosix", 1, "libkernel", posix_utimes);
    LIB_FUNCTION("0Cq8ipKr9n0", "libkernel", 1, "libkernel", sceKernelUtimes);
    LIB_FUNCTION("+0EDo7YzcoU", "libkernel", 1, "libkernel", posix_futimes);
    LIB_FUNCTION("+0EDo7YzcoU", "libScePosix", 1, "libkernel", posix_futimes);
    LIB_FUNCTION("NLq2b1jOaN0", "libkernel", 1, "libkernel", sceKernelFutimes);
    LIB_FUNCTION("NN01qLRhiqU", "libScePosix", 1, "libkernel", posix_rename);
    LIB_FUNCTION("NN01qLRhiqU", "libkernel", 1, "libkernel", posix_rename);
    LIB_FUNCTION("52NcYU9+lEo", "libkernel", 1, "libkernel", sceKernelRename);
    LIB_FUNCTION("yTj62I7kw4s", "libkernel", 1, "libkernel", sceKernelPreadv);
    LIB_FUNCTION("ezv-RSBNKqI", "libScePosix", 1, "libkernel", posix_pread);
    LIB_FUNCTION("ezv-RSBNKqI", "libkernel", 1, "libkernel", posix_pread);
    LIB_FUNCTION("+r3rMFwItV4", "libkernel", 1, "libkernel", sceKernelPread);
    LIB_FUNCTION("juWbTNM+8hw", "libScePosix", 1, "libkernel", posix_fsync);
    LIB_FUNCTION("juWbTNM+8hw", "libkernel", 1, "libkernel", posix_fsync);
    LIB_FUNCTION("fTx66l5iWIA", "libkernel", 1, "libkernel", sceKernelFsync);
    LIB_FUNCTION("KIbJFQ0I1Cg", "libkernel", 1, "libkernel", posix_fdatasync);
    LIB_FUNCTION("KIbJFQ0I1Cg", "libScePosix", 1, "libkernel", posix_fdatasync);
    LIB_FUNCTION("30Rh4ixbKy4", "libkernel", 1, "libkernel", sceKernelFdatasync);
    LIB_FUNCTION("Y2OqwJQ3lr8", "libkernel", 1, "libkernel", posix_sync);
    LIB_FUNCTION("Y2OqwJQ3lr8", "libScePosix", 1, "libkernel", posix_sync);
    LIB_FUNCTION("uvT2iYBBnkY", "libkernel", 1, "libkernel", sceKernelSync);
    LIB_FUNCTION("j2AIqSqJP0w", "libkernel", 1, "libkernel", sceKernelGetdents);
    LIB_FUNCTION("sfKygSjIbI8", "libkernel", 1, "libkernel", getdirentries);
    LIB_FUNCTION("2G6i6hMIUUY", "libkernel", 1, "libkernel", posix_getdents);
    LIB_FUNCTION("taRWhTJFTgE", "libkernel", 1, "libkernel", sceKernelGetdirentries);
    LIB_FUNCTION("C2kJ-byS5rM", "libkernel", 1, "libkernel", posix_pwrite);
    LIB_FUNCTION("C2kJ-byS5rM", "libScePosix", 1, "libkernel", posix_pwrite);
    LIB_FUNCTION("FCcmRZhWtOk", "libScePosix", 1, "libkernel", posix_pwritev);
    LIB_FUNCTION("FCcmRZhWtOk", "libkernel", 1, "libkernel", posix_pwritev);
    LIB_FUNCTION("nKWi-N2HBV4", "libkernel", 1, "libkernel", sceKernelPwrite);
    LIB_FUNCTION("mBd4AfLP+u8", "libkernel", 1, "libkernel", sceKernelPwritev);
    LIB_FUNCTION("VAzswvTOCzI", "libkernel", 1, "libkernel", posix_unlink);
    LIB_FUNCTION("AUXVxWeJU-A", "libkernel", 1, "libkernel", sceKernelUnlink);
    LIB_FUNCTION("T8fER+tIGgk", "libScePosix", 1, "libkernel", posix_select);
    LIB_FUNCTION("T8fER+tIGgk", "libkernel", 1, "libkernel", posix_select);
}

} // namespace Libraries::Kernel
