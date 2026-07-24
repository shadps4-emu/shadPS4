// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string_view>

#include "core/file_sys/ifile.h"

class ZArchiveReader;

namespace Core::FileSys {

/// IFile implementation backed by a node inside a ZArchive (.zar) file.
class ZArchiveFile final : public IFile {
public:
    ZArchiveFile(std::shared_ptr<ZArchiveReader> reader, uint32_t node, u64 size);
    ~ZArchiveFile() override = default;

    s64 Read(void* dst, u64 size) override;
    s64 Write(const void* src, u64 size) override;
    bool Seek(s64 offset, Common::FS::SeekOrigin origin) override;
    u64 Tell() const override;
    u64 Size() const override;
    bool Flush() override;
    bool IsOpen() const override;

    MmapPolicy GetMmapPolicy() const override {
        return MmapPolicy::Copy;
    }

    bool IsReadOnly() const override {
        return true;
    }

private:
    std::shared_ptr<ZArchiveReader> m_reader;
    uint32_t m_node;
    u64 m_size;
    std::atomic<u64> m_position{0};
};

// Streaming directory iterator over a ZArchive node.
class ZArchiveDirectory final : public IDirectory {
public:
    ZArchiveDirectory(std::shared_ptr<ZArchiveReader> reader, uint32_t node);
    ~ZArchiveDirectory() override = default;

    bool Next(DirEntry& out) override;
    void Rewind() override;

private:
    std::shared_ptr<ZArchiveReader> m_reader;
    uint32_t m_node;
    uint32_t m_index{0};
    uint32_t m_count{0};
};

// Backend that maps a mounted namespace onto the contents of a .zar
// file.
class ZArchiveBackend final : public IBackend {
public:
    explicit ZArchiveBackend(const std::filesystem::path& archive_path);
    ~ZArchiveBackend() override;

    bool IsOpen() const {
        return static_cast<bool>(m_reader);
    }

    bool Exists(std::string_view rel_path) override;
    bool IsDirectory(std::string_view rel_path) override;

    std::unique_ptr<IFile> Open(std::string_view rel_path, bool writable) override;
    std::unique_ptr<IDirectory> OpenDir(std::string_view rel_path) override;

    bool IsReadOnly() const override {
        return true;
    }

    std::optional<std::filesystem::path> RootHostPath() const override {
        return std::nullopt;
    }

private:
    uint32_t LookUp(std::string_view rel_path, bool allow_file, bool allow_directory);

    std::filesystem::path m_archive_path;
    std::shared_ptr<ZArchiveReader> m_reader;
};

} // namespace Core::FileSys
