// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "common/types.h"

namespace Common::FS::Zar {

/// Filesystem access for ZArchive games. Inner paths pass through the archive path, for example
/// "/games/CUSA01234.zar/sce_sys/param.sfo".

class Archive;

/// Returns whether path is an existing host ZArchive.
bool IsZarArchive(const std::filesystem::path& path);

/// Returns whether path identifies an entry within a host ZArchive.
bool IsZarInnerPath(const std::filesystem::path& path);

/// Returns a loose overlay path beside a game directory or ZArchive.
std::filesystem::path GetLooseOverlayPath(const std::filesystem::path& game_path,
                                          std::string_view suffix);

/// Searches for <game_id>.zar and returns its eboot.bin path.
std::optional<std::filesystem::path> FindGameByID(const std::filesystem::path& dir,
                                                  const std::string& game_id, int max_depth);

/// Filesystem queries for archive paths. An archive itself is a directory.
bool Exists(const std::filesystem::path& path);
bool IsDirectory(const std::filesystem::path& path);
bool IsRegularFile(const std::filesystem::path& path);
u64 GetFileSize(const std::filesystem::path& path);
std::optional<std::filesystem::file_time_type> GetLastWriteTime(const std::filesystem::path& path);

using DirectoryEntryCallback =
    std::function<void(const std::filesystem::path& entry_path, bool is_file)>;

/// Iterates an archive directory. Returns false when dir is not a directory.
bool IterateDirectory(const std::filesystem::path& dir, const DirectoryEntryCallback& callback);

/// Read handle for a file within an archive.
class FileHandle {
public:
    FileHandle(std::shared_ptr<Archive> archive, u32 node, u64 size);
    ~FileHandle();

    /// Reads at the current offset and returns the byte count.
    u64 Read(void* dest, u64 length);

    u64 GetSize() const {
        return size;
    }

    u64 GetOffset() const {
        return offset;
    }

    void SetOffset(u64 new_offset) {
        offset = new_offset;
    }

    const std::filesystem::path& GetArchivePath() const;

private:
    std::shared_ptr<Archive> archive;
    u32 node;
    u64 size;
    u64 offset = 0;
};

/// Opens an archive entry, returning nullptr when it cannot be resolved.
std::unique_ptr<FileHandle> OpenFile(const std::filesystem::path& path);

/// Copies an archive entry to a host destination, replacing it if necessary.
bool CopyFile(const std::filesystem::path& src, const std::filesystem::path& dst);

/// Materializes an archive entry as a host file and returns its temporary path.
std::optional<std::filesystem::path> MaterializeFile(FileHandle& file,
                                                     const std::filesystem::path& source_path);

/// Returns the directory used when archive entries require host-file semantics.
std::filesystem::path GetSpillDirectory();

/// Deletes stale spill files from previous runs.
void CleanupSpillFiles();

/// Drops cached readers without invalidating existing file handles.
void ClearCache();

} // namespace Common::FS::Zar
