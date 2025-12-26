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

NormalDirectory::NormalDirectory(std::string_view guest_directory)
    : guest_directory(guest_directory) {
    RebuildDirents();
}

s64 NormalDirectory::read(void* buf, u64 nbytes) {
    RebuildDirents();

    // data is contiguous. read goes like any regular file would: start at offset, read n bytes
    // output is always aligned up to 512 bytes with 0s
    // offset - classic. however at the end of read any unused (exceeding dirent buffer size) buffer
    // space will be left untouched
    // reclen always sums up to end of current alignment

    s64 bytes_available = this->dirent_cache_bin.size() - file_offset;
    if (bytes_available <= 0)
        return 0;
    bytes_available = std::min<s64>(bytes_available, static_cast<s64>(nbytes));

    // data
    memcpy(buf, this->dirent_cache_bin.data() + file_offset, bytes_available);

    file_offset += bytes_available;
    return bytes_available;
}

s32 NormalDirectory::fstat(Libraries::Kernel::OrbisKernelStat* stat) {
    stat->st_mode = 0000777u | 0040000u;
    stat->st_size = directory_size;
    stat->st_blksize = 0x8000;
    stat->st_blocks = 8;
    return ORBIS_OK;
}

s64 NormalDirectory::getdents(void* buf, u64 nbytes, s64* basep) {
    RebuildDirents();

    if (basep)
        *basep = file_offset;

    // same as others, we just don't need a variable
    if (file_offset >= directory_size)
        return 0;

    s64 bytes_written = 0;
    s64 working_offset = file_offset;
    s64 dirent_buffer_offset = 0;
    s64 aligned_count = Common::AlignDown(nbytes, 512);

    const u8* dirent_buffer = this->dirent_cache_bin.data();
    while (dirent_buffer_offset < this->dirent_cache_bin.size()) {
        const u8* normal_dirent_ptr = dirent_buffer + dirent_buffer_offset;
        const NormalDirectoryDirent* normal_dirent =
            reinterpret_cast<const NormalDirectoryDirent*>(normal_dirent_ptr);
        auto d_reclen = normal_dirent->d_reclen;

        // bad, incomplete or OOB entry
        if (normal_dirent->d_namlen == 0)
            break;

        if (working_offset >= d_reclen) {
            dirent_buffer_offset += d_reclen;
            working_offset -= d_reclen;
            continue;
        }

        if ((bytes_written + d_reclen) > aligned_count)
            // dirents are aligned to the last full one
            break;

        memcpy(static_cast<u8*>(buf) + bytes_written, normal_dirent_ptr + working_offset,
               d_reclen - working_offset);
        bytes_written += d_reclen - working_offset;
        dirent_buffer_offset += d_reclen;
        working_offset = 0;
    }

    file_offset += bytes_written;
    return bytes_written;
}

void NormalDirectory::RebuildDirents() {
    // regenerate only when target wants to read contents again
    // no reason for testing - read is always raw and dirents get processed on the go
    if (previous_file_offset == file_offset)
        return;
    previous_file_offset = file_offset;

    constexpr u32 dirent_meta_size =
        sizeof(NormalDirectoryDirent::d_fileno) + sizeof(NormalDirectoryDirent::d_type) +
        sizeof(NormalDirectoryDirent::d_namlen) + sizeof(NormalDirectoryDirent::d_reclen);

    u64 next_ceiling = 0;
    u64 dirent_offset = 0;
    u64 last_reclen_offset = 4;
    dirent_cache_bin.clear();
    dirent_cache_bin.reserve(512);

    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();

    mnt->IterateDirectory(
        guest_directory, [this, &next_ceiling, &dirent_offset, &last_reclen_offset](
                             const std::filesystem::path& ent_path, const bool ent_is_file) {
            NormalDirectoryDirent tmp{};
            std::string leaf(ent_path.filename().string());

            // prepare dirent
            tmp.d_fileno = BaseDirectory::next_fileno();
            tmp.d_namlen = leaf.size();
            strncpy(tmp.d_name, leaf.data(), tmp.d_namlen + 1);
            tmp.d_type = (ent_is_file ? 0100000 : 0040000) >> 12;
            tmp.d_reclen = Common::AlignUp(dirent_meta_size + tmp.d_namlen + 1, 4);

            // next element may break 512 byte alignment
            if (tmp.d_reclen + dirent_offset > next_ceiling) {
                // align previous dirent's size to the current ceiling
                *reinterpret_cast<u16*>(static_cast<u8*>(dirent_cache_bin.data()) +
                                        last_reclen_offset) += next_ceiling - dirent_offset;
                // set writing pointer to the aligned start position (current ceiling)
                dirent_offset = next_ceiling;
                // move the ceiling up and zero-out the buffer
                next_ceiling += 512;
                dirent_cache_bin.resize(next_ceiling);
                std::fill(dirent_cache_bin.begin() + dirent_offset,
                          dirent_cache_bin.begin() + next_ceiling, 0);
            }

            // current dirent's reclen position
            last_reclen_offset = dirent_offset + 4;
            memcpy(dirent_cache_bin.data() + dirent_offset, &tmp, tmp.d_reclen);
            dirent_offset += tmp.d_reclen;
        });

    // last reclen, as before
    *reinterpret_cast<u16*>(static_cast<u8*>(dirent_cache_bin.data()) + last_reclen_offset) +=
        next_ceiling - dirent_offset;

    // i have no idea if this is the case, but lseek returns size aligned to 512
    directory_size = next_ceiling;
}

} // namespace Core::Directories