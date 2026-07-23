// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <type_traits>

#include "common/concepts.h"
#include "common/io_file.h"
#include "common/types.h"

namespace Common::FS {

/// File wrapper for paths that may refer to either a host file or an archive entry.
class File final {
public:
    File();
    explicit File(const std::filesystem::path& path, FileAccessMode mode,
                  FileType type = FileType::BinaryFile,
                  FileShareFlag flag = FileShareFlag::ShareReadOnly);
    ~File();

    File(const File&) = delete;
    File& operator=(const File&) = delete;
    File(File&& other) noexcept;
    File& operator=(File&& other) noexcept;

    int Open(const std::filesystem::path& path, FileAccessMode mode,
             FileType type = FileType::BinaryFile,
             FileShareFlag flag = FileShareFlag::ShareReadOnly);
    void Close();

    bool IsOpen() const;
    bool IsHostFile() const;
    bool SupportsWrites() const;

    bool IsWriteOnly() const {
        return file_access_mode == FileAccessMode::Append ||
               file_access_mode == FileAccessMode::Write;
    }

    const std::filesystem::path& GetPath() const {
        return file_path;
    }

    FileAccessMode GetAccessMode() const {
        return file_access_mode;
    }

    FileType GetType() const {
        return file_type;
    }

    void Unlink();
    uintptr_t GetFileMapping();
    bool Flush() const;
    bool Commit() const;
    bool SetSize(u64 size) const;
    u64 GetSize() const;
    bool Seek(s64 offset, SeekOrigin origin = SeekOrigin::SetOrigin) const;
    s64 Tell() const;
    std::optional<std::filesystem::file_time_type> GetLastWriteTime() const;

    template <typename T>
    size_t Read(T& data) const {
        if constexpr (IsContiguousContainer<T>) {
            using ContiguousType = typename T::value_type;
            static_assert(std::is_trivially_copyable_v<ContiguousType>,
                          "Data type must be trivially copyable.");
            return ReadSpan<ContiguousType>(data);
        } else {
            return ReadObject(data) ? 1 : 0;
        }
    }

    template <typename T>
    size_t Write(const T& data) const {
        if constexpr (IsContiguousContainer<T>) {
            using ContiguousType = typename T::value_type;
            static_assert(std::is_trivially_copyable_v<ContiguousType>,
                          "Data type must be trivially copyable.");
            return WriteSpan<ContiguousType>(data);
        } else {
            static_assert(std::is_trivially_copyable_v<T>, "Data type must be trivially copyable.");
            return WriteObject(data) ? 1 : 0;
        }
    }

    template <typename T>
    size_t ReadSpan(std::span<T> data) const {
        static_assert(std::is_trivially_copyable_v<T>, "Data type must be trivially copyable.");
        return ReadRaw<T>(data.data(), data.size());
    }

    template <typename T>
    size_t ReadRaw(void* data, size_t size) const {
        return ReadBytes(data, size * sizeof(T)) / sizeof(T);
    }

    template <typename T>
    size_t WriteSpan(std::span<const T> data) const {
        static_assert(std::is_trivially_copyable_v<T>, "Data type must be trivially copyable.");
        return WriteRaw<T>(data.data(), data.size());
    }

    template <typename T>
    bool ReadObject(T& object) const {
        static_assert(std::is_trivially_copyable_v<T>, "Data type must be trivially copyable.");
        static_assert(!std::is_pointer_v<T>, "T must not be a pointer to an object.");
        return ReadBytes(&object, sizeof(T)) == sizeof(T);
    }

    template <typename T>
    size_t WriteRaw(const void* data, size_t size) const {
        return WriteBytes(data, size * sizeof(T)) / sizeof(T);
    }

    template <typename T>
    bool WriteObject(const T& object) const {
        static_assert(std::is_trivially_copyable_v<T>, "Data type must be trivially copyable.");
        static_assert(!std::is_pointer_v<T>, "T must not be a pointer to an object.");
        return WriteBytes(&object, sizeof(T)) == sizeof(T);
    }

    std::string ReadString(size_t length) const;

    size_t WriteString(std::span<const char> string) const {
        return WriteSpan(string);
    }

private:
    struct Impl;

    size_t ReadBytes(void* data, size_t size) const;
    size_t WriteBytes(const void* data, size_t size) const;

    std::unique_ptr<Impl> impl;
    std::filesystem::path file_path;
    FileAccessMode file_access_mode{};
    FileType file_type{};
};

bool Exists(const std::filesystem::path& path);
bool IsDirectory(const std::filesystem::path& path);
bool IsRegularFile(const std::filesystem::path& path);
u64 GetFileSize(const std::filesystem::path& path);
std::optional<std::filesystem::file_time_type> GetLastWriteTime(const std::filesystem::path& path);

using DirectoryEntryCallback =
    std::function<void(const std::filesystem::path& entry_path, bool is_file)>;

bool IterateDirectory(const std::filesystem::path& dir, const DirectoryEntryCallback& callback);
bool CopyFile(const std::filesystem::path& src, const std::filesystem::path& dst);

} // namespace Common::FS
