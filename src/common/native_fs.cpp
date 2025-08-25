// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <errno.h>
#include <unistd.h>

#include "native_fs.h"

namespace Common::FS::Native {

namespace fs = std::filesystem;

s64 GetSize(const fs::path path) {
    std::error_code ec{};
    bool exists = GetSize(path, ec);

    if (!ec)
        return exists;

    throw fs::filesystem_error("GetSize(): ", path,
                               std::error_code{errno, std::generic_category()});
}

bool Exists(const fs::path path) {
    std::error_code ec{};
    bool exists = Exists(path, ec);

    if (!ec)
        return exists;

    throw fs::filesystem_error("Exists(): ", path, std::error_code{errno, std::generic_category()});
}

bool IsDirectory(const fs::path path) {
    std::error_code ec{};
    bool is_directory = IsDirectory(path, ec);

    if (!ec)
        return is_directory;

    throw fs::filesystem_error("IsDirectory(): ", path,
                               std::error_code{errno, std::generic_category()});
}

fs::path AbsolutePath(const fs::path path) {
    std::error_code ec{};
    fs::path resolved_path = AbsolutePath(path, ec);

    if (!ec)
        return resolved_path;

    throw fs::filesystem_error("AbsolutePath(): ", path, ec);
}

bool Remove(const fs::path path) {
    std::error_code ec{};
    bool file_removed = Remove(path, ec);

    if (!ec)
        return file_removed;

    throw fs::filesystem_error("Remove(): ", path, ec);
}

bool CreateDirectory(const fs::path path, int mode) {
    std::error_code ec{};
    bool directory_created = CreateDirectory(path, ec, mode);

    if (!ec)
        return directory_created;

    throw fs::filesystem_error("CreateDirectory(): ", path,
                               std::error_code{errno, std::generic_category()});
}

bool CreateDirectories(const fs::path path, int mode) {
    std::error_code ec{};
    bool directories_created = CreateDirectories(path, ec, mode);

    if (!ec)
        return directories_created;

    throw fs::filesystem_error("CreateDirectories(): ", path,
                               std::error_code{errno, std::generic_category()});
}

} // namespace Common::FS::Native
