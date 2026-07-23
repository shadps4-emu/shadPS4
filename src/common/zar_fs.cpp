// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <atomic>
#include <cctype>
#include <fstream>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <zarchive/zarchivereader.h>

#include "common/logging/log.h"
#include "common/path_util.h"
#include "common/zar_fs.h"

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

namespace Common::FS::Zar {

namespace fs = std::filesystem;

// Associates a reader with its host path. ReadFromFile serializes access internally.
class Archive {
public:
    Archive(fs::path path_, std::unique_ptr<ZArchiveReader> reader_)
        : path{std::move(path_)}, reader{std::move(reader_)} {}

    const fs::path path;
    const std::unique_ptr<ZArchiveReader> reader;
};

namespace {

std::mutex g_archive_mutex;
struct CachedArchive {
    std::shared_ptr<Archive> archive;
    u64 last_use;
};

constexpr size_t max_cached_archives = 8;
std::map<fs::path, CachedArchive> g_archive_cache;
std::set<fs::path> g_failed_archives;
u64 g_archive_cache_tick{};

bool HasZarExtension(const fs::path& path) {
    const auto extension = PathToUTF8String(path.extension());
    if (extension.size() != 4) {
        return false;
    }
    static constexpr std::string_view zar_ext = ".zar";
    return std::ranges::equal(extension, zar_ext, [](char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) == b;
    });
}

std::shared_ptr<Archive> OpenArchive(const fs::path& archive_path) {
    std::scoped_lock lock{g_archive_mutex};
    if (const auto it = g_archive_cache.find(archive_path); it != g_archive_cache.end()) {
        it->second.last_use = ++g_archive_cache_tick;
        return it->second.archive;
    }
    if (g_failed_archives.contains(archive_path)) {
        return nullptr;
    }
    std::unique_ptr<ZArchiveReader> reader{ZArchiveReader::OpenFromFile(archive_path)};
    if (!reader) {
        LOG_ERROR(Common_Filesystem, "Failed to open ZArchive at {}",
                  PathToUTF8String(archive_path));
        g_failed_archives.insert(archive_path);
        return nullptr;
    }
    auto archive = std::make_shared<Archive>(archive_path, std::move(reader));
    g_archive_cache.emplace(archive_path,
                            CachedArchive{.archive = archive, .last_use = ++g_archive_cache_tick});
    if (g_archive_cache.size() > max_cached_archives) {
        const auto oldest = std::min_element(g_archive_cache.begin(), g_archive_cache.end(),
                                             [](const auto& lhs, const auto& rhs) {
                                                 return lhs.second.last_use < rhs.second.last_use;
                                             });
        g_archive_cache.erase(oldest);
    }
    return archive;
}

struct SplitPath {
    std::shared_ptr<Archive> archive;
    std::string inner; // '/'-separated path inside the archive, empty for the root
};

struct LocatedPath {
    fs::path archive_path;
    std::string inner;
};

std::optional<LocatedPath> Locate(const fs::path& path) {
    const auto normalized = path.lexically_normal();
    fs::path prefix;
    std::string inner;
    bool found = false;
    for (const auto& part : normalized) {
        if (found) {
            const auto name = PathToUTF8String(part);
            if (name.empty() || name == ".") {
                continue;
            }
            if (!inner.empty()) {
                inner += '/';
            }
            inner += name;
        } else {
            prefix /= part;
            if (HasZarExtension(part)) {
                std::error_code ec;
                if (fs::is_regular_file(prefix, ec)) {
                    found = true;
                }
            }
        }
    }
    if (!found) {
        return std::nullopt;
    }
    return LocatedPath{std::move(prefix), std::move(inner)};
}

// Splits a host path of the form <host-prefix>.zar/<inner-path> into the containing
// archive and the archive-relative path. Returns std::nullopt if no component of the
// path is a .zar file on the host filesystem.
std::optional<SplitPath> Split(const fs::path& path) {
    auto located = Locate(path);
    if (!located) {
        return std::nullopt;
    }
    auto archive = OpenArchive(located->archive_path);
    if (!archive) {
        return std::nullopt;
    }
    return SplitPath{std::move(archive), std::move(located->inner)};
}

} // Anonymous namespace

