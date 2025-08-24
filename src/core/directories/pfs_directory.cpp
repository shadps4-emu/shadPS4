// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/alignment.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/file_sys/fs.h"
#include "pfs_directory.h"

namespace Core::Directories {

std::shared_ptr<BaseDirectory> PfsDirectory::Create(std::string_view guest_directory) {
    return std::shared_ptr<BaseDirectory>(
        reinterpret_cast<Directories::BaseDirectory*>(new PfsDirectory(guest_directory)));
}

PfsDirectory::PfsDirectory(std::string_view guest_directory) {
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
    });
}

s64 PfsDirectory::read(void* buf, u64 nbytes) {
    LOG_ERROR(Kernel_Fs, "TODO");
    return 0;
}

s64 PfsDirectory::readv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) {
    LOG_ERROR(Kernel_Fs, "TODO");
    return 0;
}

s64 PfsDirectory::preadv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt, s64 offset) {
    LOG_ERROR(Kernel_Fs, "TODO");
    return 0;
}

s64 PfsDirectory::lseek(s64 offset, s32 whence) {
    LOG_ERROR(Kernel_Fs, "TODO");
    return 0;
}

s32 PfsDirectory::fstat(Libraries::Kernel::OrbisKernelStat* stat) {
    stat->st_mode = 0000777u | 0040000u;
    stat->st_size = 0x10000;
    stat->st_blksize = 0x10000;
    stat->st_blocks = 0x80;
    return 0;
}

s64 PfsDirectory::getdents(void* buf, u64 nbytes, s64* basep) {
    LOG_ERROR(Kernel_Fs, "TODO");
    return 0;
}
} // namespace Core::Directories