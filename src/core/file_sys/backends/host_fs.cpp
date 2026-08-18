// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <system_error>

#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
#include <sys/stat.h>
#endif

#include "common/string_util.h"
#include "core/file_sys/backends/host_fs.h"
#include "core/file_sys/fs.h"

namespace Core::FileSys {

HostFile::HostFile(std::filesystem::path host_path, Common::FS::FileAccessMode mode, bool read_only)
    : m_path(std::move(host_path)), m_file(m_path, mode), m_read_only(read_only) {}

s64 HostFile::Read(void* dst, u64 size) {
    if (!m_file.IsOpen()) {
        return -1;
    }
    return static_cast<s64>(m_file.ReadRaw<u8>(dst, size));
}

s64 HostFile::Write(const void* src, u64 size) {
    if (!m_file.IsOpen() || m_read_only) {
        return -1;
    }
    return static_cast<s64>(m_file.WriteRaw<u8>(src, size));
}

bool HostFile::Seek(s64 offset, Common::FS::SeekOrigin origin) {
    if (!m_file.IsOpen()) {
        return false;
    }
    return m_file.Seek(offset, origin);
}

u64 HostFile::Tell() const {
    if (!m_file.IsOpen()) {
        return 0;
    }
    return static_cast<u64>(m_file.Tell());
}

u64 HostFile::Size() const {
    if (!m_file.IsOpen()) {
        return 0;
    }
    return m_file.GetSize();
}

bool HostFile::Flush() {
    if (!m_file.IsOpen()) {
        return false;
    }
    return m_file.Flush();
}

bool HostFile::IsOpen() const {
    return m_file.IsOpen();
}

void HostFile::Stat(FileStat& out) {
    out.size = Size();
    out.is_directory = false;
#if defined(__linux__) || defined(__FreeBSD__)
    struct stat st = {};
    if (::stat(m_path.string().c_str(), &st) == 0) {
        out.mtime_sec = static_cast<s64>(st.st_mtim.tv_sec);
        out.mtime_nsec = static_cast<s64>(st.st_mtim.tv_nsec);
        out.atime_sec = static_cast<s64>(st.st_atim.tv_sec);
        out.atime_nsec = static_cast<s64>(st.st_atim.tv_nsec);
        out.ctime_sec = static_cast<s64>(st.st_ctim.tv_sec);
        out.ctime_nsec = static_cast<s64>(st.st_ctim.tv_nsec);
    }
#elif defined(__APPLE__)
    struct stat st = {};
    if (::stat(m_path.string().c_str(), &st) == 0) {
        out.mtime_sec = static_cast<s64>(st.st_mtimespec.tv_sec);
        out.mtime_nsec = static_cast<s64>(st.st_mtimespec.tv_nsec);
        out.atime_sec = static_cast<s64>(st.st_atimespec.tv_sec);
        out.atime_nsec = static_cast<s64>(st.st_atimespec.tv_nsec);
        out.ctime_sec = static_cast<s64>(st.st_ctimespec.tv_sec);
        out.ctime_nsec = static_cast<s64>(st.st_ctimespec.tv_nsec);
    }
#else
    std::error_code ec;
    const auto ft = std::filesystem::last_write_time(m_path, ec);
    if (!ec) {
        const auto sctp = std::chrono::time_point_cast<std::chrono::nanoseconds>(
            ft - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
        const auto secs = std::chrono::time_point_cast<std::chrono::seconds>(sctp);
        const auto nsecs = std::chrono::duration_cast<std::chrono::nanoseconds>(sctp - secs);
        out.mtime_sec = static_cast<s64>(secs.time_since_epoch().count());
        out.mtime_nsec = static_cast<s64>(nsecs.count());
        out.atime_sec = out.mtime_sec;
        out.atime_nsec = out.mtime_nsec;
        out.ctime_sec = out.mtime_sec;
        out.ctime_nsec = out.mtime_nsec;
    }
#endif
}

HostDirectory::HostDirectory(const std::filesystem::path& root) : m_root(root) {
    std::error_code ec;
    m_it = std::filesystem::directory_iterator(m_root, ec);
    m_end = std::filesystem::directory_iterator{};
}

bool HostDirectory::Next(DirEntry& out) {
    if (m_it == m_end) {
        return false;
    }

    const auto& entry = *m_it;
    std::error_code ec;
    out.name = entry.path().filename().string();
    out.is_directory = entry.is_directory(ec);
    out.size = out.is_directory ? 0 : entry.file_size(ec);

    ++m_it;
    return true;
}

void HostDirectory::Rewind() {
    std::error_code ec;
    m_it = std::filesystem::directory_iterator(m_root, ec);
}

HostFsBackend::HostFsBackend(std::filesystem::path root, bool read_only)
    : m_root(std::move(root)), m_read_only(read_only) {}

std::filesystem::path HostFsBackend::Resolve(std::string_view rel_path) const {
    if (rel_path.empty()) {
        return m_root;
    }
    std::filesystem::path direct = m_root;
    direct /= rel_path;
    if constexpr (!NeedsCaseInsensitiveSearch) {
        return direct;
    } else {
        std::error_code ec;
        if (std::filesystem::exists(direct, ec)) {
            return direct;
        }
        return ResolveCaseInsensitive(rel_path).value_or(std::move(direct));
    }
}

std::optional<std::filesystem::path> HostFsBackend::ResolveCaseInsensitive(
    std::string_view rel_path) const {
    std::scoped_lock lk{m_case_cache_mutex};

    std::filesystem::path current = m_root;
    std::string prefix;
    std::error_code ec;

    for (const auto& part : std::filesystem::path(rel_path)) {
        const std::string part_str = part.string();
        if (part_str.empty() || part_str == ".") {
            continue;
        }
        if (!prefix.empty()) {
            prefix += '/';
        }
        prefix += Common::ToLower(part_str);

        if (const auto it = m_case_cache.find(prefix); it != m_case_cache.end()) {
            current = it->second;
            continue;
        }

        if (std::filesystem::path candidate = current / part_str;
            std::filesystem::exists(candidate, ec)) {
            current = std::move(candidate);
            m_case_cache.emplace(prefix, current);
            continue;
        }

        const std::string part_low = Common::ToLower(part_str);
        bool found = false;
        for (const auto& entry : std::filesystem::directory_iterator(current, ec)) {
            const auto name = entry.path().filename();
            if (Common::ToLower(name.string()) != part_low) {
                continue;
            }
            current /= name;
            m_case_cache.emplace(prefix, current);
            found = true;
            break;
        }
        if (!found) {
            return std::nullopt;
        }
    }
    return current;
}

bool HostFsBackend::Exists(std::string_view rel_path) {
    std::error_code ec;
    return std::filesystem::exists(Resolve(rel_path), ec);
}

bool HostFsBackend::IsDirectory(std::string_view rel_path) {
    std::error_code ec;
    return std::filesystem::is_directory(Resolve(rel_path), ec);
}

std::unique_ptr<IFile> HostFsBackend::Open(std::string_view rel_path,
                                           Common::FS::FileAccessMode mode) {
    const bool writable = mode != Common::FS::FileAccessMode::Read;
    if (writable && m_read_only) {
        return nullptr;
    }
    const auto path = Resolve(rel_path);
    std::error_code ec;
    if (mode != Common::FS::FileAccessMode::Create && !std::filesystem::is_regular_file(path, ec)) {
        return nullptr;
    }
    auto handle = std::make_unique<HostFile>(path, mode, m_read_only || !writable);
    if (!handle->IsOpen()) {
        return nullptr;
    }
    return handle;
}

std::unique_ptr<IDirectory> HostFsBackend::OpenDir(std::string_view rel_path) {
    const auto path = Resolve(rel_path);
    std::error_code ec;
    if (!std::filesystem::is_directory(path, ec)) {
        return nullptr;
    }
    return std::make_unique<HostDirectory>(path);
}

std::optional<std::vector<u8>> HostFsBackend::ReadFile(std::string_view rel_path) const {
    const std::filesystem::path full_path = m_root / rel_path;
    std::error_code ec;
    if (!std::filesystem::is_regular_file(full_path, ec) || ec) {
        return std::nullopt;
    }

    std::ifstream in(full_path, std::ios::binary | std::ios::ate);
    if (!in.is_open()) {
        return std::nullopt;
    }
    const auto size = in.tellg();
    if (size < 0) {
        return std::nullopt;
    }
    std::vector<u8> data(static_cast<size_t>(size));
    in.seekg(0);
    in.read(reinterpret_cast<char*>(data.data()), size);
    if (!in.good() && !in.eof()) {
        return std::nullopt;
    }
    return data;
}

} // namespace Core::FileSys
