// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/alignment.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/file_sys/fs.h"
#include "normal_directory.h"

namespace Core::Directories {

std::shared_ptr<BaseDirectory> NormalDirectory::Create(std::string_view guest_directory) {
    return std::shared_ptr<BaseDirectory>(
        reinterpret_cast<Directories::BaseDirectory*>(new NormalDirectory(guest_directory)));
}

NormalDirectory::NormalDirectory(std::string_view guest_directory) {
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();

    static s32 fileno = 0;
    mnt->IterateDirectory(guest_directory, [this](const auto& ent_path, const auto ent_is_file) {
        auto& dirent = dirents.emplace_back();
        dirent.d_fileno = ++fileno;
        dirent.d_type = (ent_is_file ? 8 : 4);
        strncpy(dirent.d_name, ent_path.filename().c_str(), MAX_LENGTH + 1);
        dirent.d_namlen = ent_path.filename().string().size();
        dirent.d_reclen =
            Common::AlignUp(sizeof(dirent.d_fileno) + sizeof(dirent.d_type) +
                                sizeof(dirent.d_namlen) + sizeof(dirent.d_reclen) + dirent.d_namlen,
                            4);

        directory_size += dirent.d_reclen;
    });

    directory_size = Common::AlignUp(directory_size, 512);
}

s64 NormalDirectory::read(void* buf, u64 nbytes) {
    if (dirents_index == dirents.size()) {
        // Nothing left to read.
        return 0;
    }

    s64 bytes_remaining = nbytes > directory_size ? directory_size : nbytes;
    memset(buf, 0, bytes_remaining);

    u64 bytes_written = 0;
    char* current_dirent = (char*)buf;
    NormalDirectoryDirent dirent = dirents[dirents_index];
    while (bytes_remaining > dirent.d_reclen) {
        NormalDirectoryDirent* dirent_to_write = (NormalDirectoryDirent*)current_dirent;
        dirent_to_write->d_fileno = dirent.d_fileno;
        strncpy(dirent_to_write->d_name, dirent.d_name, dirent.d_namlen + 1);
        dirent_to_write->d_namlen = dirent.d_namlen;
        dirent_to_write->d_reclen = dirent.d_reclen;
        dirent_to_write->d_type = dirent.d_type;

        if (dirents_index == dirents.size() - 1) {
            // Last dirent's reclen gets set to the remainder of the buffer.
            dirent_to_write->d_reclen = bytes_remaining;
            bytes_written += bytes_remaining;
            dirents_index++;
            break;
        }

        current_dirent += dirent.d_reclen;
        bytes_remaining -= dirent.d_reclen;
        bytes_written += dirent.d_reclen;
        dirent = dirents[++dirents_index];
    }

    return bytes_written;
}

s64 NormalDirectory::readv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) {
    LOG_ERROR(Kernel_Fs, "TODO");
    return 0;
}

s64 NormalDirectory::preadv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt,
                            s64 offset) {
    LOG_ERROR(Kernel_Fs, "TODO");
    return 0;
}

s64 NormalDirectory::lseek(s64 offset, s32 whence) {
    LOG_ERROR(Kernel_Fs, "TODO");
    return 0;
}

s32 NormalDirectory::fstat(Libraries::Kernel::OrbisKernelStat* stat) {
    stat->st_mode = 0000777u | 0040000u;
    stat->st_size = directory_size;
    stat->st_blksize = 0x8000;
    stat->st_blocks = 8;
    return 0;
}

s64 NormalDirectory::getdents(void* buf, u64 nbytes, s64* basep) {
    // read behaves identically to getdents for normal directories.
    s64 result = read(buf, nbytes);
    if (basep != nullptr) {
        *basep = dirents_index;
    }
    return result;
}
} // namespace Core::Directories