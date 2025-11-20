// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "save_memory.h"

#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <thread>
#include <utility>
#include <fmt/format.h>

#include "boost/icl/concept/interval.hpp"
#include "common/elf_info.h"
#include "common/logging/log.h"
#include "common/path_util.h"
#include "common/singleton.h"
#include "common/thread.h"
#include "core/file_sys/fs.h"
#include "core/libraries/system/msgdialog_ui.h"
#include "save_instance.h"

using Common::FS::IOFile;
namespace fs = std::filesystem;

constexpr std::string_view sce_sys = "sce_sys"; // system folder inside save
constexpr std::string_view StandardDirnameSaveDataMemory = "sce_sdmemory";
constexpr std::string_view FilenameSaveDataMemory = "memory.dat";
constexpr std::string_view IconName = "icon0.png";
constexpr std::string_view CorruptFileName = "corrupted";

namespace Libraries::SaveData::SaveMemory {

static Core::FileSys::MntPoints* g_mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();

struct SlotData {
    OrbisUserServiceUserId user_id{};
    std::string game_serial;
    std::filesystem::path folder_path;
    PSF sfo;
    std::vector<u8> memory_cache;
    size_t memory_cache_size{};
};

static std::mutex g_slot_mtx;
static std::unordered_map<u32, SlotData> g_attached_slots;

void PersistMemory(u32 slot_id, bool lock) {
    std::unique_lock lck{g_slot_mtx, std::defer_lock};
    if (lock) {
        lck.lock();
    }
    auto& data = g_attached_slots[slot_id];
    auto memoryPath = data.folder_path / FilenameSaveDataMemory;
    fs::create_directories(memoryPath.parent_path());

    int n = 0;
    std::string errMsg;
    while (n++ < 10) {
        try {
            IOFile f;
            int r = f.Open(memoryPath, Common::FS::FileAccessMode::Create);
            if (f.IsOpen()) {
                f.WriteRaw<u8>(data.memory_cache.data(), data.memory_cache.size());
                f.Close();
                return;
            }
            const auto err = std::error_code{r, std::iostream_category()};
            throw std::filesystem::filesystem_error{err.message(), err};
        } catch (const std::filesystem::filesystem_error& e) {
            errMsg = std::string{e.what()};
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    const MsgDialog::MsgDialogState dialog{MsgDialog::MsgDialogState::UserState{
        .type = MsgDialog::ButtonType::OK,
        .msg = "Failed to persist save memory:\n" + errMsg + "\nat " +
               Common::FS::PathToUTF8String(memoryPath),
    }};
    MsgDialog::ShowMsgDialog(dialog);
}

std::string GetSaveDir(u32 slot_id) {
    std::string dir(StandardDirnameSaveDataMemory);
    if (slot_id > 0) {
        dir += std::to_string(slot_id);
    }
    return dir;
}

std::filesystem::path GetSavePath(OrbisUserServiceUserId user_id, u32 slot_id,
                                  std::string_view game_serial) {
    std::string dir(StandardDirnameSaveDataMemory);
    if (slot_id > 0) {
        dir += std::to_string(slot_id);
    }
    return SaveInstance::MakeDirSavePath(user_id, game_serial, dir);
}

size_t SetupSaveMemory(OrbisUserServiceUserId user_id, u32 slot_id, std::string_view game_serial,
                       size_t memory_size) {
    std::lock_guard lck{g_slot_mtx};

    const auto save_dir = GetSavePath(user_id, slot_id, game_serial);

    auto& data = g_attached_slots[slot_id];
    data = SlotData{
        .user_id = user_id,
        .game_serial = std::string{game_serial},
        .folder_path = save_dir,
        .sfo = {},
        .memory_cache = {},
        .memory_cache_size = memory_size,
    };

    SaveInstance::SetupDefaultParamSFO(data.sfo, GetSaveDir(slot_id), std::string{game_serial});

    auto param_sfo_path = SaveInstance::GetParamSFOPath(save_dir);
    if (!fs::exists(param_sfo_path)) {
        return 0;
    }

    if (!data.sfo.Open(param_sfo_path) || fs::exists(save_dir / CorruptFileName)) {
        if (!Backup::Restore(save_dir)) { // Could not restore the backup
            return 0;
        }
    }

    const auto memory = save_dir / FilenameSaveDataMemory;
    if (fs::exists(memory)) {
        return fs::file_size(memory);
    }

    return 0;
}

void SetIcon(u32 slot_id, void* buf, size_t buf_size) {
    std::lock_guard lck{g_slot_mtx};
    const auto& data = g_attached_slots[slot_id];
    const auto icon_path = data.folder_path / sce_sys / "icon0.png";
    if (buf == nullptr) {
        const auto& src_icon = g_mnt->GetHostPath("/app0/sce_sys/save_data.png");
        if (fs::exists(icon_path)) {
            fs::remove(icon_path);
        }
        if (fs::exists(src_icon)) {
            fs::create_directories(icon_path.parent_path());
            fs::copy_file(src_icon, icon_path);
        }
    } else {
        IOFile file(icon_path, Common::FS::FileAccessMode::Create);
        file.WriteRaw<u8>(buf, buf_size);
        file.Close();
    }
}

bool IsSaveMemoryInitialized(u32 slot_id) {
    std::lock_guard lck{g_slot_mtx};
    return g_attached_slots.contains(slot_id);
}

PSF& GetParamSFO(u32 slot_id) {
    std::lock_guard lck{g_slot_mtx};
    auto& data = g_attached_slots[slot_id];
    return data.sfo;
}

std::vector<u8> GetIcon(u32 slot_id) {
    std::lock_guard lck{g_slot_mtx};
    auto& data = g_attached_slots[slot_id];
    const auto icon_path = data.folder_path / sce_sys / "icon0.png";
    IOFile f{icon_path, Common::FS::FileAccessMode::Read};
    if (!f.IsOpen()) {
        return {};
    }
    const u64 size = f.GetSize();
    std::vector<u8> ret;
    ret.resize(size);
    f.ReadSpan(std::span{ret});
    return ret;
}

void SaveSFO(u32 slot_id) {
    std::lock_guard lck{g_slot_mtx};
    const auto& data = g_attached_slots[slot_id];
    const auto sfo_path = SaveInstance::GetParamSFOPath(data.folder_path);
    fs::create_directories(sfo_path.parent_path());
    const bool ok = data.sfo.Encode(sfo_path);
    if (!ok) {
        LOG_ERROR(Lib_SaveData, "Failed to encode param.sfo");
        throw std::filesystem::filesystem_error("Failed to write param.sfo", sfo_path,
                                                std::make_error_code(std::errc::permission_denied));
    }
}

void ReadMemory(u32 slot_id, void* buf, size_t buf_size, int64_t offset) {
    std::lock_guard lk{g_slot_mtx};
    auto& data = g_attached_slots[slot_id];
    auto& memory = data.memory_cache;
    if (memory.empty()) { // Load file
        memory.resize(data.memory_cache_size);
        IOFile f{data.folder_path / FilenameSaveDataMemory, Common::FS::FileAccessMode::Read};
        if (f.IsOpen()) {
            f.Seek(0);
            f.ReadSpan(std::span{memory});
        }
    }
    s64 read_size = buf_size;
    if (read_size + offset > memory.size()) {
        read_size = memory.size() - offset;
    }
    std::memcpy(buf, memory.data() + offset, read_size);
}

void WriteMemory(u32 slot_id, void* buf, size_t buf_size, int64_t offset) {
    std::lock_guard lk{g_slot_mtx};
    auto& data = g_attached_slots[slot_id];
    auto& memory = data.memory_cache;
    if (offset + buf_size > memory.size()) {
        memory.resize(offset + buf_size);
    }
    std::memcpy(memory.data() + offset, buf, buf_size);
    PersistMemory(slot_id, false);
    Backup::NewRequest(data.user_id, data.game_serial, GetSaveDir(slot_id),
                       Backup::OrbisSaveDataEventType::__DO_NOT_SAVE);
}
} // namespace Libraries::SaveData::SaveMemory