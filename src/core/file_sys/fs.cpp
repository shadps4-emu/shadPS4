// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <unordered_set>
#include "common/assert.h"
#include "common/string_util.h"
#include "core/file_sys/backends/host_fs.h"
#include "core/file_sys/backends/zarchive_fs.h"
#include "core/file_sys/devices/logger.h"
#include "core/file_sys/devices/nop_device.h"
#include "core/file_sys/fs.h"

namespace Core::FileSys {

// Iterates a pre-collected entry list produced by merging the backend stack.
class MergedDirectory final : public IDirectory {
public:
    explicit MergedDirectory(std::vector<DirEntry> entries) : entries{std::move(entries)} {}

    bool Next(DirEntry& out) override {
        if (index >= entries.size()) {
            return false;
        }
        out = entries[index++];
        return true;
    }

    void Rewind() override {
        index = 0;
    }

private:
    std::vector<DirEntry> entries;
    size_t index{0};
};

bool MntPoints::ignore_game_patches = false;

std::string RemoveTrailingSlashes(const std::string& path) {
    // Remove trailing slashes to make comparisons simpler.
    std::string path_sanitized = path;
    while (path_sanitized.ends_with("/")) {
        path_sanitized.pop_back();
    }
    return path_sanitized;
}

std::filesystem::path OverlayPath(const std::filesystem::path& base, std::string_view suffix) {
    std::filesystem::path result = base;
    if (result.extension() == ".zar") {
        result.replace_extension();
    }
    result += std::string{suffix};
    return result;
}

std::optional<std::filesystem::path> BaseGameFromOverlay(const std::filesystem::path& path) {
    std::filesystem::path stem_path = path;
    if (stem_path.extension() == ".zar") {
        stem_path.replace_extension();
    }
    const std::string name = stem_path.filename().string();
    static constexpr std::array<std::string_view, 3> suffixes{"-UPDATE", "-patch", "-mods"};
    for (const auto suffix : suffixes) {
        if (name.size() > suffix.size() && name.ends_with(suffix)) {
            return stem_path.parent_path() / name.substr(0, name.size() - suffix.size());
        }
    }
    return std::nullopt;
}

std::optional<std::filesystem::path> ResolveGameRoot(const std::filesystem::path& root) {
    if (std::filesystem::is_directory(root)) {
        return root;
    }
    if (root.extension() == ".zar" && std::filesystem::is_regular_file(root)) {
        return root;
    }
    std::filesystem::path with_ext = root;
    with_ext += ".zar";
    if (std::filesystem::is_regular_file(with_ext)) {
        return with_ext;
    }
    return std::nullopt;
}

void MntPoints::Mount(const std::filesystem::path& host_folder, const std::string& guest_folder,
                      bool read_only) {
    std::scoped_lock lock{m_mutex};
    const auto guest_folder_sanitized = RemoveTrailingSlashes(guest_folder);
    // Build the backend stack for this mount.
    std::vector<std::shared_ptr<IBackend>> stack;
    const bool eligible_for_overlays =
        guest_folder_sanitized == "/app0" || guest_folder_sanitized == "/hostapp";
    const auto make_backend = [](const std::filesystem::path& p,
                                 bool ro) -> std::shared_ptr<IBackend> {
        if (std::filesystem::is_directory(p)) {
            return std::make_shared<HostFsBackend>(p, ro);
        }
        const auto try_zar = [ro](const std::filesystem::path& zar) -> std::shared_ptr<IBackend> {
            if (std::filesystem::is_regular_file(zar) && zar.extension() == ".zar") {
                auto backend = std::make_shared<ZArchiveBackend>(zar);
                if (backend->IsOpen()) {
                    return backend;
                }
            }
            return nullptr;
        };
        if (auto b = try_zar(p)) {
            return b;
        }
        std::filesystem::path with_ext = p;
        with_ext += ".zar";
        return try_zar(with_ext);
    };

    const auto probe_overlay =
        [&make_backend](const std::filesystem::path& base,
                        std::string_view suffix) -> std::shared_ptr<IBackend> {
        return make_backend(OverlayPath(base, suffix), /*ro=*/true);
    };
    // check for mods , updates,patch
    if (eligible_for_overlays) {
        if (auto mods = probe_overlay(host_folder, "-mods")) {
            stack.push_back(std::move(mods));
        }
        if (!ignore_game_patches) {
            auto patch = probe_overlay(host_folder, "-UPDATE");
            if (!patch) {
                patch = probe_overlay(host_folder, "-patch");
            }
            if (patch) {
                stack.push_back(std::move(patch));
            }
        }
    }

    std::shared_ptr<IBackend> base = make_backend(host_folder, read_only);
    ASSERT_MSG(base, "Mount: base path does not resolve to a backend: {}", host_folder.string());
    stack.push_back(std::move(base));

    m_mnt_pairs.emplace_back(host_folder, guest_folder_sanitized, read_only, std::move(stack));
}

void MntPoints::Unmount(const std::filesystem::path& host_folder, const std::string& guest_folder) {
    std::scoped_lock lock{m_mutex};
    const auto guest_folder_sanitized = RemoveTrailingSlashes(guest_folder);
    auto it = std::remove_if(m_mnt_pairs.begin(), m_mnt_pairs.end(), [&](const MntPair& pair) {
        return pair.mount == guest_folder_sanitized;
    });
    m_mnt_pairs.erase(it, m_mnt_pairs.end());
}

void MntPoints::UnmountAll() {
    std::scoped_lock lock{m_mutex};
    m_mnt_pairs.clear();
}

std::filesystem::path MntPoints::GetHostPath(std::string_view path, bool* is_read_only,
                                             HostPathType path_type) {
    // Evil games like Turok2 pass double slashes e.g /app0//game.kpf
    std::string corrected_path(path);
    size_t pos = corrected_path.find("//");
    while (pos != std::string::npos) {
        corrected_path.replace(pos, 2, "/");
        pos = corrected_path.find("//", pos + 1);
    }

    if (path.length() > 255)
        return "";

    const auto* mount = GetMount(corrected_path);
    if (!mount) {
        return "";
    }

    if (is_read_only) {
        *is_read_only = mount->read_only;
    }

    const bool host_backed =
        mount->backends.empty() || mount->backends.back()->RootHostPath().has_value();

    const auto corrected_path_sanitized = RemoveTrailingSlashes(corrected_path);
    std::filesystem::path host_path = mount->host_path;

    // Update folder is either mount + "-UPDATE" or mount + "-patch"
    std::filesystem::path patch_path = OverlayPath(mount->host_path, "-UPDATE");
    if (!std::filesystem::exists(patch_path)) {
        patch_path = OverlayPath(mount->host_path, "-patch");
    }

    // Mods folder can only be at mount + "-mods"
    std::filesystem::path mods_path = OverlayPath(mount->host_path, "-mods");

    // If we're just retrieving the mount, return the correct mount path.
    if (corrected_path_sanitized == mount->mount) {
        if (path_type == HostPathType::Mod) {
            return mods_path;
        } else if (path_type == HostPathType::Patch) {
            return patch_path;
        } else {
            return host_path;
        }
    }

    // Remove device (e.g /app0) from path to retrieve relative path.
    const auto rel_path = std::string_view{corrected_path}.substr(mount->mount.size() + 1);
    host_path /= rel_path;
    patch_path /= rel_path;
    mods_path /= rel_path;

    if (path_type == HostPathType::Mod) {
        return mods_path;
    } else if (path_type == HostPathType::Patch) {
        return patch_path;
    }

    if ((corrected_path.starts_with("/app0") || corrected_path.starts_with("/hostapp")) &&
        path_type != HostPathType::Base && std::filesystem::exists(mods_path)) {
        return mods_path;
    }

    if ((corrected_path.starts_with("/app0") || corrected_path.starts_with("/hostapp")) &&
        path_type != HostPathType::Base && !ignore_game_patches &&
        std::filesystem::exists(patch_path)) {
        return patch_path;
    }

    if (!NeedsCaseInsensitiveSearch || !host_backed) {
        return host_path;
    }

    const auto search = [&](const auto host_path) {
        // If the path does not exist attempt to verify this.
        // Retrieve parent path until we find one that exists.
        std::scoped_lock lk{m_mutex};
        path_parts.clear();
        auto current_path = host_path;
        while (!current_path.empty() && !std::filesystem::exists(current_path)) {
            // We have probably cached this if it's a folder.
            if (auto it = path_cache.find(current_path); it != path_cache.end()) {
                current_path = it->second;
                break;
            }
            path_parts.emplace_back(current_path.filename());
            current_path = current_path.parent_path();
        }
        if (!current_path.empty()) {
            // We have found an anchor. Traverse parts we recoded and see if they
            // exist in filesystem but in different case.
            auto guest_path = current_path;
            while (!path_parts.empty()) {
                const auto part = path_parts.back();
                const auto add_match = [&](const auto& host_part) {
                    current_path /= host_part;
                    guest_path /= part;
                    path_cache[guest_path] = current_path;
                    path_parts.pop_back();
                };
                // Can happen when the mismatch is in upper folder.
                if (std::filesystem::exists(current_path / part)) {
                    add_match(part);
                    continue;
                }
                const auto part_low = Common::ToLower(part.string());
                bool found_match = false;
                for (const auto& path : std::filesystem::directory_iterator(current_path)) {
                    const auto candidate = path.path().filename();
                    const auto filename = Common::ToLower(candidate.string());
                    // Check if a filename matches in case insensitive manner.
                    if (filename != part_low) {
                        continue;
                    }
                    // We found a match, record the actual path in the cache.
                    add_match(candidate);
                    found_match = true;
                    break;
                }
                if (!found_match) {
                    return std::optional<std::filesystem::path>({});
                }
            }
        }
        return std::optional<std::filesystem::path>(current_path);
    };

    if ((corrected_path.starts_with("/app0") || corrected_path.starts_with("/hostapp")) &&
        path_type != HostPathType::Base) {
        if (const auto path = search(mods_path)) {
            return *path;
        }
    }

    if (path_type != HostPathType::Base && !ignore_game_patches) {
        if (const auto path = search(patch_path)) {
            return *path;
        }
    }
    if (const auto path = search(host_path)) {
        return *path;
    }

    // Opening the guest path will surely fail but at least gives
    // a better error message than the empty path.
    return host_path;
}

// Normalize a guest path
std::optional<std::string> SanitizeGuestPath(std::string_view path) {
    if (path.length() > 255) {
        return std::nullopt;
    }
    std::string corrected(path);
    size_t pos = corrected.find("//");
    while (pos != std::string::npos) {
        corrected.replace(pos, 2, "/");
        pos = corrected.find("//", pos + 1);
    }
    return corrected;
}

// Strip the mount prefix from a corrected guest path
std::string_view RelativeToMount(const std::string& corrected, const MntPoints::MntPair& mount) {
    if (corrected.size() <= mount.mount.size() + 1) {
        return {};
    }
    return std::string_view{corrected}.substr(mount.mount.size() + 1);
}

void MntPoints::IterateDirectory(std::string_view guest_directory,
                                 const IterateDirectoryCallback& callback) {
    const auto corrected_opt = SanitizeGuestPath(guest_directory);
    if (!corrected_opt) {
        return;
    }
    const auto& corrected = *corrected_opt;
    const auto mount = GetMount(corrected);
    if (!mount || mount->backends.empty()) {
        return;
    }
    const auto rel = std::string{RelativeToMount(corrected, *mount)};

    const auto& base_backend = mount->backends.back();
    std::filesystem::path base_host;
    if (auto root = base_backend->RootHostPath(); root.has_value()) {
        base_host = rel.empty() ? *root : (*root / rel);
    } else {
        base_host = std::filesystem::path(corrected);
    }

    // Prepend "." and ".." (PS4 exposes these as file entries via getdents).
    callback(base_host / ".", false);
    callback(base_host / "..", false);

    const auto resolve =
        [&](const std::string& leaf) -> std::optional<std::pair<std::filesystem::path, bool>> {
        const std::string entry_rel = rel.empty() ? leaf : rel + "/" + leaf;
        for (const auto& b : mount->backends) {
            if (!b->Exists(entry_rel)) {
                continue;
            }
            const bool is_dir = b->IsDirectory(entry_rel);
            std::filesystem::path host;
            if (auto root = b->RootHostPath(); root.has_value()) {
                host = rel.empty() ? (*root / leaf) : (*root / rel / leaf);
            } else {
                host = base_host / leaf;
            }
            return std::make_pair(host, is_dir);
        }
        return std::nullopt;
    };

    std::unordered_set<std::string> emitted;
    const auto emit_key = [](const std::string& name) {
        return NeedsCaseInsensitiveSearch ? Common::ToLower(name) : name;
    };

    if (auto dir = base_backend->OpenDir(rel)) {
        DirEntry entry;
        while (dir->Next(entry)) {
            if (auto hit = resolve(entry.name)) {
                emitted.insert(emit_key(entry.name));
                callback(hit->first, /*is_file=*/!hit->second);
            }
        }
    }

    for (size_t i = mount->backends.size() - 1; i-- > 0;) {
        const auto& overlay = mount->backends[i];
        auto dir = overlay->OpenDir(rel);
        if (!dir) {
            continue;
        }
        DirEntry entry;
        while (dir->Next(entry)) {
            if (emitted.contains(emit_key(entry.name))) {
                continue;
            }
            if (auto hit = resolve(entry.name)) {
                emitted.insert(emit_key(entry.name));
                callback(hit->first, /*is_file=*/!hit->second);
            }
        }
    }
}

bool MntPoints::Exists(std::string_view guest_path) {
    const auto corrected = SanitizeGuestPath(guest_path);
    if (!corrected) {
        return false;
    }
    const auto mount = GetMount(*corrected);
    if (!mount || mount->backends.empty()) {
        return false;
    }
    const auto rel = RelativeToMount(*corrected, *mount);
    for (const auto& backend : mount->backends) {
        if (backend->Exists(rel)) {
            return true;
        }
    }
    return false;
}

bool MntPoints::IsDirectory(std::string_view guest_path) {
    const auto corrected = SanitizeGuestPath(guest_path);
    if (!corrected) {
        return false;
    }
    const auto mount = GetMount(*corrected);
    if (!mount || mount->backends.empty()) {
        return false;
    }
    const auto rel = RelativeToMount(*corrected, *mount);
    for (const auto& backend : mount->backends) {
        if (backend->Exists(rel)) {
            return backend->IsDirectory(rel);
        }
    }
    return false;
}

std::unique_ptr<IFile> MntPoints::Open(std::string_view guest_path, bool writable) {
    return Open(guest_path, writable ? Common::FS::FileAccessMode::ReadWrite
                                     : Common::FS::FileAccessMode::Read);
}

std::unique_ptr<IFile> MntPoints::Open(std::string_view guest_path,
                                       Common::FS::FileAccessMode mode) {
    const auto corrected = SanitizeGuestPath(guest_path);
    if (!corrected) {
        return nullptr;
    }
    const auto mount = GetMount(*corrected);
    if (!mount || mount->backends.empty()) {
        return nullptr;
    }
    const bool writable = mode != Common::FS::FileAccessMode::Read;
    if (writable && mount->read_only) {
        return nullptr;
    }
    const auto rel = RelativeToMount(*corrected, *mount);
    const std::string rel_copy{rel};

    if (mode == Common::FS::FileAccessMode::Create) {
        return mount->backends.back()->Open(rel_copy, mode);
    }

    for (const auto& backend : mount->backends) {
        if (backend->Exists(rel) && !backend->IsDirectory(rel)) {
            return backend->Open(rel_copy, mode);
        }
    }
    return nullptr;
}

std::unique_ptr<IDirectory> MntPoints::OpenDir(std::string_view guest_path) {
    const auto corrected = SanitizeGuestPath(guest_path);
    if (!corrected) {
        return nullptr;
    }
    const auto mount = GetMount(*corrected);
    if (!mount || mount->backends.empty()) {
        return nullptr;
    }
    const auto rel = RelativeToMount(*corrected, *mount);
    const std::string rel_copy{rel};
    std::vector<DirEntry> entries;
    std::unordered_set<std::string> seen;
    bool found = false;
    for (const auto& backend : mount->backends) {
        if (!backend->Exists(rel) || !backend->IsDirectory(rel)) {
            continue;
        }
        found = true;
        auto dir = backend->OpenDir(rel_copy);
        if (!dir) {
            continue;
        }
        DirEntry entry;
        while (dir->Next(entry)) {
            const auto key = NeedsCaseInsensitiveSearch ? Common::ToLower(entry.name) : entry.name;
            if (!seen.insert(key).second) {
                continue;
            }
            entries.push_back(entry);
        }
    }
    if (!found) {
        return nullptr;
    }
    return std::make_unique<MergedDirectory>(std::move(entries));
}

std::optional<std::vector<u8>> MntPoints::ReadFile(std::string_view guest_path) {
    auto handle = Open(guest_path, /*writable=*/false);
    if (!handle) {
        return std::nullopt;
    }
    const u64 size = handle->Size();
    std::vector<u8> buf(size);
    if (size == 0) {
        return buf;
    }
    const s64 got = handle->Read(buf.data(), size);
    if (got < 0 || static_cast<u64>(got) != size) {
        return std::nullopt;
    }
    return buf;
}

int HandleTable::CreateHandle() {
    std::scoped_lock lock{m_mutex};

    auto* file = new File{};
    file->is_opened = false;

    int existingFilesNum = m_files.size();

    for (int index = 0; index < existingFilesNum; index++) {
        if (m_files.at(index) == nullptr) {
            m_files[index] = file;
            return index;
        }
    }

    m_files.push_back(file);
    return m_files.size() - 1;
}

void HandleTable::DeleteHandle(int d) {
    std::scoped_lock lock{m_mutex};
    delete m_files.at(d);
    m_files[d] = nullptr;
}

File* HandleTable::GetFile(int d) {
    std::scoped_lock lock{m_mutex};
    if (d < 0 || d >= m_files.size()) {
        return nullptr;
    }
    return m_files.at(d);
}

File* HandleTable::GetSocket(int d) {
    std::scoped_lock lock{m_mutex};
    if (d < 0 || d >= m_files.size()) {
        return nullptr;
    }
    auto file = m_files.at(d);
    if (!file) {
        return nullptr;
    }
    if (file->type != Core::FileSys::FileType::Socket) {
        return nullptr;
    }
    return file;
}

File* HandleTable::GetEpoll(int d) {
    std::scoped_lock lock{m_mutex};
    if (d < 0 || d >= m_files.size()) {
        return nullptr;
    }
    auto file = m_files.at(d);
    if (file->type != Core::FileSys::FileType::Epoll) {
        return nullptr;
    }
    return file;
}

File* HandleTable::GetResolver(int d) {
    std::scoped_lock lock{m_mutex};
    if (d < 0 || d >= m_files.size()) {
        return nullptr;
    }
    auto file = m_files.at(d);
    if (file->type != Core::FileSys::FileType::Resolver) {
        return nullptr;
    }
    return file;
}

File* HandleTable::GetFile(const std::filesystem::path& host_name) {
    std::scoped_lock lock{m_mutex};
    for (auto* file : m_files) {
        if (file != nullptr && file->m_host_name == host_name) {
            return file;
        }
    }
    return nullptr;
}

void HandleTable::CreateStdHandles() {
    auto setup = [this](const char* path, auto* device) {
        int fd = CreateHandle();
        auto* file = GetFile(fd);
        file->is_opened = true;
        file->type = FileType::Device;
        file->m_guest_name = path;
        file->device =
            std::shared_ptr<Devices::BaseDevice>{reinterpret_cast<Devices::BaseDevice*>(device)};
    };
    // order matters
    setup("/dev/stdin", new Devices::Logger("stdin", false));   // stdin
    setup("/dev/stdout", new Devices::Logger("stdout", false)); // stdout
    setup("/dev/stderr", new Devices::Logger("stderr", true));  // stderr
}

int HandleTable::GetFileDescriptor(File* file) {
    std::scoped_lock lock{m_mutex};
    auto it = std::find(m_files.begin(), m_files.end(), file);

    if (it != m_files.end()) {
        return std::distance(m_files.begin(), it);
    }
    return 0;
}

} // namespace Core::FileSys
