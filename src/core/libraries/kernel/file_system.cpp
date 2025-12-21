// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <map>
#include <ranges>
#include <magic_enum/magic_enum.hpp>

#include "common/assert.h"
#include "common/error.h"
#include "common/logging/log.h"
#include "common/scope_exit.h"
#include "common/singleton.h"

#include "core/file_sys/devices/console_device.h"
#include "core/file_sys/devices/deci_tty6_device.h"
#include "core/file_sys/devices/logger.h"
#include "core/file_sys/devices/nop_device.h"
#include "core/file_sys/devices/random_device.h"
#include "core/file_sys/devices/rng_device.h"
#include "core/file_sys/devices/srandom_device.h"
#include "core/file_sys/fs.h"

#include "core/libraries/kernel/file_system.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/kernel/posix_error.h"
#include "core/libraries/libs.h"
#include "core/libraries/network/sockets.h"
#include "core/memory.h"
#include "kernel.h"

#include "core/file_sys/quasifs/quasifs.h"

#ifdef _WIN32
#include <io.h>
#include <winsock2.h>
#else
#include <sys/select.h>
#endif

namespace qfs = QuasiFS;

namespace Libraries::Kernel {

static QuasiFS::QFS* g_qfs = Common::Singleton<QuasiFS::QFS>::Instance();

s32 PS4_SYSV_ABI posix_open_impl(const char* raw_path, s32 flags, u16 mode) {
    int result = g_qfs->Operation.Open(raw_path, flags, mode);
    LOG_INFO(Kernel_Fs, "path = {} flags = {:#x} mode = {:#o} result = {}", raw_path, flags, mode,
             result);
    if (result < 0) {
        *__Error() = -result;
        return -1;
    }
    return result;
}

s32 PS4_SYSV_ABI posix_open(const char* filename, s32 flags, u16 mode) {
    return posix_open_impl(filename, flags, mode);
}

s32 PS4_SYSV_ABI sceKernelOpen(const char* path, s32 flags, /* SceKernelMode*/ u16 mode) {
    s32 result = posix_open_impl(path, flags, mode);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s32 PS4_SYSV_ABI posix_close_impl(s32 fd) {
    int result = g_qfs->Operation.Close(fd);
    LOG_INFO(Kernel_Fs, "fd = {} result = {}", fd, result);
    if (result < 0) {
        *__Error() = -result;
        return -1;
    }
    return result;
}

s32 PS4_SYSV_ABI posix_close(s32 fd) {
    return posix_close_impl(fd);
}

s32 PS4_SYSV_ABI sceKernelClose(s32 fd) {
    s32 result = posix_close_impl(fd);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

//
// read/write
//

// read
s64 PS4_SYSV_ABI posix_read_impl(s32 fd, void* buf, u64 nbytes) {
    const auto* memory = Core::Memory::Instance();
    // // Invalidate up to the actual number of bytes that could be read.
    const auto remaining = g_qfs->GetSize(fd) - g_qfs->Operation.Tell(fd);

    memory->InvalidateMemory(reinterpret_cast<VAddr>(buf), std::min<u64>(nbytes, remaining));

    int result = g_qfs->Operation.Read(fd, buf, nbytes);
    if (result < 0) {
        *__Error() = -result;
        return -1;
    }
    return result;

    // if (file->type == Core::FileSys::FileType::Directory) {
    //     s64 result = file->directory->read(buf, nbytes);
    //     if (result < 0) {
    //         ErrSceToPosix(result);
    //         return -1;
    //     }
    //     return result;
    // } else if (file->type == Core::FileSys::FileType::Socket) {
    //     // Socket functions handle errnos internally.
    //     return file->socket->ReceivePacket(buf, nbytes, 0, nullptr, 0);
    // }
}

s64 PS4_SYSV_ABI posix_read(s32 fd, void* buf, u64 nbytes) {
    return posix_read_impl(fd, buf, nbytes);
}

s64 PS4_SYSV_ABI sceKernelRead(s32 fd, void* buf, u64 nbytes) {
    s64 result = posix_read_impl(fd, buf, nbytes);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

// preadv
s64 PS4_SYSV_ABI posix_preadv(s32 fd, const OrbisKernelIovec* iov, s32 iovcnt, s64 offset) {
    int result = g_qfs->Operation.PReadV(fd, iov, iovcnt, offset);
    if (result < 0) {
        *__Error() = -result;
        return -1;
    }
    return result;
}

s64 PS4_SYSV_ABI sceKernelPreadv(s32 fd, const OrbisKernelIovec* iov, s32 iovcnt, s64 offset) {
    s64 result = posix_preadv(fd, iov, iovcnt, offset);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

// pread
s64 PS4_SYSV_ABI posix_pread(s32 fd, void* buf, u64 nbytes, s64 offset) {
    const OrbisKernelIovec iovec{buf, nbytes};
    return posix_preadv(fd, &iovec, 1, offset);
}

s64 PS4_SYSV_ABI sceKernelPread(s32 fd, void* buf, u64 nbytes, s64 offset) {
    const OrbisKernelIovec iovec{buf, nbytes};
    return sceKernelPreadv(fd, &iovec, 1, offset);
}

// readv
s64 PS4_SYSV_ABI posix_readv_impl(s32 fd, const OrbisKernelIovec* iov, s32 iovcnt) {
    int result = g_qfs->Operation.ReadV(fd, iov, iovcnt);
    if (result < 0) {
        *__Error() = -result;
        return -1;
    }
    return result;
}

s64 PS4_SYSV_ABI posix_readv(s32 fd, const OrbisKernelIovec* iov, s32 iovcnt) {
    return posix_readv_impl(fd, iov, iovcnt);
}

s64 PS4_SYSV_ABI sceKernelReadv(s32 fd, const OrbisKernelIovec* iov, s32 iovcnt) {
    s64 result = posix_readv(fd, iov, iovcnt);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

// write
s64 PS4_SYSV_ABI posix_write_impl(s32 fd, const void* buf, u64 nbytes) {
    int result = g_qfs->Operation.Write(fd, buf, nbytes);
    if (result < 0) {
        *__Error() = -result;
        return -1;
    }
    return result;
}

s64 PS4_SYSV_ABI posix_write(s32 fd, const void* buf, u64 nbytes) {
    return posix_write_impl(fd, buf, nbytes);
}

s64 PS4_SYSV_ABI sceKernelWrite(s32 fd, const void* buf, u64 nbytes) {
    s64 result = posix_write_impl(fd, buf, nbytes);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

// pwritev
s64 PS4_SYSV_ABI posix_pwritev(s32 fd, const OrbisKernelIovec* iov, s32 iovcnt, s64 offset) {
    int result = g_qfs->Operation.PWriteV(fd, iov, iovcnt, offset);
    if (result < 0) {
        *__Error() = -result;
        return -1;
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

// pwrite
s64 PS4_SYSV_ABI posix_pwrite(s32 fd, void* buf, u64 nbytes, s64 offset) {
    const OrbisKernelIovec iovec{buf, nbytes};
    return posix_pwritev(fd, &iovec, 1, offset);
}

s64 PS4_SYSV_ABI sceKernelPwrite(s32 fd, void* buf, u64 nbytes, s64 offset) {
    const OrbisKernelIovec iovec{buf, nbytes};
    return sceKernelPwritev(fd, &iovec, 1, offset);
}

// writev
s64 PS4_SYSV_ABI posix_writev_impl(s32 fd, const OrbisKernelIovec* iov, s32 iovcnt) {
    int result = g_qfs->Operation.WriteV(fd, iov, iovcnt);
    if (result < 0) {
        *__Error() = -result;
        return -1;
    }
    return result;
}

s64 PS4_SYSV_ABI posix_writev(s32 fd, const OrbisKernelIovec* iov, s32 iovcnt) {
    return posix_writev_impl(fd, iov, iovcnt);
}

s64 PS4_SYSV_ABI sceKernelWritev(s32 fd, const OrbisKernelIovec* iov, s32 iovcnt) {
    s64 result = posix_writev_impl(fd, iov, iovcnt);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

//
// others
//

s32 PS4_SYSV_ABI posix_unlink(const char* path) {
    int result = g_qfs->Operation.Unlink(path);
    LOG_INFO(Kernel_Fs, "path = {} result = {}", path, result);
    if (result < 0) {
        *__Error() = -result;
        return -1;
    }
    return result;
}

s32 PS4_SYSV_ABI sceKernelUnlink(const char* path) {
    s32 result = posix_unlink(path);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s32 PS4_SYSV_ABI posix_fsync(s32 fd) {
    int result = g_qfs->Operation.FSync(fd);
    if (result < 0) {
        *__Error() = -result;
        return -1;
    }
    return result;
}

s32 PS4_SYSV_ABI sceKernelFsync(s32 fd) {
    s32 result = posix_fsync(fd);
    LOG_INFO(Kernel_Fs, "fd = {} result = {}", fd, result);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s32 PS4_SYSV_ABI posix_truncate(const char* path, s64 length) {
    int result = g_qfs->Operation.Truncate(path, length);
    if (result < 0) {
        *__Error() = -result;
        return -1;
    }
    return result;
}

s32 PS4_SYSV_ABI sceKernelTruncate(const char* path, s64 length) {
    s32 result = posix_truncate(path, length);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s32 PS4_SYSV_ABI posix_ftruncate(s32 fd, s64 length) {
    int result = g_qfs->Operation.FTruncate(fd, length);
    if (result < 0) {
        *__Error() = -result;
        return -1;
    }
    return result;
}

s32 PS4_SYSV_ABI sceKernelFtruncate(s32 fd, s64 length) {
    s32 result = posix_ftruncate(fd, length);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s64 PS4_SYSV_ABI posix_lseek(s32 fd, s64 offset, s32 whence) {
    s32 origin{};
    if (whence == 0) {
        origin = qfs::SeekOrigin::ORIGIN;
    } else if (whence == 1) {
        origin = qfs::SeekOrigin::CURRENT;
    } else if (whence == 2) {
        origin = qfs::SeekOrigin::END;
    } else if (whence == 3 || whence == 4) {
        // whence parameter belongs to an unsupported POSIX extension
        *__Error() = POSIX_ENOTTY;
        return -1;
    } else {
        // whence parameter is invalid
        *__Error() = POSIX_EINVAL;
        return -1;
    }

    int result = g_qfs->Operation.LSeek(fd, offset, origin);
    if (result < 0) {
        *__Error() = -result;
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

s32 PS4_SYSV_ABI posix_mkdir(const char* path, u16 mode) {
    int result = g_qfs->Operation.MKDir(path, mode);
    LOG_INFO(Kernel_Fs, "path = {} mode = {:#o} result = {}", path, mode, result);
    if (result < 0) {
        *__Error() = -result;
        return -1;
    }
    return result;

    // CUSA02456: path = /aotl after sceSaveDataMount(mode = 1)
    // std::error_code ec;
    // if (dir_name.empty() || !std::filesystem::create_directory(dir_name, ec)) {
    //     *__Error() = POSIX_EIO;
    //     return -1;
    // }
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
    int result = g_qfs->Operation.RMDir(path);
    LOG_INFO(Kernel_Fs, "{} result = {}", path, result);
    if (result < 0) {
        *__Error() = -result;
        return -1;
    }
    return result;
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
    int result = g_qfs->Operation.Stat(path, sb);
    LOG_INFO(Kernel_Fs, "path = {} result = {}", path, result);
    if (result < 0) {
        *__Error() = -result;
        return -1;
    }
    return result;
}

s32 PS4_SYSV_ABI sceKernelStat(const char* path, OrbisKernelStat* sb) {
    s32 result = posix_stat(path, sb);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }

    // this is correct behaviour, courtesy of @red-prig
    sb->st_dev = 0;
    sb->st_ino = 0;
    sb->st_nlink = 0;
    sb->st_uid = 0;
    sb->st_gid = 0;
    sb->st_rdev = 0;
    sb->st_flags = 0;
    sb->st_gen = 0;
    sb->st_lspare = 0;
    return result;
}

s32 PS4_SYSV_ABI posix_fstat(s32 fd, OrbisKernelStat* sb) {
    int result = g_qfs->Operation.FStat(fd, sb);
    LOG_INFO(Kernel_Fs, "fd = {} result = {}", fd, result);
    if (result < 0) {
        *__Error() = -result;
        return -1;
    }
    return result;

    // switch (file->type) {
    // case Core::FileSys::FileType::Device: {
    //     s32 result = file->device->fstat(sb);
    //     if (result < 0) {
    //         ErrSceToPosix(result);
    //         return -1;
    //     }
    //     return result;
    // }
    // case Core::FileSys::FileType::Regular: {
    //     sb->st_mode = 0000777u | 0100000u;
    //     sb->st_size = file->f.GetSize();
    //     sb->st_blksize = 512;
    //     sb->st_blocks = (sb->st_size + 511) / 512;
    //     // TODO incomplete
    //     break;
    // }
    // case Core::FileSys::FileType::Directory: {
    //     s32 result = file->directory->fstat(sb);
    //     if (result < 0) {
    //         ErrSceToPosix(result);
    //         return -1;
    //     }
    //     return result;
    // }
    // case Core::FileSys::FileType::Socket: {
    //     // Socket functions handle errnos internally
    //     return file->socket->fstat(sb);
    // }
    // case Core::FileSys::FileType::Epoll:
    // case Core::FileSys::FileType::Resolver: {
    //     LOG_ERROR(Kernel_Fs, "(STUBBED) file type {}",
    //     magic_enum::enum_name(file->type.load())); break;
    // }
    // default:
    //     UNREACHABLE_MSG("{}", u32(file->type.load()));

    // }
}

s32 PS4_SYSV_ABI sceKernelFstat(s32 fd, OrbisKernelStat* sb) {
    s32 result = posix_fstat(fd, sb);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

static s64 posix_getdirentries_impl(s32 fd, char* buf, u64 nbytes, s64* basep) {
    int result = g_qfs->Operation.GetDents(fd, buf, nbytes, basep);
    LOG_INFO(Kernel_Fs, "fd = {} count = {} result = {}", fd, nbytes, result);
    if (result < 0) {
        *__Error() = -result;
        return -1;
    }
    return result;
}

s64 PS4_SYSV_ABI posix_getdirentries(s32 fd, char* buf, u64 nbytes, s64* basep) {
    return posix_getdirentries_impl(fd, buf, nbytes, basep);
}

s64 PS4_SYSV_ABI sceKernelGetdirentries(s32 fd, char* buf, u64 nbytes, s64* basep) {
    s64 result = posix_getdirentries_impl(fd, buf, nbytes, basep);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s64 PS4_SYSV_ABI posix_getdents(s32 fd, char* buf, u64 nbytes) {
    return posix_getdirentries_impl(fd, buf, nbytes, nullptr);
}

s64 PS4_SYSV_ABI sceKernelGetdents(s32 fd, char* buf, u64 nbytes) {
    s64 result = posix_getdents(fd, buf, nbytes);
    if (result < 0) {
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

s32 PS4_SYSV_ABI posix_rename(const char* from, const char* to) {
    int result = g_qfs->Operation.Move(from, to, false);
    if (result < 0) {
        *__Error() = -result;
        return -1;
    }
    return result;

    // auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    // bool ro = false;
    // const auto src_path = mnt->GetHostPath(from, &ro);
    // if (!std::filesystem::exists(src_path)) {
    //     *__Error() = POSIX_ENOENT;
    //     return -1;
    // }
    // if (ro) {
    //     *__Error() = POSIX_EROFS;
    //     return -1;
    // }
    // const auto dst_path = mnt->GetHostPath(to, &ro);
    // if (ro) {
    //     *__Error() = POSIX_EROFS;
    //     return -1;
    // }
    // const bool src_is_dir = std::filesystem::is_directory(src_path);
    // const bool dst_is_dir = std::filesystem::is_directory(dst_path);
    // if (src_is_dir && !dst_is_dir) {
    //     *__Error() = POSIX_ENOTDIR;
    //     return -1;
    // }
    // if (!src_is_dir && dst_is_dir) {
    //     *__Error() = POSIX_EISDIR;
    //     return -1;
    // }
    // if (dst_is_dir && !std::filesystem::is_empty(dst_path)) {
    //     *__Error() = POSIX_ENOTEMPTY;
    //     return -1;
    // }

    // // On Windows, std::filesystem::rename will error if the file has been opened before.
    // std::filesystem::copy(src_path, dst_path, std::filesystem::copy_options::overwrite_existing);
    // auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    // auto file = h->GetFile(src_path);
    // if (file) {
    //     auto access_mode = file->f.GetAccessMode();
    //     file->f.Close();
    //     std::filesystem::remove(src_path);
    //     file->f.Open(dst_path, access_mode);
    // } else {
    //     std::filesystem::remove(src_path);
    // }

    // return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelRename(const char* from, const char* to) {
    s32 result = posix_rename(from, to);
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
    LOG_INFO(Kernel_Fs, "nfds = {}, readfds = {}, writefds = {}, exceptfds = {}, timeout = {}",
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

s32 PS4_SYSV_ABI sceKernelCheckReachability(const char* path) {
    QuasiFS::Resolved res;
    int result = g_qfs->Resolve(path, res);
    if (result < 0) {
        *__Error() = -result;
        return ErrnoToSceKernelError(*__Error());
    }
    return result;
}

void RegisterFileSystem(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("6c3rCVE-fTU", "libkernel", 1, "libkernel", posix_open_impl);
    LIB_FUNCTION("wuCroIGjt2g", "libScePosix", 1, "libkernel", posix_open);
    LIB_FUNCTION("wuCroIGjt2g", "libkernel", 1, "libkernel", posix_open);
    LIB_FUNCTION("1G3lF1Gg1k8", "libkernel", 1, "libkernel", sceKernelOpen);

    LIB_FUNCTION("NNtFaKJbPt0", "libkernel", 1, "libkernel", posix_close_impl);
    LIB_FUNCTION("bY-PO6JhzhQ", "libScePosix", 1, "libkernel", posix_close);
    LIB_FUNCTION("bY-PO6JhzhQ", "libkernel", 1, "libkernel", posix_close);
    LIB_FUNCTION("UK2Tl2DWUns", "libkernel", 1, "libkernel", sceKernelClose);
    // read
    LIB_FUNCTION("DRuBt2pvICk", "libkernel", 1, "libkernel", posix_read_impl);
    LIB_FUNCTION("AqBioC2vF3I", "libScePosix", 1, "libkernel", posix_read);
    LIB_FUNCTION("AqBioC2vF3I", "libkernel", 1, "libkernel", posix_read);
    LIB_FUNCTION("Cg4srZ6TKbU", "libkernel", 1, "libkernel", sceKernelRead);
    // preadv
    LIB_FUNCTION("ZaRzaapAZwM", "libScePosix", 1, "libkernel", posix_preadv);
    LIB_FUNCTION("ZaRzaapAZwM", "libkernel", 1, "libkernel", posix_preadv);
    LIB_FUNCTION("yTj62I7kw4s", "libkernel", 1, "libkernel", sceKernelPreadv);
    // pread
    LIB_FUNCTION("ezv-RSBNKqI", "libScePosix", 1, "libkernel", posix_pread);
    LIB_FUNCTION("ezv-RSBNKqI", "libkernel", 1, "libkernel", posix_pread); 
    LIB_FUNCTION("+r3rMFwItV4", "libkernel", 1, "libkernel", sceKernelPread);
    // readv
    LIB_FUNCTION("+WRlkKjZvag", "libkernel", 1, "libkernel", posix_readv_impl);
    LIB_FUNCTION("I7ImcLds-uU", "libkernel", 1, "libkernel", posix_readv);
    LIB_FUNCTION("I7ImcLds-uU", "libScePosix", 1, "libkernel", posix_readv);
    LIB_FUNCTION("QqxBetgJH+g", "libkernel", 1, "libkernel", sceKernelReadv);
    // write
    LIB_FUNCTION("FxVZqBAA7ks", "libkernel", 1, "libkernel", posix_write_impl);
    LIB_FUNCTION("FN4gaPmuFV8", "libScePosix", 1, "libkernel", posix_write);
    LIB_FUNCTION("FN4gaPmuFV8", "libkernel", 1, "libkernel", posix_write);
    LIB_FUNCTION("4wSze92BhLI", "libkernel", 1, "libkernel", sceKernelWrite);
    // pwritev
    LIB_FUNCTION("FCcmRZhWtOk", "libScePosix", 1, "libkernel", posix_pwritev);
    LIB_FUNCTION("FCcmRZhWtOk", "libkernel", 1, "libkernel", posix_pwritev);
    LIB_FUNCTION("mBd4AfLP+u8", "libkernel", 1, "libkernel", sceKernelPwritev);
    // pwrite
    LIB_FUNCTION("C2kJ-byS5rM", "libScePosix", 1, "libkernel", posix_pwrite);
    LIB_FUNCTION("C2kJ-byS5rM", "libkernel", 1, "libkernel", posix_pwrite);
    LIB_FUNCTION("nKWi-N2HBV4", "libkernel", 1, "libkernel", sceKernelPwrite);
    // writev
    LIB_FUNCTION("YSHRBRLn2pI", "libkernel", 1, "libkernel", posix_writev_impl);
    LIB_FUNCTION("Z2aKdxzS4KE", "libScePosix", 1, "libkernel", posix_writev);
    LIB_FUNCTION("Z2aKdxzS4KE", "libkernel", 1, "libkernel", posix_writev);
    LIB_FUNCTION("kAt6VDbHmro", "libkernel", 1, "libkernel", sceKernelWritev);

    // link
    // symlink
    LIB_FUNCTION("VAzswvTOCzI", "libScePosix", 1, "libkernel", posix_unlink);
    LIB_FUNCTION("VAzswvTOCzI", "libkernel", 1, "libkernel", posix_unlink);
    LIB_FUNCTION("AUXVxWeJU-A", "libkernel", 1, "libkernel", sceKernelUnlink);
    // remove

    // flush
    LIB_FUNCTION("juWbTNM+8hw", "libScePosix", 1, "libkernel", posix_fsync);
    LIB_FUNCTION("juWbTNM+8hw", "libkernel", 1, "libkernel", posix_fsync);
    LIB_FUNCTION("fTx66l5iWIA", "libkernel", 1, "libkernel", sceKernelFsync);

    LIB_FUNCTION("ayrtszI7GBg", "libScePosix", 1, "libkernel", posix_truncate);
    LIB_FUNCTION("ayrtszI7GBg", "libkernel", 1, "libkernel", posix_truncate);
    LIB_FUNCTION("WlyEA-sLDf0", "libkernel", 1, "libkernel", sceKernelTruncate);
    LIB_FUNCTION("ih4CD9-gghM", "libScePosix", 1, "libkernel", posix_ftruncate);
    LIB_FUNCTION("ih4CD9-gghM", "libkernel", 1, "libkernel", posix_ftruncate);
    LIB_FUNCTION("VW3TVZiM4-E", "libkernel", 1, "libkernel", sceKernelFtruncate);

    LIB_FUNCTION("Oy6IpwgtYOk", "libScePosix", 1, "libkernel", posix_lseek);
    LIB_FUNCTION("Oy6IpwgtYOk", "libkernel", 1, "libkernel", posix_lseek);
    LIB_FUNCTION("oib76F-12fk", "libkernel", 1, "libkernel", sceKernelLseek);
    // tell

    LIB_FUNCTION("JGMio+21L4c", "libScePosix", 1, "libkernel", posix_mkdir);
    LIB_FUNCTION("JGMio+21L4c", "libkernel", 1, "libkernel", posix_mkdir);
    LIB_FUNCTION("1-LFLmRFxxM", "libkernel", 1, "libkernel", sceKernelMkdir);

    LIB_FUNCTION("c7ZnT7V1B98", "libScePosix", 1, "libkernel", posix_rmdir);
    LIB_FUNCTION("c7ZnT7V1B98", "libkernel", 1, "libkernel", posix_rmdir);
    LIB_FUNCTION("naInUjYt3so", "libkernel", 1, "libkernel", sceKernelRmdir);

    LIB_FUNCTION("E6ao34wPw+U", "libScePosix", 1, "libkernel", posix_stat);
    LIB_FUNCTION("E6ao34wPw+U", "libkernel", 1, "libkernel", posix_stat);
    LIB_FUNCTION("eV9wAD2riIA", "libkernel", 1, "libkernel", sceKernelStat);

    LIB_FUNCTION("mqQMh1zPPT8", "libScePosix", 1, "libkernel", posix_fstat);
    LIB_FUNCTION("mqQMh1zPPT8", "libkernel", 1, "libkernel", posix_fstat);
    LIB_FUNCTION("kBwCPsYX-m4", "libkernel", 1, "libkernel", sceKernelFstat);
    // chmod
    // fchmod
    LIB_FUNCTION("sfKygSjIbI8", "libkernel", 1, "libkernel", posix_getdirentries_impl);
    LIB_FUNCTION("f09KvIPy-QY", "libScePosix", 1, "libkernel", posix_getdirentries);
    LIB_FUNCTION("f09KvIPy-QY", "libkernel", 1, "libkernel", posix_getdirentries);
    LIB_FUNCTION("taRWhTJFTgE", "libkernel", 1, "libkernel", sceKernelGetdirentries);

    LIB_FUNCTION("2G6i6hMIUUY", "libScePosix", 1, "libkernel", posix_getdents);
    LIB_FUNCTION("2G6i6hMIUUY", "libkernel", 1, "libkernel", posix_getdents);
    LIB_FUNCTION("j2AIqSqJP0w", "libkernel", 1, "libkernel", sceKernelGetdents);

    LIB_FUNCTION("NN01qLRhiqU", "libScePosix", 1, "libkernel", posix_rename);
    LIB_FUNCTION("NN01qLRhiqU", "libkernel", 1, "libkernel", posix_rename);
    LIB_FUNCTION("52NcYU9+lEo", "libkernel", 1, "libkernel", sceKernelRename);

    LIB_FUNCTION("T8fER+tIGgk", "libScePosix", 1, "libkernel", posix_select);
    LIB_FUNCTION("T8fER+tIGgk", "libkernel", 1, "libkernel", posix_select);
    LIB_FUNCTION("uWyW3v98sU4", "libkernel", 1, "libkernel", sceKernelCheckReachability);
}

} // namespace Libraries::Kernel
