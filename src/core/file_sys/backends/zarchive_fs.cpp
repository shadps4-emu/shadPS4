// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
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

SharedReader::~SharedReader() {
    delete reader;
}

ZArchiveFile::ZArchiveFile(std::shared_ptr<SharedReader> reader, uint32_t node, u64 size,
                           std::filesystem::path archive_path)
    : m_reader(std::move(reader)), m_node(node), m_size(size),
      m_archive_path(std::move(archive_path)) {}

s64 ZArchiveFile::Read(void* dst, u64 size) {
    if (!IsOpen() || size == 0) {
        return 0;
    }
    u64 pos;
    u64 clamped;
    {
        std::scoped_lock lk{m_position_mutex};
        pos = m_position;
        if (pos >= m_size) {
            return 0;
        }
        clamped = std::min(size, m_size - pos);
    }

    u64 read;
    {
        std::scoped_lock lk{m_reader->mutex};
        read = m_reader->reader->ReadFromFile(m_node, pos, clamped, dst);
    }

    {
        std::scoped_lock lk{m_position_mutex};
        m_position = pos + read;
    }
    return static_cast<s64>(read);
}

s64 ZArchiveFile::Write(const void* /*src*/, u64 /*size*/) {
    // ZArchive is read-only
    return -1;
}

bool ZArchiveFile::Seek(s64 offset, Common::FS::SeekOrigin origin) {
    std::scoped_lock lk{m_position_mutex};
    s64 base = 0;
    switch (origin) {
    case Common::FS::SeekOrigin::SetOrigin:
        base = 0;
        break;
    case Common::FS::SeekOrigin::CurrentPosition:
        base = static_cast<s64>(m_position);
        break;
    case Common::FS::SeekOrigin::End:
        base = static_cast<s64>(m_size);
        break;
    }
    const s64 target = base + offset;
    if (target < 0) {
        return false;
    }
    m_position = static_cast<u64>(target);
    return true;
}

u64 ZArchiveFile::Tell() const {
    std::scoped_lock lk{m_position_mutex};
    return m_position;
}

u64 ZArchiveFile::Size() const {
    return m_size;
}

bool ZArchiveFile::Flush() {
    // Read-only backend, nothing to flush.
    return true;
}

bool ZArchiveFile::IsOpen() const {
    return m_reader && m_reader->reader != nullptr;
}

ZArchiveDirectory::ZArchiveDirectory(std::shared_ptr<SharedReader> reader, uint32_t node)
    : m_reader(std::move(reader)), m_node(node) {
    if (m_reader && m_reader->reader) {
        std::scoped_lock lk{m_reader->mutex};
        m_count = m_reader->reader->GetDirEntryCount(m_node);
    }
}

bool ZArchiveDirectory::Next(DirEntry& out) {
    if (!m_reader || !m_reader->reader || m_index >= m_count) {
        return false;
    }
    ::ZArchiveReader::DirEntry entry{};
    {
        std::scoped_lock lk{m_reader->mutex};
        if (!m_reader->reader->GetDirEntry(m_node, m_index, entry)) {
            return false;
        }
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
    m_reader = std::make_shared<SharedReader>(raw);
}

ZArchiveBackend::~ZArchiveBackend() = default;

uint32_t ZArchiveBackend::LookUp(std::string_view rel_path, bool allow_file, bool allow_directory) {
    if (!IsOpen()) {
        return ZARCHIVE_INVALID_NODE;
    }
    const auto normalized = NormalizeRel(rel_path);
    std::scoped_lock lk{m_reader->mutex};
    return m_reader->reader->LookUp(normalized, allow_file, allow_directory);
}

bool ZArchiveBackend::Exists(std::string_view rel_path) {
    return LookUp(rel_path, /*allow_file=*/true, /*allow_directory=*/true) != ZARCHIVE_INVALID_NODE;
}

bool ZArchiveBackend::IsDirectory(std::string_view rel_path) {
    const auto node = LookUp(rel_path, /*allow_file=*/true, /*allow_directory=*/true);
    if (node == ZARCHIVE_INVALID_NODE) {
        return false;
    }
    std::scoped_lock lk{m_reader->mutex};
    return m_reader->reader->IsDirectory(node);
}

std::unique_ptr<IFile> ZArchiveBackend::Open(std::string_view rel_path, bool writable) {
    if (writable) {
        return nullptr;
    }
    const auto node = LookUp(rel_path, /*allow_file=*/true, /*allow_directory=*/false);
    if (node == ZARCHIVE_INVALID_NODE) {
        return nullptr;
    }
    u64 size;
    {
        std::scoped_lock lk{m_reader->mutex};
        if (!m_reader->reader->IsFile(node)) {
            return nullptr;
        }
        size = m_reader->reader->GetFileSize(node);
    }
    return std::make_unique<ZArchiveFile>(m_reader, node, size, m_archive_path);
}

std::unique_ptr<IDirectory> ZArchiveBackend::OpenDir(std::string_view rel_path) {
    const auto node = LookUp(rel_path, /*allow_file=*/false, /*allow_directory=*/true);
    if (node == ZARCHIVE_INVALID_NODE) {
        return nullptr;
    }
    {
        std::scoped_lock lk{m_reader->mutex};
        if (!m_reader->reader->IsDirectory(node)) {
            return nullptr;
        }
    }
    return std::make_unique<ZArchiveDirectory>(m_reader, node);
}

} // namespace Core::FileSys
