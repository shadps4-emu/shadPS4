// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <string>
#include <unordered_map>
#include <vector>

#include <miniz.h>

#include "video_core/cache_archive.h"

namespace Storage::Detail {

bool CompactArchive(const std::filesystem::path& source, const std::filesystem::path& destination) {
    std::error_code ec;
    std::filesystem::remove(destination, ec);

    mz_zip_archive reader{};
    if (!mz_zip_reader_init_file(&reader, source.string().c_str(), 0) ||
        !mz_zip_validate_archive(&reader, 0)) {
        mz_zip_reader_end(&reader);
        return false;
    }

    const auto num_files = mz_zip_reader_get_num_files(&reader);
    std::vector<std::string> names(num_files);
    std::unordered_map<std::string, mz_uint> last_entries;
    last_entries.reserve(num_files);
    for (mz_uint index = 0; index < num_files; ++index) {
        mz_zip_archive_file_stat stat{};
        if (!mz_zip_reader_file_stat(&reader, index, &stat)) {
            mz_zip_reader_end(&reader);
            return false;
        }
        names[index] = stat.m_filename;
        last_entries[names[index]] = index;
    }

    mz_zip_archive writer{};
    if (!mz_zip_writer_init_file(&writer, destination.string().c_str(), 0)) {
        mz_zip_reader_end(&reader);
        std::filesystem::remove(destination, ec);
        return false;
    }

    bool succeeded = true;
    for (mz_uint index = 0; index < num_files; ++index) {
        if (last_entries.at(names[index]) == index &&
            !mz_zip_writer_add_from_zip_reader(&writer, &reader, index)) {
            succeeded = false;
            break;
        }
    }
    if (succeeded) {
        succeeded = mz_zip_writer_finalize_archive(&writer);
    }

    mz_zip_writer_end(&writer);
    mz_zip_reader_end(&reader);
    if (!succeeded) {
        std::filesystem::remove(destination, ec);
    }
    return succeeded;
}

} // namespace Storage::Detail