bool IsZarArchive(const fs::path& path) {
    if (!HasZarExtension(path)) {
        return false;
    }
    std::error_code ec;
    return fs::is_regular_file(path, ec);
}

bool IsZarInnerPath(const fs::path& path) {
    const auto located = Locate(path);
    return located && !located->inner.empty();
}

fs::path GetLooseOverlayPath(const fs::path& game_path, std::string_view suffix) {
    auto overlay_path = game_path;
    if (IsZarArchive(overlay_path)) {
        overlay_path.replace_extension();
    }
    overlay_path += suffix;
    return overlay_path;
}

std::optional<fs::path> FindGameByID(const fs::path& dir, const std::string& game_id,
                                     int max_depth) {
    if (max_depth < 0) {
        return std::nullopt;
    }

    std::error_code ec;
    fs::directory_iterator iterator{dir, ec};
    const fs::directory_iterator end;
    while (!ec && iterator != end) {
        const auto& entry = *iterator;
        std::error_code entry_ec;
        if (entry.is_directory(entry_ec)) {
            if (auto found = FindGameByID(entry.path(), game_id, max_depth - 1)) {
                return found;
            }
        } else if (!entry_ec && entry.path().stem() == game_id && IsZarArchive(entry.path()) &&
                   Exists(entry.path() / "sce_sys" / "param.sfo") &&
                   Exists(entry.path() / "eboot.bin")) {
            return entry.path() / "eboot.bin";
        }
        iterator.increment(ec);
    }

    return std::nullopt;
}

bool Exists(const fs::path& path) {
    const auto split = Split(path);
    if (!split) {
        return false;
    }
    return split->archive->reader->LookUp(split->inner, true, true) != ZARCHIVE_INVALID_NODE;
}

bool IsDirectory(const fs::path& path) {
    const auto split = Split(path);
    if (!split) {
        return false;
    }
    const auto node = split->archive->reader->LookUp(split->inner, false, true);
    return node != ZARCHIVE_INVALID_NODE && split->archive->reader->IsDirectory(node);
}

bool IsRegularFile(const fs::path& path) {
    const auto split = Split(path);
    if (!split) {
        return false;
    }
    const auto node = split->archive->reader->LookUp(split->inner, true, false);
    return node != ZARCHIVE_INVALID_NODE && split->archive->reader->IsFile(node);
}

u64 GetFileSize(const fs::path& path) {
    const auto split = Split(path);
    if (!split) {
        return 0;
    }
    const auto node = split->archive->reader->LookUp(split->inner, true, false);
    if (node == ZARCHIVE_INVALID_NODE || !split->archive->reader->IsFile(node)) {
        return 0;
    }
    return split->archive->reader->GetFileSize(node);
}

std::optional<fs::file_time_type> GetLastWriteTime(const fs::path& path) {
    const auto split = Split(path);
    if (!split) {
        return std::nullopt;
    }
    // Entries inside an archive share the modification time of the archive itself.
    std::error_code ec;
    const auto time = fs::last_write_time(split->archive->path, ec);
    if (ec) {
        return std::nullopt;
    }
    return time;
}

bool IterateDirectory(const fs::path& dir, const DirectoryEntryCallback& callback) {
    const auto split = Split(dir);
    if (!split) {
        return false;
    }
    const auto& reader = split->archive->reader;
    const auto node = reader->LookUp(split->inner, false, true);
    if (node == ZARCHIVE_INVALID_NODE || !reader->IsDirectory(node)) {
        return false;
    }
    const u32 count = reader->GetDirEntryCount(node);
    for (u32 i = 0; i < count; i++) {
        ZArchiveReader::DirEntry entry;
        if (!reader->GetDirEntry(node, i, entry)) {
            continue;
        }
        const std::u8string_view name{reinterpret_cast<const char8_t*>(entry.name.data()),
                                      entry.name.size()};
        callback(dir / name, entry.isFile);
    }
    return true;
}

FileHandle::FileHandle(std::shared_ptr<Archive> archive_, u32 node_, u64 size_)
    : archive{std::move(archive_)}, node{node_}, size{size_} {}

FileHandle::~FileHandle() = default;

u64 FileHandle::Read(void* dest, u64 length) {
    if (offset >= size) {
        return 0;
    }
    length = std::min(length, size - offset);
    if (length == 0) {
        return 0;
    }
    const u64 bytes_read = archive->reader->ReadFromFile(node, offset, length, dest);
    offset += bytes_read;
    return bytes_read;
}

