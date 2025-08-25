// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <span>
#include <type_traits>

#include "common/assert.h"
#include "common/concepts.h"
#include "common/native_fs.h"
#include "common/types.h"
#include "enum.h"

namespace Common::FS {

namespace NativeFS = Common::FS::Native;

enum class FileAccessMode : int {
    /**
     * If the file at path exists, it opens the file for reading.
     * If the file at path does not exist, it fails to open the file.
     * Target mode: "r"
     */
    Read = 1 << 0,
    /**
     * If the file at path exists, it is opened in its original state.
     * If the file at path does not exist, it creates and opens a new empty file for writing.
     * Target mode: "w"
     */
    Write = 1 << 1,
    /**
     * If the file at path exists, it opens the file for appending.
     * If the file at path does not exist, it creates and opens a new empty file for writing.
     * Reading is not supported. All data is written at the end, file pointer is ignored.
     * Target mode: "a"
     */
    Append = 1 << 2,

    /**
     * If the file at path exists, it opens the file for reading and writing.
     * If the file at path does not exist, it fails to open the file.
     * Target mode: "r+"
     */
    ReadExtended = 1 << 3,
    /**
     * If the file at path exists, it opens the file in its original state for reading and writing.
     * If the file at path does not exist, it creates and opens a new empty file for writing.
     * Target mode: "w+"
     */
    WriteExtended = 1 << 4,
    /**
     * If the file at path exists, it opens the file for both reading and appending.
     * If the file at path does not exist, it creates and opens a new empty file.
     * All data is written at the end, file pointer is ignored.
     * Target mode: "a+"
     */
    AppendExtended = 1 << 5
};
DECLARE_ENUM_FLAG_OPERATORS(FileAccessMode);

int AccessModeToPOSIX(FileAccessMode flags, bool truncate);
int AccessModeOrbisToPOSIX(int flags);

class IOFile final {
public:
    IOFile();

    // `truncate` is a separate flag, that is implied when accessing files with streams
    // This is not the case with native calls, desire to do so must be stated explicitly
    // To conform with current stream-like convention, files are truncated by default
    explicit IOFile(const std::string& path, FileAccessMode flags, bool truncate = true) {
        Open(path, flags, truncate);
    }
    explicit IOFile(std::string_view path, FileAccessMode flags, bool truncate = true) {
        Open(path, flags, truncate);
    }
    explicit IOFile(const std::filesystem::path& path, FileAccessMode flags, bool truncate = true) {
        Open(path, flags, truncate);
    }

    // All flags must be converted to Linux-native
    explicit IOFile(const std::string& path, int flags, int mode = 0644) {
        Open(path, flags, mode);
    }

    ~IOFile() {
        Close();
    }

    IOFile(const IOFile&) = delete;
    IOFile& operator=(const IOFile&) = delete;

    IOFile(IOFile&& other) noexcept;
    IOFile& operator=(IOFile&& other) noexcept;

    // Simplified access, convert to stream-equivalent modes at own discretion
    int Open(const std::filesystem::path& path, FileAccessMode flags, bool truncate = true,
             int mode = 0644);
    int Open(const std::filesystem::path& path, int flags, int mode = 0644);

    void Close();

    void Unlink() const;

    bool Flush() const;
    bool Commit() const;

    bool SetSize(u64 size) const;
    u64 GetSize() const;

    bool Seek(s64 offset, SeekOrigin origin = SeekOrigin::SetOrigin) const;
    s64 Tell() const;

    std::string ReadString(size_t length) const;
    size_t WriteString(std::span<const char> string) const;

    std::filesystem::path GetPath() const {
        return file_path;
    }

    int GetAccessMode() const {
        return file_access_mode;
    }

    int GetAccessPermissions() const {
        return file_access_permissions;
    }

    uintptr_t GetNativeFileDescriptor() const {
        return file_descriptor;
    }

    bool IsOpen() const {
        if (-1 == file_descriptor)
            return false;

        return NativeFS::IsOpen(this->file_descriptor);
    }

    static size_t WriteBytes(const std::filesystem::path path, const auto data) {
        IOFile out(path, FileAccessMode::Write, true);
        return out.Write(data);
    }

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

        if (!IsOpen()) {
            return 0;
        }

        return ReadRaw<T>(data.data(), data.size());
    }

    template <typename T>
    size_t ReadRaw(void* data, size_t size) const {
        // Ignored in current context, errno is preserved and valid
        std::error_code _;
        return NativeFS::Read(file_descriptor, _, data, sizeof(T) * size);
    }

    template <typename T>
    size_t WriteSpan(std::span<const T> data) const {
        static_assert(std::is_trivially_copyable_v<T>, "Data type must be trivially copyable.");

        if (!IsOpen()) {
            return 0;
        }

        // Ignored in current context, errno is preserved and valid
        std::error_code _;
        return NativeFS::Write(file_descriptor, _, data.data(), sizeof(T) * data.size());
    }

    template <typename T>
    bool ReadObject(T& object) const {
        static_assert(std::is_trivially_copyable_v<T>, "Data type must be trivially copyable.");
        static_assert(!std::is_pointer_v<T>, "T must not be a pointer to an object.");

        if (!IsOpen()) {
            return false;
        }

        // Ignored in current context, errno is preserved and valid
        std::error_code _;
        return NativeFS::Read(file_descriptor, _, &object, sizeof(T)) == sizeof(T);
    }

    template <typename T>
    size_t WriteRaw(const void* data, size_t size) const {
        // Ignored in current context, errno is preserved and valid
        std::error_code _;
        return NativeFS::Write(file_descriptor, _, data, sizeof(T) * size);
    }

    template <typename T>
    bool WriteObject(const T& object) const {
        static_assert(std::is_trivially_copyable_v<T>, "Data type must be trivially copyable.");
        static_assert(!std::is_pointer_v<T>, "T must not be a pointer to an object.");

        if (!IsOpen()) {
            return false;
        }

        // Ignored in current context, errno is preserved and valid
        std::error_code _;
        return NativeFS::Write(file_descriptor, _, &object, sizeof(T)) == sizeof(T);
    }

private:
    std::filesystem::path file_path{};
    int file_access_mode = 0;
    int file_access_permissions = 0;

#ifdef _WIN32
    HANDLE file_descriptor = -1;
#else
    int file_descriptor = -1;
#endif
};

} // namespace Common::FS
