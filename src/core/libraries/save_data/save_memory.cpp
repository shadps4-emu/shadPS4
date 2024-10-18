// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "save_memory.h"

#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <utility>
#include <fmt/format.h>

#include <core/libraries/system/msgdialog_ui.h>

#include "common/assert.h"
#include "common/logging/log.h"
#include "common/polyfill_thread.h"
#include "common/singleton.h"
#include "common/thread.h"
#include "core/file_sys/fs.h"
#include "save_instance.h"

using Common::FS::IOFile;
namespace fs = std::filesystem;

constexpr std::string_view sce_sys = "sce_sys"; // system folder inside save
constexpr std::string_view DirnameSaveDataMemory = "sce_sdmemory";
constexpr std::string_view FilenameSaveDataMemory = "memory.dat";

namespace Libraries::SaveData::SaveMemory {

static Core::FileSys::MntPoints* g_mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();

static OrbisUserServiceUserId g_user_id{};
static std::string g_game_serial{};
static std::filesystem::path g_save_path{};
static std::filesystem::path g_param_sfo_path{};
static PSF g_param_sfo;

static bool g_save_memory_initialized = false;
static std::mutex g_saving_memory_mutex;
static std::vector<u8> g_save_memory;

static std::filesystem::path g_icon_path;
static std::vector<u8> g_icon_memory;

static std::condition_variable g_trigger_save_memory;
static std::atomic_bool g_saving_memory = false;
static std::atomic_bool g_save_event = false;
static std::jthread g_save_memory_thread;

static std::atomic_bool g_memory_dirty = false;
static std::atomic_bool g_param_dirty = false;
static std::atomic_bool g_icon_dirty = false;

static void SaveFileSafe(void* buf, size_t count, const std::filesystem::path& path) {
    const auto& dir = path.parent_path();
    const auto& name = path.filename();
    const auto tmp_path = dir / (name.string() + ".tmp");

    IOFile file(tmp_path, Common::FS::FileAccessMode::Write);
    file.WriteRaw<u8>(buf, count);
    file.Close();

    fs::remove(path);
    fs::rename(tmp_path, path);
}

[[noreturn]] void SaveThreadLoop() {
    Common::SetCurrentThreadName("shadPS4:SaveData_SaveDataMemoryThread");
    std::mutex mtx;
    while (true) {
        {
            std::unique_lock lk{mtx};
            g_trigger_save_memory.wait(lk);
        }
        // Save the memory
        g_saving_memory = true;
        std::scoped_lock lk{g_saving_memory_mutex};
        try {
            LOG_DEBUG(Lib_SaveData, "Saving save data memory {}", fmt::UTF(g_save_path.u8string()));

            if (g_memory_dirty) {
                g_memory_dirty = false;
                SaveFileSafe(g_save_memory.data(), g_save_memory.size(),
                             g_save_path / FilenameSaveDataMemory);
            }
            if (g_param_dirty) {
                g_param_dirty = false;
                static std::vector<u8> buf;
                g_param_sfo.Encode(buf);
                SaveFileSafe(buf.data(), buf.size(), g_param_sfo_path);
            }
            if (g_icon_dirty) {
                g_icon_dirty = false;
                SaveFileSafe(g_icon_memory.data(), g_icon_memory.size(), g_icon_path);
            }

            if (g_save_event) {
                Backup::PushBackupEvent(Backup::BackupRequest{
                    .user_id = g_user_id,
                    .title_id = g_game_serial,
                    .dir_name = std::string{DirnameSaveDataMemory},
                    .origin = Backup::OrbisSaveDataEventType::SAVE_DATA_MEMORY_SYNC,
                    .save_path = g_save_path,
                });
                g_save_event = false;
            }
        } catch (const fs::filesystem_error& e) {
            LOG_ERROR(Lib_SaveData, "Failed to save save data memory: {}", e.what());
            MsgDialog::ShowMsgDialog(MsgDialog::MsgDialogState{
                MsgDialog::MsgDialogState::UserState{
                    .type = MsgDialog::ButtonType::OK,
                    .msg = fmt::format("Failed to save save data memory.\nCode: <{}>\n{}",
                                       e.code().message(), e.what()),
                },
            });
        }
        g_saving_memory = false;
    }
}

void SetDirectories(OrbisUserServiceUserId user_id, std::string _game_serial) {
    g_user_id = user_id;
    g_game_serial = std::move(_game_serial);
    g_save_path = SaveInstance::MakeDirSavePath(user_id, g_game_serial, DirnameSaveDataMemory);
    g_param_sfo_path = SaveInstance::GetParamSFOPath(g_save_path);
    g_param_sfo = PSF();
    g_icon_path = g_save_path / sce_sys / "icon0.png";
}

const std::filesystem::path& GetSavePath() {
    return g_save_path;
}

size_t CreateSaveMemory(size_t memory_size) {
    size_t existed_size = 0;

    static std::once_flag init_save_thread_flag;
    std::call_once(init_save_thread_flag,
                   [] { g_save_memory_thread = std::jthread{SaveThreadLoop}; });

    g_save_memory.resize(memory_size);
    SaveInstance::SetupDefaultParamSFO(g_param_sfo, std::string{DirnameSaveDataMemory},
                                       g_game_serial);

    g_save_memory_initialized = true;

    if (!fs::exists(g_param_sfo_path)) {
        // Create save memory
        fs::create_directories(g_save_path / sce_sys);

        IOFile memory_file{g_save_path / FilenameSaveDataMemory, Common::FS::FileAccessMode::Write};
        bool ok = memory_file.SetSize(memory_size);
        if (!ok) {
            LOG_ERROR(Lib_SaveData, "Failed to set memory size");
            throw std::filesystem::filesystem_error(
                "Failed to set save memory size", g_save_path / FilenameSaveDataMemory,
                std::make_error_code(std::errc::no_space_on_device));
        }
        memory_file.Close();
    } else {
        // Load save memory

        bool ok = g_param_sfo.Open(g_param_sfo_path);
        if (!ok) {
            LOG_ERROR(Lib_SaveData, "Failed to open SFO at {}",
                      fmt::UTF(g_param_sfo_path.u8string()));
            throw std::filesystem::filesystem_error(
                "failed to open SFO", g_param_sfo_path,
                std::make_error_code(std::errc::illegal_byte_sequence));
        }

        IOFile memory_file{g_save_path / FilenameSaveDataMemory, Common::FS::FileAccessMode::Read};
        if (!memory_file.IsOpen()) {
            LOG_ERROR(Lib_SaveData, "Failed to open save memory");
            throw std::filesystem::filesystem_error(
                "failed to open save memory", g_save_path / FilenameSaveDataMemory,
                std::make_error_code(std::errc::permission_denied));
        }
        size_t save_size = memory_file.GetSize();
        existed_size = save_size;
        memory_file.Seek(0);
        memory_file.ReadRaw<u8>(g_save_memory.data(), std::min(save_size, memory_size));
        memory_file.Close();
    }

    return existed_size;
}

void SetIcon(void* buf, size_t buf_size) {
    if (buf == nullptr) {
        const auto& src_icon = g_mnt->GetHostPath("/app0/sce_sys/save_data.png");
        if (fs::exists(src_icon)) {
            if (fs::exists(g_icon_path)) {
                fs::remove(g_icon_path);
            }
            fs::copy_file(src_icon, g_icon_path);
        }
        if (fs::exists(g_icon_path)) {
            IOFile file(g_icon_path, Common::FS::FileAccessMode::Read);
            size_t size = file.GetSize();
            file.Seek(0);
            g_icon_memory.resize(size);
            file.ReadRaw<u8>(g_icon_memory.data(), size);
            file.Close();
        }
    } else {
        g_icon_memory.resize(buf_size);
        std::memcpy(g_icon_memory.data(), buf, buf_size);
        IOFile file(g_icon_path, Common::FS::FileAccessMode::Write);
        file.Seek(0);
        file.WriteRaw<u8>(g_icon_memory.data(), buf_size);
        file.Close();
    }
}

void WriteIcon(void* buf, size_t buf_size) {
    if (buf_size != g_icon_memory.size()) {
        g_icon_memory.resize(buf_size);
    }
    std::memcpy(g_icon_memory.data(), buf, buf_size);
    g_icon_dirty = true;
}

bool IsSaveMemoryInitialized() {
    return g_save_memory_initialized;
}

PSF& GetParamSFO() {
    return g_param_sfo;
}

std::span<u8> GetIcon() {
    return {g_icon_memory};
}

void SaveSFO(bool sync) {
    if (!sync) {
        g_param_dirty = true;
        return;
    }
    const bool ok = g_param_sfo.Encode(g_param_sfo_path);
    if (!ok) {
        LOG_ERROR(Lib_SaveData, "Failed to encode param.sfo");
        throw std::filesystem::filesystem_error("Failed to write param.sfo", g_param_sfo_path,
                                                std::make_error_code(std::errc::permission_denied));
    }
}
bool IsSaving() {
    return g_saving_memory;
}

bool TriggerSaveWithoutEvent() {
    if (g_saving_memory) {
        return false;
    }
    g_trigger_save_memory.notify_one();
    return true;
}

bool TriggerSave() {
    if (g_saving_memory) {
        return false;
    }
    g_save_event = true;
    g_trigger_save_memory.notify_one();
    return true;
}

void ReadMemory(void* buf, size_t buf_size, int64_t offset) {
    std::scoped_lock lk{g_saving_memory_mutex};
    if (offset + buf_size > g_save_memory.size()) {
        UNREACHABLE_MSG("ReadMemory out of bounds");
    }
    std::memcpy(buf, g_save_memory.data() + offset, buf_size);
}

void WriteMemory(void* buf, size_t buf_size, int64_t offset) {
    std::scoped_lock lk{g_saving_memory_mutex};
    if (offset + buf_size > g_save_memory.size()) {
        g_save_memory.resize(offset + buf_size);
    }
    std::memcpy(g_save_memory.data() + offset, buf, buf_size);
    g_memory_dirty = true;
}

} // namespace Libraries::SaveData::SaveMemory