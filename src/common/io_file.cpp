// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <span>
#include <vector>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/error.h"
#include "common/io_file.h"
#include "common/logging/log.h"
#include "common/path_util.h"
#include "core/libraries/kernel/file_system.h"

#include "io_file.h"

namespace Common::FS {

namespace fs = std::filesystem;

IOFile::IOFile() = default;

IOFile::IOFile(IOFile&& other) noexcept {
    std::swap(file_path, other.file_path);
    std::swap(file_access_mode, other.file_access_mode);
    std::swap(file_descriptor, other.file_descriptor);
}

IOFile& IOFile::operator=(IOFile&& other) noexcept {
    std::swap(file_path, other.file_path);
    std::swap(file_access_mode, other.file_access_mode);
    std::swap(file_descriptor, other.file_descriptor);
    return *this;
}

void IOFile::Close() {
    if (!IsOpen()) {
        return;
    }

    errno = 0;

    if (!CloseImpl()) {
        const auto ec = std::error_code{errno, std::generic_category()};
        LOG_ERROR(Common_Filesystem, "Failed to close the file at path={}, ec_message={}",
                  PathToUTF8String(file_path), ec.message());
    }

    file_descriptor = 0;
}

void IOFile::Unlink() {
    if (!IsOpen()) {
        return;
    }

    if (UnlinkImpl())
        return;

    const auto ec = std::error_code{errno, std::generic_category()};
    LOG_ERROR(Common_Filesystem, "Failed to unlink the file at path={}, ec_message={}",
              PathToUTF8String(file_path), ec.message());
}

bool IOFile::Flush() const {
    if (!IsOpen()) {
        return false;
    }

    if (FlushImpl())
        return true;

    const auto ec = std::error_code{errno, std::generic_category()};
    LOG_ERROR(Common_Filesystem, "Failed to flush the file at path={}, ec_message={}",
              PathToUTF8String(file_path), ec.message());

    return false;
}

bool IOFile::Commit() const {
    if (!IsOpen()) {
        return false;
    }

    errno = 0;
    if (CommitImpl())
        return true;

    const auto ec = std::error_code{errno, std::generic_category()};
    LOG_ERROR(Common_Filesystem, "Failed to commit the file at path={}, ec_message={}",
              PathToUTF8String(file_path), ec.message());

    return false;
}

bool IOFile::SetSize(u64 size) const {
    if (!IsOpen()) {
        return false;
    }

    errno = 0;
    if (SetSizeImpl(size))
        return true;

    const auto ec = std::error_code{errno, std::generic_category()};
    LOG_ERROR(Common_Filesystem, "Failed to resize the file at path={}, size={}, ec_message={}",
              PathToUTF8String(file_path), size, ec.message());

    return false;
}

u64 IOFile::GetSize() const {
    if (!IsOpen()) {
        return 0;
    }

    std::error_code ec;

    const u64 file_size = GetSizeImpl();
    // TODO: catch error (if any)

    return file_size;
}
uintptr_t IOFile::GetFileMapping() {
    return GetFileMappingImpl();
}
bool IOFile::Seek(s64 offset, SeekOrigin origin) const {
    if (!IsOpen()) {
        return false;
    }

    errno = 0;
    const bool seek_result = SeekImpl(offset, origin);

    if (!seek_result) {
        const auto ec = std::error_code{errno, std::generic_category()};
        LOG_ERROR(Common_Filesystem,
                  "Failed to seek the file at path={}, offset={}, origin={}, ec_message={}",
                  PathToUTF8String(file_path), offset, static_cast<u32>(origin), ec.message());
    }

    return seek_result;
}

s64 IOFile::Tell() const {
    if (!IsOpen()) {
        return 0;
    }

    errno = 0;

    return TellImpl();
}

std::string IOFile::ReadString(size_t length) const {
    std::vector<char> string_buffer(length);

    const auto chars_read = ReadSpan<char>(string_buffer);
    const auto string_size = chars_read != length ? chars_read : length;

    return std::string{string_buffer.data(), string_size};
}

size_t IOFile::WriteString(std::span<const char> string) const {
    return WriteSpan(string);
}

u64 GetDirectorySize(const std::filesystem::path& path) {
    if (!fs::exists(path)) {
        return 0;
    }

    return _GetDirectorySizeImpl(path);
}

} // namespace Common::FS