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

// Normalize a guest path
std::optional<std::string> SanitizeGuestPath(std::string_view path) {
    if (path.length() > 255) {
        return std::nullopt;
    }
    // Evil games like Turok2 pass double slashes e.g /app0//game.kpf
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

// Host path of an explicitly requested overlay layer, whether or not it exists.
static std::filesystem::path OverlayLayerPath(const std::filesystem::path& mount_host_path,
                                              MntPoints::HostPathType path_type) {
    if (path_type == MntPoints::HostPathType::Mod) {
        // Mods folder can only be at mount + "-mods".
        return OverlayPath(mount_host_path, "-mods");
    }
    // Update folder is either mount + "-UPDATE" or mount + "-patch".
    std::filesystem::path patch_path = OverlayPath(mount_host_path, "-UPDATE");
    if (!std::filesystem::exists(patch_path)) {
        patch_path = OverlayPath(mount_host_path, "-patch");
    }
    return patch_path;
}

MntPoints::Resolution MntPoints::ResolvePath(std::string_view guest_path, HostPathType path_type) {
    Resolution res;

    const auto corrected = SanitizeGuestPath(guest_path);
    if (!corrected) {
        return res;
    }
    res.guest_path = *corrected;

    const auto* mount = GetMount(res.guest_path);
    if (!mount) {
        return res;
    }
    res.mount = mount;
    res.read_only = mount->read_only;
    res.rel = std::string{RelativeToMount(RemoveTrailingSlashes(res.guest_path), *mount)};

    const auto host_path_under = [&](const std::filesystem::path& root) {
        return res.rel.empty() ? root : root / res.rel;
    };

    // An explicit Mod/Patch lookup names one specific layer and is used to
    // build target paths, so it answers from the layout rather than the stack.
    if (path_type == HostPathType::Mod || path_type == HostPathType::Patch) {
        res.host_path = host_path_under(OverlayLayerPath(mount->host_path, path_type));
        std::error_code ec;
        const auto status = std::filesystem::status(res.host_path, ec);
        res.exists = std::filesystem::exists(status);
        res.is_directory = std::filesystem::is_directory(status);
        return res;
    }

    const auto& stack = mount->backends;
    if (stack.empty()) {
        res.host_path = host_path_under(mount->host_path);
        return res;
    }
    const auto& base_backend = stack.back();
    const size_t first = path_type == HostPathType::Base ? stack.size() - 1 : 0;
    for (size_t i = first; i < stack.size(); ++i) {
        const auto& backend = stack[i];
        const auto info = backend->Query(res.rel);
        if (!info.exists) {
            continue;
        }
        const auto& owner = res.rel.empty() ? base_backend : backend;
        res.backend = owner.get();
        res.exists = true;
        res.is_directory = info.is_directory;
        res.read_only = mount->read_only || owner->IsReadOnly();
        if (auto host = owner->ResolveHostPath(res.rel)) {
            res.host_path = std::move(*host);
        }
        break;
    }

    if (res.host_path.empty()) {
        if (auto host = base_backend->ResolveHostPath(res.rel)) {
            res.host_path = std::move(*host);
        } else {
            res.host_path = host_path_under(mount->host_path);
        }
    }
    return res;
}

std::filesystem::path MntPoints::GetHostPath(std::string_view path, bool* is_read_only,
                                             HostPathType path_type) {
    const auto res = ResolvePath(path, path_type);
    if (!res.IsMounted()) {
        return "";
    }
    if (is_read_only) {
        *is_read_only = res.read_only;
    }
    return res.host_path;
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

    // Prepend "." and ".."
    callback(base_host / ".", false);
    callback(base_host / "..", false);

    const auto resolve =
        [&](const std::string& leaf,
            size_t limit) -> std::optional<std::pair<std::filesystem::path, bool>> {
        const std::string entry_rel = rel.empty() ? leaf : rel + "/" + leaf;
        for (size_t i = 0; i < limit; ++i) {
            const auto& b = mount->backends[i];
            const auto info = b->Query(entry_rel);
            if (!info.exists) {
                continue;
            }
            std::filesystem::path host;
            if (auto root = b->RootHostPath(); root.has_value()) {
                host = rel.empty() ? (*root / leaf) : (*root / rel / leaf);
            } else {
                host = base_host / leaf;
            }
            return std::make_pair(host, info.is_directory);
        }
        return std::nullopt;
    };

    std::unordered_set<std::string> emitted;
    const auto emit_key = [](const std::string& name) {
        return NeedsCaseInsensitiveSearch ? Common::ToLower(name) : name;
    };

    const bool has_overlays = mount->backends.size() > 1;

    if (auto dir = base_backend->OpenDir(rel)) {
        DirEntry entry;
        while (dir->Next(entry)) {
            emitted.insert(emit_key(entry.name));
            if (!has_overlays) {
                callback(base_host / entry.name, /*is_file=*/!entry.is_directory);
                continue;
            }
            if (auto hit = resolve(entry.name, mount->backends.size() - 1)) {
                callback(hit->first, /*is_file=*/!hit->second);
            } else {
                callback(base_host / entry.name, /*is_file=*/!entry.is_directory);
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
            if (auto hit = resolve(entry.name, mount->backends.size())) {
                emitted.insert(emit_key(entry.name));
                callback(hit->first, /*is_file=*/!hit->second);
            }
        }
    }
}

bool MntPoints::Exists(std::string_view guest_path) {
    return ResolvePath(guest_path).exists;
}

bool MntPoints::IsDirectory(std::string_view guest_path) {
    return ResolvePath(guest_path).is_directory;
}

std::unique_ptr<IFile> MntPoints::Open(std::string_view guest_path, bool writable) {
    return Open(guest_path, writable ? Common::FS::FileAccessMode::ReadWrite
                                     : Common::FS::FileAccessMode::Read);
}

std::unique_ptr<IFile> MntPoints::Open(std::string_view guest_path,
                                       Common::FS::FileAccessMode mode) {
    return OpenResolved(ResolvePath(guest_path), mode);
}

std::unique_ptr<IFile> MntPoints::OpenResolved(const Resolution& resolution,
                                               Common::FS::FileAccessMode mode) {
    if (!resolution.IsMounted() || resolution.mount->backends.empty()) {
        return nullptr;
    }
    const bool writable = mode != Common::FS::FileAccessMode::Read;
    if (writable && resolution.mount->read_only) {
        return nullptr;
    }
    if (mode == Common::FS::FileAccessMode::Create) {
        // Creation always lands on the writable base layer.
        return resolution.mount->backends.back()->Open(resolution.rel, mode);
    }
    if (resolution.backend == nullptr || resolution.is_directory) {
        return nullptr;
    }
    return resolution.backend->Open(resolution.rel, mode);
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
