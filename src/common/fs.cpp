// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "fs.h"
#include "io_file.h"

#include "logging/log.h"
#include "path_util.h"

namespace Common::FS {

namespace fs = std::filesystem;

// File Operations

bool NewFile(const fs::path& path, u64 size) {
    if (!ValidatePath(path)) {
        // LOG_ERROR(Common_Filesystem, "Input path is not valid, path={}", PathToUTF8String(path));
        return false;
    }

    if (!Exists(path.parent_path())) {
        // LOG_ERROR(Common_Filesystem, "Parent directory of path={} does not exist",
        //           PathToUTF8String(path));
        return false;
    }

    if (Exists(path)) {
        // LOG_ERROR(Common_Filesystem, "Filesystem object at path={} exists",
        // PathToUTF8String(path));
        return false;
    }

    IOFile io_file{path, FileAccessMode::Write};

    if (!io_file.IsOpen()) {
        // LOG_ERROR(Common_Filesystem, "Failed to create a file at path={}",
        // PathToUTF8String(path));
        return false;
    }

    if (!io_file.SetSize(size)) {
        // LOG_ERROR(Common_Filesystem, "Failed to resize the file at path={} to size={}",
          //        PathToUTF8String(path), size);
                  return false;
    }

    io_file.Close();

    // LOG_DEBUG(Common_Filesystem, "Successfully created a file at path={} with size={}",
    //           PathToUTF8String(path), size);

    return true;
}

bool RemoveFile(const fs::path& path) {
    if (!ValidatePath(path)) {
        //   LOG_ERROR(Common_Filesystem, "Input path is not valid, path={}",
        //   PathToUTF8String(path));
        return false;
    }

    if (!Exists(path)) {
        // LOG_DEBUG(Common_Filesystem, "Filesystem object at path={} does not exist",
        //          PathToUTF8String(path));
        return true;
    }

    if (!IsFile(path)) {
        // LOG_ERROR(Common_Filesystem, "Filesystem object at path={} is not a file",
        //           PathToUTF8String(path));
        return false;
    }

    std::error_code ec;

    fs::remove(path, ec);

    if (ec) {
        // LOG_ERROR(Common_Filesystem, "Failed to remove the file at path={}, ec_message={}",
        //          PathToUTF8String(path), ec.message());
        return false;
    }

    // LOG_DEBUG(Common_Filesystem, "Successfully removed the file at path={}",
    //           PathToUTF8String(path));

    return true;
}

bool RenameFile(const fs::path& old_path, const fs::path& new_path) {
    if (!ValidatePath(old_path) || !ValidatePath(new_path)) {
        // LOG_ERROR(Common_Filesystem,
        //           "One or both input path(s) is not valid, old_path={}, new_path={}",
        //          PathToUTF8String(old_path), PathToUTF8String(new_path));
        return false;
    }

    if (!Exists(old_path)) {
        // LOG_ERROR(Common_Filesystem, "Filesystem object at old_path={} does not exist",
        //           PathToUTF8String(old_path));
        return false;
    }

    if (!IsFile(old_path)) {
        // LOG_ERROR(Common_Filesystem, "Filesystem object at old_path={} is not a file",
        //           PathToUTF8String(old_path));
        return false;
    }

    if (Exists(new_path)) {
        // LOG_ERROR(Common_Filesystem, "Filesystem object at new_path={} exists",
        //           PathToUTF8String(new_path));
        return false;
    }

    std::error_code ec;

    fs::rename(old_path, new_path, ec);

    if (ec) {
        // LOG_ERROR(Common_Filesystem,
        //          "Failed to rename the file from old_path={} to new_path={}, ec_message={}",
        //          PathToUTF8String(old_path), PathToUTF8String(new_path), ec.message());
        return false;
    }

    // LOG_DEBUG(Common_Filesystem, "Successfully renamed the file from old_path={} to new_path={}",
    //           PathToUTF8String(old_path), PathToUTF8String(new_path));

    return true;
}

std::shared_ptr<IOFile> FileOpen(const fs::path& path, FileAccessMode mode, FileType type,
                                 FileShareFlag flag) {
    if (!ValidatePath(path)) {
        // LOG_ERROR(Common_Filesystem, "Input path is not valid, path={}", PathToUTF8String(path));
        return nullptr;
    }

    if (Exists(path) && !IsFile(path)) {
        // LOG_ERROR(Common_Filesystem,
        //           "Filesystem object at path={} exists and is not a regular file",
        //           PathToUTF8String(path));
        return nullptr;
    }

    auto io_file = std::make_shared<IOFile>(path, mode, type, flag);

    if (!io_file->IsOpen()) {
        io_file.reset();

        // LOG_ERROR(Common_Filesystem,
        //          "Failed to open the file at path={} with mode={}, type={}, flag={}",
        //          PathToUTF8String(path), mode, type, flag);

        return nullptr;
    }

    // LOG_DEBUG(Common_Filesystem,
    //           "Successfully opened the file at path={} with mode={}, type={}, flag={}",
    //           PathToUTF8String(path), mode, type, flag);

    return io_file;
}

// Directory Operations

bool CreateDir(const fs::path& path) {
    if (!ValidatePath(path)) {
        //  LOG_ERROR(Common_Filesystem, "Input path is not valid, path={}",
        //  PathToUTF8String(path));
        return false;
    }

    if (!Exists(path.parent_path())) {
        // LOG_ERROR(Common_Filesystem, "Parent directory of path={} does not exist",
        //           PathToUTF8String(path));
        return false;
    }

    if (IsDir(path)) {
        // LOG_DEBUG(Common_Filesystem, "Filesystem object at path={} exists and is a directory",
        //          PathToUTF8String(path));
        return true;
    }

    std::error_code ec;

    fs::create_directory(path, ec);

    if (ec) {
        // LOG_ERROR(Common_Filesystem, "Failed to create the directory at path={}, ec_message={}",
        //           PathToUTF8String(path), ec.message());
        return false;
    }

    // LOG_DEBUG(Common_Filesystem, "Successfully created the directory at path={}",
    //          PathToUTF8String(path));

    return true;
}

bool CreateDirs(const fs::path& path) {
    if (!ValidatePath(path)) {
        // LOG_ERROR(Common_Filesystem, "Input path is not valid, path={}", PathToUTF8String(path));
        return false;
    }

    if (IsDir(path)) {
        // LOG_DEBUG(Common_Filesystem, "Filesystem object at path={} exists and is a directory",
        //           PathToUTF8String(path));
        return true;
    }

    std::error_code ec;

    fs::create_directories(path, ec);

    if (ec) {
        // LOG_ERROR(Common_Filesystem, "Failed to create the directories at path={},
        // ec_message={}",
        //           PathToUTF8String(path), ec.message());
        return false;
    }

    // LOG_DEBUG(Common_Filesystem, "Successfully created the directories at path={}",
    //           PathToUTF8String(path));

    return true;
}

bool CreateParentDir(const fs::path& path) {
    return CreateDir(path.parent_path());
}

bool CreateParentDirs(const fs::path& path) {
    return CreateDirs(path.parent_path());
}

bool RemoveDir(const fs::path& path) {
    if (!ValidatePath(path)) {
        // LOG_ERROR(Common_Filesystem, "Input path is not valid, path={}", PathToUTF8String(path));
        return false;
    }

    if (!Exists(path)) {
        // LOG_DEBUG(Common_Filesystem, "Filesystem object at path={} does not exist",
        //           PathToUTF8String(path));
        return true;
    }

    if (!IsDir(path)) {
        // LOG_ERROR(Common_Filesystem, "Filesystem object at path={} is not a directory",
        //             PathToUTF8String(path));
        return false;
    }

    std::error_code ec;

    fs::remove(path, ec);

    if (ec) {
        // LOG_ERROR(Common_Filesystem, "Failed to remove the directory at path={}, ec_message={}",
        //           PathToUTF8String(path), ec.message());
        return false;
    }

    // LOG_DEBUG(Common_Filesystem, "Successfully removed the directory at path={}",
    //           PathToUTF8String(path));

    return true;
}

bool RemoveDirRecursively(const fs::path& path) {
    if (!ValidatePath(path)) {
        // LOG_ERROR(Common_Filesystem, "Input path is not valid, path={}", PathToUTF8String(path));
        return false;
    }

    if (!Exists(path)) {
        // LOG_DEBUG(Common_Filesystem, "Filesystem object at path={} does not exist",
        //           PathToUTF8String(path));
        return true;
    }

    if (!IsDir(path)) {
        // LOG_ERROR(Common_Filesystem, "Filesystem object at path={} is not a directory",
        //           PathToUTF8String(path));
        return false;
    }

    std::error_code ec;

    fs::remove_all(path, ec);

    if (ec) {
        // LOG_ERROR(Common_Filesystem,
        //           "Failed to remove the directory and its contents at path={}, ec_message={}",
        //          PathToUTF8String(path), ec.message());
        return false;
    }

    // LOG_DEBUG(Common_Filesystem, "Successfully removed the directory and its contents at
    // path={}",
    //           PathToUTF8String(path));

    return true;
}

bool RemoveDirContentsRecursively(const fs::path& path) {
    if (!ValidatePath(path)) {
        //   LOG_ERROR(Common_Filesystem, "Input path is not valid, path={}",
        //   PathToUTF8String(path));
        return false;
    }

    if (!Exists(path)) {
        // LOG_DEBUG(Common_Filesystem, "Filesystem object at path={} does not exist",
        //            PathToUTF8String(path));
        return true;
    }

    if (!IsDir(path)) {
        //  LOG_ERROR(Common_Filesystem, "Filesystem object at path={} is not a directory",
        //           PathToUTF8String(path));
        return false;
    }

    std::error_code ec;

    // TODO (Morph): Replace this with recursive_directory_iterator once it's fixed in MSVC.
    for (const auto& entry : fs::directory_iterator(path, ec)) {
        if (ec) {
            //   LOG_ERROR(Common_Filesystem,
            //             "Failed to completely enumerate the directory at path={}, ec_message={}",
            //              PathToUTF8String(path), ec.message());
            break;
        }

        fs::remove(entry.path(), ec);

        if (ec) {
            // LOG_ERROR(Common_Filesystem,
            //           "Failed to remove the filesystem object at path={}, ec_message={}",
            //           PathToUTF8String(entry.path()), ec.message());
            break;
        }

        // TODO (Morph): Remove this when MSVC fixes recursive_directory_iterator.
        // recursive_directory_iterator throws an exception despite passing in a std::error_code.
        if (entry.status().type() == fs::file_type::directory) {
            return RemoveDirContentsRecursively(entry.path());
        }
    }

    if (ec) {
        // LOG_ERROR(Common_Filesystem,
        //          "Failed to remove all the contents of the directory at path={}, ec_message={}",
        //           PathToUTF8String(path), ec.message());
        return false;
    }

    // LOG_DEBUG(Common_Filesystem,
    //           "Successfully removed all the contents of the directory at path={}",
    //           PathToUTF8String(path));

    return true;
}

bool RenameDir(const fs::path& old_path, const fs::path& new_path) {
    if (!ValidatePath(old_path) || !ValidatePath(new_path)) {
        //  LOG_ERROR(Common_Filesystem,
        //            "One or both input path(s) is not valid, old_path={}, new_path={}",
        //            PathToUTF8String(old_path), PathToUTF8String(new_path));
        return false;
    }

    if (!Exists(old_path)) {
        // LOG_ERROR(Common_Filesystem, "Filesystem object at old_path={} does not exist",
        //           PathToUTF8String(old_path));
        return false;
    }

    if (!IsDir(old_path)) {
        //  LOG_ERROR(Common_Filesystem, "Filesystem object at old_path={} is not a directory",
        //            PathToUTF8String(old_path));
        return false;
    }

    if (Exists(new_path)) {
        // LOG_ERROR(Common_Filesystem, "Filesystem object at new_path={} exists",
        //         PathToUTF8String(new_path));
        return false;
    }

    std::error_code ec;

    fs::rename(old_path, new_path, ec);

    if (ec) {
        // LOG_ERROR(Common_Filesystem,
        //           "Failed to rename the file from old_path={} to new_path={}, ec_message={}",
        //           PathToUTF8String(old_path), PathToUTF8String(new_path), ec.message());
        return false;
    }

    // LOG_DEBUG(Common_Filesystem, "Successfully renamed the file from old_path={} to new_path={}",
    //          PathToUTF8String(old_path), PathToUTF8String(new_path));

    return true;
}

void IterateDirEntries(const std::filesystem::path& path, const DirEntryCallable& callback,
                       DirEntryFilter filter) {
    if (!ValidatePath(path)) {
       // LOG_ERROR(Common_Filesystem, "Input path is not valid, path={}", PathToUTF8String(path));
        return;
    }

    if (!Exists(path)) {
     //  LOG_ERROR(Common_Filesystem, "Filesystem object at path={} does not exist",
      //            PathToUTF8String(path));
        return;
    }

    if (!IsDir(path)) {
       // LOG_ERROR(Common_Filesystem, "Filesystem object at path={} is not a directory",
       //           PathToUTF8String(path));
        return;
    }

    bool callback_error = false;

    std::error_code ec;

    for (const auto& entry : fs::directory_iterator(path, ec)) {
        if (ec) {
            break;
        }

        if (True(filter & DirEntryFilter::File) &&
            entry.status().type() == fs::file_type::regular) {
            if (!callback(entry)) {
                callback_error = true;
                break;
            }
        }

        if (True(filter & DirEntryFilter::Directory) &&
            entry.status().type() == fs::file_type::directory) {
            if (!callback(entry)) {
                callback_error = true;
                break;
            }
        }
    }

    if (callback_error || ec) {
       // LOG_ERROR(Common_Filesystem,
       //           "Failed to visit all the directory entries of path={}, ec_message={}",
       //           PathToUTF8String(path), ec.message());
        return;
    }

    //LOG_DEBUG(Common_Filesystem, "Successfully visited all the directory entries of path={}",
    //          PathToUTF8String(path));
}

void IterateDirEntriesRecursively(const std::filesystem::path& path,
                                  const DirEntryCallable& callback, DirEntryFilter filter) {
    if (!ValidatePath(path)) {
       // LOG_ERROR(Common_Filesystem, "Input path is not valid, path={}", PathToUTF8String(path));
        return;
    }

    if (!Exists(path)) {
       // LOG_ERROR(Common_Filesystem, "Filesystem object at path={} does not exist",
          //        PathToUTF8String(path));
        return;
    }

    if (!IsDir(path)) {
       // LOG_ERROR(Common_Filesystem, "Filesystem object at path={} is not a directory",
              //    PathToUTF8String(path));
        return;
    }

    bool callback_error = false;

    std::error_code ec;

    // TODO (Morph): Replace this with recursive_directory_iterator once it's fixed in MSVC.
    for (const auto& entry : fs::directory_iterator(path, ec)) {
        if (ec) {
            break;
        }

        if (True(filter & DirEntryFilter::File) &&
            entry.status().type() == fs::file_type::regular) {
            if (!callback(entry)) {
                callback_error = true;
                break;
            }
        }

        if (True(filter & DirEntryFilter::Directory) &&
            entry.status().type() == fs::file_type::directory) {
            if (!callback(entry)) {
                callback_error = true;
                break;
            }
        }

        // TODO (Morph): Remove this when MSVC fixes recursive_directory_iterator.
        // recursive_directory_iterator throws an exception despite passing in a std::error_code.
        if (entry.status().type() == fs::file_type::directory) {
            IterateDirEntriesRecursively(entry.path(), callback, filter);
        }
    }

    if (callback_error || ec) {
        //LOG_ERROR(Common_Filesystem,
        //          "Failed to visit all the directory entries of path={}, ec_message={}",
       //           PathToUTF8String(path), ec.message());
        return;
    }

    //LOG_DEBUG(Common_Filesystem, "Successfully visited all the directory entries of path={}",
     //         PathToUTF8String(path));
}

// Generic Filesystem Operations

bool Exists(const fs::path& path) {
    std::error_code ec;
#ifdef ANDROID
    if (Android::IsContentUri(path)) {
        return Android::Exists(path);
    } else {
        return fs::exists(path, ec);
    }
#else
    return fs::exists(path, ec);
#endif
}

bool IsFile(const fs::path& path) {
    std::error_code ec;
#ifdef ANDROID
    if (Android::IsContentUri(path)) {
        return !Android::IsDirectory(path);
    } else {
        return fs::is_regular_file(path, ec);
    }
#else
    return fs::is_regular_file(path, ec);
#endif
}

bool IsDir(const fs::path& path) {
    std::error_code ec;
#ifdef ANDROID
    if (Android::IsContentUri(path)) {
        return Android::IsDirectory(path);
    } else {
        return fs::is_directory(path, ec);
    }
#else
    return fs::is_directory(path, ec);
#endif
}

fs::path GetCurrentDir() {
    std::error_code ec;

    const auto current_path = fs::current_path(ec);

    if (ec) {
    //    LOG_ERROR(Common_Filesystem, "Failed to get the current path, ec_message={}", ec.message());
        return {};
    }

    return current_path;
}

bool SetCurrentDir(const fs::path& path) {
    std::error_code ec;

    fs::current_path(path, ec);

    if (ec) {
     //   LOG_ERROR(Common_Filesystem, "Failed to set the current path to path={}, ec_message={}",
     //             PathToUTF8String(path), ec.message());
        return false;
    }

    return true;
}

fs::file_type GetEntryType(const fs::path& path) {
    std::error_code ec;

    const auto file_status = fs::status(path, ec);

    if (ec) {
    //    LOG_ERROR(Common_Filesystem, "Failed to retrieve the entry type of path={}, ec_message={}",
     //             PathToUTF8String(path), ec.message());
        return fs::file_type::not_found;
    }

    return file_status.type();
}

#if 0
u64 GetSize(const fs::path& path) {
#ifdef ANDROID
    if (Android::IsContentUri(path)) {
        return Android::GetSize(path);
    }
#endif

    std::error_code ec;

    const auto file_size = fs::file_size(path, ec);

    if (ec) {
        LOG_ERROR(Common_Filesystem, "Failed to retrieve the file size of path={}, ec_message={}",
                  PathToUTF8String(path), ec.message());
        return 0;
    }

    return file_size;
}

u64 GetFreeSpaceSize(const fs::path& path) {
    std::error_code ec;

    const auto space_info = fs::space(path, ec);

    if (ec) {
        LOG_ERROR(Common_Filesystem,
                  "Failed to retrieve the available free space of path={}, ec_message={}",
                  PathToUTF8String(path), ec.message());
        return 0;
    }

    return space_info.free;
}

u64 GetTotalSpaceSize(const fs::path& path) {
    std::error_code ec;

    const auto space_info = fs::space(path, ec);

    if (ec) {
        LOG_ERROR(Common_Filesystem,
                  "Failed to retrieve the total capacity of path={}, ec_message={}",
                  PathToUTF8String(path), ec.message());
        return 0;
    }

    return space_info.capacity;
}
#endif
} // namespace Common::FS
