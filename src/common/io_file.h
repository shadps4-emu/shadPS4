// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdio>
#include <filesystem>
#include <span>
#include <type_traits>

#include "common/concepts.h"
#include "common/types.h"
#include "enum.h"

namespace Common::FS {

enum class FileAccessMode {
    /**
     * If the file at path exists, it opens the file for reading.
     * If the file at path does not exist, it fails to open the file.
     */
    Read = 1 << 0,
    /**
     * If the file at path exists, the existing contents of the file are erased.
     * The empty file is then opened for writing.
     * If the file at path does not exist, it creates and opens a new empty file for writing.
     */
    Write = 1 << 1,
    /**
     * If the file at path exists, it opens the file for reading and writing.
     * If the file at path does not exist, it fails to open the file.
     */
    ReadWrite = Read | Write,
    /**
     * If the file at path exists, it opens the file for appending.
     * If the file at path does not exist, it creates and opens a new empty file for appending.
     */
    Append = 1 << 2,
    /**
     * If the file at path exists, it opens the file for both reading and appending.
     * If the file at path does not exist, it creates and opens a new empty file for both
     * reading and appending.
     */
    ReadAppend = Read | Append,
};
DECLARE_ENUM_FLAG_OPERATORS(FileAccessMode);

enum class FileType {
    BinaryFile,
    TextFile,
};

enum class FileShareFlag {
    ShareNone,      // Provides exclusive access to the file.
    ShareReadOnly,  // Provides read only shared access to the file.
    ShareWriteOnly, // Provides write only shared access to the file.
    ShareReadWrite, // Provides read and write shared access to the file.
};

enum class SeekOrigin : u32 {
    SetOrigin,       // Seeks from the start of the file.
    CurrentPosition, // Seeks from the current file pointer position.
    End,             // Seeks from the end of the file.
};

class IOFile final {
public:
    IOFile();

    explicit IOFile(const std::string& path, FileAccessMode mode,
                    FileType type = FileType::BinaryFile,
                    FileShareFlag flag = FileShareFlag::ShareReadOnly);

    explicit IOFile(std::string_view path, FileAccessMode mode,
                    FileType type = FileType::BinaryFile,
                    FileShareFlag flag = FileShareFlag::ShareReadOnly);
    explicit IOFile(const std::filesystem::path& path, FileAccessMode mode,
                    FileType type = FileType::BinaryFile,
                    FileShareFlag flag = FileShareFlag::ShareReadOnly);

    ~IOFile();

    IOFile(const IOFile&) = delete;
    IOFile& operator=(const IOFile&) = delete;

    IOFile(IOFile&& other) noexcept;
    IOFile& operator=(IOFile&& other) noexcept;

    std::filesystem::path GetPath() const {
        return file_path;
    }

    FileAccessMode GetAccessMode() const {
        return file_access_mode;
    }

    FileType GetType() const {
        return file_type;
    }

    bool IsOpen() const {
        return file != nullptr;
    }

    uintptr_t GetFileMapping();

    int Open(const std::filesystem::path& path, FileAccessMode mode,
             FileType type = FileType::BinaryFile,
             FileShareFlag flag = FileShareFlag::ShareReadOnly);
    void Close();

    void Unlink();

    bool Flush() const;
    bool Commit() const;

    bool SetSize(u64 size) const;
    u64 GetSize() const;

    bool Seek(s64 offset, SeekOrigin origin = SeekOrigin::SetOrigin) const;
    s64 Tell() const;

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
        return std::fread(data, sizeof(T), size, file);
    }

    template <typename T>
    size_t WriteSpan(std::span<const T> data) const {
        static_assert(std::is_trivially_copyable_v<T>, "Data type must be trivially copyable.");

        if (!IsOpen()) {
            return 0;
        }

        return std::fwrite(data.data(), sizeof(T), data.size(), file);
    }

    template <typename T>
    bool ReadObject(T& object) const {
        static_assert(std::is_trivially_copyable_v<T>, "Data type must be trivially copyable.");
        static_assert(!std::is_pointer_v<T>, "T must not be a pointer to an object.");

        if (!IsOpen()) {
            return false;
        }

        return std::fread(&object, sizeof(T), 1, file) == 1;
    }

    template <typename T>
    size_t WriteRaw(const void* data, size_t size) const {
        auto bytes = std::fwrite(data, sizeof(T), size, file);
        std::fflush(file);
        return bytes;
    }

    template <typename T>
    bool WriteObject(const T& object) const {
        static_assert(std::is_trivially_copyable_v<T>, "Data type must be trivially copyable.");
        static_assert(!std::is_pointer_v<T>, "T must not be a pointer to an object.");

        if (!IsOpen()) {
            return false;
        }

        return std::fwrite(&object, sizeof(T), 1, file) == 1;
    }

    std::string ReadString(size_t length) const;

    size_t WriteString(std::span<const char> string) const {
        return WriteSpan(string);
    }

    static size_t WriteBytes(const std::filesystem::path path, const auto& data) {
        IOFile out(path, FileAccessMode::Write);
        return out.Write(data);
    }
    std::FILE* file = nullptr;

private:
    std::filesystem::path file_path;
    FileAccessMode file_access_mode{};
    FileType file_type{};

    uintptr_t file_mapping = 0;
};

u64 GetDirectorySize(const std::filesystem::path& path);

} // namespace Common::FS
