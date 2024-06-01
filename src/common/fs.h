// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <memory>
#include <functional>
#include "io_file.h"

namespace Common::FS {

#define DECLARE_ENUM_FLAG_OPERATORS(type)                                                          \
    [[nodiscard]] constexpr type operator|(type a, type b) noexcept {                              \
        using T = std::underlying_type_t<type>;                                                    \
        return static_cast<type>(static_cast<T>(a) | static_cast<T>(b));                           \
    }                                                                                              \
    [[nodiscard]] constexpr type operator&(type a, type b) noexcept {                              \
        using T = std::underlying_type_t<type>;                                                    \
        return static_cast<type>(static_cast<T>(a) & static_cast<T>(b));                           \
    }                                                                                              \
    [[nodiscard]] constexpr type operator^(type a, type b) noexcept {                              \
        using T = std::underlying_type_t<type>;                                                    \
        return static_cast<type>(static_cast<T>(a) ^ static_cast<T>(b));                           \
    }                                                                                              \
    [[nodiscard]] constexpr type operator<<(type a, type b) noexcept {                             \
        using T = std::underlying_type_t<type>;                                                    \
        return static_cast<type>(static_cast<T>(a) << static_cast<T>(b));                          \
    }                                                                                              \
    [[nodiscard]] constexpr type operator>>(type a, type b) noexcept {                             \
        using T = std::underlying_type_t<type>;                                                    \
        return static_cast<type>(static_cast<T>(a) >> static_cast<T>(b));                          \
    }                                                                                              \
    constexpr type& operator|=(type& a, type b) noexcept {                                         \
        a = a | b;                                                                                 \
        return a;                                                                                  \
    }                                                                                              \
    constexpr type& operator&=(type& a, type b) noexcept {                                         \
        a = a & b;                                                                                 \
        return a;                                                                                  \
    }                                                                                              \
    constexpr type& operator^=(type& a, type b) noexcept {                                         \
        a = a ^ b;                                                                                 \
        return a;                                                                                  \
    }                                                                                              \
    constexpr type& operator<<=(type& a, type b) noexcept {                                        \
        a = a << b;                                                                                \
        return a;                                                                                  \
    }                                                                                              \
    constexpr type& operator>>=(type& a, type b) noexcept {                                        \
        a = a >> b;                                                                                \
        return a;                                                                                  \
    }                                                                                              \
    [[nodiscard]] constexpr type operator~(type key) noexcept {                                    \
        using T = std::underlying_type_t<type>;                                                    \
        return static_cast<type>(~static_cast<T>(key));                                            \
    }                                                                                              \
    [[nodiscard]] constexpr bool True(type key) noexcept {                                         \
        using T = std::underlying_type_t<type>;                                                    \
        return static_cast<T>(key) != 0;                                                           \
    }                                                                                              \
    [[nodiscard]] constexpr bool False(type key) noexcept {                                        \
        using T = std::underlying_type_t<type>;                                                    \
        return static_cast<T>(key) == 0;                                                           \
    }

class IOFile;

enum class DirEntryFilter {
    File = 1 << 0,
    Directory = 1 << 1,
    All = File | Directory,
};
DECLARE_ENUM_FLAG_OPERATORS(DirEntryFilter);

/**
 * A callback function which takes in the path of a directory entry.
 *
 * @param path The path of a directory entry
 *
 * @returns A boolean value.
 *          Return true to indicate whether the callback is successful, false otherwise.
 */
using DirEntryCallable = std::function<bool(const std::filesystem::directory_entry& entry)>;


// File Operations

/**
 * Creates a new file at path with the specified size.
 *
 * Failures occur when:
 * - Input path is not valid
 * - The input path's parent directory does not exist
 * - Filesystem object at path exists
 * - Filesystem at path is read only
 *
 * @param path Filesystem path
 * @param size File size
 *
 * @returns True if the file creation succeeds, false otherwise.
 */
[[nodiscard]] bool NewFile(const std::filesystem::path& path, u64 size = 0);

#ifdef _WIN32
template <typename Path>
[[nodiscard]] bool NewFile(const Path& path, u64 size = 0) {
    if constexpr (IsChar<typename Path::value_type>) {
        return NewFile(ToU8String(path), size);
    } else {
        return NewFile(std::filesystem::path{path}, size);
    }
}
#endif

/**
 * Removes a file at path.
 *
 * Failures occur when:
 * - Input path is not valid
 * - Filesystem object at path is not a regular file
 * - Filesystem at path is read only
 *
 * @param path Filesystem path
 *
 * @returns True if file removal succeeds or file does not exist, false otherwise.
 */
bool RemoveFile(const std::filesystem::path& path);

#ifdef _WIN32
template <typename Path>
bool RemoveFile(const Path& path) {
    if constexpr (IsChar<typename Path::value_type>) {
        return RemoveFile(ToU8String(path));
    } else {
        return RemoveFile(std::filesystem::path{path});
    }
}
#endif

/**
 * Renames a file from old_path to new_path.
 *
 * Failures occur when:
 * - One or both input path(s) is not valid
 * - Filesystem object at old_path does not exist
 * - Filesystem object at old_path is not a regular file
 * - Filesystem object at new_path exists
 * - Filesystem at either path is read only
 *
 * @param old_path Old filesystem path
 * @param new_path New filesystem path
 *
 * @returns True if file rename succeeds, false otherwise.
 */
[[nodiscard]] bool RenameFile(const std::filesystem::path& old_path,
                              const std::filesystem::path& new_path);

#ifdef _WIN32
template <typename Path1, typename Path2>
[[nodiscard]] bool RenameFile(const Path1& old_path, const Path2& new_path) {
    using ValueType1 = typename Path1::value_type;
    using ValueType2 = typename Path2::value_type;
    if constexpr (IsChar<ValueType1> && IsChar<ValueType2>) {
        return RenameFile(ToU8String(old_path), ToU8String(new_path));
    } else if constexpr (IsChar<ValueType1> && !IsChar<ValueType2>) {
        return RenameFile(ToU8String(old_path), new_path);
    } else if constexpr (!IsChar<ValueType1> && IsChar<ValueType2>) {
        return RenameFile(old_path, ToU8String(new_path));
    } else {
        return RenameFile(std::filesystem::path{old_path}, std::filesystem::path{new_path});
    }
}
#endif

/**
 * Opens a file at path with the specified file access mode.
 * This function behaves differently depending on the FileAccessMode.
 * These behaviors are documented in each enum value of FileAccessMode.
 *
 * Failures occur when:
 * - Input path is not valid
 * - Filesystem object at path exists and is not a regular file
 * - The file is not open
 *
 * @param path Filesystem path
 * @param mode File access mode
 * @param type File type, default is BinaryFile. Use TextFile to open the file as a text file
 * @param flag (Windows only) File-share access flag, default is ShareReadOnly
 *
 * @returns A shared pointer to the opened file. Returns nullptr on failure.
 */
[[nodiscard]] std::shared_ptr<IOFile> FileOpen(const std::filesystem::path& path,
                                               FileAccessMode mode,
                                               FileType type = FileType::BinaryFile,
                                               FileShareFlag flag = FileShareFlag::ShareReadOnly);

#ifdef _WIN32
template <typename Path>
[[nodiscard]] std::shared_ptr<IOFile> FileOpen(const Path& path, FileAccessMode mode,
                                               FileType type = FileType::BinaryFile,
                                               FileShareFlag flag = FileShareFlag::ShareReadOnly) {
    if constexpr (IsChar<typename Path::value_type>) {
        return FileOpen(ToU8String(path), mode, type, flag);
    } else {
        return FileOpen(std::filesystem::path{path}, mode, type, flag);
    }
}
#endif

// Directory Operations

/**
 * Creates a directory at path.
 * Note that this function will *always* assume that the input path is a directory. For example,
 * if the input path is /path/to/directory/file.txt, it will create a directory called "file.txt".
 * If you intend to create the parent directory of a file, use CreateParentDir instead.
 *
 * Failures occur when:
 * - Input path is not valid
 * - The input path's parent directory does not exist
 * - Filesystem at path is read only
 *
 * @param path Filesystem path
 *
 * @returns True if directory creation succeeds or directory already exists, false otherwise.
 */
[[nodiscard]] bool CreateDir(const std::filesystem::path& path);

#ifdef _WIN32
template <typename Path>
[[nodiscard]] bool CreateDir(const Path& path) {
    if constexpr (IsChar<typename Path::value_type>) {
        return CreateDir(ToU8String(path));
    } else {
        return CreateDir(std::filesystem::path{path});
    }
}
#endif

/**
 * Recursively creates a directory at path.
 * Note that this function will *always* assume that the input path is a directory. For example,
 * if the input path is /path/to/directory/file.txt, it will create a directory called "file.txt".
 * If you intend to create the parent directory of a file, use CreateParentDirs instead.
 * Unlike CreateDir, this creates all of input path's parent directories if they do not exist.
 *
 * Failures occur when:
 * - Input path is not valid
 * - Filesystem at path is read only
 *
 * @param path Filesystem path
 *
 * @returns True if directory creation succeeds or directory already exists, false otherwise.
 */
[[nodiscard]] bool CreateDirs(const std::filesystem::path& path);

#ifdef _WIN32
template <typename Path>
[[nodiscard]] bool CreateDirs(const Path& path) {
    if constexpr (IsChar<typename Path::value_type>) {
        return CreateDirs(ToU8String(path));
    } else {
        return CreateDirs(std::filesystem::path{path});
    }
}
#endif

/**
 * Creates the parent directory of a given path.
 * This function calls CreateDir(path.parent_path()), see CreateDir for more details.
 *
 * @param path Filesystem path
 *
 * @returns True if directory creation succeeds or directory already exists, false otherwise.
 */
[[nodiscard]] bool CreateParentDir(const std::filesystem::path& path);

#ifdef _WIN32
template <typename Path>
[[nodiscard]] bool CreateParentDir(const Path& path) {
    if constexpr (IsChar<typename Path::value_type>) {
        return CreateParentDir(ToU8String(path));
    } else {
        return CreateParentDir(std::filesystem::path{path});
    }
}
#endif

/**
 * Recursively creates the parent directory of a given path.
 * This function calls CreateDirs(path.parent_path()), see CreateDirs for more details.
 *
 * @param path Filesystem path
 *
 * @returns True if directory creation succeeds or directory already exists, false otherwise.
 */
[[nodiscard]] bool CreateParentDirs(const std::filesystem::path& path);

#ifdef _WIN32
template <typename Path>
[[nodiscard]] bool CreateParentDirs(const Path& path) {
    if constexpr (IsChar<typename Path::value_type>) {
        return CreateParentDirs(ToU8String(path));
    } else {
        return CreateParentDirs(std::filesystem::path{path});
    }
}
#endif

/**
 * Removes a directory at path.
 *
 * Failures occur when:
 * - Input path is not valid
 * - Filesystem object at path is not a directory
 * - The given directory is not empty
 * - Filesystem at path is read only
 *
 * @param path Filesystem path
 *
 * @returns True if directory removal succeeds or directory does not exist, false otherwise.
 */
bool RemoveDir(const std::filesystem::path& path);

#ifdef _WIN32
template <typename Path>
bool RemoveDir(const Path& path) {
    if constexpr (IsChar<typename Path::value_type>) {
        return RemoveDir(ToU8String(path));
    } else {
        return RemoveDir(std::filesystem::path{path});
    }
}
#endif

/**
 * Removes all the contents within the given directory and removes the directory itself.
 *
 * Failures occur when:
 * - Input path is not valid
 * - Filesystem object at path is not a directory
 * - Filesystem at path is read only
 *
 * @param path Filesystem path
 *
 * @returns True if the directory and all of its contents are removed successfully, false otherwise.
 */
bool RemoveDirRecursively(const std::filesystem::path& path);

#ifdef _WIN32
template <typename Path>
bool RemoveDirRecursively(const Path& path) {
    if constexpr (IsChar<typename Path::value_type>) {
        return RemoveDirRecursively(ToU8String(path));
    } else {
        return RemoveDirRecursively(std::filesystem::path{path});
    }
}
#endif

/**
 * Removes all the contents within the given directory without removing the directory itself.
 *
 * Failures occur when:
 * - Input path is not valid
 * - Filesystem object at path is not a directory
 * - Filesystem at path is read only
 *
 * @param path Filesystem path
 *
 * @returns True if all of the directory's contents are removed successfully, false otherwise.
 */
bool RemoveDirContentsRecursively(const std::filesystem::path& path);

#ifdef _WIN32
template <typename Path>
bool RemoveDirContentsRecursively(const Path& path) {
    if constexpr (IsChar<typename Path::value_type>) {
        return RemoveDirContentsRecursively(ToU8String(path));
    } else {
        return RemoveDirContentsRecursively(std::filesystem::path{path});
    }
}
#endif

/**
 * Renames a directory from old_path to new_path.
 *
 * Failures occur when:
 * - One or both input path(s) is not valid
 * - Filesystem object at old_path does not exist
 * - Filesystem object at old_path is not a directory
 * - Filesystem object at new_path exists
 * - Filesystem at either path is read only
 *
 * @param old_path Old filesystem path
 * @param new_path New filesystem path
 *
 * @returns True if directory rename succeeds, false otherwise.
 */
[[nodiscard]] bool RenameDir(const std::filesystem::path& old_path,
                             const std::filesystem::path& new_path);

#ifdef _WIN32
template <typename Path1, typename Path2>
[[nodiscard]] bool RenameDir(const Path1& old_path, const Path2& new_path) {
    using ValueType1 = typename Path1::value_type;
    using ValueType2 = typename Path2::value_type;
    if constexpr (IsChar<ValueType1> && IsChar<ValueType2>) {
        return RenameDir(ToU8String(old_path), ToU8String(new_path));
    } else if constexpr (IsChar<ValueType1> && !IsChar<ValueType2>) {
        return RenameDir(ToU8String(old_path), new_path);
    } else if constexpr (!IsChar<ValueType1> && IsChar<ValueType2>) {
        return RenameDir(old_path, ToU8String(new_path));
    } else {
        return RenameDir(std::filesystem::path{old_path}, std::filesystem::path{new_path});
    }
}
#endif

/**
 * Iterates over the directory entries of a given directory.
 * This does not iterate over the sub-directories of the given directory.
 * The DirEntryCallable callback is called for each visited directory entry.
 * A filter can be set to control which directory entries are visited based on their type.
 * By default, both files and directories are visited.
 * If the callback returns false or there is an error, the iteration is immediately halted.
 *
 * Failures occur when:
 * - Input path is not valid
 * - Filesystem object at path is not a directory
 *
 * @param path Filesystem path
 * @param callback Callback to be called for each visited directory entry
 * @param filter Directory entry type filter
 */
void IterateDirEntries(const std::filesystem::path& path, const DirEntryCallable& callback,
                       DirEntryFilter filter = DirEntryFilter::All);

#ifdef _WIN32
template <typename Path>
void IterateDirEntries(const Path& path, const DirEntryCallable& callback,
                       DirEntryFilter filter = DirEntryFilter::All) {
    if constexpr (IsChar<typename Path::value_type>) {
        IterateDirEntries(ToU8String(path), callback, filter);
    } else {
        IterateDirEntries(std::filesystem::path{path}, callback, filter);
    }
}
#endif

/**
 * Iterates over the directory entries of a given directory and its sub-directories.
 * The DirEntryCallable callback is called for each visited directory entry.
 * A filter can be set to control which directory entries are visited based on their type.
 * By default, both files and directories are visited.
 * If the callback returns false or there is an error, the iteration is immediately halted.
 *
 * Failures occur when:
 * - Input path is not valid
 * - Filesystem object at path does not exist
 * - Filesystem object at path is not a directory
 *
 * @param path Filesystem path
 * @param callback Callback to be called for each visited directory entry
 * @param filter Directory entry type filter
 */
void IterateDirEntriesRecursively(const std::filesystem::path& path,
                                  const DirEntryCallable& callback,
                                  DirEntryFilter filter = DirEntryFilter::All);

#ifdef _WIN32
template <typename Path>
void IterateDirEntriesRecursively(const Path& path, const DirEntryCallable& callback,
                                  DirEntryFilter filter = DirEntryFilter::All) {
    if constexpr (IsChar<typename Path::value_type>) {
        IterateDirEntriesRecursively(ToU8String(path), callback, filter);
    } else {
        IterateDirEntriesRecursively(std::filesystem::path{path}, callback, filter);
    }
}
#endif

// Generic Filesystem Operations

/**
 * Returns whether a filesystem object at path exists.
 *
 * @param path Filesystem path
 *
 * @returns True if a filesystem object at path exists, false otherwise.
 */
[[nodiscard]] bool Exists(const std::filesystem::path& path);

#ifdef _WIN32
template <typename Path>
[[nodiscard]] bool Exists(const Path& path) {
    if constexpr (IsChar<typename Path::value_type>) {
        return Exists(ToU8String(path));
    } else {
        return Exists(std::filesystem::path{path});
    }
}
#endif

/**
 * Returns whether a filesystem object at path is a regular file.
 * A regular file is a file that stores text or binary data.
 * It is not a directory, symlink, FIFO, socket, block device, or character device.
 *
 * @param path Filesystem path
 *
 * @returns True if a filesystem object at path is a regular file, false otherwise.
 */
[[nodiscard]] bool IsFile(const std::filesystem::path& path);

#ifdef _WIN32
template <typename Path>
[[nodiscard]] bool IsFile(const Path& path) {
    if constexpr (IsChar<typename Path::value_type>) {
        return IsFile(ToU8String(path));
    } else {
        return IsFile(std::filesystem::path{path});
    }
}
#endif

/**
 * Returns whether a filesystem object at path is a directory.
 *
 * @param path Filesystem path
 *
 * @returns True if a filesystem object at path is a directory, false otherwise.
 */
[[nodiscard]] bool IsDir(const std::filesystem::path& path);

#ifdef _WIN32
template <typename Path>
[[nodiscard]] bool IsDir(const Path& path) {
    if constexpr (IsChar<typename Path::value_type>) {
        return IsDir(ToU8String(path));
    } else {
        return IsDir(std::filesystem::path{path});
    }
}
#endif

/**
 * Gets the current working directory.
 *
 * @returns The current working directory. Returns an empty path on failure.
 */
[[nodiscard]] std::filesystem::path GetCurrentDir();

/**
 * Sets the current working directory to path.
 *
 * @returns True if the current working directory is successfully set, false otherwise.
 */
[[nodiscard]] bool SetCurrentDir(const std::filesystem::path& path);

#ifdef _WIN32
template <typename Path>
[[nodiscard]] bool SetCurrentDir(const Path& path) {
    if constexpr (IsChar<typename Path::value_type>) {
        return SetCurrentDir(ToU8String(path));
    } else {
        return SetCurrentDir(std::filesystem::path{path});
    }
}
#endif

/**
 * Gets the entry type of the filesystem object at path.
 *
 * @param path Filesystem path
 *
 * @returns The entry type of the filesystem object. Returns file_type::not_found on failure.
 */
[[nodiscard]] std::filesystem::file_type GetEntryType(const std::filesystem::path& path);

#ifdef _WIN32
template <typename Path>
[[nodiscard]] std::filesystem::file_type GetEntryType(const Path& path) {
    if constexpr (IsChar<typename Path::value_type>) {
        return GetEntryType(ToU8String(path));
    } else {
        return GetEntryType(std::filesystem::path{path});
    }
}
#endif

#if 0
/**
 * Gets the size of the filesystem object at path.
 *
 * @param path Filesystem path
 *
 * @returns The size in bytes of the filesystem object. Returns 0 on failure.
 */
[[nodiscard]] u64 GetSize(const std::filesystem::path& path);

#ifdef _WIN32
template <typename Path>
[[nodiscard]] u64 GetSize(const Path& path) {
    if constexpr (IsChar<typename Path::value_type>) {
        return GetSize(ToU8String(path));
    } else {
        return GetSize(std::filesystem::path{path});
    }
}
#endif


/**
 * Gets the free space size of the filesystem at path.
 *
 * @param path Filesystem path
 *
 * @returns The free space size in bytes of the filesystem at path. Returns 0 on failure.
 */
[[nodiscard]] u64 GetFreeSpaceSize(const std::filesystem::path& path);

#ifdef _WIN32
template <typename Path>
[[nodiscard]] u64 GetFreeSpaceSize(const Path& path) {
    if constexpr (IsChar<typename Path::value_type>) {
        return GetFreeSpaceSize(ToU8String(path));
    } else {
        return GetFreeSpaceSize(std::filesystem::path{path});
    }
}
#endif

/**
 * Gets the total capacity of the filesystem at path.
 *
 * @param path Filesystem path
 *
 * @returns The total capacity in bytes of the filesystem at path. Returns 0 on failure.
 */
[[nodiscard]] u64 GetTotalSpaceSize(const std::filesystem::path& path);

#ifdef _WIN32
template <typename Path>
[[nodiscard]] u64 GetTotalSpaceSize(const Path& path) {
    if constexpr (IsChar<typename Path::value_type>) {
        return GetTotalSpaceSize(ToU8String(path));
    } else {
        return GetTotalSpaceSize(std::filesystem::path{path});
    }
}
#endif

#endif

} // namespace Common::FS
