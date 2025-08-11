// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>

#include "common/types.h"
#include "enum.h"

namespace Common::FS {
enum class SeekOrigin : u32 {
    SetOrigin,       // Seeks from the start of the file.
    CurrentPosition, // Seeks from the current file pointer position.
    End,             // Seeks from the end of the file.
};
}

namespace Common::FS::Native {

namespace fs = std::filesystem;

[[nodiscard]] bool Open(const fs::path& path, int flags, int mode);
[[nodiscard]] bool Close(const int fd);

[[nodiscard]] bool Unlink(const fs::path path);
[[nodiscard]] bool Flush(const int fd);
[[nodiscard]] bool Commit(const int fd);
[[nodiscard]] bool SetSize(const int fd, u64 size);
[[nodiscard]] u64 GetSize(const int fd);
[[nodiscard]] bool Seek(const int fd, s64 offset, SeekOrigin origin = SeekOrigin::SetOrigin);

[[nodiscard]] s64 Tell(int fd);
[[nodiscard]] s64 Write(int __fd, const void* __buf, size_t __n);
[[nodiscard]] s64 Read(int __fd, void* __buf, size_t __n);

[[nodiscard]] constexpr int ToSeekOrigin(SeekOrigin origin);
[[nodiscard]] u64 GetDirectorySize(const fs::path& path);

[[nodiscard]] bool Exists(const fs::path& path);
[[nodiscard]] bool Exists(const fs::path& path, std::error_code& ec) noexcept;

[[nodiscard]] bool IsDirectory(const fs::path& path);
[[nodiscard]] bool IsDirectory(const fs::path& path, std::error_code& ec) noexcept;

// 0777 to mimic default C++ mode (std::filesystem::perms::all)
bool CreateDirectory(const fs::path& path, int mode = 0777);
bool CreateDirectory(const fs::path& path, const fs::path& existing_path, int mode = 0777) = delete;
bool CreateDirectories(const fs::path& path, int mode = 0777);

[[nodiscard]] fs::path AbsolutePath(const fs::path& path);
[[nodiscard]] bool Remove(const fs::path& path);

[[nodiscard]] bool Open(const fs::path& path, std::error_code& ec, int flags, int mode) noexcept;
[[nodiscard]] bool Close(const int fd, std::error_code& ec) noexcept;
[[nodiscard]] bool Unlink(const fs::path path, std::error_code& ec) noexcept;
[[nodiscard]] bool Flush(const int fd, std::error_code& ec) noexcept;
[[nodiscard]] bool Commit(const int fd, std::error_code& ec) noexcept;
[[nodiscard]] bool SetSize(const int fd, std::error_code& ec, u64 size) noexcept;
[[nodiscard]] u64 GetSize(const int fd, std::error_code& ec) noexcept;
[[nodiscard]] bool Seek(const int fd, std::error_code& ec, s64 offset,
                        SeekOrigin origin = SeekOrigin::SetOrigin) noexcept;
[[nodiscard]] s64 Tell(int fd, std::error_code& ec) noexcept;
[[nodiscard]] s64 Write(int __fd, std::error_code& ec, const void* __buf, size_t __n) noexcept;
[[nodiscard]] s64 Read(int __fd, std::error_code& ec, void* __buf, size_t __n) noexcept;

bool CreateDirectory(const fs::path& path, std::error_code& ec, int mode = 0777) noexcept;
bool CreateDirectory(const fs::path& path, const fs::path& existing_path, std::error_code& ec,
                     int mode = 0777) = delete;
bool CreateDirectories(const fs::path& path, std::error_code& ec, int mode = 0777) noexcept;

[[nodiscard]] fs::path AbsolutePath(const fs::path& path, std::error_code& ec) noexcept;
[[nodiscard]] bool Remove(const fs::path& path, std::error_code& ec) noexcept;

} // namespace Common::FS::Native
