// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/elf_info.h"
#include "common/io_file.h"
#include "common/polyfill_thread.h"
#include "common/thread.h"
#include "core/emulator_settings.h"

#include "video_core/cache_archive.h"
#include "video_core/cache_storage.h"
#include "video_core/cache_task_queue.h"

#include <miniz.h>

#include <array>
#include <functional>
#include <future>
#include <string>
#include <unordered_set>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

Storage::Detail::CacheTaskQueue request_queue{};

std::mutex database_mutex{};
mz_zip_archive zip_ar{};
bool ar_is_writer{};
bool archive_dirty{};

void ProcessIO(const std::stop_token& stoken) {
    Common::SetCurrentThreadName("shadPS4:PipelineCacheIO");
    while (auto request = request_queue.Take(stoken)) {
        (*request)();
    }
}

bool CommitArchive(const std::filesystem::path& source, const std::filesystem::path& destination) {
#ifdef _WIN32
    DWORD last_error{};
#else
    std::error_code last_error;
#endif
    for (int attempt = 0; attempt < 10; ++attempt) {
#ifdef _WIN32
        if (MoveFileExW(source.c_str(), destination.c_str(),
                        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            return true;
        }
        last_error = GetLastError();
#else
        std::filesystem::rename(source, destination, last_error);
        if (!last_error)
            return true;
#endif
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
#ifdef _WIN32
    LOG_ERROR(Render, "Failed to publish pipeline cache {}: Win32 error {}", destination.string(),
              last_error);
#else
    LOG_ERROR(Render, "Failed to publish pipeline cache {}: {}", destination.string(),
              last_error.message());
#endif
    return false;
}

bool ArchiveEntriesAreUnique() {
    std::unordered_set<std::string> entries;
    const auto num_files = mz_zip_reader_get_num_files(&zip_ar);
    entries.reserve(num_files);
    for (mz_uint index = 0; index < num_files; ++index) {
        std::array<char, MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE> file_name{};
        if (!mz_zip_reader_get_filename(&zip_ar, index, file_name.data(), file_name.size())) {
            return false;
        }
        if (!entries.emplace(file_name.data()).second)
            return false;
    }
    return true;
}

} // namespace

namespace Storage {

constexpr std::string_view GetBlobFileExtension(BlobType type) {
    constexpr std::array extensions{"meta", "spv", "key", "bin"};
    return extensions[static_cast<size_t>(type)];
}

void DataBase::Open() {
    std::lock_guard lock{database_mutex};
    if (IsOpened())
        return;

    const auto& game_info = Common::ElfInfo::Instance();
    using namespace Common::FS;
    archive_mode = EmulatorSettings.IsPipelineCacheArchived();

    if (archive_mode) {
        mz_zip_zero_struct(&zip_ar);
        ar_is_writer = archive_dirty = false;

        cache_path = GetUserPath(PathType::CacheDir) /
                     std::filesystem::path{game_info.GameSerial()}.replace_extension(".zip");
        archive_work_path = cache_path.string() + ".working";
        archive_publish_path = cache_path.string() + ".publishing";

        auto remove_file = [](const std::filesystem::path& p) {
            std::error_code ec;
            std::filesystem::remove(p, ec);
        };

        bool stage_publishing{};
        if (std::filesystem::exists(archive_publish_path)) {
            mz_zip_archive publishing_ar{};
            const bool publishing_valid =
                mz_zip_reader_init_file(&publishing_ar, archive_publish_path.string().c_str(), 0) &&
                mz_zip_validate_archive(&publishing_ar, 0);
            mz_zip_reader_end(&publishing_ar);
            if (publishing_valid) {
                LOG_INFO(Render, "Found valid publishing pipeline cache, attempting recovery...");
                stage_publishing = !CommitArchive(archive_publish_path, cache_path);
            } else {
                LOG_WARNING(Render, "Discarding incomplete publishing pipeline cache {}",
                            archive_publish_path.string());
                remove_file(archive_publish_path);
            }
        }
        remove_file(archive_work_path);

        std::error_code ec;
        const auto& stage_path = stage_publishing ? archive_publish_path : cache_path;
        if (std::filesystem::exists(stage_path)) {
            std::filesystem::copy_file(stage_path, archive_work_path,
                                       std::filesystem::copy_options::overwrite_existing, ec);
            if (ec) {
                LOG_WARNING(Render, "Failed to stage pipeline cache {}: {}", stage_path.string(),
                            ec.message());
                remove_file(archive_work_path);
            }
        }

        bool archive_valid = mz_zip_reader_init_file(&zip_ar, archive_work_path.string().c_str(),
                                                     MZ_ZIP_FLAG_READ_ALLOW_WRITING) &&
                             mz_zip_validate_archive(&zip_ar, 0);
        bool entries_unique = archive_valid && ArchiveEntriesAreUnique();

        if (archive_valid && !entries_unique) {
            LOG_WARNING(Render, "Pipeline cache {} contains duplicate entries. Repairing...",
                        cache_path.string());
            mz_zip_reader_end(&zip_ar);
            if (Detail::CompactArchive(archive_work_path, archive_publish_path)) {
                std::filesystem::copy_file(archive_publish_path, archive_work_path,
                                           std::filesystem::copy_options::overwrite_existing, ec);
                if (!ec)
                    CommitArchive(archive_publish_path, cache_path);
            }
            archive_valid = mz_zip_reader_init_file(&zip_ar, archive_work_path.string().c_str(),
                                                    MZ_ZIP_FLAG_READ_ALLOW_WRITING) &&
                            mz_zip_validate_archive(&zip_ar, 0) && ArchiveEntriesAreUnique();
            if (!archive_valid)
                remove_file(archive_publish_path);
        }

        if (!archive_valid) {
            LOG_INFO(Render, "Cache archive {} is not found or archive is corrupted",
                     cache_path.string());
            mz_zip_reader_end(&zip_ar);
            remove_file(archive_work_path);
            if (!mz_zip_writer_init_file(&zip_ar, archive_work_path.string().c_str(), 0)) {
                LOG_ERROR(Render, "Failed to create pipeline cache staging archive {}",
                          archive_work_path.string());
                return;
            }
            ar_is_writer = true;
        }
    } else {
        cache_path = GetUserPath(PathType::CacheDir) / game_info.GameSerial();
        if (!std::filesystem::exists(cache_path)) {
            std::filesystem::create_directories(cache_path);
        }
    }

    request_queue.StartAccepting();
    io_worker = std::jthread{ProcessIO};
    opened.store(true, std::memory_order_release);
}

void DataBase::Close() {
    if (!opened.exchange(false, std::memory_order_acq_rel))
        return;

    request_queue.StopAccepting();
    io_worker.request_stop();
    if (io_worker.joinable())
        io_worker.join();

    std::lock_guard lock{database_mutex};
    if (archive_mode) {
        if (ar_is_writer) {
            bool finalized = !archive_dirty || mz_zip_writer_finalize_archive(&zip_ar);
            const auto finalize_error = mz_zip_get_last_error(&zip_ar);
            mz_zip_writer_end(&zip_ar);
            if (archive_dirty) {
                if (!finalized) {
                    LOG_ERROR(Render, "Failed to finalize pipeline cache {}: {}",
                              cache_path.string(), mz_zip_get_error_string(finalize_error));
                } else if (!Detail::CompactArchive(archive_work_path, archive_publish_path)) {
                    LOG_ERROR(Render, "Failed to compact pipeline cache {}", cache_path.string());
                } else {
                    CommitArchive(archive_publish_path, cache_path);
                }
            }
        } else {
            mz_zip_reader_end(&zip_ar);
        }
        std::error_code ec;
        std::filesystem::remove(archive_work_path, ec);
    }
    LOG_INFO(Render, "Cache dumped");
}

bool DataBase::Reset() {
    Close();
    std::error_code ec;
    if (archive_mode) {
        std::filesystem::remove(cache_path, ec);
        if (ec) {
            LOG_ERROR(Render, "Failed to remove incompatible pipeline cache {}: {}",
                      cache_path.string(), ec.message());
            return false;
        }
        std::filesystem::remove(archive_work_path, ec);
        if (ec) {
            LOG_ERROR(Render, "Failed to remove pipeline cache staging file {}: {}",
                      archive_work_path.string(), ec.message());
            return false;
        }
        std::filesystem::remove(archive_publish_path, ec);
        if (ec) {
            LOG_ERROR(Render, "Failed to remove pipeline cache publishing file {}: {}",
                      archive_publish_path.string(), ec.message());
            return false;
        }
    } else {
        std::filesystem::remove_all(cache_path, ec);
        if (ec) {
            LOG_ERROR(Render, "Failed to reset incompatible pipeline cache {}: {}",
                      cache_path.string(), ec.message());
            return false;
        }
    }
    Open();
    return IsOpened();
}

template <typename T>
bool WriteVector(const BlobType type, std::filesystem::path&& path_, std::vector<T>&& v,
                 bool archive_mode) {
    auto request = std::packaged_task<void()>{
        [type, path = std::move(path_), data = std::move(v), archive_mode]() mutable {
            path.replace_extension(GetBlobFileExtension(type));
            if (archive_mode) {
                std::lock_guard lock{database_mutex};
                ASSERT_MSG(ar_is_writer,
                           "The archive is read-only. Did you forget to call `FinishPreload`?");
                if (!mz_zip_writer_add_mem(&zip_ar, path.string().c_str(), data.data(),
                                           data.size() * sizeof(T), MZ_BEST_COMPRESSION)) {
                    LOG_ERROR(Render, "Failed to add {} to the archive", path.string().c_str());
                } else {
                    archive_dirty = true;
                }
            } else {
                using namespace Common::FS;
                IOFile{path, FileAccessMode::Create}.Write(data);
            }
        }};
    return request_queue.Submit(std::move(request));
}

template <typename T>
bool IsValidVectorSize(u64 byte_size, const std::vector<T>& data) {
    return byte_size % sizeof(T) == 0 && byte_size / sizeof(T) <= data.max_size();
}

template <typename T>
void LoadVector(BlobType type, std::filesystem::path& path, std::vector<T>& v, bool archive_mode) {
    using namespace Common::FS;
    v.clear();
    path.replace_extension(GetBlobFileExtension(type));
    if (archive_mode) {
        std::lock_guard lock{database_mutex};
        const int index = mz_zip_reader_locate_file(&zip_ar, path.string().c_str(), nullptr, 0);
        if (index < 0) {
            LOG_WARNING(Render, "File {} is not found in the archive", path.string().c_str());
            return;
        }
        mz_zip_archive_file_stat stat{};
        if (!mz_zip_reader_file_stat(&zip_ar, index, &stat) ||
            !IsValidVectorSize(stat.m_uncomp_size, v)) {
            LOG_WARNING(Render, "Cache entry {} has an invalid size", path.string());
            return;
        }
        v.resize(stat.m_uncomp_size / sizeof(T));
        if (stat.m_uncomp_size != 0 &&
            !mz_zip_reader_extract_to_mem(&zip_ar, index, v.data(), stat.m_uncomp_size, 0)) {
            LOG_WARNING(Render, "Failed to extract cache entry {}", path.string());
            v.clear();
        }
    } else {
        const IOFile file{path, FileAccessMode::Read};
        if (!file.IsOpen())
            return;
        const auto file_size = file.GetSize();
        if (!IsValidVectorSize(file_size, v)) {
            LOG_WARNING(Render, "Cache entry {} has an invalid size", path.string());
            return;
        }
        v.resize(file_size / sizeof(T));
        if (file.Read(v) != v.size()) {
            LOG_WARNING(Render, "Failed to read cache entry {}", path.string());
            v.clear();
        }
    }
}

template <typename T>
bool DataBase::Save(BlobType type, const std::string& name, std::vector<T>&& data) {
    if (!IsOpened())
        return false;
    {
        std::lock_guard lock{database_mutex};
        if (archive_mode && !ar_is_writer)
            return false;
    }
    return WriteVector(type, archive_mode ? std::filesystem::path{name} : cache_path / name,
                       std::move(data), archive_mode);
}

template bool DataBase::Save(BlobType, const std::string&, std::vector<u8>&&);
template bool DataBase::Save(BlobType, const std::string&, std::vector<u32>&&);

template <typename T>
void DataBase::Load(BlobType type, const std::string& name, std::vector<T>& data) {
    if (!IsOpened())
        return;
    auto path = archive_mode ? std::filesystem::path{name} : cache_path / name;
    LoadVector(type, path, data, archive_mode);
}

template void DataBase::Load(BlobType, const std::string&, std::vector<u8>&);
template void DataBase::Load(BlobType, const std::string&, std::vector<u32>&);

void DataBase::ForEachBlob(BlobType type, const std::function<void(std::vector<u8>&& data)>& func) {
    if (!IsOpened())
        return;

    const auto extension = "." + std::string{GetBlobFileExtension(type)};
    if (archive_mode) {
        std::vector<std::vector<u8>> blobs;
        {
            std::lock_guard lock{database_mutex};
            const auto num_files = mz_zip_reader_get_num_files(&zip_ar);
            blobs.reserve(num_files);
            for (int index = 0; index < num_files; ++index) {
                std::array<char, MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE> file_name{};
                if (!mz_zip_reader_get_filename(&zip_ar, index, file_name.data(),
                                                file_name.size())) {
                    continue;
                }

                if (std::string_view{file_name.data()}.ends_with(extension)) {
                    mz_zip_archive_file_stat stat{};
                    std::vector<u8> data;
                    if (!mz_zip_reader_file_stat(&zip_ar, index, &stat) ||
                        stat.m_uncomp_size == 0 || !IsValidVectorSize(stat.m_uncomp_size, data)) {
                        LOG_WARNING(Render, "Skipping invalid cache entry {}", file_name.data());
                        continue;
                    }
                    data.resize(stat.m_uncomp_size);
                    if (!mz_zip_reader_extract_to_mem(&zip_ar, index, data.data(), data.size(),
                                                      0)) {
                        LOG_WARNING(Render, "Failed to extract cache entry {}", file_name.data());
                        continue;
                    }
                    blobs.emplace_back(std::move(data));
                }
            }
        }
        for (auto& data : blobs)
            func(std::move(data));
    } else {
        for (const auto& entry : std::filesystem::directory_iterator{cache_path}) {
            const auto& path = entry.path();
            if (path.extension() == extension) {
                using namespace Common::FS;
                const IOFile file{path, FileAccessMode::Read};
                if (file.IsOpen()) {
                    std::vector<u8> data;
                    const auto file_size = file.GetSize();
                    if (file_size == 0 || !IsValidVectorSize(file_size, data)) {
                        LOG_WARNING(Render, "Skipping invalid cache entry {}", path.string());
                        continue;
                    }
                    data.resize(file_size);
                    if (file.Read(data) == data.size()) {
                        func(std::move(data));
                    } else {
                        LOG_WARNING(Render, "Skipping invalid cache entry {}", path.string());
                    }
                }
            }
        }
    }
}

bool DataBase::FinishPreload() {
    if (!IsOpened())
        return false;
    std::lock_guard lock{database_mutex};
    if (archive_mode && !ar_is_writer) {
        if (!mz_zip_writer_init_from_reader(&zip_ar, archive_work_path.string().c_str())) {
            LOG_ERROR(Render, "Failed to make pipeline cache writable: {}",
                      archive_work_path.string());
            return false;
        }
        ar_is_writer = true;
    }
    return true;
}

} // namespace Storage
