// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/alignment.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/file_sys/directories/pfs_directory.h"
#include "core/file_sys/fs.h"

namespace Core::Directories {

std::shared_ptr<BaseDirectory> PfsDirectory::Create(std::string_view guest_directory) {
    return std::static_pointer_cast<BaseDirectory>(std::make_shared<PfsDirectory>(guest_directory));
}

PfsDirectory::PfsDirectory(std::string_view guest_directory) {
    constexpr u32 dirent_meta_size =
        sizeof(PfsDirectoryDirent::d_fileno) + sizeof(PfsDirectoryDirent::d_type) +
        sizeof(PfsDirectoryDirent::d_namlen) + sizeof(PfsDirectoryDirent::d_reclen);

    dirent_cache_bin.reserve(512);

    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();

    mnt->IterateDirectory(
        guest_directory, [this](const std::filesystem::path& ent_path, const bool ent_is_file) {
            PfsDirectoryDirent tmp{};
            std::string leaf(ent_path.filename().string());

            tmp.d_fileno = BaseDirectory::next_fileno();
            tmp.d_namlen = leaf.size();
            strncpy(tmp.d_name, leaf.data(), tmp.d_namlen + 1);
            tmp.d_type = ent_is_file ? 2 : 4;
            tmp.d_reclen = Common::AlignUp(dirent_meta_size + tmp.d_namlen + 1, 8);
            auto dirent_ptr = reinterpret_cast<const u8*>(&tmp);

            dirent_cache_bin.insert(dirent_cache_bin.end(), dirent_ptr, dirent_ptr + tmp.d_reclen);
        });

    directory_size = Common::AlignUp(dirent_cache_bin.size(), 0x10000);
}

s64 PfsDirectory::read(void* buf, u64 nbytes) {
    s64 bytes_available = this->dirent_cache_bin.size() - file_offset;
    if (bytes_available <= 0)
        return 0;

    bytes_available = std::min<s64>(bytes_available, static_cast<s64>(nbytes));
    memcpy(buf, this->dirent_cache_bin.data() + file_offset, bytes_available);

    s64 to_fill =
        (std::min<s64>(directory_size, static_cast<s64>(nbytes))) - bytes_available - file_offset;
    if (to_fill < 0) {
        LOG_ERROR(Kernel_Fs, "Dirent may have leaked {} bytes", -to_fill);
        return -to_fill + bytes_available;
    }
    memset(static_cast<u8*>(buf) + bytes_available, 0, to_fill);
    file_offset += to_fill + bytes_available;
    return to_fill + bytes_available;
}

s64 PfsDirectory::readv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt) {
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

s64 PfsDirectory::preadv(const Libraries::Kernel::OrbisKernelIovec* iov, s32 iovcnt, s64 offset) {
    const u64 old_file_pointer = file_offset;
    file_offset = offset;
    const s64 bytes_read = readv(iov, iovcnt);
    file_offset = old_file_pointer;
    return bytes_read;
}

s32 PfsDirectory::fstat(Libraries::Kernel::OrbisKernelStat* stat) {
    stat->st_mode = 0000777u | 0040000u;
    stat->st_size = directory_size;
    stat->st_blksize = 0x10000;
    stat->st_blocks = 0x80;
    return ORBIS_OK;
}

s64 PfsDirectory::getdents(void* buf, u64 nbytes, s64* basep) {
    if (basep)
        *basep = file_offset;

    // same as others, we just don't need a variable
    if (file_offset >= directory_size)
        return 0;

    u64 bytes_written = 0;
    u64 starting_offset = 0;
    u64 buffer_position = 0;
    while (buffer_position < this->dirent_cache_bin.size()) {
        const PfsDirectoryDirent* pfs_dirent =
            reinterpret_cast<PfsDirectoryDirent*>(this->dirent_cache_bin.data() + buffer_position);

        // bad, incomplete or OOB entry
        if (pfs_dirent->d_namlen == 0)
            break;

        if (starting_offset < file_offset) {
            // reading starts from the nearest full dirent
            starting_offset += pfs_dirent->d_reclen;
            buffer_position = bytes_written + starting_offset;
            continue;
        }

        if ((bytes_written + pfs_dirent->d_reclen) > nbytes)
            // dirents are aligned to the last full one
            break;

        // if this dirent breaks alignment, skip
        // dirents are count-aligned here, excess data is simply not written
        // if (Common::AlignUp(buffer_position, count) !=
        //     Common::AlignUp(buffer_position + pfs_dirent->d_reclen, count))
        //     break;

        // reclen for both is the same despite difference in var sizes, extra 0s are padded after
        // the name
        NormalDirectoryDirent normal_dirent{};
        normal_dirent.d_fileno = pfs_dirent->d_fileno;
        normal_dirent.d_reclen = pfs_dirent->d_reclen;
        normal_dirent.d_type = (pfs_dirent->d_type == 2) ? 8 : 4;
        normal_dirent.d_namlen = pfs_dirent->d_namlen;
        memcpy(normal_dirent.d_name, pfs_dirent->d_name, pfs_dirent->d_namlen);

        memcpy(static_cast<u8*>(buf) + bytes_written, &normal_dirent, normal_dirent.d_reclen);
        bytes_written += normal_dirent.d_reclen;
        buffer_position = bytes_written + starting_offset;
    }

    file_offset = (buffer_position >= this->dirent_cache_bin.size())
                      ? directory_size
                      : (file_offset + bytes_written);
    return bytes_written;
}
} // namespace Core::Directories