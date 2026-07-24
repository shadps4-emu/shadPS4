// SPDX-FileCopyrightText: Copyright 2026shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <memory>
#include <string_view>
#include "common/io_file.h"
#include "core/file_sys/ifile.h"

namespace Core::FileSys {

/// IFile implementation backed by a real host file.
class HostFile final : public IFile {
public:
    HostFile(std::filesystem::path host_path, Common::FS::FileAccessMode mode, bool read_only);
    ~HostFile() override = default;

    s64 Read(void* dst, u64 size) override;
    s64 Write(const void* src, u64 size) override;
    bool Seek(s64 offset, Common::FS::SeekOrigin origin) override;
    u64 Tell() const override;
    u64 Size() const override;
    bool Flush() override;
    bool IsOpen() const override;

    std::optional<std::filesystem::path> GetHostPath() const override {
        return m_path;
    }

    Common::FS::IOFile* GetHostFile() override {
        return &m_file;
    }

    MmapPolicy GetMmapPolicy() const override {
        return MmapPolicy::Native;
    }

    bool IsReadOnly() const override {
        return m_read_only;
    }

private:
    std::filesystem::path m_path;
    Common::FS::IOFile m_file;
    bool m_read_only;
};

/// Streaming iterator over a host directory.
class HostDirectory final : public IDirectory {
public:
    explicit HostDirectory(const std::filesystem::path& root);
    ~HostDirectory() override = default;

    bool Next(DirEntry& out) override;
    void Rewind() override;

private:
    std::filesystem::path m_root;
    std::filesystem::directory_iterator m_it;
    std::filesystem::directory_iterator m_end;
};

class HostFsBackend final : public IBackend {
public:
    HostFsBackend(std::filesystem::path root, bool read_only);
    ~HostFsBackend() override = default;

    bool Exists(std::string_view rel_path) override;
    bool IsDirectory(std::string_view rel_path) override;

    std::unique_ptr<IFile> Open(std::string_view rel_path, bool writable) override;
    std::unique_ptr<IDirectory> OpenDir(std::string_view rel_path) override;

    bool IsReadOnly() const override {
        return m_read_only;
    }

    std::optional<std::filesystem::path> RootHostPath() const override {
        return m_root;
    }

private:
    std::filesystem::path Resolve(std::string_view rel_path) const;

    std::filesystem::path m_root;
    bool m_read_only;
};

} // namespace Core::FileSys
