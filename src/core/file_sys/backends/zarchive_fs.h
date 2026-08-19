// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string_view>
#include <thread>

#include "core/file_sys/ifile.h"

class ZArchiveReader;

namespace Core::FileSys {

struct SharedReader {
    explicit SharedReader(ZArchiveReader* r);
    ~SharedReader();

    SharedReader(const SharedReader&) = delete;
    SharedReader& operator=(const SharedReader&) = delete;

    using JobFn = void (*)(void*);
    void Dispatch(JobFn fn, void* ctx);

    ZArchiveReader* reader{nullptr};
    std::mutex mutex;

private:
    void WorkerLoop();

    std::mutex m_job_mutex;
    std::condition_variable m_job_cv;
    JobFn m_job_fn{nullptr};
    void* m_job_ctx{nullptr};
    bool m_has_job{false};
    bool m_stop{false};
    std::thread::id m_worker_id;
    std::thread m_worker;
};

// IFile implementation backed by a node inside a ZArchive (.zar) file.
class ZArchiveFile final : public IFile {
public:
    ZArchiveFile(std::shared_ptr<SharedReader> reader, uint32_t node, u64 size,
                 std::filesystem::path archive_path);
    ~ZArchiveFile() override = default;

    s64 Read(void* dst, u64 size) override;
    s64 Write(const void* src, u64 size) override;
    bool Seek(s64 offset, Common::FS::SeekOrigin origin) override;
    u64 Tell() const override;
    u64 Size() const override;
    bool Flush() override;
    bool IsOpen() const override;

    std::optional<std::filesystem::path> GetMetadataHostPath() const override {
        return m_archive_path;
    }

    void Stat(FileStat& out) override;

    MmapPolicy GetMmapPolicy() const override {
        return MmapPolicy::Copy;
    }

    bool Map(u8* addr, u64 size, u64 offset, u32 raw_prot, const FileMapContext& ctx) override;

    bool IsReadOnly() const override {
        return true;
    }

private:
    std::shared_ptr<SharedReader> m_reader;
    uint32_t m_node;
    u64 m_size;
    std::filesystem::path m_archive_path;
    mutable std::mutex m_position_mutex;
    u64 m_position{0};
};

// Streaming directory iterator over a ZArchive node.
class ZArchiveDirectory final : public IDirectory {
public:
    ZArchiveDirectory(std::shared_ptr<SharedReader> reader, uint32_t node);
    ~ZArchiveDirectory() override = default;

    bool Next(DirEntry& out) override;
    void Rewind() override;

private:
    std::shared_ptr<SharedReader> m_reader;
    uint32_t m_node;
    uint32_t m_index{0};
    uint32_t m_count{0};
};

// Backend that maps a mounted namespace onto the contents of a .zar
class ZArchiveBackend final : public IBackend {
public:
    explicit ZArchiveBackend(const std::filesystem::path& archive_path);
    ~ZArchiveBackend() override;

    bool IsOpen() const {
        return m_reader && m_reader->reader != nullptr;
    }

    bool Exists(std::string_view rel_path) override;
    bool IsDirectory(std::string_view rel_path) override;

    std::unique_ptr<IFile> Open(std::string_view rel_path,
                                Common::FS::FileAccessMode mode) override;
    std::unique_ptr<IDirectory> OpenDir(std::string_view rel_path) override;

    bool IsReadOnly() const override {
        return true;
    }

    std::optional<std::filesystem::path> RootHostPath() const override {
        return std::nullopt;
    }

    [[nodiscard]] std::filesystem::path RootPath() const override {
        return m_archive_path;
    }

    std::optional<std::vector<u8>> ReadFile(std::string_view rel_path) const override;

private:
    uint32_t LookUp(std::string_view rel_path, bool allow_file, bool allow_directory);

    std::filesystem::path m_archive_path;
    std::shared_ptr<SharedReader> m_reader;
};

} // namespace Core::FileSys
