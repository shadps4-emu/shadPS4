// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <istream>
#include <streambuf>
#include <string>

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

namespace {

// The reader reads through the handle the archive identity is taken from.
class IOFileStreamBuf final : public std::streambuf {
public:
    explicit IOFileStreamBuf(std::shared_ptr<Common::FS::IOFile> file) : m_file(std::move(file)) {}

protected:
    pos_type seekoff(off_type offset, std::ios_base::seekdir dir,
                     std::ios_base::openmode) override {
        Common::FS::SeekOrigin origin = Common::FS::SeekOrigin::SetOrigin;
        if (dir == std::ios_base::cur) {
            origin = Common::FS::SeekOrigin::CurrentPosition;
        } else if (dir == std::ios_base::end) {
            origin = Common::FS::SeekOrigin::End;
        }
        if (!m_file->Seek(offset, origin)) {
            return pos_type(off_type(-1));
        }
        return pos_type(m_file->Tell());
    }

    pos_type seekpos(pos_type pos, std::ios_base::openmode mode) override {
        return seekoff(off_type(pos), std::ios_base::beg, mode);
    }

    std::streamsize xsgetn(char* dst, std::streamsize count) override {
        return static_cast<std::streamsize>(
            std::fread(dst, 1, static_cast<size_t>(count), m_file->file));
    }

private:
    std::shared_ptr<Common::FS::IOFile> m_file;
};

class IOFileStream final : public std::istream {
public:
    explicit IOFileStream(std::shared_ptr<Common::FS::IOFile> file)
        : std::istream(nullptr), m_buf(std::move(file)) {
        rdbuf(&m_buf);
    }

private:
    IOFileStreamBuf m_buf;
};

bool GetArchiveFileStat(SharedReader& reader, uint32_t node, FileStat& out,
                        std::optional<u32> inode = std::nullopt) {
    out = {};
    std::scoped_lock lk{reader.mutex};
    const auto info = reader.archive_file->GetInfo();
    if (!info) {
        return false;
    }
    if (!inode) {
        inode = GetGuestInode(FileIdentity{
            .kind = FileIdentity::Kind::ArchiveEntry, .host = info->identity, .node = node});
    }
    if (!inode || *inode == 0) {
        return false;
    }
    const bool is_directory = reader.reader->IsDirectory(node);
    out = {
        .size = is_directory ? 0 : reader.reader->GetFileSize(node),
        .inode = *inode,
        .mtime_sec = info->mtime_sec,
        .mtime_nsec = info->mtime_nsec,
        .atime_sec = info->atime_sec,
        .atime_nsec = info->atime_nsec,
        .ctime_sec = info->ctime_sec,
        .ctime_nsec = info->ctime_nsec,
        .is_directory = is_directory,
    };
    return true;
}

} // namespace

SharedReader::SharedReader(ZArchiveReader* r, std::shared_ptr<Common::FS::IOFile> file)
    : reader(r), archive_file(std::move(file)) {
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
      m_archive_path(std::move(archive_path)) {
    FileStat stat{};
    if (IsOpen() && GetArchiveFileStat(*m_reader, m_node, stat)) {
        m_inode = stat.inode;
    }
}

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
    out = {};
    if (IsOpen()) {
        GetArchiveFileStat(*m_reader, m_node, out, m_inode);
    }
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
    auto file = std::make_shared<Common::FS::IOFile>(archive_path, Common::FS::FileAccessMode::Read,
                                                     Common::FS::FileType::BinaryFile,
                                                     Common::FS::FileShareFlag::ShareReadWrite);
    if (!file->IsOpen()) {
        LOG_ERROR(Kernel_Fs, "Failed to open ZArchive: {}", archive_path.string());
        return;
    }
    ZArchiveReader* raw = ZArchiveReader::OpenFromStream(std::make_unique<IOFileStream>(file));
    if (!raw) {
        LOG_ERROR(Kernel_Fs, "Failed to open ZArchive: {}", archive_path.string());
        return;
    }
    m_reader = std::make_shared<SharedReader>(raw, std::move(file));
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

bool ZArchiveBackend::Stat(std::string_view rel_path, FileStat& out) {
    const auto node = LookUp(rel_path, /*allow_file=*/true, /*allow_directory=*/true);
    if (node == ZARCHIVE_INVALID_NODE) {
        return false;
    }
    return GetArchiveFileStat(*m_reader, node, out);
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
    std::scoped_lock lk{m_reader->mutex};
    ZArchiveReader* reader = m_reader->reader;

    const auto node =
        reader->LookUp(NormalizeRel(rel_path), /*allow_file=*/true, /*allow_directory=*/false);
    if (node == ZARCHIVE_INVALID_NODE || !reader->IsFile(node)) {
        return std::nullopt;
    }
    const u64 size = reader->GetFileSize(node);
    std::vector<u8> data(size);
    struct ReadJob {
        ZArchiveReader* reader;
        uint32_t node;
        u64 pos;
        u64 size;
        void* dst;
        u64 out;
    };

    u64 total_read = 0;
    while (total_read < size) {
        ReadJob job{reader, node, total_read, size - total_read, data.data() + total_read, 0};
        m_reader->Dispatch(
            [](void* p) {
                auto* j = static_cast<ReadJob*>(p);
                j->out = j->reader->ReadFromFile(j->node, j->pos, j->size, j->dst);
            },
            &job);
        if (job.out == 0) {
            break;
        }
        total_read += job.out;
    }
    if (total_read != size) {
        LOG_ERROR(Common_Filesystem, "Short read from ZArchive entry: {} ({}/{} bytes)", rel_path,
                  total_read, size);
        return std::nullopt;
    }
    return data;
}

} // namespace Core::FileSys
