// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include <tsl/robin_map.h>
#include "common/io_file.h"
#include "common/logging/formatter.h"
#include "core/file_sys/devices/base_device.h"
#include "core/file_sys/directories/base_directory.h"

namespace Libraries::Net {
struct Socket;
struct Epoll;
struct Resolver;
} // namespace Libraries::Net

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

    const std::optional<MntPair> GetMount(const std::string& guest_path) {
        std::scoped_lock lock{m_mutex};
        const auto it = std::ranges::find_if(m_mnt_pairs, [&](const auto& mount) {
            // When doing starts-with check, add a trailing slash to make sure we don't match
            // against only part of the mount path.
            return guest_path == mount.mount || guest_path.starts_with(mount.mount + "/");
        });
        if (it == m_mnt_pairs.end()) {
            return std::nullopt;
        }
        return *it;
    }

private:
    std::vector<MntPair> m_mnt_pairs;
    std::vector<std::filesystem::path> path_parts;
    tsl::robin_map<std::filesystem::path, std::filesystem::path> path_cache;
    std::mutex m_mutex;
};

enum class FileType {
    Regular, // standard file
    Directory,
    Device,
    Socket,
    Epoll,
    Resolver
};

struct File {
    std::atomic_bool is_opened{};
    std::atomic<FileType> type{FileType::Regular};
    std::filesystem::path m_host_name;
    std::string m_guest_name;
    Common::FS::IOFile f;
    std::mutex m_mutex;
    std::shared_ptr<Directories::BaseDirectory> directory; // only valid for type == Directory
    std::shared_ptr<Devices::BaseDevice> device;           // only valid for type == Device
    std::shared_ptr<Libraries::Net::Socket> socket;        // only valid for type == Socket
    std::shared_ptr<Libraries::Net::Epoll> epoll;          // only valid for type == Epoll
    std::shared_ptr<Libraries::Net::Resolver> resolver;    // only valid for type == Resolver
};

class HandleTable {
public:
    HandleTable() = default;
    virtual ~HandleTable() = default;

    int CreateHandle();
    void DeleteHandle(int d);
    File* GetFile(int d);
    File* GetSocket(int d);
    File* GetEpoll(int d);
    File* GetResolver(int d);
    File* GetFile(const std::filesystem::path& host_name);
    int GetFileDescriptor(File* file);

    void CreateStdHandles();

private:
    std::vector<File*> m_files;
    std::mutex m_mutex;
};

} // namespace Core::FileSys
