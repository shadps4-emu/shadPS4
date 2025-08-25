// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/alignment.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/file_sys/directories/normal_directory.h"
#include "core/file_sys/fs.h"

namespace Core::Directories {

std::shared_ptr<BaseDirectory> NormalDirectory::Create(std::string_view guest_directory) {
    return std::static_pointer_cast<BaseDirectory>(
        std::make_shared<NormalDirectory>(guest_directory));
}

NormalDirectory::NormalDirectory(std::string_view guest_directory) {
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();

    static s32 fileno = 0;
    mnt->IterateDirectory(guest_directory, [this](const auto& ent_path, const auto ent_is_file) {
        auto& dirent = dirents.emplace_back();
        dirent.d_fileno = ++fileno;
        dirent.d_type = (ent_is_file ? 8 : 4);
        strncpy(dirent.d_name, ent_path.filename().string().data(), MAX_LENGTH + 1);
        dirent.d_namlen = ent_path.filename().string().size();

        // Calculate the appropriate length for this dirent.
        // Account for the null terminator in d_name too.
        dirent.d_reclen = Common::AlignUp(sizeof(dirent.d_fileno) + sizeof(dirent.d_type) +
                                              sizeof(dirent.d_namlen) + sizeof(dirent.d_reclen) +
                                              (dirent.d_namlen + 1),
                                          4);

        directory_size += dirent.d_reclen;
    });

    // The last entry of a normal directory should have d_reclen covering the remaining data.
    // Since the dirents of a folder are constant by this point, we can modify the last dirent
    // before creating the emulated file buffer.
    const u64 filler_count = Common::AlignUp(directory_size, DIRECTORY_ALIGNMENT) - directory_size;
    dirents[dirents.size() - 1].d_reclen += filler_count;

    // Reading from standard directories seems to be based around file pointer logic.
    // Keep an internal buffer representing the raw contents of this file descriptor,
    // then emulate the various read functions with that.
    directory_size = Common::AlignUp(directory_size, DIRECTORY_ALIGNMENT);
    data_buffer.reserve(directory_size);
    memset(data_buffer.data(), 0, directory_size);

    u8* current_dirent = data_buffer.data();
    for (const NormalDirectoryDirent& dirent : dirents) {
        NormalDirectoryDirent* dirent_to_write =
            reinterpret_cast<NormalDirectoryDirent*>(current_dirent);
        dirent_to_write->d_fileno = dirent.d_fileno;

        // Using size d_namlen + 1 to account for null terminator.
        strncpy(dirent_to_write->d_name, dirent.d_name, dirent.d_namlen + 1);
        dirent_to_write->d_namlen = dirent.d_namlen;
        dirent_to_write->d_reclen = dirent.d_reclen;
        dirent_to_write->d_type = dirent.d_type;

        current_dirent += dirent.d_reclen;
    }
}

s64 NormalDirectory::read(void* buf, u64 nbytes) {
    // Nothing left to read.
    if (file_offset >= directory_size) {
        return ORBIS_OK;
    }

    const s64 remaining_data = directory_size - file_offset;
    const s64 bytes = nbytes > remaining_data ? remaining_data : nbytes;

    std::memcpy(buf, data_buffer.data() + file_offset, bytes);

    file_offset += bytes;
    return bytes;
}

s64 NormalDirectory::readv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) {
    s64 bytes_read = 0;
    for (s32 i = 0; i < iovcnt; i++) {
        const s64 result = read(iov[i].iov_base, iov[i].iov_len);
        if (result < 0) {
            return result;
        }
        bytes_read += result;
    }
    return bytes_read;
}

s64 NormalDirectory::preadv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt,
                            s64 offset) {
    const u64 old_file_pointer = file_offset;
    file_offset = offset;
    const s64 bytes_read = readv(iov, iovcnt);
    file_offset = old_file_pointer;
    return bytes_read;
}

s64 NormalDirectory::lseek(s64 offset, s32 whence) {
    switch (whence) {
    case 0: {
        file_offset = offset;
        break;
    }
    case 1: {
        file_offset += offset;
        break;
    }
    case 2: {
        file_offset = directory_size + offset;
        break;
    }
    default: {
        UNREACHABLE_MSG("lseek with unknown whence {}", whence);
    }
    }
    return file_offset;
}

s32 NormalDirectory::fstat(Libraries::Kernel::OrbisKernelStat* stat) {
    stat->st_mode = 0000777u | 0040000u;
    stat->st_size = directory_size;
    stat->st_blksize = 0x8000;
    stat->st_blocks = 8;
    return ORBIS_OK;
}

s64 NormalDirectory::getdents(void* buf, u64 nbytes, s64* basep) {
    if (basep != nullptr) {
        *basep = file_offset;
    }
    // read behaves identically to getdents for normal directories.
    return read(buf, nbytes);
}
} // namespace Core::Directories