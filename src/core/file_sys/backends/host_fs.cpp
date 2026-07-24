// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <system_error>

#include "core/file_sys/backends/host_fs.h"

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
    std::filesystem::path p = m_root;
    p /= rel_path;
    return p;
}

bool HostFsBackend::Exists(std::string_view rel_path) {
    std::error_code ec;
    return std::filesystem::exists(Resolve(rel_path), ec);
}

bool HostFsBackend::IsDirectory(std::string_view rel_path) {
    std::error_code ec;
    return std::filesystem::is_directory(Resolve(rel_path), ec);
}

std::unique_ptr<IFile> HostFsBackend::Open(std::string_view rel_path, bool writable) {
    if (writable && m_read_only) {
        return nullptr;
    }
    const auto path = Resolve(rel_path);
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec)) {
        return nullptr;
    }
    const auto mode =
        writable ? Common::FS::FileAccessMode::ReadWrite : Common::FS::FileAccessMode::Read;
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

} // namespace Core::FileSys
