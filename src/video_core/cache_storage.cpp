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
    if (MoveFileExW(source.c_str(), destination.c_str(),
                    MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        return true;
    }
    LOG_ERROR(Render, "Failed to publish pipeline cache {}: Win32 error {}", destination.string(),
              GetLastError());
    return false;
#else
    std::error_code ec;
    std::filesystem::rename(source, destination, ec);
    if (!ec) {
        return true;
    }
    LOG_ERROR(Render, "Failed to publish pipeline cache {}: {}", destination.string(),
              ec.message());
    return false;
#endif
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
        if (!entries.emplace(file_name.data()).second) {
            return false;
        }
    }
    return true;
}

} // namespace

namespace Storage {

constexpr std::string_view GetBlobFileExtension(BlobType type) {
    switch (type) {
    case BlobType::ShaderMeta: {
        return "meta";
    }
    case BlobType::ShaderBinary: {
        return "spv";
    }
    case BlobType::PipelineKey: {
        return "key";
    }
    case BlobType::ShaderProfile: {
        return "bin";
    }
    default:
        UNREACHABLE();
    }
}

void DataBase::Open() {
    if (IsOpened()) {
        return;
    }

    const auto& game_info = Common::ElfInfo::Instance();

    using namespace Common::FS;
    archive_mode = EmulatorSettings.IsPipelineCacheArchived();
    if (archive_mode) {
        mz_zip_zero_struct(&zip_ar);
        ar_is_writer = false;
        archive_dirty = false;

        cache_path = GetUserPath(PathType::CacheDir) /
                     std::filesystem::path{game_info.GameSerial()}.replace_extension(".zip");
        archive_work_path = cache_path;
        archive_work_path += ".working";
        archive_publish_path = cache_path;
        archive_publish_path += ".publishing";

        std::error_code ec;
        std::filesystem::remove(archive_work_path, ec);
        ec.clear();
        std::filesystem::remove(archive_publish_path, ec);
        ec.clear();
        if (std::filesystem::exists(cache_path)) {
            std::filesystem::copy_file(cache_path, archive_work_path,
                                       std::filesystem::copy_options::overwrite_existing, ec);
            if (ec) {
                LOG_WARNING(Render, "Failed to stage pipeline cache {}: {}", cache_path.string(),
                            ec.message());
                std::filesystem::remove(archive_work_path, ec);
            }
        }

        const bool archive_valid =
            mz_zip_reader_init_file(&zip_ar, archive_work_path.string().c_str(),
                                    MZ_ZIP_FLAG_READ_ALLOW_WRITING) &&
            mz_zip_validate_archive(&zip_ar, 0);
        const bool entries_unique = archive_valid && ArchiveEntriesAreUnique();
        if (!archive_valid || !entries_unique) {
            if (archive_valid) {
                LOG_WARNING(Render,
                            "Pipeline cache {} contains duplicate entries. Rebuilding the cache",
                            cache_path.string());
            } else {
                LOG_INFO(Render, "Cache archive {} is not found or archive is corrupted",
                         cache_path.string());
            }
            mz_zip_reader_end(&zip_ar);
            std::filesystem::remove(archive_work_path, ec);
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
    if (!opened.exchange(false, std::memory_order_acq_rel)) {
        return;
    }

    request_queue.StopAccepting();
    io_worker.request_stop();
    io_worker.join();

    if (archive_mode) {
        if (ar_is_writer) {
            bool finalized = true;
            mz_zip_error finalize_error = MZ_ZIP_NO_ERROR;
            if (archive_dirty) {
                finalized = mz_zip_writer_finalize_archive(&zip_ar);
                finalize_error = mz_zip_get_last_error(&zip_ar);
            }
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
        ec.clear();
        std::filesystem::remove(archive_publish_path, ec);
    } else {
        std::filesystem::remove_all(cache_path, ec);
    }
    if (ec) {
        LOG_ERROR(Render, "Failed to reset incompatible pipeline cache {}: {}", cache_path.string(),
                  ec.message());
        return false;
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
                ASSERT_MSG(ar_is_writer,
                           "The archive is read-only. Did you forget to call `FinishPreload`?");
                const auto archive_name = path.string();
                if (!mz_zip_writer_add_mem(&zip_ar, archive_name.c_str(), data.data(),
                                           data.size() * sizeof(T), MZ_BEST_COMPRESSION)) {
                    LOG_ERROR(Render, "Failed to add {} to the archive", path.string().c_str());
                } else {
                    archive_dirty = true;
                }
            } else {
                using namespace Common::FS;
                const auto file = IOFile{path, FileAccessMode::Create};
                file.Write(data);
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
        const auto file = IOFile{path, FileAccessMode::Read};
        if (!file.IsOpen()) {
            return;
        }
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

bool DataBase::Save(BlobType type, const std::string& name, std::vector<u8>&& data) {
    if (!IsOpened() || (archive_mode && !ar_is_writer)) {
        return false;
    }

    auto path = archive_mode ? std::filesystem::path{name} : cache_path / name;
    return WriteVector(type, std::move(path), std::move(data), archive_mode);
}

bool DataBase::Save(BlobType type, const std::string& name, std::vector<u32>&& data) {
    if (!IsOpened() || (archive_mode && !ar_is_writer)) {
        return false;
    }

    auto path = archive_mode ? std::filesystem::path{name} : cache_path / name;
    return WriteVector(type, std::move(path), std::move(data), archive_mode);
}

void DataBase::Load(BlobType type, const std::string& name, std::vector<u8>& data) {
    if (!IsOpened()) {
        return;
    }

    auto path = archive_mode ? std::filesystem::path{name} : cache_path / name;
    return LoadVector(type, path, data, archive_mode);
}

void DataBase::Load(BlobType type, const std::string& name, std::vector<u32>& data) {
    if (!IsOpened()) {
        return;
    }

    auto path = archive_mode ? std::filesystem::path{name} : cache_path / name;
    return LoadVector(type, path, data, archive_mode);
}

void DataBase::ForEachBlob(BlobType type, const std::function<void(std::vector<u8>&& data)>& func) {
    if (!IsOpened()) {
        return;
    }

    const auto extension = "." + std::string{GetBlobFileExtension(type)};
    if (archive_mode) {
        const auto num_files = mz_zip_reader_get_num_files(&zip_ar);
        for (int index = 0; index < num_files; ++index) {
            std::array<char, MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE> file_name{};
            if (!mz_zip_reader_get_filename(&zip_ar, index, file_name.data(), file_name.size())) {
                continue;
            }
            if (std::string_view{file_name.data()}.ends_with(extension)) {
                mz_zip_archive_file_stat stat{};
                std::vector<u8> data;
                if (!mz_zip_reader_file_stat(&zip_ar, index, &stat) || stat.m_uncomp_size == 0 ||
                    !IsValidVectorSize(stat.m_uncomp_size, data)) {
                    LOG_WARNING(Render, "Skipping invalid cache entry {}", file_name.data());
                    continue;
                }
                data.resize(stat.m_uncomp_size);
                if (!mz_zip_reader_extract_to_mem(&zip_ar, index, data.data(), data.size(), 0)) {
                    LOG_WARNING(Render, "Failed to extract cache entry {}", file_name.data());
                    continue;
                }
                func(std::move(data));
            }
        }
    } else {
        for (const auto& file_name : std::filesystem::directory_iterator{cache_path}) {
            if (file_name.path().extension() == extension) {
                using namespace Common::FS;
                const auto& file = IOFile{file_name, FileAccessMode::Read};
                if (file.IsOpen()) {
                    std::vector<u8> data;
                    const auto file_size = file.GetSize();
                    if (file_size == 0 || !IsValidVectorSize(file_size, data)) {
                        LOG_WARNING(Render, "Skipping invalid cache entry {}",
                                    file_name.path().string());
                        continue;
                    }
                    data.resize(file_size);
                    if (file.Read(data) == data.size()) {
                        func(std::move(data));
                    } else {
                        LOG_WARNING(Render, "Skipping invalid cache entry {}",
                                    file_name.path().string());
                    }
                }
            }
        }
    }
}

bool DataBase::FinishPreload() {
    if (!IsOpened()) {
        return false;
    }
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
