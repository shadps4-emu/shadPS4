// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <vector>
#include <tsl/robin_map.h>
#include "common/io_file.h"

namespace Core::FileSys {

class MntPoints {
#ifdef _WIN64
    static constexpr bool NeedsCaseInsensitiveSearch = false;
#else
    static constexpr bool NeedsCaseInsensitiveSearch = true;
#endif
public:
    struct MntPair {
        std::filesystem::path host_path;
        std::string mount; // e.g /app0/
        bool read_only;
    };

    explicit MntPoints() = default;
    ~MntPoints() = default;

    void Mount(const std::filesystem::path& host_folder, const std::string& guest_folder,
               bool read_only = false);
    void Unmount(const std::filesystem::path& host_folder, const std::string& guest_folder);
    void UnmountAll();

    std::filesystem::path GetHostPath(std::string_view guest_directory,
                                      bool* is_read_only = nullptr);

    const MntPair* GetMount(const std::string& guest_path) {
        std::scoped_lock lock{m_mutex};
        const auto it = std::ranges::find_if(
            m_mnt_pairs, [&](const auto& mount) { return guest_path.starts_with(mount.mount); });
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

struct File {
    std::atomic_bool is_opened{};
    std::atomic_bool is_directory{};
    std::filesystem::path m_host_name;
    std::string m_guest_name;
    Common::FS::IOFile f;
    std::vector<DirEntry> dirents;
    u32 dirents_index;
    std::mutex m_mutex;
};

class HandleTable {
public:
    HandleTable() = default;
    virtual ~HandleTable() = default;

    int CreateHandle();
    void DeleteHandle(int d);
    File* GetFile(int d);
    File* GetFile(const std::filesystem::path& host_name);

private:
    std::vector<File*> m_files;
    std::mutex m_mutex;
};

} // namespace Core::FileSys
