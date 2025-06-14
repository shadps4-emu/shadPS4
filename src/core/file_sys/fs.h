// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <vector>
#include <tsl/robin_map.h>
#include "common/io_file.h"
#include "common/logging/formatter.h"
#include "core/devices/base_device.h"

namespace Core::FileSys {

class MntPoints {
#ifdef _WIN64
    static constexpr bool NeedsCaseInsensitiveSearch = false;
#else
    static constexpr bool NeedsCaseInsensitiveSearch = true;
#endif
public:
    static bool ignore_game_patches;
    struct MntPair {
        std::filesystem::path host_path;
        std::string mount; // e.g /app0
        bool read_only;
    };

    explicit MntPoints() = default;
    ~MntPoints() = default;

    void Mount(const std::filesystem::path& host_folder, const std::string& guest_folder,
               bool read_only = false);
    void Unmount(const std::filesystem::path& host_folder, const std::string& guest_folder);
    void UnmountAll();

    std::filesystem::path GetHostPath(std::string_view guest_directory,
                                      bool* is_read_only = nullptr, bool force_base_path = false);
    using IterateDirectoryCallback =
        std::function<void(const std::filesystem::path& host_path, bool is_file)>;
    void IterateDirectory(std::string_view guest_directory,
                          const IterateDirectoryCallback& callback);

    const MntPair* GetMountFromHostPath(const std::string& host_path) {
        std::scoped_lock lock{m_mutex};
        const auto it = std::ranges::find_if(m_mnt_pairs, [&](const MntPair& mount) {
            return host_path.starts_with(std::string{fmt::UTF(mount.host_path.u8string()).data});
        });
        return it == m_mnt_pairs.end() ? nullptr : &*it;
    }

    const MntPair* GetMount(const std::string& guest_path) {
        std::scoped_lock lock{m_mutex};
        const auto it = std::ranges::find_if(m_mnt_pairs, [&](const auto& mount) {
            // When doing starts-with check, add a trailing slash to make sure we don't match
            // against only part of the mount path.
            return guest_path == mount.mount || guest_path.starts_with(mount.mount + "/");
        });
        return it == m_mnt_pairs.end() ? nullptr : &*it;
    }

private:
    std::vector<MntPair> m_mnt_pairs;
    std::vector<std::filesystem::path> path_parts;
    tsl::robin_map<std::filesystem::path, std::filesystem::path> path_cache;
    std::mutex m_mutex;
};

struct DirEntry {
    std::string name;
    bool isFile;
};

enum class FileType {
    Regular, // standard file
    Directory,
    Device,
};

struct File {
    std::atomic_bool is_opened{};
    std::atomic<FileType> type{FileType::Regular};
    std::filesystem::path m_host_name;
    std::string m_guest_name;
    Common::FS::IOFile f;
    std::vector<DirEntry> dirents;
    u32 dirents_index;
    std::mutex m_mutex;
    std::shared_ptr<Devices::BaseDevice> device; // only valid for type == Device
};

class HandleTable {
public:
    HandleTable() = default;
    virtual ~HandleTable() = default;

    int CreateHandle();
    void DeleteHandle(int d);
    File* GetFile(int d);
    File* GetFile(const std::filesystem::path& host_name);
    int GetFileDescriptor(File* file);

    void CreateStdHandles();

private:
    std::vector<File*> m_files;
    std::mutex m_mutex;
};

} // namespace Core::FileSys
