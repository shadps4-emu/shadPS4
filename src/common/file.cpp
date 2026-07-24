// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cerrno>
#include <vector>

#include "common/assert.h"
#include "common/file.h"
#include "common/logging/log.h"
#include "common/path_util.h"
#include "common/zar_fs.h"

namespace Common::FS {

namespace fs = std::filesystem;

namespace {

bool IsArchivePath(const fs::path& path) {
    return Zar::IsZarArchive(path) || Zar::IsZarInnerPath(path);
}

} // Anonymous namespace

struct File::Impl {
    IOFile host_file;
    std::unique_ptr<Zar::FileHandle> archive_file;
    bool archive_backed{};
};

File::File() : impl{std::make_unique<Impl>()} {}

File::File(const fs::path& path, FileAccessMode mode, FileType type, FileShareFlag flag) : File() {
    Open(path, mode, type, flag);
}

File::~File() = default;

File::File(File&& other) noexcept = default;

File& File::operator=(File&& other) noexcept = default;

int File::Open(const fs::path& path, FileAccessMode mode, FileType type, FileShareFlag flag) {
    Close();
    file_path = path;
    file_access_mode = mode;
    file_type = type;

    if (!Zar::IsZarInnerPath(path)) {
        return impl->host_file.Open(path, mode, type, flag);
    }

    impl->archive_backed = true;
    if (mode != FileAccessMode::Read) {
        LOG_ERROR(Common_Filesystem, "Cannot open file inside ZArchive for writing: {}",
                  PathToUTF8String(path));
        return EACCES;
    }

    impl->archive_file = Zar::OpenFile(path);
    if (!impl->archive_file) {
        LOG_ERROR(Common_Filesystem, "Failed to open file inside ZArchive: {}",
                  PathToUTF8String(path));
        return ENOENT;
    }
    return 0;
}

void File::Close() {
    if (!impl) {
        return;
    }
    impl->archive_file.reset();
    impl->host_file.Close();
    impl->archive_backed = false;
}

bool File::IsOpen() const {
    return impl && (impl->archive_file != nullptr || impl->host_file.IsOpen());
}

bool File::SupportsWrites() const {
    return IsOpen() && !impl->archive_backed;
}

bool File::IsHostFile() const {
    return IsOpen() && !impl->archive_backed;
}

void File::Unlink() {
    if (impl->archive_file) {
        return;
    }
    impl->host_file.Unlink();
}

uintptr_t File::GetFileMapping() {
    if (impl->archive_file) {
        const auto offset = impl->archive_file->GetOffset();
        const auto spill_path = Zar::MaterializeFile(*impl->archive_file, file_path);
        if (!spill_path || impl->host_file.Open(*spill_path, FileAccessMode::Read) != 0) {
            return 0;
        }
        impl->host_file.Seek(static_cast<s64>(offset));
        impl->archive_file.reset();
    }
    return impl->host_file.GetFileMapping();
}

bool File::Flush() const {
    return impl->archive_file ? true : impl->host_file.Flush();
}

bool File::Commit() const {
    return impl->archive_file ? true : impl->host_file.Commit();
}

bool File::SetSize(u64 size) const {
    return impl->archive_file ? false : impl->host_file.SetSize(size);
}

u64 File::GetSize() const {
    return impl->archive_file ? impl->archive_file->GetSize() : impl->host_file.GetSize();
}

bool File::Seek(s64 offset, SeekOrigin origin) const {
    if (!impl->archive_file) {
        return impl->host_file.Seek(offset, origin);
    }

    s64 base = 0;
    switch (origin) {
    case SeekOrigin::SetOrigin:
        break;
    case SeekOrigin::CurrentPosition:
        base = static_cast<s64>(impl->archive_file->GetOffset());
        break;
    case SeekOrigin::End:
        base = static_cast<s64>(impl->archive_file->GetSize());
        break;
    default:
        UNREACHABLE_MSG("Impossible SeekOrigin {}", static_cast<u32>(origin));
    }

    const s64 target = base + offset;
    if (target < 0) {
        errno = EINVAL;
        return false;
    }
    impl->archive_file->SetOffset(static_cast<u64>(target));
    return true;
}

s64 File::Tell() const {
    return impl->archive_file ? static_cast<s64>(impl->archive_file->GetOffset())
                              : impl->host_file.Tell();
}

std::optional<fs::file_time_type> File::GetLastWriteTime() const {
    return Common::FS::GetLastWriteTime(file_path);
}

size_t File::ReadBytes(void* data, size_t size) const {
    if (!IsOpen()) {
        return 0;
    }
    if (impl->archive_file) {
        return static_cast<size_t>(impl->archive_file->Read(data, size));
    }
    return impl->host_file.ReadRaw<u8>(data, size);
}

size_t File::WriteBytes(const void* data, size_t size) const {
    if (!IsOpen() || impl->archive_file) {
        return 0;
    }
    return impl->host_file.WriteRaw<u8>(data, size);
}

std::string File::ReadString(size_t length) const {
    std::vector<char> string_buffer(length);
    const auto chars_read = ReadSpan<char>(string_buffer);
    const auto string_size = chars_read != length ? chars_read : length;
    return std::string{string_buffer.data(), string_size};
}

bool Exists(const fs::path& path) {
    if (IsArchivePath(path)) {
        return Zar::Exists(path);
    }
    std::error_code ec;
    return fs::exists(path, ec);
}

bool IsDirectory(const fs::path& path) {
    if (IsArchivePath(path)) {
        return Zar::IsDirectory(path);
    }
    std::error_code ec;
    return fs::is_directory(path, ec);
}

bool IsRegularFile(const fs::path& path) {
    if (IsArchivePath(path)) {
        return Zar::IsRegularFile(path);
    }
    std::error_code ec;
    return fs::is_regular_file(path, ec);
}

u64 GetFileSize(const fs::path& path) {
    if (Zar::IsZarInnerPath(path)) {
        return Zar::GetFileSize(path);
    }
    std::error_code ec;
    const auto size = fs::file_size(path, ec);
    return ec ? 0 : size;
}

std::optional<fs::file_time_type> GetLastWriteTime(const fs::path& path) {
    if (IsArchivePath(path)) {
        return Zar::GetLastWriteTime(path);
    }
    std::error_code ec;
    const auto time = fs::last_write_time(path, ec);
    return ec ? std::nullopt : std::optional{time};
}

bool IterateDirectory(const fs::path& dir, const DirectoryEntryCallback& callback) {
    if (IsArchivePath(dir)) {
        return Zar::IterateDirectory(dir, callback);
    }

    std::error_code ec;
    fs::directory_iterator iterator{dir, ec};
    const fs::directory_iterator end;
    while (!ec && iterator != end) {
        const auto& entry = *iterator;
        std::error_code entry_ec;
        const bool is_directory = entry.is_directory(entry_ec);
        if (!entry_ec) {
            callback(entry.path(), !is_directory);
        }
        iterator.increment(ec);
    }
    return !ec;
}

bool CopyFile(const fs::path& src, const fs::path& dst) {
    if (Zar::IsZarInnerPath(src)) {
        return Zar::CopyFile(src, dst);
    }
    std::error_code ec;
    return fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
}

} // namespace Common::FS
