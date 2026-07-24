// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <string>

#include <zarchive/zarchivereader.h>
#include "common/logging/log.h"
#include "core/file_sys/backends/zarchive_fs.h"

namespace Core::FileSys {

// ZArchive lookups do not accept a leading slash.
std::string_view NormalizeRel(std::string_view rel) {
    while (!rel.empty() && rel.front() == '/') {
        rel.remove_prefix(1);
    }
    return rel;
}

ZArchiveFile::ZArchiveFile(std::shared_ptr<ZArchiveReader> reader, uint32_t node, u64 size)
    : m_reader(std::move(reader)), m_node(node), m_size(size) {}

s64 ZArchiveFile::Read(void* dst, u64 size) {
    if (!m_reader || size == 0) {
        return 0;
    }
    const u64 pos = m_position.load(std::memory_order_relaxed);
    if (pos >= m_size) {
        return 0;
    }
    const u64 clamped = std::min(size, m_size - pos);
    const u64 read = m_reader->ReadFromFile(m_node, pos, clamped, dst);
    m_position.fetch_add(read, std::memory_order_relaxed);
    return static_cast<s64>(read);
}

s64 ZArchiveFile::Write(const void* /*src*/, u64 /*size*/) {
    // ZArchive is read-only
    return -1;
}

bool ZArchiveFile::Seek(s64 offset, Common::FS::SeekOrigin origin) {
    s64 base = 0;
    switch (origin) {
    case Common::FS::SeekOrigin::SetOrigin:
        base = 0;
        break;
    case Common::FS::SeekOrigin::CurrentPosition:
        base = static_cast<s64>(m_position.load(std::memory_order_relaxed));
        break;
    case Common::FS::SeekOrigin::End:
        base = static_cast<s64>(m_size);
        break;
    }
    const s64 target = base + offset;
    if (target < 0) {
        return false;
    }
    m_position.store(static_cast<u64>(target), std::memory_order_relaxed);
    return true;
}

u64 ZArchiveFile::Tell() const {
    return m_position.load(std::memory_order_relaxed);
}

u64 ZArchiveFile::Size() const {
    return m_size;
}

bool ZArchiveFile::Flush() {
    // Read-only backend, nothing to flush.
    return true;
}

bool ZArchiveFile::IsOpen() const {
    return static_cast<bool>(m_reader);
}

ZArchiveDirectory::ZArchiveDirectory(std::shared_ptr<ZArchiveReader> reader, uint32_t node)
    : m_reader(std::move(reader)), m_node(node) {
    if (m_reader) {
        m_count = m_reader->GetDirEntryCount(m_node);
    }
}

bool ZArchiveDirectory::Next(DirEntry& out) {
    if (!m_reader || m_index >= m_count) {
        return false;
    }
    ::ZArchiveReader::DirEntry entry{};
    if (!m_reader->GetDirEntry(m_node, m_index, entry)) {
        return false;
    }
    ++m_index;
    out.name.assign(entry.name.data(), entry.name.size());
    out.is_directory = entry.isDirectory;
    out.size = entry.isFile ? entry.size : 0;
    return true;
}

void ZArchiveDirectory::Rewind() {
    m_index = 0;
}

ZArchiveBackend::ZArchiveBackend(const std::filesystem::path& archive_path)
    : m_archive_path(archive_path) {
    ZArchiveReader* raw = ZArchiveReader::OpenFromFile(archive_path);
    if (!raw) {
        LOG_ERROR(Kernel_Fs, "Failed to open ZArchive: {}", archive_path.string());
        return;
    }
    m_reader = std::shared_ptr<ZArchiveReader>(raw, [](ZArchiveReader* r) { delete r; });
}

ZArchiveBackend::~ZArchiveBackend() = default;

uint32_t ZArchiveBackend::LookUp(std::string_view rel_path, bool allow_file, bool allow_directory) {
    if (!m_reader) {
        return ZARCHIVE_INVALID_NODE;
    }
    const auto normalized = NormalizeRel(rel_path);
    return m_reader->LookUp(normalized, allow_file, allow_directory);
}

bool ZArchiveBackend::Exists(std::string_view rel_path) {
    return LookUp(rel_path, /*allow_file=*/true, /*allow_directory=*/true) != ZARCHIVE_INVALID_NODE;
}

bool ZArchiveBackend::IsDirectory(std::string_view rel_path) {
    const auto node = LookUp(rel_path, /*allow_file=*/true, /*allow_directory=*/true);
    if (node == ZARCHIVE_INVALID_NODE) {
        return false;
    }
    return m_reader->IsDirectory(node);
}

std::unique_ptr<IFile> ZArchiveBackend::Open(std::string_view rel_path, bool writable) {
    if (writable) {
        return nullptr;
    }
    const auto node = LookUp(rel_path, /*allow_file=*/true, /*allow_directory=*/false);
    if (node == ZARCHIVE_INVALID_NODE || !m_reader->IsFile(node)) {
        return nullptr;
    }
    const auto size = m_reader->GetFileSize(node);
    return std::make_unique<ZArchiveFile>(m_reader, node, size);
}

std::unique_ptr<IDirectory> ZArchiveBackend::OpenDir(std::string_view rel_path) {
    const auto node = LookUp(rel_path, /*allow_file=*/false, /*allow_directory=*/true);
    if (node == ZARCHIVE_INVALID_NODE || !m_reader->IsDirectory(node)) {
        return nullptr;
    }
    return std::make_unique<ZArchiveDirectory>(m_reader, node);
}

} // namespace Core::FileSys
