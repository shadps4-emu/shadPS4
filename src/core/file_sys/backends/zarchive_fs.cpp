// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <string>

#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
#include <sys/stat.h>
#endif

#include <zarchive/zarchivereader.h>

#include "common/logging/log.h"
#include "common/thread.h"
#include "core/file_sys/backends/zarchive_fs.h"

namespace Core::FileSys {

// ZArchive lookups do not accept a leading slash.
std::string_view NormalizeRel(std::string_view rel) {
    while (!rel.empty() && rel.front() == '/') {
        rel.remove_prefix(1);
    }
    return rel;
}

SharedReader::SharedReader(ZArchiveReader* r) : reader(r) {
    if (reader == nullptr) {
        return;
    }
    m_worker = std::thread([this] { WorkerLoop(); });
    m_worker_id = m_worker.get_id();
}

SharedReader::~SharedReader() {
    if (m_worker.joinable()) {
        {
            std::scoped_lock lk{m_job_mutex};
            m_stop = true;
        }
        m_job_cv.notify_all();
        m_worker.join();
    }
    delete reader;
}

void SharedReader::WorkerLoop() {
    Common::SetCurrentThreadName("shadPS4:ZArchiveIO");
    std::unique_lock lk{m_job_mutex};
    while (true) {
        m_job_cv.wait(lk, [this] { return m_has_job || m_stop; });
        if (!m_has_job) {
            break;
        }
        const auto fn = m_job_fn;
        auto* const ctx = m_job_ctx;
        lk.unlock();
        fn(ctx);
        lk.lock();
        m_has_job = false;
        m_job_cv.notify_all();
    }
}

void SharedReader::Dispatch(JobFn fn, void* ctx) {
    if (!m_worker.joinable() || std::this_thread::get_id() == m_worker_id) {
        fn(ctx);
        return;
    }
    std::unique_lock lk{m_job_mutex};
    m_job_cv.wait(lk, [this] { return !m_has_job; });
    m_job_fn = fn;
    m_job_ctx = ctx;
    m_has_job = true;
    m_job_cv.notify_all();
    m_job_cv.wait(lk, [this] { return !m_has_job; });
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

    struct ReadJob {
        ZArchiveReader* reader;
        uint32_t node;
        u64 pos;
        u64 size;
        void* dst;
        u64 out;
    } job{m_reader->reader, m_node, pos, clamped, dst, 0};

    {
        std::scoped_lock lk{m_reader->mutex};
        m_reader->Dispatch(
            [](void* p) {
                auto* j = static_cast<ReadJob*>(p);
                j->out = j->reader->ReadFromFile(j->node, j->pos, j->size, j->dst);
            },
            &job);
    }
    const u64 read = job.out;

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

void ZArchiveFile::Stat(FileStat& out) {
    out.size = m_size;
    out.is_directory = false;
    // Every entry inside a .zar shares the archive's mtime
#if defined(__linux__) || defined(__FreeBSD__)
    struct stat st = {};
    if (::stat(m_archive_path.string().c_str(), &st) == 0) {
        out.mtime_sec = static_cast<s64>(st.st_mtim.tv_sec);
        out.mtime_nsec = static_cast<s64>(st.st_mtim.tv_nsec);
        out.atime_sec = static_cast<s64>(st.st_atim.tv_sec);
        out.atime_nsec = static_cast<s64>(st.st_atim.tv_nsec);
        out.ctime_sec = static_cast<s64>(st.st_ctim.tv_sec);
        out.ctime_nsec = static_cast<s64>(st.st_ctim.tv_nsec);
    }
#elif defined(__APPLE__)
    struct stat st = {};
    if (::stat(m_archive_path.string().c_str(), &st) == 0) {
        out.mtime_sec = static_cast<s64>(st.st_mtimespec.tv_sec);
        out.mtime_nsec = static_cast<s64>(st.st_mtimespec.tv_nsec);
        out.atime_sec = static_cast<s64>(st.st_atimespec.tv_sec);
        out.atime_nsec = static_cast<s64>(st.st_atimespec.tv_nsec);
        out.ctime_sec = static_cast<s64>(st.st_ctimespec.tv_sec);
        out.ctime_nsec = static_cast<s64>(st.st_ctimespec.tv_nsec);
    }
#else
    std::error_code ec;
    const auto ft = std::filesystem::last_write_time(m_archive_path, ec);
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

bool ZArchiveFile::Map(u8* addr, u64 size, u64 offset, u32 raw_prot, const FileMapContext& ctx) {
    if (!IsOpen()) {
        return false;
    }
    // Reserve writable anonymous pages, populate them from the archive at
    // the requested offset, then drop to the requested protection.
    ctx.map_anonymous(addr, size);

    // Read straight into the mapped region.
    struct MapJob {
        ZArchiveReader* reader;
        uint32_t node;
        u8* dst;
        u64 remaining;
        u64 pos;
    } job{m_reader->reader, m_node, addr, size, offset};

    {
        std::scoped_lock lk{m_reader->mutex};
        m_reader->Dispatch(
            [](void* p) {
                auto* j = static_cast<MapJob*>(p);
                while (j->remaining > 0) {
                    const u64 got = j->reader->ReadFromFile(j->node, j->pos, j->remaining, j->dst);
                    if (got == 0) {
                        std::memset(j->dst, 0, j->remaining);
                        break;
                    }
                    j->dst += got;
                    j->pos += got;
                    j->remaining -= got;
                }
            },
            &job);
    }

    ctx.protect(addr, size, raw_prot);
    return true;
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

std::unique_ptr<IFile> ZArchiveBackend::Open(std::string_view rel_path,
                                             Common::FS::FileAccessMode mode) {
    // ZArchive is read-only,any write/create/append mode is rejected.
    if (mode != Common::FS::FileAccessMode::Read) {
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

std::optional<std::vector<u8>> ZArchiveBackend::ReadFile(std::string_view rel_path) const {
    if (!IsOpen()) {
        return std::nullopt;
    }
    const auto node =
        file_reader->LookUp(NormalizeRel(rel_path), /*allow_file=*/true, /*allow_directory=*/false);
    if (node == ZARCHIVE_INVALID_NODE || !file_reader->IsFile(node)) {
        return std::nullopt;
    }
    const u64 size = file_reader->GetFileSize(node);
    std::vector<u8> data(size);
    u64 total_read = 0;
    while (total_read < size) {
        const u64 got = file_reader->ReadFromFile(node, total_read, size - total_read,
                                                  data.data() + total_read);
        if (got == 0) {
            break;
        }
        total_read += got;
    }
    if (total_read != size) {
        LOG_ERROR(Common_Filesystem, "Short read from ZArchive entry: {} ({}/{} bytes)", rel_path,
                  total_read, size);
        return std::nullopt;
    }
    return data;
}

} // namespace Core::FileSys