const fs::path& FileHandle::GetArchivePath() const {
    return archive->path;
}

std::unique_ptr<FileHandle> OpenFile(const fs::path& path) {
    const auto split = Split(path);
    if (!split || split->inner.empty()) {
        return nullptr;
    }
    const auto& reader = split->archive->reader;
    const auto node = reader->LookUp(split->inner, true, false);
    if (node == ZARCHIVE_INVALID_NODE || !reader->IsFile(node)) {
        return nullptr;
    }
    const u64 size = reader->GetFileSize(node);
    return std::make_unique<FileHandle>(split->archive, node, size);
}

bool CopyFile(const fs::path& src, const fs::path& dst) {
    auto handle = OpenFile(src);
    if (!handle) {
        return false;
    }
    std::ofstream out(dst, std::ios::binary | std::ios::trunc);
    if (!out) {
        return false;
    }
    std::vector<u8> buffer(1_MB);
    u64 remaining = handle->GetSize();
    while (remaining > 0) {
        const u64 chunk = handle->Read(buffer.data(), std::min<u64>(remaining, buffer.size()));
        if (chunk == 0) {
            return false;
        }
        out.write(reinterpret_cast<const char*>(buffer.data()),
                  static_cast<std::streamsize>(chunk));
        if (!out) {
            return false;
        }
        remaining -= chunk;
    }
    return out.good();
}

std::optional<fs::path> MaterializeFile(FileHandle& file, const fs::path& source_path) {
    const auto spill_dir = GetSpillDirectory();
    std::error_code ec;
    fs::create_directories(spill_dir, ec);
    if (ec) {
        LOG_ERROR(Common_Filesystem, "Failed to create ZArchive spill directory at {}: {}",
                  PathToUTF8String(spill_dir), ec.message());
        return std::nullopt;
    }

    static std::atomic<u32> spill_counter{};
#ifdef _WIN32
    const u32 pid = static_cast<u32>(_getpid());
#else
    const u32 pid = static_cast<u32>(getpid());
#endif
    const auto spill_path = spill_dir / fmt::format("{}_{}_{}", pid, spill_counter++,
                                                    PathToUTF8String(source_path.filename()));

    std::ofstream out(spill_path, std::ios::binary | std::ios::trunc);
    if (!out) {
        LOG_ERROR(Common_Filesystem, "Failed to create ZArchive spill file at {}",
                  PathToUTF8String(spill_path));
        return std::nullopt;
    }

    const u64 old_offset = file.GetOffset();
    file.SetOffset(0);
    std::vector<u8> buffer(1_MB);
    u64 remaining = file.GetSize();
    while (remaining > 0) {
        const u64 chunk = file.Read(buffer.data(), std::min<u64>(remaining, buffer.size()));
        if (chunk == 0) {
            file.SetOffset(old_offset);
            return std::nullopt;
        }
        out.write(reinterpret_cast<const char*>(buffer.data()),
                  static_cast<std::streamsize>(chunk));
        if (!out) {
            file.SetOffset(old_offset);
            return std::nullopt;
        }
        remaining -= chunk;
    }
    file.SetOffset(old_offset);

    LOG_INFO(Common_Filesystem, "Extracted {} from ZArchive to {}", PathToUTF8String(source_path),
             PathToUTF8String(spill_path));
    return spill_path;
}

fs::path GetSpillDirectory() {
    return GetUserPath(PathType::TempDataDir) / "zar_spill";
}

void CleanupSpillFiles() {
    std::error_code ec;
    const auto spill_dir = GetSpillDirectory();
    if (!fs::exists(spill_dir, ec)) {
        return;
    }
    fs::directory_iterator iterator{spill_dir, ec};
    const fs::directory_iterator end;
    while (!ec && iterator != end) {
        // Files still open in another emulator process fail to delete and are skipped.
        std::error_code remove_ec;
        fs::remove(iterator->path(), remove_ec);
        iterator.increment(ec);
    }
}

void ClearCache() {
    std::scoped_lock lock{g_archive_mutex};
    g_archive_cache.clear();
    g_failed_archives.clear();
    g_archive_cache_tick = 0;
}

} // namespace Common::FS::Zar
