// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "save_memory.h"

#include <algorithm>
#include <cstring>
#include <mutex>
#include <system_error>
#include <unordered_map>
#include <fmt/format.h>

#include "common/io_file.h"
#include "common/logging/formatter.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/file_sys/fs.h"
#include "save_backup.h"
#include "save_db.h"
#include "save_instance.h"

using Common::FS::IOFile;
namespace fs = std::filesystem;

namespace Libraries::SaveData::SaveMemory {

namespace {

constexpr std::string_view SceSysDir = "sce_sys";
constexpr std::string_view StandardDirnameSaveDataMemory = "sce_sdmemory";
constexpr std::string_view FilenameSaveDataMemory = "memory.dat";
constexpr std::string_view IconName = "icon0.png";
constexpr std::string_view CorruptFileName = "corrupted";

Core::FileSys::MntPoints* g_mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();

struct SlotData {
    Libraries::UserService::OrbisUserServiceUserId user_id{};
    std::string game_serial;
    fs::path folder_path;
    PSF sfo;
    std::vector<u8> memory_cache;
    size_t memory_cache_size{};
};

std::mutex g_slot_mtx;
std::unordered_map<u32, SlotData> g_attached_slots;

bool PersistMemoryLocked(SlotData& data) {
    const fs::path memory_path = data.folder_path / FilenameSaveDataMemory;
    std::error_code ec;
    fs::create_directories(memory_path.parent_path(), ec);
    if (ec) {
        LOG_ERROR(Lib_SaveData, "SaveMemory: failed to create {}: {}",
                  fmt::UTF(memory_path.parent_path().u8string()), ec.message());
        return false;
    }

    const fs::path tmp_path = fs::path(memory_path).concat(".tmp");
    IOFile f{tmp_path, Common::FS::FileAccessMode::Create};
    if (!f.IsOpen()) {
        LOG_ERROR(Lib_SaveData, "SaveMemory: failed to open {} for write",
                  fmt::UTF(tmp_path.u8string()));
        return false;
    }
    const size_t written = data.memory_cache.empty()
                               ? 0
                               : f.WriteRaw<u8>(data.memory_cache.data(), data.memory_cache.size());
    f.Close();
    if (written != data.memory_cache.size()) {
        fs::remove(tmp_path, ec);
        return false;
    }
    fs::rename(tmp_path, memory_path, ec);
    if (ec) {
        fs::remove(tmp_path, ec);
        return false;
    }
    return true;
}

bool SaveSFOLocked(const SlotData& data, u32 slot_id) {
    const fs::path sfo_path = SaveInstance::GetParamSFOPath(data.folder_path);
    if (!SaveInstance::WriteParamSFOAtomic(data.sfo, sfo_path)) {
        LOG_ERROR(Lib_SaveData, "Failed to encode SaveMemory param.sfo");
        return false;
    }
    SaveDb::Finalize(data.user_id, SaveInstance::BuildDbEntryFromSfo(
                                       data.sfo, data.user_id, data.game_serial, data.game_serial,
                                       GetSaveDir(slot_id), data.folder_path, 96, false, false));
    return true;
}

} // namespace

void ClearAll() {
    std::scoped_lock lck{g_slot_mtx};
    g_attached_slots.clear();
}

bool PersistMemory(u32 slot_id, bool lock) {
    std::unique_lock lck{g_slot_mtx, std::defer_lock};
    if (lock) {
        lck.lock();
    }
    const auto it = g_attached_slots.find(slot_id);
    if (it == g_attached_slots.end()) {
        return false;
    }
    const auto backup_lck = Backup::LockForWrite();
    if (!PersistMemoryLocked(it->second)) {
        return false;
    }
    SaveSFOLocked(it->second, slot_id);
    return true;
}

std::string GetSaveDir(u32 slot_id) {
    std::string dir(StandardDirnameSaveDataMemory);
    if (slot_id > 0) {
        dir += std::to_string(slot_id);
    }
    return dir;
}

fs::path GetSavePath(Libraries::UserService::OrbisUserServiceUserId user_id, u32 slot_id,
                     std::string_view game_serial) {
    return GetSaveDirPath(user_id, game_serial, GetSaveDir(slot_id));
}

size_t SetupSaveMemory(Libraries::UserService::OrbisUserServiceUserId user_id, u32 slot_id,
                       std::string_view game_serial, size_t memory_size) {
    std::scoped_lock lck{g_slot_mtx};

    const fs::path save_dir = GetSavePath(user_id, slot_id, game_serial);
    const std::string dir_name = GetSaveDir(slot_id);

    auto& data = g_attached_slots[slot_id];
    data = SlotData{
        .user_id = user_id,
        .game_serial = std::string{game_serial},
        .folder_path = save_dir,
        .sfo = {},
        .memory_cache = {},
        .memory_cache_size = memory_size,
    };

    SaveInstance::SetupDefaultParamSFO(data.sfo, dir_name, game_serial, user_id, 96);

    const fs::path param_sfo_path = SaveInstance::GetParamSFOPath(save_dir);
    std::error_code ec;
    size_t existing_size = 0;

    if (fs::exists(param_sfo_path, ec)) {
        if (!data.sfo.Open(param_sfo_path) || fs::exists(save_dir / CorruptFileName, ec)) {
            if (!Backup::Restore(user_id, game_serial, dir_name) ||
                !data.sfo.Open(param_sfo_path)) {
                SaveInstance::SetupDefaultParamSFO(data.sfo, dir_name, game_serial, user_id, 96);
            }
        }
        const fs::path memory = save_dir / FilenameSaveDataMemory;
        if (fs::exists(memory, ec)) {
            existing_size = static_cast<size_t>(fs::file_size(memory, ec));
        }
    } else {
        fs::create_directories(param_sfo_path.parent_path(), ec);
    }

    const size_t alloc_size = std::max(memory_size, existing_size);
    data.memory_cache.assign(alloc_size, 0);
    if (existing_size > 0) {
        IOFile f{save_dir / FilenameSaveDataMemory, Common::FS::FileAccessMode::Read};
        if (f.IsOpen()) {
            f.Seek(0);
            f.ReadRaw<u8>(data.memory_cache.data(), existing_size);
        }
    }

    if (existing_size == 0 && memory_size > 0) {
        const auto backup_lck = Backup::LockForWrite();
        PersistMemoryLocked(data);
        SaveSFOLocked(data, slot_id);
    }

    return existing_size;
}

bool SetIcon(u32 slot_id, const void* buf, size_t buf_size) {
    std::scoped_lock lck{g_slot_mtx};
    const auto it = g_attached_slots.find(slot_id);
    if (it == g_attached_slots.end()) {
        return false;
    }
    const auto backup_lck = Backup::LockForWrite();
    const auto& data = it->second;
    const fs::path icon_path = data.folder_path / SceSysDir / IconName;
    const fs::path meta_icon = GetMetaIconPath(data.user_id, data.game_serial, GetSaveDir(slot_id));
    std::error_code ec;

    if (buf == nullptr) {
        fs::remove(icon_path, ec);
        fs::remove(meta_icon, ec);
        if (auto bytes = g_mnt->ReadFile("/app0/sce_sys/save_data.png")) {
            fs::create_directories(icon_path.parent_path(), ec);
            IOFile dst{icon_path, Common::FS::FileAccessMode::Create};
            if (dst.IsOpen()) {
                dst.WriteRaw<u8>(bytes->data(), bytes->size());
                dst.Close();
            }
            fs::create_directories(meta_icon.parent_path(), ec);
            IOFile meta_dst{meta_icon, Common::FS::FileAccessMode::Create};
            if (meta_dst.IsOpen()) {
                meta_dst.WriteRaw<u8>(bytes->data(), bytes->size());
                meta_dst.Close();
            }
        }
        return true;
    }

    const std::span<const u8> span{static_cast<const u8*>(buf), buf_size};
    if (!ValidateIconPng(span)) {
        return false;
    }

    fs::create_directories(icon_path.parent_path(), ec);
    IOFile file{icon_path, Common::FS::FileAccessMode::Create};
    if (!file.IsOpen()) {
        return false;
    }
    const size_t written = file.WriteRaw<u8>(span.data(), span.size());
    file.Close();
    if (written != span.size()) {
        return false;
    }

    fs::create_directories(meta_icon.parent_path(), ec);
    IOFile meta_file{meta_icon, Common::FS::FileAccessMode::Create};
    if (meta_file.IsOpen()) {
        meta_file.WriteRaw<u8>(span.data(), span.size());
        meta_file.Close();
    }
    return true;
}

bool IsSaveMemoryInitialized(u32 slot_id) {
    std::scoped_lock lck{g_slot_mtx};
    return g_attached_slots.contains(slot_id);
}

bool IsSlotOwnedByUser(u32 slot_id, Libraries::UserService::OrbisUserServiceUserId user_id) {
    std::scoped_lock lck{g_slot_mtx};
    const auto it = g_attached_slots.find(slot_id);
    return it != g_attached_slots.end() && it->second.user_id == user_id;
}

size_t GetMemorySize(u32 slot_id) {
    std::scoped_lock lck{g_slot_mtx};
    const auto it = g_attached_slots.find(slot_id);
    return it != g_attached_slots.end() ? it->second.memory_cache_size : 0;
}

PSF& GetParamSFO(u32 slot_id) {
    std::scoped_lock lck{g_slot_mtx};
    return g_attached_slots[slot_id].sfo;
}

std::vector<u8> GetIcon(u32 slot_id) {
    std::scoped_lock lck{g_slot_mtx};
    const auto it = g_attached_slots.find(slot_id);
    if (it == g_attached_slots.end()) {
        return {};
    }
    const fs::path icon_path = it->second.folder_path / SceSysDir / IconName;
    IOFile f{icon_path, Common::FS::FileAccessMode::Read};
    if (!f.IsOpen()) {
        return {};
    }
    const u64 size = f.GetSize();
    std::vector<u8> ret(size);
    if (size > 0) {
        f.ReadSpan(std::span{ret});
    }
    return ret;
}

bool SaveSFO(u32 slot_id) {
    std::scoped_lock lck{g_slot_mtx};
    const auto it = g_attached_slots.find(slot_id);
    if (it == g_attached_slots.end()) {
        return false;
    }
    const auto backup_lck = Backup::LockForWrite();
    return SaveSFOLocked(it->second, slot_id);
}

bool ReadMemory(u32 slot_id, void* buf, size_t buf_size, int64_t offset) {
    std::scoped_lock lk{g_slot_mtx};
    const auto it = g_attached_slots.find(slot_id);
    if (it == g_attached_slots.end() || offset < 0) {
        return false;
    }
    auto& data = it->second;
    auto& memory = data.memory_cache;
    if (static_cast<size_t>(offset) >= data.memory_cache_size) {
        return false;
    }
    const size_t avail =
        std::min(memory.size(), data.memory_cache_size) - static_cast<size_t>(offset);
    if (buf_size > avail) {
        return false;
    }
    std::memcpy(buf, memory.data() + offset, buf_size);
    return true;
}

bool WriteMemory(u32 slot_id, const void* buf, size_t buf_size, int64_t offset) {
    std::scoped_lock lk{g_slot_mtx};
    const auto it = g_attached_slots.find(slot_id);
    if (it == g_attached_slots.end() || offset < 0) {
        return false;
    }
    auto& data = it->second;
    auto& memory = data.memory_cache;
    if (static_cast<size_t>(offset) + buf_size > data.memory_cache_size) {
        return false;
    }
    if (static_cast<size_t>(offset) + buf_size > memory.size()) {
        memory.resize(static_cast<size_t>(offset) + buf_size);
    }
    std::memcpy(memory.data() + offset, buf, buf_size);
    const auto backup_lck = Backup::LockForWrite();
    if (!PersistMemoryLocked(data)) {
        return false;
    }
    SaveSFOLocked(data, slot_id);
    Backup::NewRequest(data.user_id, data.game_serial, GetSaveDir(slot_id),
                       Backup::OrbisSaveDataEventType::__DO_NOT_SAVE);
    return true;
}

bool RestoreLoad(Libraries::UserService::OrbisUserServiceUserId user_id, u32 slot_id,
                 std::string_view game_serial) {
    const std::string dir_name = GetSaveDir(slot_id);
    if (!Backup::Restore(user_id, game_serial, dir_name)) {
        return false;
    }
    std::scoped_lock lk{g_slot_mtx};
    const auto it = g_attached_slots.find(slot_id);
    if (it != g_attached_slots.end() && it->second.user_id == user_id) {
        const fs::path memory_file = it->second.folder_path / FilenameSaveDataMemory;
        std::error_code ec;
        if (fs::exists(memory_file, ec)) {
            const size_t file_size = static_cast<size_t>(fs::file_size(memory_file, ec));
            std::fill(it->second.memory_cache.begin(), it->second.memory_cache.end(), 0);
            const size_t to_read = std::min(file_size, it->second.memory_cache.size());
            IOFile f{memory_file, Common::FS::FileAccessMode::Read};
            if (f.IsOpen() && to_read > 0) {
                f.ReadRaw<u8>(it->second.memory_cache.data(), to_read);
            }
        }
        it->second.sfo.Open(SaveInstance::GetParamSFOPath(it->second.folder_path));
    }
    return true;
}

} // namespace Libraries::SaveData::SaveMemory
