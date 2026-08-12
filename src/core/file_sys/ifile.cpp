// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fstream>
#include <system_error>

#include "common/logging/log.h"
#include "common/path_util.h"
#include "core/file_sys/backends/host_fs.h"
#include "core/file_sys/backends/zarchive_fs.h"
#include "core/file_sys/fs.h"
#include "core/file_sys/ifile.h"

namespace Core::FileSys {

bool IsZArchiveFile(const std::filesystem::path& path) {
    std::error_code ec;
    return path.extension() == ".zar" && std::filesystem::is_regular_file(path, ec) && !ec;
}

std::filesystem::path StripZArchiveExtension(const std::filesystem::path& path) {
    if (path.extension() == ".zar") {
        std::filesystem::path stripped = path;
        stripped.replace_extension();
        return stripped;
    }
    return path;
}

std::unique_ptr<IBackend> OpenGameBackend(const std::filesystem::path& root) {
    const auto resolved = ResolveGameRoot(root);
    if (!resolved.has_value()) {
        return nullptr;
    }

    if (IsZArchiveFile(*resolved)) {
        auto backend = std::make_unique<ZArchiveBackend>(*resolved);
        if (!backend->IsOpen()) {
            return nullptr;
        }
        return backend;
    }

    return std::make_unique<HostFsBackend>(*resolved, true);
}

std::optional<std::vector<u8>> ReadGameFile(const std::filesystem::path& game_root,
                                            std::string_view rel_path) {
    const auto backend = OpenGameBackend(game_root);
    if (!backend) {
        return std::nullopt;
    }
    return backend->ReadFile(rel_path);
}

std::optional<std::filesystem::path> ResolveGameFilePath(const std::filesystem::path& game_root,
                                                         std::string_view rel_path) {
    const auto backend = OpenGameBackend(game_root);
    if (!backend) {
        return std::nullopt;
    }

    // Directory-backed: just hand back the real path if the file exists.
    if (const auto host_root = backend->RootHostPath()) {
        const std::filesystem::path full_path = *host_root / rel_path;
        std::error_code ec;
        if (std::filesystem::is_regular_file(full_path, ec) && !ec) {
            return full_path;
        }
        return std::nullopt;
    }

    const std::string archive_key = std::to_string(
        std::hash<std::string>{}(std::filesystem::absolute(backend->RootPath()).string()));
    const std::filesystem::path cache_dir =
        Common::FS::GetUserPath(Common::FS::PathType::CacheDir) / "zar_meta" / archive_key;
    const std::filesystem::path dest = cache_dir / std::filesystem::path(rel_path);

    std::error_code ec;
    const auto archive_time = std::filesystem::last_write_time(backend->RootPath(), ec);
    if (!ec && std::filesystem::is_regular_file(dest, ec) && !ec) {
        const auto dest_time = std::filesystem::last_write_time(dest, ec);
        if (!ec && dest_time >= archive_time) {
            return dest;
        }
    }

    const auto data = backend->ReadFile(rel_path);
    if (!data.has_value()) {
        return std::nullopt;
    }

    std::error_code mkdir_ec;
    std::filesystem::create_directories(dest.parent_path(), mkdir_ec);

    std::ofstream out(dest, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        LOG_ERROR(Common_Filesystem, "Failed to open destination for archive extraction: {}",
                  dest.string());
        return std::nullopt;
    }
    out.write(reinterpret_cast<const char*>(data->data()),
              static_cast<std::streamsize>(data->size()));
    if (!out.good()) {
        return std::nullopt;
    }
    return dest;
}

u64 GetGameRootSize(const std::filesystem::path& game_root) {
    std::error_code ec;

    if (IsZArchiveFile(game_root)) {
        const auto size = std::filesystem::file_size(game_root, ec);
        return ec ? 0ull : static_cast<u64>(size);
    }

    if (!std::filesystem::is_directory(game_root, ec) || ec) {
        return 0ull;
    }

    u64 total = 0;
    for (std::filesystem::recursive_directory_iterator
             it(game_root, std::filesystem::directory_options::skip_permission_denied, ec),
         end;
         it != end; it.increment(ec)) {
        if (ec) {
            break;
        }
        std::error_code entry_ec;
        if (it->is_regular_file(entry_ec) && !entry_ec) {
            const auto size = it->file_size(entry_ec);
            if (!entry_ec) {
                total += static_cast<u64>(size);
            }
        }
    }
    return total;
}

} // namespace Core::FileSys
