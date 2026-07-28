// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include <tsl/robin_map.h>
#include "common/io_file.h"
#include "common/logging/formatter.h"
#include "core/file_sys/devices/base_device.h"
#include "core/file_sys/directories/base_directory.h"
#include "core/file_sys/ifile.h"

namespace Libraries::Net {
struct Socket;
struct Epoll;
struct Resolver;
} // namespace Libraries::Net

namespace Core::FileSys {

/// Builds the path of an overlay that sits next to a game
[[nodiscard]] std::filesystem::path OverlayPath(const std::filesystem::path& base,
                                                std::string_view suffix);

/// Inverse of OverlayPath.
[[nodiscard]] std::optional<std::filesystem::path> BaseGameFromOverlay(
    const std::filesystem::path& path);

[[nodiscard]] std::optional<std::filesystem::path> ResolveGameRoot(
    const std::filesystem::path& root);

#ifdef _WIN64
inline constexpr bool NeedsCaseInsensitiveSearch = false;
#else
inline constexpr bool NeedsCaseInsensitiveSearch = true;
#endif

class MntPoints {
public:
    static bool ignore_game_patches;
    struct MntPair {
        std::filesystem::path host_path;
        std::string mount; // e.g /app0
        bool read_only;
        std::vector<std::shared_ptr<IBackend>> backends;
    };

    enum class HostPathType {
        Default, // Prioritizes Mod, then patch, then base
        Base,
        Patch,
        Mod
    };

    explicit MntPoints() = default;
    ~MntPoints() = default;

    void Mount(const std::filesystem::path& host_folder, const std::string& guest_folder,
               bool read_only = false);
    void Unmount(const std::filesystem::path& host_folder, const std::string& guest_folder);
    void UnmountAll();

    std::filesystem::path GetHostPath(std::string_view guest_directory,
                                      bool* is_read_only = nullptr,
                                      HostPathType host_path = HostPathType::Default);
    using IterateDirectoryCallback =
        std::function<void(const std::filesystem::path& host_path, bool is_file)>;
    void IterateDirectory(std::string_view guest_directory,
                          const IterateDirectoryCallback& callback);

    /// Returns true if the guest path exists in any backend of the
    /// mount stack. Mirrors fs::exists() on the resolved host path.
    bool Exists(std::string_view guest_path);

    /// Returns true if the guest path resolves to a directory in any
    /// backend of the mount stack.
    bool IsDirectory(std::string_view guest_path);

    /// Opens the guest path through the mount's backend stack. Returns
    /// nullptr when the path does not exist or the caller requested
    /// writable access on a read-only mount.
    std::unique_ptr<IFile> Open(std::string_view guest_path, bool writable = false);
    // Open with an explicit host access mode.
    std::unique_ptr<IFile> Open(std::string_view guest_path, Common::FS::FileAccessMode mode);

    /// Opens a directory through the mount's backend stack.
    std::unique_ptr<IDirectory> OpenDir(std::string_view guest_path);

    /// open + read the whole file as bytes. Returns nullopt
    /// when the file does not exist or is unreadable.
    std::optional<std::vector<u8>> ReadFile(std::string_view guest_path);

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
        if (it == m_mnt_pairs.end()) {
            return nullptr;
        }
        return &*it;
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
    Resolver,
    Equeue
};

struct File {
    std::atomic_bool is_opened{};
    std::atomic<FileType> type{FileType::Regular};
    std::filesystem::path m_host_name;
    std::string m_guest_name;
    std::unique_ptr<IFile> handle;
    std::mutex m_mutex;
    std::shared_ptr<Directories::BaseDirectory> directory; // only valid for type == Directory
    std::shared_ptr<Devices::BaseDevice> device;           // only valid for type == Device
    std::shared_ptr<Libraries::Net::Socket> socket;        // only valid for type == Socket
    std::shared_ptr<Libraries::Net::Epoll> epoll;          // only valid for type == Epoll
    std::shared_ptr<Libraries::Net::Resolver> resolver;    // only valid for type == Resolver

    bool IsBackendOpen() const {
        return handle && handle->IsOpen();
    }

    s64 Read(void* dst, u64 size) {
        return handle ? handle->Read(dst, size) : -1;
    }

    s64 Write(const void* src, u64 size) {
        return handle ? handle->Write(src, size) : -1;
    }

    bool Seek(s64 offset, Common::FS::SeekOrigin origin = Common::FS::SeekOrigin::SetOrigin) {
        return handle ? handle->Seek(offset, origin) : false;
    }

    s64 Tell() const {
        return handle ? static_cast<s64>(handle->Tell()) : -1;
    }

    u64 GetSize() const {
        return handle ? handle->Size() : 0;
    }

    bool Flush() {
        return handle ? handle->Flush() : false;
    }

    bool IsWriteOnly() const {
        return handle ? handle->IsWriteOnly() : false;
    }

    Common::FS::IOFile* GetHostFile() const {
        return handle ? handle->GetHostFile() : nullptr;
    }
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
