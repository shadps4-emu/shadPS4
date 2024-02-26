// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdio>
#include <filesystem>
#include <span>
#include <type_traits>

#include "common/concepts.h"
#include "common/types.h"

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

    /**
     * An IOFile is a lightweight wrapper on C Library file operations.
     * Automatically closes an open file on the destruction of an IOFile object.
     *
     * @param path Filesystem path
     * @param mode File access mode
     * @param type File type, default is BinaryFile. Use TextFile to open the file as a text file
     * @param flag (Windows only) File-share access flag, default is ShareReadOnly
     */
    explicit IOFile(const std::filesystem::path& path, FileAccessMode mode,
                    FileType type = FileType::BinaryFile,
                    FileShareFlag flag = FileShareFlag::ShareReadOnly);

    ~IOFile();

    IOFile(const IOFile&) = delete;
    IOFile& operator=(const IOFile&) = delete;

    IOFile(IOFile&& other) noexcept;
    IOFile& operator=(IOFile&& other) noexcept;

    /**
     * Gets the path of the file.
     *
     * @returns The path of the file.
     */
    std::filesystem::path GetPath() const {
        return file_path;
    }

    /**
     * Gets the access mode of the file.
     *
     * @returns The access mode of the file.
     */
    FileAccessMode GetAccessMode() const {
        return file_access_mode;
    }

    /**
     * Gets the type of the file.
     *
     * @returns The type of the file.
     */
    FileType GetType() const {
        return file_type;
    }

    /**
     * Opens a file at path with the specified file access mode.
     * This function behaves differently depending on the FileAccessMode.
     * These behaviors are documented in each enum value of FileAccessMode.
     *
     * @param path Filesystem path
     * @param mode File access mode
     * @param type File type, default is BinaryFile. Use TextFile to open the file as a text file
     * @param flag (Windows only) File-share access flag, default is ShareReadOnly
     */
    void Open(const std::filesystem::path& path, FileAccessMode mode,
              FileType type = FileType::BinaryFile,
              FileShareFlag flag = FileShareFlag::ShareReadOnly);

    /// Closes the file if it is opened.
    void Close();

    /**
     * Checks whether the file is open.
     * Use this to check whether the calls to Open() or Close() succeeded.
     *
     * @returns True if the file is open, false otherwise.
     */
    bool IsOpen() const {
        return file != nullptr;
    }

    /**
     * Helper function which deduces the value type of a contiguous STL container used in ReadSpan.
     * If T is not a contiguous container as defined by the concept IsContiguousContainer, this
     * calls ReadObject and T must be a trivially copyable object.
     *
     * See ReadSpan for more details if T is a contiguous container.
     * See ReadObject for more details if T is a trivially copyable object.
     *
     * @tparam T Contiguous container or trivially copyable object
     *
     * @param data Container of T::value_type data or reference to object
     *
     * @returns Count of T::value_type data or objects successfully read.
     */
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

    /**
     * Helper function which deduces the value type of a contiguous STL container used in WriteSpan.
     * If T is not a contiguous STL container as defined by the concept IsContiguousContainer, this
     * calls WriteObject and T must be a trivially copyable object.
     *
     * See WriteSpan for more details if T is a contiguous container.
     * See WriteObject for more details if T is a trivially copyable object.
     *
     * @tparam T Contiguous container or trivially copyable object
     *
     * @param data Container of T::value_type data or const reference to object
     *
     * @returns Count of T::value_type data or objects successfully written.
     */
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

    /**
     * Reads a span of T data from a file sequentially.
     * This function reads from the current position of the file pointer and
     * advances it by the (count of T * sizeof(T)) bytes successfully read.
     *
     * Failures occur when:
     * - The file is not open
     * - The opened file lacks read permissions
     * - Attempting to read beyond the end-of-file
     *
     * @tparam T Data type
     *
     * @param data Span of T data
     *
     * @returns Count of T data successfully read.
     */
    template <typename T>
    size_t ReadSpan(std::span<T> data) const {
        static_assert(std::is_trivially_copyable_v<T>, "Data type must be trivially copyable.");

        if (!IsOpen()) {
            return 0;
        }

        return ReadRaw<T>(data.data(), data.size());
    }

    /**
     * Reads data from a file sequentially to a provided memory address.
     * This function reads from the current position of the file pointer and
     * advances it by the (count of T * sizeof(T)) bytes successfully read.
     */
    template <typename T>
    size_t ReadRaw(void* data, size_t size) const {
        return std::fread(data, sizeof(T), size, file);
    }

    /**
     * Writes a span of T data to a file sequentially.
     * This function writes from the current position of the file pointer and
     * advances it by the (count of T * sizeof(T)) bytes successfully written.
     *
     * Failures occur when:
     * - The file is not open
     * - The opened file lacks write permissions
     *
     * @tparam T Data type
     *
     * @param data Span of T data
     *
     * @returns Count of T data successfully written.
     */
    template <typename T>
    size_t WriteSpan(std::span<const T> data) const {
        static_assert(std::is_trivially_copyable_v<T>, "Data type must be trivially copyable.");

        if (!IsOpen()) {
            return 0;
        }

        return std::fwrite(data.data(), sizeof(T), data.size(), file);
    }

    /**
     * Reads a T object from a file sequentially.
     * This function reads from the current position of the file pointer and
     * advances it by the sizeof(T) bytes successfully read.
     *
     * Failures occur when:
     * - The file is not open
     * - The opened file lacks read permissions
     * - Attempting to read beyond the end-of-file
     *
     * @tparam T Data type
     *
     * @param object Reference to object
     *
     * @returns True if the object is successfully read from the file, false otherwise.
     */
    template <typename T>
    bool ReadObject(T& object) const {
        static_assert(std::is_trivially_copyable_v<T>, "Data type must be trivially copyable.");
        static_assert(!std::is_pointer_v<T>, "T must not be a pointer to an object.");

        if (!IsOpen()) {
            return false;
        }

        return std::fread(&object, sizeof(T), 1, file) == 1;
    }

    /**
     * Writes a T object to a file sequentially.
     * This function writes from the current position of the file pointer and
     * advances it by the sizeof(T) bytes successfully written.
     *
     * Failures occur when:
     * - The file is not open
     * - The opened file lacks write permissions
     *
     * @tparam T Data type
     *
     * @param object Const reference to object
     *
     * @returns True if the object is successfully written to the file, false otherwise.
     */
    template <typename T>
    bool WriteObject(const T& object) const {
        static_assert(std::is_trivially_copyable_v<T>, "Data type must be trivially copyable.");
        static_assert(!std::is_pointer_v<T>, "T must not be a pointer to an object.");

        if (!IsOpen()) {
            return false;
        }

        return std::fwrite(&object, sizeof(T), 1, file) == 1;
    }

    /**
     * Specialized function to read a string of a given length from a file sequentially.
     * This function writes from the current position of the file pointer and
     * advances it by the number of characters successfully read.
     * The size of the returned string may not match length if not all bytes are successfully read.
     *
     * @param length Length of the string
     *
     * @returns A string read from the file.
     */
    std::string ReadString(size_t length) const;

    /**
     * Specialized function to write a string to a file sequentially.
     * This function writes from the current position of the file pointer and
     * advances it by the number of characters successfully written.
     *
     * @param string Span of const char backed std::string or std::string_view
     *
     * @returns Number of characters successfully written.
     */
    size_t WriteString(std::span<const char> string) const {
        return WriteSpan(string);
    }

    /**
     * Attempts to flush any unwritten buffered data into the file.
     *
     * @returns True if the flush was successful, false otherwise.
     */
    bool Flush() const;

    /**
     * Attempts to commit the file into the disk.
     * Note that this is an expensive operation as this forces the operating system to write
     * the contents of the file associated with the file descriptor into the disk.
     *
     * @returns True if the commit was successful, false otherwise.
     */
    bool Commit() const;

    /**
     * Resizes the file to a given size.
     * If the file is resized to a smaller size, the remainder of the file is discarded.
     * If the file is resized to a larger size, the new area appears as if zero-filled.
     *
     * Failures occur when:
     * - The file is not open
     *
     * @param size File size in bytes
     *
     * @returns True if the file resize succeeded, false otherwise.
     */
    bool SetSize(u64 size) const;

    /**
     * Gets the size of the file.
     *
     * Failures occur when:
     * - The file is not open
     *
     * @returns The file size in bytes of the file. Returns 0 on failure.
     */
    u64 GetSize() const;

    /**
     * Moves the current position of the file pointer with the specified offset and seek origin.
     *
     * @param offset Offset from seek origin
     * @param origin Seek origin
     *
     * @returns True if the file pointer has moved to the specified offset, false otherwise.
     */
    bool Seek(s64 offset, SeekOrigin origin = SeekOrigin::SetOrigin) const;

    /**
     * Gets the current position of the file pointer.
     *
     * @returns The current position of the file pointer.
     */
    s64 Tell() const;

private:
    std::filesystem::path file_path;
    FileAccessMode file_access_mode{};
    FileType file_type{};

    std::FILE* file = nullptr;
};

} // namespace Common::FS
