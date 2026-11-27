// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "common/assert.h"
#include "common/elf_info.h"
#include "common/logging/log.h"
#include "common/path_util.h"
#include "common/singleton.h"
#include "core/file_sys/fs.h"
#include "core/libraries/kernel/process.h"
#include "core/libraries/libs.h"
#include "core/libraries/save_data/save_backup.h"
#include "core/libraries/save_data/save_db.h"
#include "core/libraries/save_data/save_instance.h"
#include "core/libraries/save_data/save_memory.h"
#include "core/libraries/save_data/savedata.h"
#include "core/libraries/save_data/savedata_error.h"
#include "core/libraries/system/userservice.h"
#include "core/user_manager.h"
#include "core/user_settings.h"

namespace Libraries::SaveData {

namespace fs = std::filesystem;

namespace {

struct ActiveMountSlot {
    int slot_index = -1;
    std::unique_ptr<SaveInstance> instance;
};

std::recursive_mutex g_mutex;
bool g_initialized = false;
bool g_shutting_down = false;
int g_init_count = 0;

std::string g_test_title_id;
u32 g_test_sdk_ver = 0;
u64 g_last_required_blocks = 0;
bool g_abort_requested = false;
bool g_fake_broken_supported = false;
s32 g_library_user = Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID;
u32 g_client_thread_priority = 700;

std::vector<ActiveMountSlot> g_mount_slots;
std::vector<std::string> g_sys_mounts;

OrbisSaveDataEventCallbackFunc g_event_callback = nullptr;
void* g_event_userdata = nullptr;
bool g_has_last_event_result = false;
OrbisSaveDataEventResult g_last_event_result{};
OrbisSaveDataEventInfo g_stored_event_info{};
bool g_has_stored_event_info = false;

std::string GetGameTitleId() {
    if (!g_test_title_id.empty()) {
        return g_test_title_id;
    }
    if (Common::ElfInfo::Instance().IsInitialized()) {
        return std::string{Common::ElfInfo::Instance().GameSerial()};
    }
    return "CUSA00000";
}

u32 GetCompiledSdkVersion() {
    if (g_test_sdk_ver != 0) {
        return g_test_sdk_ver;
    }
    if (Common::ElfInfo::Instance().IsInitialized()) {
        return Common::ElfInfo::Instance().CompiledSdkVer();
    }
    return Common::ElfInfo::FW_1150;
}

s32 Translate(OrbisSaveDataCommand cmd, u32 internal_err) {
    return static_cast<s32>(TranslateInternalError(cmd, GetCompiledSdkVersion(), internal_err));
}

bool ValidateReservedZero(const void* data, size_t size) {
    const auto* bytes = static_cast<const u8*>(data);
    for (size_t i = 0; i < size; ++i) {
        if (bytes[i] != 0) {
            return false;
        }
    }
    return true;
}

bool IsValidUtf8(std::string_view str) {
    const size_t n = str.size();
    size_t i = 0;
    while (i < n) {
        const u8 b0 = static_cast<u8>(str[i]);
        if (b0 <= 0x7F) {
            ++i;
            continue;
        }
        size_t len = 0;
        u32 cp = 0;
        u32 min_cp = 0;
        if ((b0 & 0xE0) == 0xC0) {
            len = 2;
            cp = b0 & 0x1F;
            min_cp = 0x80;
        } else if ((b0 & 0xF0) == 0xE0) {
            len = 3;
            cp = b0 & 0x0F;
            min_cp = 0x800;
        } else if ((b0 & 0xF8) == 0xF0) {
            len = 4;
            cp = b0 & 0x07;
            min_cp = 0x10000;
        } else {
            return false;
        }
        if (i + len > n) {
            return false;
        }
        for (size_t j = 1; j < len; ++j) {
            const u8 bx = static_cast<u8>(str[i + j]);
            if ((bx & 0xC0) != 0x80) {
                return false;
            }
            cp = (cp << 6) | (bx & 0x3F);
        }
        if (cp < min_cp || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
            return false;
        }
        i += len;
    }
    return true;
}

u32 ValidateUserId(Libraries::UserService::OrbisUserServiceUserId user_id) {
    if (user_id <= 0) {
        return static_cast<u32>(OrbisSaveDataError::INVALID_LOGIN_USER);
    }
    const auto& users = UserManagement.GetAllUsers();
    if (!users.empty() && UserManagement.GetUserByID(user_id) == nullptr) {
        return static_cast<u32>(OrbisSaveDataError::INVALID_LOGIN_USER);
    }
    return 0;
}

u32 ValidateTitleId(const OrbisSaveDataTitleId* title_id, bool allow_null, std::string& out_tid) {
    if (title_id == nullptr) {
        if (!allow_null) {
            return static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT);
        }
        out_tid = GetGameTitleId();
        return 0;
    }
    if (title_id->data[9] != '\0') {
        return static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT);
    }
    for (size_t i = 10; i < sizeof(title_id->data); ++i) {
        if (title_id->data[i] != '\0') {
            return static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT);
        }
    }
    for (int i = 0; i < 4; ++i) {
        const unsigned char c = static_cast<unsigned char>(title_id->data[i]);
        if (c < 'A' || c > 'Z') {
            return static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT);
        }
    }
    for (int i = 4; i < 9; ++i) {
        const unsigned char c = static_cast<unsigned char>(title_id->data[i]);
        if (c < '0' || c > '9') {
            return static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT);
        }
    }
    out_tid.assign(title_id->data, 9);
    return 0;
}

bool IsReservedSaveMemoryDirName(std::string_view dir_name) {
    return dir_name == "sce_sdmemory" || dir_name == "sce_sdmemory1" ||
           dir_name == "sce_sdmemory2" || dir_name == "sce_sdmemory3";
}

u32 ValidateDirName(const OrbisSaveDataDirName* dir_name, bool allow_null, bool allow_sdmemory,
                    std::string& out_dir) {
    if (dir_name == nullptr) {
        if (!allow_null) {
            return static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT);
        }
        out_dir.clear();
        return 0;
    }
    const size_t len = strnlen(dir_name->data, ORBIS_SAVE_DATA_DIRNAME_DATA_MAXSIZE);
    if (len == 0 || len >= ORBIS_SAVE_DATA_DIRNAME_DATA_MAXSIZE) {
        return static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT);
    }
    for (size_t i = 0; i < len; ++i) {
        const unsigned char c = static_cast<unsigned char>(dir_name->data[i]);
        const bool ok = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
                        (c >= 'a' && c <= 'z') || c == '_' || c == '-';
        if (!ok) {
            return static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT);
        }
    }
    std::string_view sv(dir_name->data, len);
    if (!allow_sdmemory && IsReservedSaveMemoryDirName(sv)) {
        return static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT);
    }
    out_dir.assign(sv);
    return 0;
}

u32 ValidateFingerprint(const OrbisSaveDataFingerprint* fp, bool allow_null) {
    if (fp == nullptr) {
        return allow_null ? 0 : static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT);
    }
    if (!ValidateReservedZero(fp->padding, sizeof(fp->padding))) {
        return static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT);
    }
    const size_t len = strnlen(fp->data, ORBIS_SAVE_DATA_FINGERPRINT_DATA_SIZE);
    if (len >= ORBIS_SAVE_DATA_FINGERPRINT_DATA_SIZE) {
        return static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT);
    }
    if (len == 0) {
        return 0;
    }
    if (len != 64) {
        return static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT);
    }
    for (size_t i = 0; i < 64; ++i) {
        if (!std::isxdigit(static_cast<unsigned char>(fp->data[i]))) {
            return static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT);
        }
    }
    return 0;
}

u32 ValidateMountPoint(const OrbisSaveDataMountPoint* mp, std::string& out_mp) {
    if (mp == nullptr) {
        return static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT);
    }
    const size_t len = strnlen(mp->data, ORBIS_SAVE_DATA_MOUNT_POINT_DATA_MAXSIZE);
    if (len < 2 || len >= ORBIS_SAVE_DATA_MOUNT_POINT_DATA_MAXSIZE || mp->data[0] != '/') {
        return static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT);
    }
    for (size_t i = 1; i < len; ++i) {
        const unsigned char c = static_cast<unsigned char>(mp->data[i]);
        const bool ok = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
                        (c >= 'a' && c <= 'z') || c == '_' || c == '-';
        if (!ok) {
            return static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT);
        }
    }
    out_mp.assign(mp->data, len);
    return 0;
}

u32 ValidateStringField(const char* buf, size_t max_bytes, bool allow_newline) {
    const size_t len = strnlen(buf, max_bytes);
    if (len >= max_bytes) {
        return static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT);
    }
    std::string_view sv(buf, len);
    if (!IsValidUtf8(sv)) {
        return static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT);
    }
    if (!allow_newline) {
        for (char c : sv) {
            if (c == '\r' || c == '\n') {
                return static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT);
            }
        }
    }
    return 0;
}

u32 ValidateParam(const OrbisSaveDataParam* param, OrbisSaveDataParamType type,
                  bool check_reserved_tail) {
    if (param == nullptr) {
        return static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT);
    }
    switch (type) {
    case OrbisSaveDataParamType::ALL:
        if (check_reserved_tail &&
            !ValidateReservedZero(param->reserved, sizeof(param->reserved))) {
            return static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT);
        }
        if (auto err = ValidateStringField(param->title, ORBIS_SAVE_DATA_TITLE_MAXSIZE, false)) {
            return err;
        }
        if (auto err =
                ValidateStringField(param->subTitle, ORBIS_SAVE_DATA_SUBTITLE_MAXSIZE, false)) {
            return err;
        }
        return ValidateStringField(param->detail, ORBIS_SAVE_DATA_DETAIL_MAXSIZE, true);
    case OrbisSaveDataParamType::TITLE:
        return ValidateStringField(param->title, ORBIS_SAVE_DATA_TITLE_MAXSIZE, false);
    case OrbisSaveDataParamType::SUB_TITLE:
        return ValidateStringField(param->subTitle, ORBIS_SAVE_DATA_SUBTITLE_MAXSIZE, false);
    case OrbisSaveDataParamType::DETAIL:
        return ValidateStringField(param->detail, ORBIS_SAVE_DATA_DETAIL_MAXSIZE, true);
    case OrbisSaveDataParamType::USER_PARAM:
    case OrbisSaveDataParamType::MTIME:
        return 0;
    default:
        return static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT);
    }
}

u32 ValidateMountMode(u32 mode, u64 blocks) {
    constexpr u32 allowed_bits = static_cast<u32>(OrbisSaveDataMountMode::RDONLY) |
                                 static_cast<u32>(OrbisSaveDataMountMode::RDWR) |
                                 static_cast<u32>(OrbisSaveDataMountMode::CREATE) |
                                 static_cast<u32>(OrbisSaveDataMountMode::DESTRUCTIVE) |
                                 static_cast<u32>(OrbisSaveDataMountMode::COPY_ICON) |
                                 static_cast<u32>(OrbisSaveDataMountMode::CREATE2);
    if (mode == 0 || (mode & ~allowed_bits) != 0) {
        return static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT);
    }
    const bool rdonly = (mode & static_cast<u32>(OrbisSaveDataMountMode::RDONLY)) != 0;
    const bool rdwr = (mode & static_cast<u32>(OrbisSaveDataMountMode::RDWR)) != 0;
    const bool create = (mode & static_cast<u32>(OrbisSaveDataMountMode::CREATE)) != 0;
    const bool create2 = (mode & static_cast<u32>(OrbisSaveDataMountMode::CREATE2)) != 0;
    const bool destructive = (mode & static_cast<u32>(OrbisSaveDataMountMode::DESTRUCTIVE)) != 0;
    const bool copy_icon = (mode & static_cast<u32>(OrbisSaveDataMountMode::COPY_ICON)) != 0;

    if (rdonly == rdwr) {
        return static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT);
    }
    if (rdonly && (create || create2 || destructive || copy_icon)) {
        return static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT);
    }
    if (create && create2) {
        return static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT);
    }
    if ((create || create2 || destructive) &&
        (blocks < OrbisSaveDataBlocksMin2 || blocks > OrbisSaveDataBlocksMax)) {
        return static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT);
    }
    return 0;
}

bool ParseVirtualSavePath(const char* path, s32& out_uid, std::string& out_tid,
                          std::string& out_dir) {
    if (path == nullptr) {
        return false;
    }
    std::string_view sv(path);
    constexpr std::string_view kPrefix = "/user/home/";
    if (!sv.starts_with(kPrefix)) {
        return false;
    }
    sv.remove_prefix(kPrefix.size());
    if (sv.size() < 8 + 10 + 9 + 2 || sv[8] != '/') {
        return false;
    }
    std::string_view uid_hex = sv.substr(0, 8);
    u32 uid_val = 0;
    for (char c : uid_hex) {
        uid_val <<= 4;
        if (c >= '0' && c <= '9') {
            uid_val |= static_cast<u32>(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            uid_val |= static_cast<u32>(c - 'a' + 10);
        } else {
            return false;
        }
    }
    if (uid_val == 0) {
        return false;
    }
    sv.remove_prefix(9);
    constexpr std::string_view kSaveMid = "savedata/";
    if (!sv.starts_with(kSaveMid)) {
        return false;
    }
    sv.remove_prefix(kSaveMid.size());
    if (sv.size() < 9 + 1 + 1 || sv[9] != '/') {
        return false;
    }
    OrbisSaveDataTitleId tid{};
    std::memcpy(tid.data, sv.data(), 9);
    if (ValidateTitleId(&tid, false, out_tid) != 0) {
        return false;
    }
    sv.remove_prefix(10);
    if (sv.empty() || sv.size() >= ORBIS_SAVE_DATA_DIRNAME_DATA_MAXSIZE) {
        return false;
    }
    OrbisSaveDataDirName dir{};
    std::memcpy(dir.data, sv.data(), sv.size());
    if (ValidateDirName(&dir, false, false, out_dir) != 0) {
        return false;
    }
    out_uid = static_cast<s32>(uid_val);
    return true;
}

bool ValidateSetupMemory2(const OrbisSaveDataMemorySetup2* setup, size_t& out_alloc_size) {
    if (setup == nullptr || setup->option >= 8 || setup->slotId >= OrbisSaveDataMemorySlotCount) {
        return false;
    }
    if (!ValidateReservedZero(setup->reserved, sizeof(setup->reserved))) {
        return false;
    }
    const bool large = (setup->option & 4U) != 0;
    const bool doubled = (setup->option & 2U) != 0;
    const size_t max_mem =
        large ? (doubled ? OrbisSaveDataMemoryLargeHalfMax : OrbisSaveDataMemoryMaxTotalSize)
              : (doubled ? OrbisSaveDataMemorySmallHalfMax : OrbisSaveDataMemorySmallMaxSize);
    if (setup->memorySize == 0 || setup->memorySize > max_mem) {
        return false;
    }
    if (setup->iconMemorySize > SaveInstance::IconMaxBytes) {
        return false;
    }
    if (setup->initParam != nullptr &&
        ValidateParam(setup->initParam, OrbisSaveDataParamType::ALL, true) != 0) {
        return false;
    }
    if (setup->initIcon != nullptr) {
        if (!ValidateReservedZero(setup->initIcon->reserved, sizeof(setup->initIcon->reserved))) {
            return false;
        }
        if (setup->initIcon->dataSize > 0 &&
            (setup->initIcon->buf == nullptr ||
             setup->initIcon->dataSize > SaveInstance::IconMaxBytes ||
             setup->initIcon->bufSize < setup->initIcon->dataSize)) {
            return false;
        }
        if (setup->initIcon->dataSize > 0) {
            std::span<const u8> png_span(static_cast<const u8*>(setup->initIcon->buf),
                                         setup->initIcon->dataSize);
            if (!ValidateIconPng(png_span)) {
                return false;
            }
        }
    }
    out_alloc_size = doubled ? (setup->memorySize * 2) : setup->memorySize;
    return true;
}

int AllocateMountSlotIndex() {
    for (int i = 0; i < static_cast<int>(OrbisSaveDataMaxMounts); ++i) {
        bool in_use = false;
        for (const auto& s : g_mount_slots) {
            if (s.slot_index == i) {
                in_use = true;
                break;
            }
        }
        if (!in_use) {
            return i;
        }
    }
    return -1;
}

ActiveMountSlot* FindMountSlotByMountPoint(std::string_view mp) {
    for (auto& slot : g_mount_slots) {
        if (slot.instance->GetMountPoint() == mp) {
            return &slot;
        }
    }
    return nullptr;
}

bool IsSaveMounted(s32 user_id, std::string_view title_id, std::string_view dir_name) {
    for (const auto& slot : g_mount_slots) {
        if (slot.instance->GetUserId() == user_id && slot.instance->GetTitleId() == title_id &&
            slot.instance->GetDirName() == dir_name) {
            return true;
        }
    }
    return false;
}

u32 CheckDiskSpaceForBlocks(const fs::path& probe_dir, u64 blocks, u64& out_required_blocks) {
    std::error_code ec;
    fs::create_directories(probe_dir, ec);
    const fs::space_info space = fs::space(probe_dir, ec);
    if (ec) {
        return 0;
    }
    const u64 need_bytes = blocks * ORBIS_SAVE_DATA_BLOCK_SIZE;
    if (space.available < need_bytes) {
        const u64 deficit = need_bytes - space.available;
        out_required_blocks =
            (deficit + ORBIS_SAVE_DATA_BLOCK_SIZE - 1) / ORBIS_SAVE_DATA_BLOCK_SIZE;
        g_last_required_blocks = out_required_blocks;
        return static_cast<u32>(OrbisSaveDataError::NO_SPACE_FS);
    }
    return 0;
}

void CopyIconToSave(const fs::path& save_dir, const fs::path& meta_icon) {
    const fs::path target_icon = save_dir / "sce_sys" / "icon0.png";
    std::error_code ec;
    if (fs::exists(target_icon, ec)) {
        fs::copy_file(target_icon, meta_icon, fs::copy_options::overwrite_existing, ec);
        return;
    }
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    const fs::path game_icon = mnt->GetHostPath("/app0/sce_sys/save_data.png");
    if (!game_icon.empty() && fs::exists(game_icon, ec)) {
        fs::create_directories(target_icon.parent_path(), ec);
        fs::copy_file(game_icon, target_icon, fs::copy_options::overwrite_existing, ec);
        fs::copy_file(game_icon, meta_icon, fs::copy_options::overwrite_existing, ec);
    }
}

u32 PerformMount(OrbisSaveDataCommand cmd, s32 user_id, const std::string& save_tid,
                 const std::string& dir_name, u64 blocks, u32 mount_mode,
                 OrbisSaveDataMountResult* result) {
    if (auto err = ValidateUserId(user_id)) {
        return err;
    }
    if (auto err = ValidateMountMode(mount_mode, blocks)) {
        return err;
    }
    if (g_shutting_down) {
        return static_cast<u32>(OrbisSaveDataError::BUSY);
    }
    if (IsSaveMounted(user_id, save_tid, dir_name)) {
        return static_cast<u32>(OrbisSaveDataError::BUSY);
    }
    if (Backup::IsBackupExecutingFor(GetSaveDirPath(user_id, save_tid, dir_name))) {
        return static_cast<u32>(OrbisSaveDataError::BUSY);
    }
    const int slot_idx = AllocateMountSlotIndex();
    if (slot_idx < 0) {
        return static_cast<u32>(OrbisSaveDataError::MOUNT_FULL);
    }

    const fs::path save_dir = GetSaveDirPath(user_id, save_tid, dir_name);
    const fs::path sfo_path = save_dir / "sce_sys" / "param.sfo";
    std::error_code ec;
    const bool already_exists = fs::exists(save_dir, ec) && fs::is_directory(save_dir, ec);

    const bool mode_create = (mount_mode & static_cast<u32>(OrbisSaveDataMountMode::CREATE)) != 0;
    const bool mode_create2 = (mount_mode & static_cast<u32>(OrbisSaveDataMountMode::CREATE2)) != 0;
    const bool mode_destructive =
        (mount_mode & static_cast<u32>(OrbisSaveDataMountMode::DESTRUCTIVE)) != 0;
    const bool mode_copy_icon =
        (mount_mode & static_cast<u32>(OrbisSaveDataMountMode::COPY_ICON)) != 0;
    const bool mode_rdonly = (mount_mode & static_cast<u32>(OrbisSaveDataMountMode::RDONLY)) != 0;

    if (already_exists && mode_create) {
        return static_cast<u32>(OrbisSaveDataError::EXISTS);
    }
    if (!already_exists && !mode_create && !mode_create2) {
        return static_cast<u32>(OrbisSaveDataError::NOT_FOUND);
    }

    // Check broken status recorded in DB if save directory already exists.
    if (already_exists && !mode_destructive) {
        if (auto db_entry = SaveDb::GetEntry(user_id, save_tid, dir_name);
            db_entry && db_entry->is_broken) {
            return static_cast<u32>(OrbisSaveDataError::BROKEN);
        }
        if (fs::exists(sfo_path, ec)) {
            PSF psf;
            if (!psf.Open(sfo_path)) {
                SaveDb::MarkBroken(user_id, save_tid, dir_name, true);
                return static_cast<u32>(OrbisSaveDataError::BROKEN);
            }
        }
    }

    const bool creating = !already_exists || mode_destructive;
    u64 required_blocks = 0;
    if (creating) {
        if (auto space_err = CheckDiskSpaceForBlocks(GetSaveTitlePath(user_id, save_tid), blocks,
                                                     required_blocks)) {
            if (result != nullptr) {
                std::memset(result, 0, sizeof(*result));
                result->requiredBlocks = required_blocks;
            }
            return space_err;
        }
    }

    if (already_exists && mode_destructive) {
        fs::remove_all(save_dir, ec);
        if (ec) {
            return static_cast<u32>(OrbisSaveDataError::INTERNAL);
        }
        SaveDb::Remove(user_id, save_tid, dir_name);
    }

    const u64 effective_blocks = creating ? blocks : std::max<u64>(blocks, OrbisSaveDataBlocksMin2);
    const std::string game_tid = GetGameTitleId();
    auto instance = std::make_unique<SaveInstance>(slot_idx, user_id, save_tid, game_tid, dir_name,
                                                   effective_blocks, mode_rdonly);
    if (!instance->Mount()) {
        return static_cast<u32>(OrbisSaveDataError::INTERNAL);
    }
    if (creating && mode_copy_icon) {
        CopyIconToSave(save_dir, GetMetaIconPath(user_id, save_tid, dir_name));
    }

    std::memset(result, 0, sizeof(*result));
    const std::string mp = instance->GetMountPoint();
    std::memcpy(result->mountPoint.data, mp.data(),
                std::min(mp.size(), sizeof(result->mountPoint.data) - 1));
    result->mountPoint.data[sizeof(result->mountPoint.data) - 1] = '\0';
    result->requiredBlocks = 0;
    result->mountStatus = creating ? static_cast<u32>(OrbisSaveDataMountStatus::CREATED)
                                   : static_cast<u32>(OrbisSaveDataMountStatus::EXISTING);
    g_mount_slots.push_back(ActiveMountSlot{slot_idx, std::move(instance)});
    (void)cmd;
    return 0;
}

std::vector<s32> CollectTargetUsers() {
    std::vector<s32> users;
    const auto& all = UserManagement.GetAllUsers();
    for (const auto& u : all) {
        if (u.user_id > 0) {
            users.push_back(u.user_id);
        }
    }
    if (users.empty() && g_library_user > 0) {
        users.push_back(g_library_user);
    }
    if (users.empty()) {
        users.push_back(1);
    }
    return users;
}

s32 DoDeleteSaveData(OrbisSaveDataCommand cmd, s32 user_id, const std::string& title_id,
                     const std::string& dir_name) {
    if (auto err = ValidateUserId(user_id)) {
        return Translate(cmd, err);
    }
    if (IsSaveMounted(user_id, title_id, dir_name) ||
        Backup::IsBackupExecutingFor(GetSaveDirPath(user_id, title_id, dir_name))) {
        return Translate(cmd, static_cast<u32>(OrbisSaveDataError::BUSY));
    }
    const fs::path save_dir = GetSaveDirPath(user_id, title_id, dir_name);
    std::error_code ec;
    if (!fs::exists(save_dir, ec)) {
        return Translate(cmd, static_cast<u32>(OrbisSaveDataError::NOT_FOUND));
    }
    fs::remove_all(save_dir, ec);
    if (ec) {
        return Translate(cmd, static_cast<u32>(OrbisSaveDataError::INTERNAL));
    }
    SaveDb::Remove(user_id, title_id, dir_name);
    fs::remove(GetMetaIconPath(user_id, title_id, dir_name), ec);
    Backup::DeleteBackup(user_id, title_id, dir_name);
    return ORBIS_OK;
}

s32 DoBackup(OrbisSaveDataCommand cmd, const OrbisSaveDataBackup* backup) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (backup == nullptr || !ValidateReservedZero(backup->reserved, sizeof(backup->reserved))) {
        return Translate(cmd, static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    if (auto err = ValidateUserId(backup->userId)) {
        return Translate(cmd, err);
    }
    std::string tid;
    if (auto err = ValidateTitleId(backup->titleId, true, tid)) {
        return Translate(cmd, err);
    }
    std::string dir;
    if (auto err = ValidateDirName(backup->dirName, false, false, dir)) {
        return Translate(cmd, err);
    }
    if (auto err = ValidateFingerprint(backup->fingerprint, true)) {
        return Translate(cmd, err);
    }
    if (IsSaveMounted(backup->userId, tid, dir) ||
        Backup::IsBackupExecutingFor(GetSaveDirPath(backup->userId, tid, dir))) {
        return Translate(cmd, static_cast<u32>(OrbisSaveDataError::BUSY));
    }
    const fs::path save_dir = GetSaveDirPath(backup->userId, tid, dir);
    std::error_code ec;
    if (!fs::exists(save_dir, ec)) {
        return Translate(cmd, static_cast<u32>(OrbisSaveDataError::NOT_FOUND));
    }
    Backup::NewRequest(backup->userId, tid, dir, Backup::OrbisSaveDataEventType::BACKUP);
    return ORBIS_OK;
}

s32 DoCheckBackupData(OrbisSaveDataCommand cmd, s32 user_id, const OrbisSaveDataTitleId* title_id,
                      const OrbisSaveDataDirName* dir_name, OrbisSaveDataParam* param,
                      OrbisSaveDataIcon* icon) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (auto err = ValidateUserId(user_id)) {
        return Translate(cmd, err);
    }
    std::string tid;
    if (auto err = ValidateTitleId(title_id, true, tid)) {
        return Translate(cmd, err);
    }
    std::string dir;
    if (auto err = ValidateDirName(dir_name, false, false, dir)) {
        return Translate(cmd, err);
    }
    if (icon != nullptr) {
        if (!ValidateReservedZero(icon->reserved, sizeof(icon->reserved))) {
            return Translate(cmd, static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
        }
        if (icon->buf != nullptr && icon->bufSize == 0) {
            return Translate(cmd, static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
        }
    }
    if (!Backup::HasBackup(user_id, tid, dir)) {
        return Translate(cmd, static_cast<u32>(OrbisSaveDataError::NOT_FOUND));
    }
    const fs::path bu_dir = GetBackupDirPath(user_id, tid, dir);
    if (param != nullptr) {
        std::memset(param, 0, sizeof(*param));
        PSF psf;
        const fs::path sfo_path = bu_dir / "sce_sys" / "param.sfo";
        const fs::path meta_sfo = GetMetaBackupSfoPath(user_id, tid, dir);
        const fs::path chosen_sfo = fs::exists(sfo_path) ? sfo_path : meta_sfo;
        if (psf.Open(chosen_sfo)) {
            if (auto v = psf.GetString("MAINTITLE")) {
                TruncateUtf8(*v, param->title, sizeof(param->title));
            }
            if (auto v = psf.GetString("SUBTITLE")) {
                TruncateUtf8(*v, param->subTitle, sizeof(param->subTitle));
            }
            if (auto v = psf.GetString("DETAIL")) {
                TruncateUtf8(*v, param->detail, sizeof(param->detail));
            }
            if (auto v = psf.GetInteger("SAVEDATA_LIST_PARAM")) {
                param->userParam = static_cast<u32>(*v);
            }
            if (auto v = psf.GetString("MTIME")) {
                param->mtime = Iso8601ToUtcTime(*v);
            }
        }
    }
    if (icon != nullptr) {
        const fs::path bu_icon = bu_dir / "sce_sys" / "icon0.png";
        const fs::path meta_icon = GetMetaBackupIconPath(user_id, tid, dir);
        const fs::path chosen_icon = fs::exists(bu_icon) ? bu_icon : meta_icon;
        std::error_code ec;
        if (!fs::exists(chosen_icon, ec)) {
            icon->dataSize = 0;
        } else {
            const size_t sz = static_cast<size_t>(fs::file_size(chosen_icon, ec));
            icon->dataSize = sz;
            if (icon->buf != nullptr) {
                if (icon->bufSize < sz) {
                    return Translate(cmd, static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
                }
                Common::FS::IOFile f(chosen_icon, Common::FS::FileAccessMode::Read);
                if (!f.IsOpen() || f.ReadRaw<u8>(icon->buf, sz) != sz) {
                    return Translate(cmd, static_cast<u32>(OrbisSaveDataError::INTERNAL));
                }
            }
        }
    }
    return ORBIS_OK;
}

s32 DoRestoreBackupData(OrbisSaveDataCommand cmd, s32 user_id, const OrbisSaveDataTitleId* title_id,
                        const OrbisSaveDataDirName* dir_name,
                        const OrbisSaveDataFingerprint* fingerprint) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (auto err = ValidateUserId(user_id)) {
        return Translate(cmd, err);
    }
    std::string tid;
    if (auto err = ValidateTitleId(title_id, true, tid)) {
        return Translate(cmd, err);
    }
    std::string dir;
    if (auto err = ValidateDirName(dir_name, false, false, dir)) {
        return Translate(cmd, err);
    }
    if (auto err = ValidateFingerprint(fingerprint, true)) {
        return Translate(cmd, err);
    }
    if (IsSaveMounted(user_id, tid, dir) ||
        Backup::IsBackupExecutingFor(GetSaveDirPath(user_id, tid, dir))) {
        return Translate(cmd, static_cast<u32>(OrbisSaveDataError::BUSY));
    }
    if (!Backup::HasBackup(user_id, tid, dir)) {
        return Translate(cmd, static_cast<u32>(OrbisSaveDataError::NOT_FOUND));
    }
    if (!Backup::Restore(user_id, tid, dir)) {
        return Translate(cmd, static_cast<u32>(OrbisSaveDataError::INTERNAL));
    }
    SaveDb::RebuildFromDisk(user_id);
    return ORBIS_OK;
}

} // namespace

void DispatchBackupEvent(const Backup::BackupRequest& req) {
    OrbisSaveDataEventCallbackFunc cb = nullptr;
    void* userdata = nullptr;
    OrbisSaveDataEvent ev{};
    ev.type = static_cast<s32>(req.origin);
    ev.errorCode = (req.error_code == 0)
                       ? ORBIS_OK
                       : Translate(OrbisSaveDataCommand::BACKUP, static_cast<u32>(req.error_code));
    ev.userId = req.user_id;
    std::memcpy(ev.titleId.data, req.title_id.data(),
                std::min(req.title_id.size(), sizeof(ev.titleId.data) - 1));
    std::memcpy(ev.dirName.data, req.dir_name.data(),
                std::min(req.dir_name.size(), sizeof(ev.dirName.data) - 1));
    {
        std::scoped_lock lk(g_mutex);
        g_has_last_event_result = true;
        g_last_event_result = {};
        g_last_event_result.type = ev.type;
        g_last_event_result.errorCode = ev.errorCode;
        g_last_event_result.userId = ev.userId;
        g_last_event_result.titleId = ev.titleId;
        g_last_event_result.dirName = ev.dirName;
        cb = g_event_callback;
        userdata = g_event_userdata;
    }
    if (cb != nullptr) {
        cb(&ev, userdata);
    }
}

void SetTestEnvironment(std::string_view title_id, u32 sdk_ver) {
    std::scoped_lock lk(g_mutex);
    g_test_title_id = std::string(title_id);
    g_test_sdk_ver = sdk_ver;
}

void ResetForTest() {
    Backup::StopThread();
    {
        std::scoped_lock lk(g_mutex);
        auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
        for (auto& slot : g_mount_slots) {
            if (slot.instance) {
                mnt->Unmount(slot.instance->GetMountPoint());
            }
        }
        g_mount_slots.clear();
        for (const auto& mp : g_sys_mounts) {
            mnt->Unmount(mp);
        }
        g_sys_mounts.clear();
        SaveMemory::ClearAll();
        SaveDb::CloseAll();
        g_initialized = false;
        g_shutting_down = false;
        g_init_count = 0;
        g_test_title_id.clear();
        g_test_sdk_ver = 0;
        g_last_required_blocks = 0;
        g_abort_requested = false;
        g_fake_broken_supported = false;
        g_library_user = Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID;
        g_event_callback = nullptr;
        g_event_userdata = nullptr;
        g_has_last_event_result = false;
        g_last_event_result = {};
        g_stored_event_info = {};
        g_has_stored_event_info = false;
        Backup::ClearProgress();
    }
}

s32 PS4_SYSV_ABI sceSaveDataInitialize(const void* /*params*/) {
    return sceSaveDataInitialize3(nullptr);
}

s32 PS4_SYSV_ABI sceSaveDataInitialize2(const void* /*params*/) {
    return sceSaveDataInitialize3(nullptr);
}

s32 PS4_SYSV_ABI sceSaveDataInitialize3(const OrbisSaveDataInitParams3* params) {
    if (params != nullptr && params->size != sizeof(OrbisSaveDataInitParams3)) {
        return ORBIS_SAVE_DATA_ERROR_INVALID_ARGUMENT;
    }
    std::scoped_lock lk(g_mutex);
    if (!g_initialized) {
        g_shutting_down = false;
        g_abort_requested = false;
        for (s32 uid : CollectTargetUsers()) {
            SaveDb::EnsureDatabase(uid);
        }
        Backup::StartThread();
        g_initialized = true;
    }
    ++g_init_count;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataInitializeForCdlg(const void* /*params*/) {
    return sceSaveDataInitialize3(nullptr);
}

s32 PS4_SYSV_ABI sceSaveDataTerminate() {
    bool stop_backup = false;
    {
        std::scoped_lock lk(g_mutex);
        if (!g_initialized) {
            return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
        }
        if (g_init_count > 1) {
            --g_init_count;
            return ORBIS_OK;
        }
        g_init_count = 0;
        g_initialized = false;
        g_shutting_down = false;
        for (auto& slot : g_mount_slots) {
            if (slot.instance) {
                slot.instance->Umount();
            }
        }
        g_mount_slots.clear();
        auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
        for (const auto& mp : g_sys_mounts) {
            mnt->Unmount(mp);
        }
        g_sys_mounts.clear();
        SaveMemory::ClearAll();
        SaveDb::CloseAll();
        stop_backup = true;
    }
    if (stop_backup) {
        Backup::StopThread();
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataShutdownStart() {
    std::scoped_lock lk(g_mutex);
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    g_shutting_down = true;
    for (s32 uid : CollectTargetUsers()) {
        for (u32 slot = 0; slot < OrbisSaveDataMemorySlotCount; ++slot) {
            if (SaveMemory::IsSaveMemoryInitialized(slot) &&
                SaveMemory::IsSlotOwnedByUser(slot, uid)) {
                SaveMemory::PersistMemory(slot);
            }
        }
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataMount(const OrbisSaveDataMount* mount,
                                  OrbisSaveDataMountResult* mountResult) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (mount == nullptr || mountResult == nullptr ||
        !ValidateReservedZero(mount->reserved, sizeof(mount->reserved))) {
        return Translate(OrbisSaveDataCommand::MOUNT,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    std::string tid;
    if (auto err = ValidateTitleId(mount->titleId, true, tid)) {
        return Translate(OrbisSaveDataCommand::MOUNT, err);
    }
    std::string dir;
    if (auto err = ValidateDirName(mount->dirName, false, false, dir)) {
        return Translate(OrbisSaveDataCommand::MOUNT, err);
    }
    if (auto err = ValidateFingerprint(mount->fingerprint, true)) {
        return Translate(OrbisSaveDataCommand::MOUNT, err);
    }
    std::scoped_lock lk(g_mutex);
    const u32 err = PerformMount(OrbisSaveDataCommand::MOUNT, mount->userId, tid, dir,
                                 mount->blocks, mount->mountMode, mountResult);
    return err == 0 ? ORBIS_OK : Translate(OrbisSaveDataCommand::MOUNT, err);
}

s32 PS4_SYSV_ABI sceSaveDataMount2(const OrbisSaveDataMount2* mount,
                                   OrbisSaveDataMountResult* mountResult) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (mount == nullptr || mountResult == nullptr ||
        !ValidateReservedZero(mount->reserved, sizeof(mount->reserved))) {
        return Translate(OrbisSaveDataCommand::MOUNT2,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    std::string dir;
    if (auto err = ValidateDirName(mount->dirName, false, false, dir)) {
        return Translate(OrbisSaveDataCommand::MOUNT2, err);
    }
    const std::string tid = GetGameTitleId();
    std::scoped_lock lk(g_mutex);
    const u32 err = PerformMount(OrbisSaveDataCommand::MOUNT2, mount->userId, tid, dir,
                                 mount->blocks, mount->mountMode, mountResult);
    return err == 0 ? ORBIS_OK : Translate(OrbisSaveDataCommand::MOUNT2, err);
}

s32 PS4_SYSV_ABI sceSaveDataTransferringMount(const OrbisSaveDataTransferringMount* mount,
                                              OrbisSaveDataMountResult* mountResult) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (mount == nullptr || mountResult == nullptr ||
        !ValidateReservedZero(mount->reserved, sizeof(mount->reserved))) {
        return Translate(OrbisSaveDataCommand::TRANSFERRING_MOUNT,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    if ((mount->titleId == nullptr) != (mount->fingerprint == nullptr)) {
        return Translate(OrbisSaveDataCommand::TRANSFERRING_MOUNT,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    std::string tid;
    if (auto err = ValidateTitleId(mount->titleId, true, tid)) {
        return Translate(OrbisSaveDataCommand::TRANSFERRING_MOUNT, err);
    }
    std::string dir;
    if (auto err = ValidateDirName(mount->dirName, false, false, dir)) {
        return Translate(OrbisSaveDataCommand::TRANSFERRING_MOUNT, err);
    }
    if (auto err = ValidateFingerprint(mount->fingerprint, true)) {
        return Translate(OrbisSaveDataCommand::TRANSFERRING_MOUNT, err);
    }
    std::scoped_lock lk(g_mutex);
    const u32 err = PerformMount(OrbisSaveDataCommand::TRANSFERRING_MOUNT, mount->userId, tid, dir,
                                 0, static_cast<u32>(OrbisSaveDataMountMode::RDONLY), mountResult);
    return err == 0 ? ORBIS_OK : Translate(OrbisSaveDataCommand::TRANSFERRING_MOUNT, err);
}

s32 PS4_SYSV_ABI sceSaveDataMount5(const OrbisSaveDataMount5* mount,
                                   OrbisSaveDataMountResult* mountResult) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (mount == nullptr || mountResult == nullptr ||
        !ValidateReservedZero(mount->reserved, sizeof(mount->reserved))) {
        return Translate(OrbisSaveDataCommand::MOUNT5,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    if (auto err = ValidateFingerprint(mount->fingerprint, true)) {
        return Translate(OrbisSaveDataCommand::MOUNT5, err);
    }
    s32 uid = 0;
    std::string tid;
    std::string dir;
    if (!ParseVirtualSavePath(mount->path, uid, tid, dir)) {
        return Translate(OrbisSaveDataCommand::MOUNT5,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    std::scoped_lock lk(g_mutex);
    const u32 err = PerformMount(OrbisSaveDataCommand::MOUNT5, uid, tid, dir, mount->blocks,
                                 mount->mountMode, mountResult);
    return err == 0 ? ORBIS_OK : Translate(OrbisSaveDataCommand::MOUNT5, err);
}

s32 PS4_SYSV_ABI sceSaveDataMountInternal(const OrbisSaveDataMountInternal* mount,
                                          OrbisSaveDataMountResult* mountResult) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (mount == nullptr || mountResult == nullptr ||
        !ValidateReservedZero(mount->reserved, sizeof(mount->reserved))) {
        return Translate(OrbisSaveDataCommand::MOUNT_INTERNAL,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    std::string tid;
    if (auto err = ValidateTitleId(mount->titleId, true, tid)) {
        return Translate(OrbisSaveDataCommand::MOUNT_INTERNAL, err);
    }
    std::string dir;
    if (auto err = ValidateDirName(mount->dirName, false, true, dir)) {
        return Translate(OrbisSaveDataCommand::MOUNT_INTERNAL, err);
    }
    if (auto err = ValidateFingerprint(mount->fingerprint, true)) {
        return Translate(OrbisSaveDataCommand::MOUNT_INTERNAL, err);
    }
    std::scoped_lock lk(g_mutex);
    const u32 err = PerformMount(OrbisSaveDataCommand::MOUNT_INTERNAL, mount->userId, tid, dir,
                                 mount->blocks, mount->mountMode, mountResult);
    return err == 0 ? ORBIS_OK : Translate(OrbisSaveDataCommand::MOUNT_INTERNAL, err);
}

s32 PS4_SYSV_ABI sceSaveDataUmount(const OrbisSaveDataMountPoint* mountPoint) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    std::string mp;
    if (auto err = ValidateMountPoint(mountPoint, mp)) {
        return Translate(OrbisSaveDataCommand::UMOUNT, err);
    }
    std::scoped_lock lk(g_mutex);
    ActiveMountSlot* slot = FindMountSlotByMountPoint(mp);
    if (slot == nullptr) {
        return Translate(OrbisSaveDataCommand::UMOUNT,
                         static_cast<u32>(OrbisSaveDataError::NOT_MOUNTED));
    }
    if (!slot->instance->Umount()) {
        return Translate(OrbisSaveDataCommand::UMOUNT, static_cast<u32>(OrbisSaveDataError::BUSY));
    }
    const int idx = slot->slot_index;
    g_mount_slots.erase(
        std::remove_if(g_mount_slots.begin(), g_mount_slots.end(),
                       [idx](const ActiveMountSlot& s) { return s.slot_index == idx; }),
        g_mount_slots.end());
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataUmountWithBackup(const OrbisSaveDataMountPoint* mountPoint) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    std::string mp;
    if (auto err = ValidateMountPoint(mountPoint, mp)) {
        return Translate(OrbisSaveDataCommand::UMOUNT_WITH_BACKUP, err);
    }
    s32 user_id = 0;
    std::string tid;
    std::string dir;
    {
        std::scoped_lock lk(g_mutex);
        ActiveMountSlot* slot = FindMountSlotByMountPoint(mp);
        if (slot == nullptr) {
            return Translate(OrbisSaveDataCommand::UMOUNT_WITH_BACKUP,
                             static_cast<u32>(OrbisSaveDataError::NOT_MOUNTED));
        }
        if (slot->instance->IsReadOnly()) {
            return Translate(OrbisSaveDataCommand::UMOUNT_WITH_BACKUP,
                             static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
        }
        if (!slot->instance->Umount()) {
            return Translate(OrbisSaveDataCommand::UMOUNT_WITH_BACKUP,
                             static_cast<u32>(OrbisSaveDataError::BUSY));
        }
        user_id = slot->instance->GetUserId();
        tid = std::string(slot->instance->GetTitleId());
        dir = std::string(slot->instance->GetDirName());
        const int idx = slot->slot_index;
        g_mount_slots.erase(
            std::remove_if(g_mount_slots.begin(), g_mount_slots.end(),
                           [idx](const ActiveMountSlot& s) { return s.slot_index == idx; }),
            g_mount_slots.end());
    }
    Backup::NewRequest(user_id, tid, dir, Backup::OrbisSaveDataEventType::UMOUNT_BACKUP);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataBackup(const OrbisSaveDataBackup* backup) {
    return DoBackup(OrbisSaveDataCommand::BACKUP, backup);
}

s32 PS4_SYSV_ABI sceSaveDataRestoreBackupData(const OrbisSaveDataRestoreBackupData* restore) {
    if (restore == nullptr || !ValidateReservedZero(restore->reserved, sizeof(restore->reserved))) {
        return Translate(OrbisSaveDataCommand::RESTORE_BACKUP_DATA,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    return DoRestoreBackupData(OrbisSaveDataCommand::RESTORE_BACKUP_DATA, restore->userId,
                               restore->titleId, restore->dirName, restore->fingerprint);
}

s32 PS4_SYSV_ABI
sceSaveDataRestoreBackupDataForCdlg(const OrbisSaveDataRestoreBackupDataForCdlg* restore) {
    if (restore == nullptr || !ValidateReservedZero(restore->reserved, sizeof(restore->reserved)) ||
        !ValidateReservedZero(restore->reserved2, sizeof(restore->reserved2))) {
        return Translate(OrbisSaveDataCommand::RESTORE_BACKUP_DATA_FOR_CDLG,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    return DoRestoreBackupData(OrbisSaveDataCommand::RESTORE_BACKUP_DATA_FOR_CDLG, restore->userId,
                               restore->titleId, restore->dirName, restore->fingerprint);
}

s32 PS4_SYSV_ABI sceSaveDataCheckBackupData(const OrbisSaveDataCheckBackupData* check) {
    if (check == nullptr || !ValidateReservedZero(check->reserved, sizeof(check->reserved))) {
        return Translate(OrbisSaveDataCommand::CHECK_BACKUP_DATA,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    return DoCheckBackupData(OrbisSaveDataCommand::CHECK_BACKUP_DATA, check->userId, check->titleId,
                             check->dirName, check->param, check->icon);
}

s32 PS4_SYSV_ABI
sceSaveDataCheckBackupDataInternal(const OrbisSaveDataCheckBackupDataInternal* check) {
    if (check == nullptr || !ValidateReservedZero(check->reserved, sizeof(check->reserved))) {
        return Translate(OrbisSaveDataCommand::CHECK_BACKUP_DATA_INTERNAL,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    return DoCheckBackupData(OrbisSaveDataCommand::CHECK_BACKUP_DATA_INTERNAL, check->userId,
                             check->titleId, check->dirName, check->param, check->icon);
}

s32 PS4_SYSV_ABI
sceSaveDataCheckBackupDataForCdlg(const OrbisSaveDataCheckBackupDataForCdlg* check) {
    if (check == nullptr || !ValidateReservedZero(check->reserved, sizeof(check->reserved))) {
        return Translate(OrbisSaveDataCommand::CHECK_BACKUP_DATA_FOR_CDLG,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    return DoCheckBackupData(OrbisSaveDataCommand::CHECK_BACKUP_DATA_FOR_CDLG, check->userId,
                             check->titleId, check->dirName, check->param, check->icon);
}

s32 PS4_SYSV_ABI sceSaveDataChangeInternal(const OrbisSaveDataChangeInternal* change) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (change == nullptr || !ValidateReservedZero(change->reserved, sizeof(change->reserved))) {
        return Translate(OrbisSaveDataCommand::CHANGE_INTERNAL,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    if (auto err = ValidateUserId(change->userId)) {
        return Translate(OrbisSaveDataCommand::CHANGE_INTERNAL, err);
    }
    std::string tid;
    if (auto err = ValidateTitleId(change->titleId, true, tid)) {
        return Translate(OrbisSaveDataCommand::CHANGE_INTERNAL, err);
    }
    std::string dir;
    if (auto err = ValidateDirName(change->dirName, false, true, dir)) {
        return Translate(OrbisSaveDataCommand::CHANGE_INTERNAL, err);
    }
    if (auto err = ValidateFingerprint(change->fingerprint, true)) {
        return Translate(OrbisSaveDataCommand::CHANGE_INTERNAL, err);
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataGetMountInfo(const OrbisSaveDataMountPoint* mountPoint,
                                         OrbisSaveDataMountInfo* info) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (info == nullptr) {
        return Translate(OrbisSaveDataCommand::GET_MOUNT_INFO,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    std::string mp;
    if (auto err = ValidateMountPoint(mountPoint, mp)) {
        return Translate(OrbisSaveDataCommand::GET_MOUNT_INFO, err);
    }
    std::scoped_lock lk(g_mutex);
    ActiveMountSlot* slot = FindMountSlotByMountPoint(mp);
    if (slot == nullptr) {
        return Translate(OrbisSaveDataCommand::GET_MOUNT_INFO,
                         static_cast<u32>(OrbisSaveDataError::NOT_MOUNTED));
    }
    std::memset(info, 0, sizeof(*info));
    const u64 max_blocks = slot->instance->GetMaxBlocks();
    info->blocks = max_blocks;
    info->freeBlocks = ComputeFreeBlocks(slot->instance->GetSavePath(), max_blocks);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataDelete(const OrbisSaveDataDelete* del) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (del == nullptr || !ValidateReservedZero(del->reserved, sizeof(del->reserved))) {
        return Translate(OrbisSaveDataCommand::Delete,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    std::string tid;
    if (auto err = ValidateTitleId(del->titleId, true, tid)) {
        return Translate(OrbisSaveDataCommand::Delete, err);
    }
    std::string dir;
    if (auto err = ValidateDirName(del->dirName, false, false, dir)) {
        return Translate(OrbisSaveDataCommand::Delete, err);
    }
    std::scoped_lock lk(g_mutex);
    return DoDeleteSaveData(OrbisSaveDataCommand::Delete, del->userId, tid, dir);
}

s32 PS4_SYSV_ABI sceSaveDataDelete5(const OrbisSaveDataDelete5* del) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (del == nullptr || !ValidateReservedZero(del->reserved, sizeof(del->reserved))) {
        return Translate(OrbisSaveDataCommand::DELETE5,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    s32 uid = 0;
    std::string tid;
    std::string dir;
    if (!ParseVirtualSavePath(del->path, uid, tid, dir)) {
        return Translate(OrbisSaveDataCommand::DELETE5,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    std::scoped_lock lk(g_mutex);
    return DoDeleteSaveData(OrbisSaveDataCommand::DELETE5, uid, tid, dir);
}

s32 PS4_SYSV_ABI sceSaveDataDeleteUser(const OrbisSaveDataDeleteUser* del) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (del == nullptr || !ValidateReservedZero(del->reserved, sizeof(del->reserved))) {
        return Translate(OrbisSaveDataCommand::DELETE_USER,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    if (del->userId <= 0) {
        return Translate(OrbisSaveDataCommand::DELETE_USER,
                         static_cast<u32>(OrbisSaveDataError::INVALID_LOGIN_USER));
    }
    std::scoped_lock lk(g_mutex);
    for (const auto& s : g_mount_slots) {
        if (s.instance->GetUserId() == del->userId) {
            return Translate(OrbisSaveDataCommand::DELETE_USER,
                             static_cast<u32>(OrbisSaveDataError::BUSY));
        }
    }
    std::error_code ec;
    fs::remove_all(GetSaveRootPath(del->userId), ec);
    fs::remove_all(GetMetaRootPath(del->userId), ec);
    SaveDb::RemoveUser(del->userId);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataDeleteAllUser() {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    std::scoped_lock lk(g_mutex);
    if (!g_mount_slots.empty()) {
        return Translate(OrbisSaveDataCommand::DELETE_ALL_USER,
                         static_cast<u32>(OrbisSaveDataError::BUSY));
    }
    for (s32 uid : CollectTargetUsers()) {
        std::error_code ec;
        fs::remove_all(GetSaveRootPath(uid), ec);
        fs::remove_all(GetMetaRootPath(uid), ec);
        SaveDb::RemoveUser(uid);
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataCopy5(const OrbisSaveDataCopy5* copy) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (copy == nullptr || !ValidateReservedZero(copy->reserved, sizeof(copy->reserved))) {
        return Translate(OrbisSaveDataCommand::COPY5,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    s32 src_uid = 0, dst_uid = 0;
    std::string src_tid, dst_tid, src_dir, dst_dir;
    if (!ParseVirtualSavePath(copy->srcPath, src_uid, src_tid, src_dir) ||
        !ParseVirtualSavePath(copy->dstPath, dst_uid, dst_tid, dst_dir)) {
        return Translate(OrbisSaveDataCommand::COPY5,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    std::scoped_lock lk(g_mutex);
    if (IsSaveMounted(src_uid, src_tid, src_dir) || IsSaveMounted(dst_uid, dst_tid, dst_dir)) {
        return Translate(OrbisSaveDataCommand::COPY5, static_cast<u32>(OrbisSaveDataError::BUSY));
    }
    const fs::path src_p = GetSaveDirPath(src_uid, src_tid, src_dir);
    const fs::path dst_p = GetSaveDirPath(dst_uid, dst_tid, dst_dir);
    std::error_code ec;
    if (!fs::exists(src_p, ec)) {
        return Translate(OrbisSaveDataCommand::COPY5,
                         static_cast<u32>(OrbisSaveDataError::NOT_FOUND));
    }
    fs::remove_all(dst_p, ec);
    fs::create_directories(dst_p.parent_path(), ec);
    fs::copy(src_p, dst_p, fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
    if (ec) {
        return Translate(OrbisSaveDataCommand::COPY5,
                         static_cast<u32>(OrbisSaveDataError::INTERNAL));
    }
    SaveDb::RebuildFromDisk(dst_uid);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataPromote5(const OrbisSaveDataPromote5* promote) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (promote == nullptr || !ValidateReservedZero(promote->reserved, sizeof(promote->reserved))) {
        return Translate(OrbisSaveDataCommand::PROMOTE5,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    s32 src_uid = 0, dst_uid = 0;
    std::string src_tid, dst_tid, src_dir, dst_dir;
    if (!ParseVirtualSavePath(promote->srcPath, src_uid, src_tid, src_dir) ||
        !ParseVirtualSavePath(promote->dstPath, dst_uid, dst_tid, dst_dir)) {
        return Translate(OrbisSaveDataCommand::PROMOTE5,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    std::scoped_lock lk(g_mutex);
    if (IsSaveMounted(src_uid, src_tid, src_dir) || IsSaveMounted(dst_uid, dst_tid, dst_dir)) {
        return Translate(OrbisSaveDataCommand::PROMOTE5,
                         static_cast<u32>(OrbisSaveDataError::BUSY));
    }
    const fs::path src_p = GetSaveDirPath(src_uid, src_tid, src_dir);
    const fs::path dst_p = GetSaveDirPath(dst_uid, dst_tid, dst_dir);
    std::error_code ec;
    if (!fs::exists(src_p, ec)) {
        return Translate(OrbisSaveDataCommand::PROMOTE5,
                         static_cast<u32>(OrbisSaveDataError::NOT_FOUND));
    }
    fs::remove_all(dst_p, ec);
    fs::create_directories(dst_p.parent_path(), ec);
    fs::rename(src_p, dst_p, ec);
    if (ec) {
        fs::copy(src_p, dst_p, fs::copy_options::recursive | fs::copy_options::overwrite_existing,
                 ec);
        if (ec) {
            return Translate(OrbisSaveDataCommand::PROMOTE5,
                             static_cast<u32>(OrbisSaveDataError::INTERNAL));
        }
        fs::remove_all(src_p, ec);
    }
    SaveDb::Remove(src_uid, src_tid, src_dir);
    SaveDb::RebuildFromDisk(dst_uid);
    return ORBIS_OK;
}

s32 DoDirNameSearch(OrbisSaveDataCommand cmd, const OrbisSaveDataDirNameSearchCond* cond,
                    OrbisSaveDataDirNameSearchResult* result) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (cond == nullptr || result == nullptr || cond->reserved != 0 ||
        !ValidateReservedZero(cond->reserved2, sizeof(cond->reserved2)) || cond->key == 4 ||
        cond->key > 5 || cond->order > 1 ||
        !ValidateReservedZero(result->reserved2, sizeof(result->reserved2))) {
        return Translate(cmd, static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    if (auto err = ValidateUserId(cond->userId)) {
        return Translate(cmd, err);
    }
    std::string tid;
    if (auto err = ValidateTitleId(cond->titleId, true, tid)) {
        return Translate(cmd, err);
    }
    std::string dir_filter;
    if (cond->dirName != nullptr) {
        const size_t len = strnlen(cond->dirName->data, ORBIS_SAVE_DATA_DIRNAME_DATA_MAXSIZE);
        if (len == 0 || len >= ORBIS_SAVE_DATA_DIRNAME_DATA_MAXSIZE) {
            return Translate(cmd, static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
        }
        dir_filter.assign(cond->dirName->data, len);
    }

    std::scoped_lock lk(g_mutex);
    std::vector<SaveDbEntry> entries;
    SaveDb::QueryDirNames(
        cond->userId, tid,
        dir_filter.empty() ? std::nullopt : std::optional<std::string_view>{dir_filter},
        static_cast<DbSortKey>(cond->key), static_cast<DbSortOrder>(cond->order), false, entries);
    result->hitNum = static_cast<u32>(entries.size());
    const u32 to_copy = std::min<u32>(result->dirNamesNum, static_cast<u32>(entries.size()));
    result->setNum = to_copy;

    for (u32 i = 0; i < to_copy; ++i) {
        const SaveDbEntry& e = entries[i];
        if (result->dirNames != nullptr) {
            std::memset(&result->dirNames[i], 0, sizeof(OrbisSaveDataDirName));
            std::memcpy(result->dirNames[i].data, e.dir_name.data(),
                        std::min(e.dir_name.size(), sizeof(result->dirNames[i].data) - 1));
        }
        if (result->params != nullptr) {
            std::memset(&result->params[i], 0, sizeof(OrbisSaveDataParam));
            TruncateUtf8(e.main_title, result->params[i].title, sizeof(result->params[i].title));
            TruncateUtf8(e.sub_title, result->params[i].subTitle,
                         sizeof(result->params[i].subTitle));
            TruncateUtf8(e.detail, result->params[i].detail, sizeof(result->params[i].detail));
            result->params[i].userParam = e.user_param;
            result->params[i].mtime = e.mtime;
        }
        if (result->infos != nullptr) {
            std::memset(&result->infos[i], 0, sizeof(OrbisSaveDataSearchInfo));
            result->infos[i].blocks = e.blocks;
            result->infos[i].freeBlocks = e.free_blocks;
        }
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataDirNameSearch(const OrbisSaveDataDirNameSearchCond* cond,
                                          OrbisSaveDataDirNameSearchResult* result) {
    return DoDirNameSearch(OrbisSaveDataCommand::DIR_NAME_SEARCH, cond, result);
}

s32 PS4_SYSV_ABI sceSaveDataDirNameSearchInternal(const OrbisSaveDataDirNameSearchCond* cond,
                                                  OrbisSaveDataDirNameSearchResult* result) {
    return DoDirNameSearch(OrbisSaveDataCommand::DIR_NAME_SEARCH_INTERNAL, cond, result);
}

s32 PS4_SYSV_ABI sceSaveDataSetParam(const OrbisSaveDataMountPoint* mountPoint, u32 paramType,
                                     const void* paramBuf, size_t paramBufSize) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    std::string mp;
    if (auto err = ValidateMountPoint(mountPoint, mp)) {
        return Translate(OrbisSaveDataCommand::SET_PARAM, err);
    }
    if (paramBuf == nullptr) {
        return Translate(OrbisSaveDataCommand::SET_PARAM,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    const auto ptype = static_cast<OrbisSaveDataParamType>(paramType);
    const auto* param = static_cast<const OrbisSaveDataParam*>(paramBuf);

    switch (ptype) {
    case OrbisSaveDataParamType::ALL:
        if (paramBufSize < sizeof(OrbisSaveDataParam)) {
            return Translate(OrbisSaveDataCommand::SET_PARAM,
                             static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
        }
        break;
    case OrbisSaveDataParamType::TITLE:
    case OrbisSaveDataParamType::SUB_TITLE:
        if (paramBufSize < ORBIS_SAVE_DATA_TITLE_MAXSIZE) {
            return Translate(OrbisSaveDataCommand::SET_PARAM,
                             static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
        }
        break;
    case OrbisSaveDataParamType::DETAIL:
        if (paramBufSize < ORBIS_SAVE_DATA_DETAIL_MAXSIZE) {
            return Translate(OrbisSaveDataCommand::SET_PARAM,
                             static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
        }
        break;
    case OrbisSaveDataParamType::USER_PARAM:
        if (paramBufSize < sizeof(u32)) {
            return Translate(OrbisSaveDataCommand::SET_PARAM,
                             static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
        }
        break;
    case OrbisSaveDataParamType::MTIME:
        if (paramBufSize < sizeof(s64)) {
            return Translate(OrbisSaveDataCommand::SET_PARAM,
                             static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
        }
        break;
    default:
        return Translate(OrbisSaveDataCommand::SET_PARAM,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }

    if (ptype == OrbisSaveDataParamType::ALL) {
        if (auto err = ValidateParam(param, ptype, true)) {
            return Translate(OrbisSaveDataCommand::SET_PARAM, err);
        }
    } else if (ptype == OrbisSaveDataParamType::TITLE) {
        if (auto err = ValidateStringField(static_cast<const char*>(paramBuf),
                                           ORBIS_SAVE_DATA_TITLE_MAXSIZE, false)) {
            return Translate(OrbisSaveDataCommand::SET_PARAM, err);
        }
    } else if (ptype == OrbisSaveDataParamType::SUB_TITLE) {
        if (auto err = ValidateStringField(static_cast<const char*>(paramBuf),
                                           ORBIS_SAVE_DATA_SUBTITLE_MAXSIZE, false)) {
            return Translate(OrbisSaveDataCommand::SET_PARAM, err);
        }
    } else if (ptype == OrbisSaveDataParamType::DETAIL) {
        if (auto err = ValidateStringField(static_cast<const char*>(paramBuf),
                                           ORBIS_SAVE_DATA_DETAIL_MAXSIZE, true)) {
            return Translate(OrbisSaveDataCommand::SET_PARAM, err);
        }
    }

    std::scoped_lock lk(g_mutex);
    ActiveMountSlot* slot = FindMountSlotByMountPoint(mp);
    if (slot == nullptr) {
        return Translate(OrbisSaveDataCommand::SET_PARAM,
                         static_cast<u32>(OrbisSaveDataError::NOT_MOUNTED));
    }
    if (slot->instance->IsReadOnly()) {
        return Translate(OrbisSaveDataCommand::SET_PARAM,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }

    PSF& psf = slot->instance->GetParamSFO();
    switch (ptype) {
    case OrbisSaveDataParamType::ALL:
        psf.AddString("MAINTITLE", param->title, true);
        psf.AddString("SUBTITLE", param->subTitle, true);
        psf.AddString("DETAIL", param->detail, true);
        psf.AddInteger("SAVEDATA_LIST_PARAM", static_cast<s32>(param->userParam), true);
        if (param->mtime != 0) {
            psf.AddString("MTIME", UtcTimeToIso8601(param->mtime), true);
        }
        break;
    case OrbisSaveDataParamType::TITLE:
        psf.AddString("MAINTITLE", static_cast<const char*>(paramBuf), true);
        break;
    case OrbisSaveDataParamType::SUB_TITLE:
        psf.AddString("SUBTITLE", static_cast<const char*>(paramBuf), true);
        break;
    case OrbisSaveDataParamType::DETAIL:
        psf.AddString("DETAIL", static_cast<const char*>(paramBuf), true);
        break;
    case OrbisSaveDataParamType::USER_PARAM:
        psf.AddInteger("SAVEDATA_LIST_PARAM", *static_cast<const s32*>(paramBuf), true);
        break;
    case OrbisSaveDataParamType::MTIME:
        psf.AddString("MTIME", UtcTimeToIso8601(*static_cast<const s64*>(paramBuf)), true);
        break;
    }
    slot->instance->MarkSfoDirty();
    slot->instance->SaveParamSFO(true);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataGetParam(const OrbisSaveDataMountPoint* mountPoint, u32 paramType,
                                     void* paramBuf, size_t paramBufSize, size_t* gotSize) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    std::string mp;
    if (auto err = ValidateMountPoint(mountPoint, mp)) {
        return Translate(OrbisSaveDataCommand::GET_PARAM, err);
    }
    if (paramBuf == nullptr) {
        return Translate(OrbisSaveDataCommand::GET_PARAM,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    const auto ptype = static_cast<OrbisSaveDataParamType>(paramType);
    size_t required = 0;
    switch (ptype) {
    case OrbisSaveDataParamType::ALL:
        required = sizeof(OrbisSaveDataParam);
        break;
    case OrbisSaveDataParamType::TITLE:
    case OrbisSaveDataParamType::SUB_TITLE:
        required = ORBIS_SAVE_DATA_TITLE_MAXSIZE;
        break;
    case OrbisSaveDataParamType::DETAIL:
        required = ORBIS_SAVE_DATA_DETAIL_MAXSIZE;
        break;
    case OrbisSaveDataParamType::USER_PARAM:
        required = sizeof(u32);
        break;
    case OrbisSaveDataParamType::MTIME:
        required = sizeof(s64);
        break;
    default:
        return Translate(OrbisSaveDataCommand::GET_PARAM,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    if (paramBufSize < required) {
        return Translate(OrbisSaveDataCommand::GET_PARAM,
                         static_cast<u32>(OrbisSaveDataError::BUF_NO_SPACE));
    }

    std::scoped_lock lk(g_mutex);
    ActiveMountSlot* slot = FindMountSlotByMountPoint(mp);
    if (slot == nullptr) {
        return Translate(OrbisSaveDataCommand::GET_PARAM,
                         static_cast<u32>(OrbisSaveDataError::NOT_MOUNTED));
    }

    const PSF& psf = slot->instance->GetParamSFO();
    std::memset(paramBuf, 0, required);

    auto get_str = [&](const char* key) -> std::string {
        auto v = psf.GetString(key);
        return v ? std::string(*v) : std::string{};
    };
    const s32 user_param = psf.GetInteger("SAVEDATA_LIST_PARAM").value_or(0);
    const std::string mtime_s = get_str("MTIME");
    const s64 mtime = mtime_s.empty() ? 0 : Iso8601ToUtcTime(mtime_s);

    switch (ptype) {
    case OrbisSaveDataParamType::ALL: {
        auto* out = static_cast<OrbisSaveDataParam*>(paramBuf);
        TruncateUtf8(get_str("MAINTITLE"), out->title, sizeof(out->title));
        TruncateUtf8(get_str("SUBTITLE"), out->subTitle, sizeof(out->subTitle));
        TruncateUtf8(get_str("DETAIL"), out->detail, sizeof(out->detail));
        out->userParam = static_cast<u32>(user_param);
        out->mtime = mtime;
        break;
    }
    case OrbisSaveDataParamType::TITLE:
        TruncateUtf8(get_str("MAINTITLE"), static_cast<char*>(paramBuf),
                     ORBIS_SAVE_DATA_TITLE_MAXSIZE);
        break;
    case OrbisSaveDataParamType::SUB_TITLE:
        TruncateUtf8(get_str("SUBTITLE"), static_cast<char*>(paramBuf),
                     ORBIS_SAVE_DATA_SUBTITLE_MAXSIZE);
        break;
    case OrbisSaveDataParamType::DETAIL:
        TruncateUtf8(get_str("DETAIL"), static_cast<char*>(paramBuf),
                     ORBIS_SAVE_DATA_DETAIL_MAXSIZE);
        break;
    case OrbisSaveDataParamType::USER_PARAM:
        *static_cast<u32*>(paramBuf) = static_cast<u32>(user_param);
        break;
    case OrbisSaveDataParamType::MTIME:
        *static_cast<s64*>(paramBuf) = mtime;
        break;
    }
    if (gotSize != nullptr) {
        *gotSize = required;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataSaveIcon(const OrbisSaveDataMountPoint* mountPoint,
                                     const OrbisSaveDataIcon* icon) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    std::string mp;
    if (auto err = ValidateMountPoint(mountPoint, mp)) {
        return Translate(OrbisSaveDataCommand::SAVE_ICON, err);
    }
    if (icon == nullptr || !ValidateReservedZero(icon->reserved, sizeof(icon->reserved))) {
        return Translate(OrbisSaveDataCommand::SAVE_ICON,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    if (icon->buf == nullptr || icon->dataSize == 0 || icon->dataSize > icon->bufSize ||
        icon->dataSize > SaveInstance::IconMaxBytes) {
        return Translate(OrbisSaveDataCommand::SAVE_ICON,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    std::span<const u8> png_span(static_cast<const u8*>(icon->buf), icon->dataSize);
    if (!ValidateIconPng(png_span)) {
        return Translate(OrbisSaveDataCommand::SAVE_ICON,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }

    std::scoped_lock lk(g_mutex);
    ActiveMountSlot* slot = FindMountSlotByMountPoint(mp);
    if (slot == nullptr) {
        return Translate(OrbisSaveDataCommand::SAVE_ICON,
                         static_cast<u32>(OrbisSaveDataError::NOT_MOUNTED));
    }
    if (slot->instance->IsReadOnly()) {
        return Translate(OrbisSaveDataCommand::SAVE_ICON,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    const fs::path icon_path = slot->instance->GetIconPath();
    std::error_code ec;
    fs::create_directories(icon_path.parent_path(), ec);
    Common::FS::IOFile file(icon_path, Common::FS::FileAccessMode::Create);
    if (!file.IsOpen() || file.WriteRaw<u8>(icon->buf, icon->dataSize) != icon->dataSize) {
        return Translate(OrbisSaveDataCommand::SAVE_ICON,
                         static_cast<u32>(OrbisSaveDataError::INTERNAL));
    }
    file.Close();
    slot->instance->SyncMetaIcon();
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataLoadIcon(const OrbisSaveDataMountPoint* mountPoint,
                                     OrbisSaveDataIcon* icon) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    std::string mp;
    if (auto err = ValidateMountPoint(mountPoint, mp)) {
        return Translate(OrbisSaveDataCommand::LOAD_ICON, err);
    }
    if (icon == nullptr || icon->buf == nullptr ||
        !ValidateReservedZero(icon->reserved, sizeof(icon->reserved))) {
        return Translate(OrbisSaveDataCommand::LOAD_ICON,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }

    std::scoped_lock lk(g_mutex);
    ActiveMountSlot* slot = FindMountSlotByMountPoint(mp);
    if (slot == nullptr) {
        return Translate(OrbisSaveDataCommand::LOAD_ICON,
                         static_cast<u32>(OrbisSaveDataError::NOT_MOUNTED));
    }
    const fs::path icon_path = slot->instance->GetIconPath();
    std::error_code ec;
    if (!fs::exists(icon_path, ec)) {
        return Translate(OrbisSaveDataCommand::LOAD_ICON,
                         static_cast<u32>(OrbisSaveDataError::NOT_FOUND));
    }
    const size_t sz = static_cast<size_t>(fs::file_size(icon_path, ec));
    icon->dataSize = sz;
    if (icon->bufSize < sz) {
        return Translate(OrbisSaveDataCommand::LOAD_ICON,
                         static_cast<u32>(OrbisSaveDataError::BUF_NO_SPACE));
    }
    Common::FS::IOFile file(icon_path, Common::FS::FileAccessMode::Read);
    if (!file.IsOpen() || file.ReadRaw<u8>(icon->buf, sz) != sz) {
        return Translate(OrbisSaveDataCommand::LOAD_ICON,
                         static_cast<u32>(OrbisSaveDataError::INTERNAL));
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataRegisterEventCallback(OrbisSaveDataEventCallbackFunc cb,
                                                  void* userdata) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    std::scoped_lock lk(g_mutex);
    g_event_callback = cb;
    g_event_userdata = userdata;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataUnregisterEventCallback() {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    std::scoped_lock lk(g_mutex);
    g_event_callback = nullptr;
    g_event_userdata = nullptr;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataRebuildDatabase() {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    std::scoped_lock lk(g_mutex);
    bool ok = true;
    for (s32 uid : CollectTargetUsers()) {
        if (!SaveDb::RebuildFromDisk(uid)) {
            ok = false;
        }
    }
    return ok ? ORBIS_OK
              : Translate(OrbisSaveDataCommand::REBUILD_DATABASE,
                          static_cast<u32>(OrbisSaveDataError::INTERNAL));
}

s32 PS4_SYSV_ABI sceSaveDataChangeDatabase(s32 userId, u32 /*param*/) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (auto err = ValidateUserId(userId)) {
        return Translate(OrbisSaveDataCommand::CHANGE_DATABASE, err);
    }
    std::scoped_lock lk(g_mutex);
    return SaveDb::EnsureDatabase(userId)
               ? ORBIS_OK
               : Translate(OrbisSaveDataCommand::CHANGE_DATABASE,
                           static_cast<u32>(OrbisSaveDataError::INTERNAL));
}

s32 PS4_SYSV_ABI sceSaveDataAbort() {
    g_abort_requested = true;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataMountSys(s32 userId, s32 mountMode,
                                     OrbisSaveDataMountResult* mountResult) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (mountResult == nullptr || (mountMode != 1 && mountMode != 2)) {
        return Translate(OrbisSaveDataCommand::MOUNT_SYS,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    if (auto err = ValidateUserId(userId)) {
        return Translate(OrbisSaveDataCommand::MOUNT_SYS, err);
    }
    std::scoped_lock lk(g_mutex);
    const std::string mp = "/savedata_sys";
    const fs::path host_path = GetSaveRootPath(userId);
    std::error_code ec;
    fs::create_directories(host_path, ec);
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    mnt->Mount(host_path, mp, mountMode == 1);
    g_sys_mounts.push_back(mp);
    std::memset(mountResult, 0, sizeof(*mountResult));
    std::memcpy(mountResult->mountPoint.data, mp.data(),
                std::min(mp.size(), sizeof(mountResult->mountPoint.data) - 1));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataUmountSys(const OrbisSaveDataMountPoint* mountPoint) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    std::string mp;
    if (auto err = ValidateMountPoint(mountPoint, mp)) {
        return Translate(OrbisSaveDataCommand::UMOUNT_SYS, err);
    }
    std::scoped_lock lk(g_mutex);
    auto it = std::find(g_sys_mounts.begin(), g_sys_mounts.end(), mp);
    if (it == g_sys_mounts.end()) {
        return Translate(OrbisSaveDataCommand::UMOUNT_SYS,
                         static_cast<u32>(OrbisSaveDataError::NOT_MOUNTED));
    }
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    if (!mnt->Unmount(mp)) {
        return Translate(OrbisSaveDataCommand::UMOUNT_SYS,
                         static_cast<u32>(OrbisSaveDataError::BUSY));
    }
    g_sys_mounts.erase(it);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataGetSavePoint() {
    return 2; // HDD
}

s32 PS4_SYSV_ABI sceSaveDataGetDataBaseFilePath(s32 savePoint, char* buf, size_t bufSize) {
    if (buf == nullptr || bufSize < OrbisSaveDataMountMaxPathDataSize) {
        return Translate(OrbisSaveDataCommand::GET_DATA_BASE_FILE_PATH,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    const char* rel = nullptr;
    char usb_buf[32]{};
    if (savePoint == 2) {
        rel = "user/savedata.db";
    } else if (savePoint == 3) {
        rel = "cloud/savedata.db";
    } else if (savePoint >= 4 && savePoint <= 11) {
        std::snprintf(usb_buf, sizeof(usb_buf), "usb%d/savedata.db", savePoint - 4);
        rel = usb_buf;
    } else {
        return Translate(OrbisSaveDataCommand::GET_DATA_BASE_FILE_PATH,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    std::snprintf(buf, bufSize, "%s", rel);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataGetSaveDataRootPath(s32 savePoint, s32 userId, char* buf,
                                                size_t bufSize) {
    if (buf == nullptr || bufSize < OrbisSaveDataMountMaxPathDataSize || savePoint != 2) {
        return Translate(OrbisSaveDataCommand::GET_SAVE_DATA_ROOT_PATH,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    if (auto err = ValidateUserId(userId)) {
        return Translate(OrbisSaveDataCommand::GET_SAVE_DATA_ROOT_PATH, err);
    }
    std::snprintf(buf, bufSize, "/user/home/%08x/savedata", static_cast<u32>(userId));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataGetSaveDataRootUsbPath(s32 savePoint, u64 accountId, char* buf,
                                                   size_t bufSize) {
    if (buf == nullptr || bufSize < OrbisSaveDataMountMaxPathDataSize || savePoint < 4 ||
        savePoint > 11) {
        return Translate(OrbisSaveDataCommand::GET_SAVE_DATA_ROOT_USB_PATH,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    std::snprintf(buf, bufSize, "/usb%d/PS4/SAVEDATA/%016llx", savePoint - 4,
                  static_cast<unsigned long long>(accountId));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataGetSaveDataRootDir(s32 savePoint, s32 userId, char* buf,
                                               size_t bufSize) {
    return sceSaveDataGetSaveDataRootPath(savePoint, userId, buf, bufSize);
}

s32 PS4_SYSV_ABI sceSaveDataIsMounted(s32 userId) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (auto err = ValidateUserId(userId)) {
        return Translate(OrbisSaveDataCommand::IS_MOUNTED, err);
    }
    std::scoped_lock lk(g_mutex);
    for (const auto& s : g_mount_slots) {
        if (s.instance->GetUserId() == userId) {
            return 1;
        }
    }
    return 0;
}

s32 PS4_SYSV_ABI sceSaveDataGetMountedSaveDataCount(u32* count) {
    if (count == nullptr) {
        return ORBIS_SAVE_DATA_ERROR_INVALID_ARGUMENT;
    }
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    std::scoped_lock lk(g_mutex);
    *count = static_cast<u32>(g_mount_slots.size());
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataGetUpdatedDataCount(s32 userId, u32* count) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (count == nullptr) {
        return Translate(OrbisSaveDataCommand::GET_UPDATED_DATA_COUNT,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    if (auto err = ValidateUserId(userId)) {
        return Translate(OrbisSaveDataCommand::GET_UPDATED_DATA_COUNT, err);
    }
    *count = 0;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataBindPsnAccount(s32 userId, u64 /*accountId*/) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (auto err = ValidateUserId(userId)) {
        return Translate(OrbisSaveDataCommand::BIND_PSN_ACCOUNT, err);
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataBindPsnAccountForSystemBackup(s32 userId, u64 /*accountId*/) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (auto err = ValidateUserId(userId)) {
        return Translate(OrbisSaveDataCommand::BIND_PSN_ACCOUNT, err);
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataGetBoundPsnAccountCount(s32* count) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (count == nullptr) {
        return ORBIS_SAVE_DATA_ERROR_INVALID_ARGUMENT;
    }
    *count = 0;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataGetAllSize(u64* size) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (size == nullptr) {
        return Translate(OrbisSaveDataCommand::GET_ALL_SIZE,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    std::scoped_lock lk(g_mutex);
    u64 total = 0;
    const std::string tid = GetGameTitleId();
    for (s32 uid : CollectTargetUsers()) {
        if (g_abort_requested) {
            return Translate(OrbisSaveDataCommand::GET_ALL_SIZE,
                             static_cast<u32>(OrbisSaveDataError::ABORTED));
        }
        total += SaveDb::GetTotalSizeBytes(uid, tid).value_or(0);
    }
    *size = total;
    return ORBIS_OK;
}

s32 DoSetupSaveMemory(OrbisSaveDataCommand cmd, const OrbisSaveDataMemorySetup2* setup,
                      OrbisSaveDataMemorySetupResult* result) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    size_t alloc_size = 0;
    if (!ValidateSetupMemory2(setup, alloc_size)) {
        return Translate(cmd, static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    if (auto err = ValidateUserId(setup->userId)) {
        return Translate(cmd, err);
    }
    std::scoped_lock lk(g_mutex);
    size_t used_other = 0;
    for (u32 i = 0; i < OrbisSaveDataMemorySlotCount; ++i) {
        if (i != setup->slotId && SaveMemory::IsSaveMemoryInitialized(i)) {
            used_other += SaveMemory::GetMemorySize(i);
        }
    }
    if (used_other + alloc_size > OrbisSaveDataMemoryMaxTotalSize) {
        return Translate(cmd, static_cast<u32>(OrbisSaveDataError::NO_SPACE_FS));
    }

    const std::string tid = GetGameTitleId();
    std::vector<u8> init_icon;
    if (setup->initIcon != nullptr && setup->initIcon->dataSize > 0) {
        const auto* p = static_cast<const u8*>(setup->initIcon->buf);
        init_icon.assign(p, p + setup->initIcon->dataSize);
    }
    const size_t existed_size =
        SaveMemory::SetupSaveMemory(setup->userId, setup->slotId, tid, alloc_size);
    if ((setup->option & 2U) != 0 && existed_size > 0) {
        std::vector<u8> zeros(alloc_size, 0);
        SaveMemory::WriteMemory(setup->slotId, zeros.data(), zeros.size(), 0);
    }
    if (existed_size == 0 && setup->initParam != nullptr) {
        PSF& psf = SaveMemory::GetParamSFO(setup->slotId);
        psf.AddString("MAINTITLE", setup->initParam->title, true);
        psf.AddString("SUBTITLE", setup->initParam->subTitle, true);
        psf.AddString("DETAIL", setup->initParam->detail, true);
        psf.AddInteger("SAVEDATA_LIST_PARAM", static_cast<s32>(setup->initParam->userParam), true);
        SaveMemory::SaveSFO(setup->slotId);
    }
    if (!init_icon.empty()) {
        SaveMemory::SetIcon(setup->slotId, init_icon.data(), init_icon.size());
    }
    SaveMemory::PersistMemory(setup->slotId);
    if (result != nullptr) {
        std::memset(result, 0, sizeof(*result));
        result->existedMemorySize = existed_size;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataSetupSaveDataMemory(s32 userId, size_t memorySize,
                                                const OrbisSaveDataParam* param,
                                                const OrbisSaveDataIcon* icon, u32 /*value*/) {
    OrbisSaveDataMemorySetup2 s2{};
    s2.option = 0;
    s2.userId = userId;
    s2.memorySize = memorySize;
    s2.iconMemorySize = icon != nullptr ? icon->dataSize : 0;
    s2.initParam = param;
    s2.initIcon = icon;
    s2.slotId = 0;
    return DoSetupSaveMemory(OrbisSaveDataCommand::SETUP_SAVE_DATA_MEMORY, &s2, nullptr);
}

s32 PS4_SYSV_ABI sceSaveDataSetupSaveDataMemory2(const OrbisSaveDataMemorySetup2* setupParam,
                                                 OrbisSaveDataMemorySetupResult* result) {
    return DoSetupSaveMemory(OrbisSaveDataCommand::SETUP_SAVE_DATA_MEMORY2, setupParam, result);
}

s32 DoGetSaveMemory(OrbisSaveDataCommand cmd, const OrbisSaveDataMemoryGet2* getParam) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (getParam == nullptr || getParam->slotId >= OrbisSaveDataMemorySlotCount ||
        !ValidateReservedZero(getParam->reserved, sizeof(getParam->reserved)) ||
        !ValidateReservedZero(getParam->reserved2, sizeof(getParam->reserved2))) {
        return Translate(cmd, static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    if (auto err = ValidateUserId(getParam->userId)) {
        return Translate(cmd, err);
    }
    std::scoped_lock lk(g_mutex);
    if (!SaveMemory::IsSaveMemoryInitialized(getParam->slotId) ||
        !SaveMemory::IsSlotOwnedByUser(getParam->slotId, getParam->userId)) {
        return Translate(cmd, static_cast<u32>(OrbisSaveDataError::NOT_FOUND));
    }
    if (getParam->data != nullptr) {
        const auto* d = getParam->data;
        if (d->buf == nullptr || d->offset < 0 ||
            !ValidateReservedZero(d->reserved, sizeof(d->reserved))) {
            return Translate(cmd, static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
        }
        const size_t slot_sz = SaveMemory::GetMemorySize(getParam->slotId);
        if (static_cast<size_t>(d->offset) + d->bufSize > slot_sz) {
            return Translate(cmd, static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
        }
        SaveMemory::ReadMemory(getParam->slotId, d->buf, d->bufSize, d->offset);
    }
    if (getParam->param != nullptr) {
        std::memset(getParam->param, 0, sizeof(OrbisSaveDataParam));
        const PSF& psf = SaveMemory::GetParamSFO(getParam->slotId);
        if (auto v = psf.GetString("MAINTITLE")) {
            TruncateUtf8(*v, getParam->param->title, sizeof(getParam->param->title));
        }
        if (auto v = psf.GetString("SUBTITLE")) {
            TruncateUtf8(*v, getParam->param->subTitle, sizeof(getParam->param->subTitle));
        }
        if (auto v = psf.GetString("DETAIL")) {
            TruncateUtf8(*v, getParam->param->detail, sizeof(getParam->param->detail));
        }
        if (auto v = psf.GetInteger("SAVEDATA_LIST_PARAM")) {
            getParam->param->userParam = static_cast<u32>(*v);
        }
        if (auto v = psf.GetString("MTIME")) {
            getParam->param->mtime = Iso8601ToUtcTime(*v);
        }
    }
    if (getParam->icon != nullptr) {
        if (getParam->icon->buf == nullptr ||
            !ValidateReservedZero(getParam->icon->reserved, sizeof(getParam->icon->reserved))) {
            return Translate(cmd, static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
        }
        std::vector<u8> png = SaveMemory::GetIcon(getParam->slotId);
        getParam->icon->dataSize = png.size();
        if (getParam->icon->bufSize < png.size()) {
            return Translate(cmd, static_cast<u32>(OrbisSaveDataError::BUF_NO_SPACE));
        }
        if (!png.empty()) {
            std::memcpy(getParam->icon->buf, png.data(), png.size());
        }
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataGetSaveDataMemory(s32 userId, void* buf, size_t bufSize, s64 offset) {
    OrbisSaveDataMemoryData data{};
    data.buf = buf;
    data.bufSize = bufSize;
    data.offset = offset;
    OrbisSaveDataMemoryGet2 get2{};
    get2.userId = userId;
    get2.data = &data;
    get2.slotId = 0;
    return DoGetSaveMemory(OrbisSaveDataCommand::GET_SAVE_DATA_MEMORY, &get2);
}

s32 PS4_SYSV_ABI sceSaveDataGetSaveDataMemory2(const OrbisSaveDataMemoryGet2* getParam) {
    return DoGetSaveMemory(OrbisSaveDataCommand::GET_SAVE_DATA_MEMORY2, getParam);
}

s32 DoSetSaveMemory(OrbisSaveDataCommand cmd, const OrbisSaveDataMemorySet2* setParam) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (setParam == nullptr || setParam->slotId >= OrbisSaveDataMemorySlotCount ||
        !ValidateReservedZero(setParam->reserved, sizeof(setParam->reserved)) ||
        !ValidateReservedZero(setParam->reserved2, sizeof(setParam->reserved2))) {
        return Translate(cmd, static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    if (auto err = ValidateUserId(setParam->userId)) {
        return Translate(cmd, err);
    }
    if (setParam->param != nullptr &&
        ValidateParam(setParam->param, OrbisSaveDataParamType::ALL, true) != 0) {
        return Translate(cmd, static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    if (setParam->icon != nullptr) {
        if (setParam->icon->buf == nullptr || setParam->icon->dataSize == 0 ||
            setParam->icon->dataSize > setParam->icon->bufSize ||
            setParam->icon->dataSize > SaveInstance::IconMaxBytes ||
            !ValidateReservedZero(setParam->icon->reserved, sizeof(setParam->icon->reserved))) {
            return Translate(cmd, static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
        }
        std::span<const u8> png(static_cast<const u8*>(setParam->icon->buf),
                                setParam->icon->dataSize);
        if (!ValidateIconPng(png)) {
            return Translate(cmd, static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
        }
    }
    std::scoped_lock lk(g_mutex);
    if (!SaveMemory::IsSaveMemoryInitialized(setParam->slotId) ||
        !SaveMemory::IsSlotOwnedByUser(setParam->slotId, setParam->userId)) {
        return Translate(cmd, static_cast<u32>(OrbisSaveDataError::NOT_FOUND));
    }
    if (setParam->data != nullptr) {
        const auto* d = setParam->data;
        if (d->buf == nullptr || d->offset < 0 ||
            !ValidateReservedZero(d->reserved, sizeof(d->reserved))) {
            return Translate(cmd, static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
        }
        const size_t slot_sz = SaveMemory::GetMemorySize(setParam->slotId);
        if (static_cast<size_t>(d->offset) + d->bufSize > slot_sz) {
            return Translate(cmd, static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
        }
        SaveMemory::WriteMemory(setParam->slotId, d->buf, d->bufSize, d->offset);
    }
    if (setParam->param != nullptr) {
        PSF& psf = SaveMemory::GetParamSFO(setParam->slotId);
        psf.AddString("MAINTITLE", setParam->param->title, true);
        psf.AddString("SUBTITLE", setParam->param->subTitle, true);
        psf.AddString("DETAIL", setParam->param->detail, true);
        psf.AddInteger("SAVEDATA_LIST_PARAM", static_cast<s32>(setParam->param->userParam), true);
        SaveMemory::SaveSFO(setParam->slotId);
    }
    if (setParam->icon != nullptr) {
        SaveMemory::SetIcon(setParam->slotId, setParam->icon->buf, setParam->icon->dataSize);
    }
    SaveMemory::PersistMemory(setParam->slotId);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataSetSaveDataMemory(s32 userId, const void* buf, size_t bufSize,
                                              s64 offset) {
    OrbisSaveDataMemoryData data{};
    data.buf = const_cast<void*>(buf);
    data.bufSize = bufSize;
    data.offset = offset;
    OrbisSaveDataMemorySet2 set2{};
    set2.userId = userId;
    set2.data = &data;
    set2.slotId = 0;
    return DoSetSaveMemory(OrbisSaveDataCommand::SET_SAVE_DATA_MEMORY, &set2);
}

s32 PS4_SYSV_ABI sceSaveDataSetSaveDataMemory2(const OrbisSaveDataMemorySet2* setParam) {
    return DoSetSaveMemory(OrbisSaveDataCommand::SET_SAVE_DATA_MEMORY2, setParam);
}

s32 PS4_SYSV_ABI sceSaveDataSyncSaveDataMemory(const OrbisSaveDataMemorySync* syncParam) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (syncParam == nullptr || syncParam->slotId >= OrbisSaveDataMemorySlotCount ||
        !ValidateReservedZero(syncParam->reserved, sizeof(syncParam->reserved))) {
        return Translate(OrbisSaveDataCommand::SYNC_SAVE_DATA_MEMORY,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    if (auto err = ValidateUserId(syncParam->userId)) {
        return Translate(OrbisSaveDataCommand::SYNC_SAVE_DATA_MEMORY, err);
    }
    std::scoped_lock lk(g_mutex);
    if (!SaveMemory::IsSaveMemoryInitialized(syncParam->slotId) ||
        !SaveMemory::IsSlotOwnedByUser(syncParam->slotId, syncParam->userId)) {
        return Translate(OrbisSaveDataCommand::SYNC_SAVE_DATA_MEMORY,
                         static_cast<u32>(OrbisSaveDataError::NOT_FOUND));
    }
    SaveMemory::PersistMemory(syncParam->slotId);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
sceSaveDataRestoreLoadSaveDataMemory(const OrbisSaveDataRestoreLoadMemory* restoreParam) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (restoreParam == nullptr || restoreParam->slotId >= OrbisSaveDataMemorySlotCount ||
        !ValidateReservedZero(restoreParam->reserved, sizeof(restoreParam->reserved))) {
        return Translate(OrbisSaveDataCommand::RESTORE_LOAD_SAVE_DATA_MEMORY,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    if (auto err = ValidateUserId(restoreParam->userId)) {
        return Translate(OrbisSaveDataCommand::RESTORE_LOAD_SAVE_DATA_MEMORY, err);
    }
    std::scoped_lock lk(g_mutex);
    if (!SaveMemory::RestoreLoad(restoreParam->slotId, restoreParam->userId, GetGameTitleId())) {
        return Translate(OrbisSaveDataCommand::RESTORE_LOAD_SAVE_DATA_MEMORY,
                         static_cast<u32>(OrbisSaveDataError::NOT_FOUND));
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataSupportedFakeBrokenStatus(s32 flag) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    g_fake_broken_supported = (flag != 0);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataGetFormat() {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    return 0; // PfsSaveData
}

s32 PS4_SYSV_ABI sceSaveDataSetSaveDataLibraryUser(s32 userId) {
    if (auto err = ValidateUserId(userId)) {
        return Translate(OrbisSaveDataCommand::SET_LIBRARY_USER, err);
    }
    g_library_user = userId;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataGetSaveDataCount(s32 userId) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (auto err = ValidateUserId(userId)) {
        return Translate(OrbisSaveDataCommand::GET_SAVE_DATA_COUNT, err);
    }
    std::scoped_lock lk(g_mutex);
    return static_cast<s32>(SaveDb::GetCount(userId, GetGameTitleId()).value_or(0));
}

s32 PS4_SYSV_ABI sceSaveDataGetProgress(float* progress) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (progress == nullptr) {
        return ORBIS_SAVE_DATA_ERROR_INVALID_ARGUMENT;
    }
    *progress = Backup::GetProgress();
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataClearProgress() {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    Backup::ClearProgress();
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataGetEventResult(const OrbisSaveDataEventParam* eventParam,
                                           OrbisSaveDataEventResult* eventResult) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (eventParam == nullptr || eventResult == nullptr) {
        return ORBIS_SAVE_DATA_ERROR_INVALID_ARGUMENT;
    }
    std::scoped_lock lk(g_mutex);
    if (!g_has_last_event_result) {
        return ORBIS_SAVE_DATA_ERROR_NOT_FOUND;
    }
    *eventResult = g_last_event_result;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataSetEventInfo(const OrbisSaveDataEventInfo* info) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (info == nullptr || !ValidateReservedZero(info->reserved, sizeof(info->reserved))) {
        return ORBIS_SAVE_DATA_ERROR_INVALID_ARGUMENT;
    }
    std::scoped_lock lk(g_mutex);
    g_stored_event_info = *info;
    g_has_stored_event_info = true;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataGetEventInfo(u32 /*infoType*/, OrbisSaveDataEventInfo* info) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (info == nullptr) {
        return ORBIS_SAVE_DATA_ERROR_INVALID_ARGUMENT;
    }
    std::scoped_lock lk(g_mutex);
    if (!g_has_stored_event_info) {
        return ORBIS_SAVE_DATA_ERROR_NOT_FOUND;
    }
    *info = g_stored_event_info;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataGetAppLaunchedUser(s32 userId, s32* launchedUser) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (launchedUser == nullptr) {
        return ORBIS_SAVE_DATA_ERROR_INVALID_ARGUMENT;
    }
    if (userId > 0) {
        *launchedUser = userId;
        return ORBIS_OK;
    }
    if (g_library_user > 0) {
        *launchedUser = g_library_user;
        return ORBIS_OK;
    }
    const auto& users = UserManagement.GetAllUsers();
    if (!users.empty() && users.front().user_id > 0) {
        *launchedUser = users.front().user_id;
        return ORBIS_OK;
    }
    *launchedUser = 1;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataCheckSaveDataBroken(const OrbisSaveDataCheckSaveDataBroken* check,
                                                OrbisSaveDataBrokenResult* result) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (check == nullptr || result == nullptr ||
        !ValidateReservedZero(check->reserved, sizeof(check->reserved))) {
        return Translate(OrbisSaveDataCommand::CHECK_SAVE_DATA_BROKEN,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    if (auto err = ValidateUserId(check->userId)) {
        return Translate(OrbisSaveDataCommand::CHECK_SAVE_DATA_BROKEN, err);
    }
    std::string tid;
    if (auto err = ValidateTitleId(check->titleId, true, tid)) {
        return Translate(OrbisSaveDataCommand::CHECK_SAVE_DATA_BROKEN, err);
    }
    std::string dir;
    if (auto err = ValidateDirName(check->dirName, false, true, dir)) {
        return Translate(OrbisSaveDataCommand::CHECK_SAVE_DATA_BROKEN, err);
    }
    std::scoped_lock lk(g_mutex);
    auto entry = SaveDb::GetEntry(check->userId, tid, dir);
    if (!entry) {
        return Translate(OrbisSaveDataCommand::CHECK_SAVE_DATA_BROKEN,
                         static_cast<u32>(OrbisSaveDataError::NOT_FOUND));
    }
    std::memset(result, 0, sizeof(*result));
    result->broken = entry->is_broken ? 1U : 0U;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSaveDataCheckSaveDataVersion(const OrbisSaveDataCheckVersion* check) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (check == nullptr || !ValidateReservedZero(check->reserved, sizeof(check->reserved))) {
        return Translate(OrbisSaveDataCommand::CHECK_SAVE_DATA_VERSION,
                         static_cast<u32>(OrbisSaveDataError::INVALID_ARGUMENT));
    }
    if (auto err = ValidateUserId(check->userId)) {
        return Translate(OrbisSaveDataCommand::CHECK_SAVE_DATA_VERSION, err);
    }
    std::string tid;
    if (auto err = ValidateTitleId(check->titleId, true, tid)) {
        return Translate(OrbisSaveDataCommand::CHECK_SAVE_DATA_VERSION, err);
    }
    std::string dir;
    if (auto err = ValidateDirName(check->dirName, false, true, dir)) {
        return Translate(OrbisSaveDataCommand::CHECK_SAVE_DATA_VERSION, err);
    }
    std::scoped_lock lk(g_mutex);
    const fs::path sfo_path = GetSaveDirPath(check->userId, tid, dir) / "sce_sys" / "param.sfo";
    if (!fs::exists(sfo_path)) {
        return Translate(OrbisSaveDataCommand::CHECK_SAVE_DATA_VERSION,
                         static_cast<u32>(OrbisSaveDataError::NOT_FOUND));
    }
    return 0; // latest compatible format
}

s32 PS4_SYSV_ABI sceSaveDataCheckSaveDataVersionLatest(const OrbisSaveDataCheckVersion* check) {
    return sceSaveDataCheckSaveDataVersion(check);
}

// Cloud / PSN / Debug stubs required for symbol parity
s32 PS4_SYSV_ABI sceSaveDataCreateUploadData(const void*) {
    return ORBIS_SAVE_DATA_ERROR_NOT_SUPPORTED;
}
s32 PS4_SYSV_ABI sceSaveDataSyncCloudList(const void*) {
    return ORBIS_SAVE_DATA_ERROR_NOT_SUPPORTED;
}
s32 PS4_SYSV_ABI sceSaveDataGetCloudQuotaInfo(const void*, void*) {
    return ORBIS_SAVE_DATA_ERROR_NOT_SUPPORTED;
}
s32 PS4_SYSV_ABI sceSaveDataUpload(const void*) {
    return ORBIS_SAVE_DATA_ERROR_NOT_SUPPORTED;
}
s32 PS4_SYSV_ABI sceSaveDataDownload(const void*) {
    return ORBIS_SAVE_DATA_ERROR_NOT_SUPPORTED;
}
s32 PS4_SYSV_ABI sceSaveDataDeleteCloudData(const void*) {
    return ORBIS_SAVE_DATA_ERROR_NOT_SUPPORTED;
}
s32 PS4_SYSV_ABI sceSaveDataCheckCloudData(const void*, void*) {
    return ORBIS_SAVE_DATA_ERROR_NOT_SUPPORTED;
}
s32 PS4_SYSV_ABI sceSaveDataGetAutoUploadSetting(const void*, void*) {
    return ORBIS_SAVE_DATA_ERROR_NOT_SUPPORTED;
}
s32 PS4_SYSV_ABI sceSaveDataSetAutoUploadSetting(const void*) {
    return ORBIS_SAVE_DATA_ERROR_NOT_SUPPORTED;
}
s32 PS4_SYSV_ABI sceSaveDataGetAutoUploadRequestInfo(const void*, void*) {
    return ORBIS_SAVE_DATA_ERROR_NOT_SUPPORTED;
}
s32 PS4_SYSV_ABI sceSaveDataGetAutoUploadConditions(const void*, void*) {
    return ORBIS_SAVE_DATA_ERROR_NOT_SUPPORTED;
}
s32 PS4_SYSV_ABI Func_02E4C4D201716422() {
    return g_initialized ? ORBIS_OK : ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
}
s32 PS4_SYSV_ABI sceSaveDataDebugCleanMount() {
    return ORBIS_OK;
}
s32 PS4_SYSV_ABI sceSaveDataDebugCreateSaveDataRoot() {
    return ORBIS_OK;
}
s32 PS4_SYSV_ABI sceSaveDataDebugRemoveSaveDataRoot() {
    return ORBIS_OK;
}
s32 PS4_SYSV_ABI sceSaveDataDebugTarget() {
    return ORBIS_OK;
}
s32 PS4_SYSV_ABI sceSaveDataDebugCompiledSdkVersion() {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    return ORBIS_OK;
}
s32 PS4_SYSV_ABI sceSaveDataGetClientThreadPriority(u32* priority) {
    if (priority == nullptr) {
        return ORBIS_SAVE_DATA_ERROR_INVALID_ARGUMENT;
    }
    *priority = g_client_thread_priority;
    return ORBIS_OK;
}
s32 PS4_SYSV_ABI sceSaveDataDebugGetThreadId() {
    return 0;
}
s32 PS4_SYSV_ABI sceSaveDataDebug() {
    return ORBIS_OK;
}
s32 PS4_SYSV_ABI sceSaveDataCheckIpmiIfSize() {
    return ORBIS_OK;
}
s32 PS4_SYSV_ABI sceSaveDataIsDeletingUsbDb() {
    return 0;
}
s32 PS4_SYSV_ABI Func_C09EA88AEF93A8AF(u64* requiredBlocks) {
    if (!g_initialized) {
        return ORBIS_SAVE_DATA_ERROR_NOT_INITIALIZED;
    }
    if (requiredBlocks == nullptr) {
        return ORBIS_SAVE_DATA_ERROR_INVALID_ARGUMENT;
    }
    std::scoped_lock lk(g_mutex);
    *requiredBlocks = g_last_required_blocks;
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("ZkZhskCPXFw", "libSceSaveData", 1, "libSceSaveData", sceSaveDataInitialize);
    LIB_FUNCTION("l1NmDeDpNGU", "libSceSaveData", 1, "libSceSaveData", sceSaveDataInitialize2);
    LIB_FUNCTION("TywrFKCoLGY", "libSceSaveData", 1, "libSceSaveData", sceSaveDataInitialize3);
    LIB_FUNCTION("g9uwUI3BlQ8", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataInitializeForCdlg);
    LIB_FUNCTION("yKDy8S5yLA0", "libSceSaveData", 1, "libSceSaveData", sceSaveDataTerminate);
    LIB_FUNCTION("32HQAQdwM2o", "libSceSaveData", 1, "libSceSaveData", sceSaveDataMount);
    LIB_FUNCTION("0z45PIH+SNI", "libSceSaveData", 1, "libSceSaveData", sceSaveDataMount2);
    LIB_FUNCTION("WAzWTZm1H+I", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataTransferringMount);
    LIB_FUNCTION("BMR4F-Uek3E", "libSceSaveData", 1, "libSceSaveData", sceSaveDataUmount);
    LIB_FUNCTION("VwadwBBBJ80", "libSceSaveData", 1, "libSceSaveData", sceSaveDataUmountWithBackup);
    LIB_FUNCTION("lU9YRFsgwSU", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataRestoreBackupData);
    LIB_FUNCTION("HuToUt1GQ8w", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataRestoreBackupDataForCdlg);
    LIB_FUNCTION("RQOqDbk3bSU", "libSceSaveData", 1, "libSceSaveData", sceSaveDataCheckBackupData);
    LIB_FUNCTION("v1TrX+3ZB10", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataCheckBackupDataInternal);
    LIB_FUNCTION("rYvLW1z2poM", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataCheckBackupDataForCdlg);
    LIB_FUNCTION("G0hFeOdRCUs", "libSceSaveData", 1, "libSceSaveData", sceSaveDataChangeInternal);
    LIB_FUNCTION("z1JA8-iJt3k", "libSceSaveData", 1, "libSceSaveData", sceSaveDataBackup);
    LIB_FUNCTION("65VH0Qaaz6s", "libSceSaveData", 1, "libSceSaveData", sceSaveDataGetMountInfo);
    LIB_FUNCTION("S1GkePI17zQ", "libSceSaveData", 1, "libSceSaveData", sceSaveDataDelete);
    LIB_FUNCTION("dyIhnXq-0SM", "libSceSaveData", 1, "libSceSaveData", sceSaveDataDirNameSearch);
    LIB_FUNCTION("85zul--eGXs", "libSceSaveData", 1, "libSceSaveData", sceSaveDataSetParam);
    LIB_FUNCTION("XgvSuIdnMlw", "libSceSaveData", 1, "libSceSaveData", sceSaveDataGetParam);
    LIB_FUNCTION("c88Yy54Mx0w", "libSceSaveData", 1, "libSceSaveData", sceSaveDataSaveIcon);
    LIB_FUNCTION("cGjO3wM3V28", "libSceSaveData", 1, "libSceSaveData", sceSaveDataLoadIcon);
    LIB_FUNCTION("hsKd5c21sQc", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataRegisterEventCallback);
    LIB_FUNCTION("v-AK1AxQhS0", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataUnregisterEventCallback);
    LIB_FUNCTION("SgIY-XYA2Xg", "libSceSaveData", 1, "libSceSaveData", sceSaveDataRebuildDatabase);
    LIB_FUNCTION("xz0YMi6BfNk", "libSceSaveData", 1, "libSceSaveData", sceSaveDataMount5);
    LIB_FUNCTION("YbCO38BOOl4", "libSceSaveData", 1, "libSceSaveData", sceSaveDataCopy5);
    LIB_FUNCTION("dQ2GohUHXzk", "libSceSaveData", 1, "libSceSaveData", sceSaveDataAbort);
    LIB_FUNCTION("SQWusLoK8Pw", "libSceSaveData", 1, "libSceSaveData", sceSaveDataDelete5);
    LIB_FUNCTION("uNu7j3pL2mQ", "libSceSaveData", 1, "libSceSaveData", sceSaveDataPromote5);
    LIB_FUNCTION("-XYmdxjOqyA", "libSceSaveData", 1, "libSceSaveData", sceSaveDataMountSys);
    LIB_FUNCTION("2-8NWLS8QSA", "libSceSaveData", 1, "libSceSaveData", sceSaveDataUmountSys);
    LIB_FUNCTION("kb24-4DLyNo", "libSceSaveData", 1, "libSceSaveData", sceSaveDataGetSavePoint);
    LIB_FUNCTION("CWlBd2Ay1M4", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataGetDataBaseFilePath);
    LIB_FUNCTION("3luF0xq0DkQ", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataGetSaveDataRootPath);
    LIB_FUNCTION("DwAvlQGvf1o", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataGetSaveDataRootUsbPath);
    LIB_FUNCTION("uZqc4JpFdeY", "libSceSaveData", 1, "libSceSaveData", sceSaveDataDeleteUser);
    LIB_FUNCTION("pJrlpCgR8h4", "libSceSaveData", 1, "libSceSaveData", sceSaveDataDeleteAllUser);
    LIB_FUNCTION("ieP6jP138Qo", "libSceSaveData", 1, "libSceSaveData", sceSaveDataIsMounted);
    LIB_FUNCTION("OYmnApJ9q+U", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataGetUpdatedDataCount);
    LIB_FUNCTION("kLJQ3XioYiU", "libSceSaveData", 1, "libSceSaveData", sceSaveDataBindPsnAccount);
    LIB_FUNCTION("hHHCPRqA3+g", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataBindPsnAccountForSystemBackup);
    LIB_FUNCTION("kbIIP9aXK9A", "libSceSaveData", 1, "libSceSaveData", sceSaveDataCreateUploadData);
    LIB_FUNCTION("LMSQUTxmGVg", "libSceSaveData", 1, "libSceSaveData", sceSaveDataSyncCloudList);
    LIB_FUNCTION("6lZYZqQPfkY", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataGetCloudQuotaInfo);
    LIB_FUNCTION("COwz3WBj+5s", "libSceSaveData", 1, "libSceSaveData", sceSaveDataUpload);
    LIB_FUNCTION("h1nP9EYv3uc", "libSceSaveData", 1, "libSceSaveData", sceSaveDataDownload);
    LIB_FUNCTION("fU43mJUgKcM", "libSceSaveData", 1, "libSceSaveData", sceSaveDataDeleteCloudData);
    LIB_FUNCTION("-eczr5e4dsI", "libSceSaveData", 1, "libSceSaveData", sceSaveDataCheckCloudData);
    LIB_FUNCTION("ykwIZfVD08s", "libSceSaveData", 1, "libSceSaveData", sceSaveDataChangeDatabase);
    LIB_FUNCTION("G12foE0S77E", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataGetAutoUploadSetting);
    LIB_FUNCTION("0VFHv-Fa4w8", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataSetAutoUploadSetting);
    LIB_FUNCTION("A1ThglSGUwA", "libSceSaveData", 1, "libSceSaveData", sceSaveDataGetAllSize);
    LIB_FUNCTION("v7AAAMo0Lz4", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataSetupSaveDataMemory);
    LIB_FUNCTION("oQySEUfgXRA", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataSetupSaveDataMemory2);
    LIB_FUNCTION("7Bt5pBC-Aco", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataGetSaveDataMemory);
    LIB_FUNCTION("QwOO7vegnV8", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataGetSaveDataMemory2);
    LIB_FUNCTION("h3YURzXGSVQ", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataSetSaveDataMemory);
    LIB_FUNCTION("cduy9v4YmT4", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataSetSaveDataMemory2);
    LIB_FUNCTION("+orZm32HB1s", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataSupportedFakeBrokenStatus);
    LIB_FUNCTION("UMpxor4AlKQ", "libSceSaveData", 1, "libSceSaveData", sceSaveDataGetFormat);
    LIB_FUNCTION("AuTE0gFxZCI", "libSceSaveData", 1, "libSceSaveData", Func_02E4C4D201716422);
    LIB_FUNCTION("zMgXM79jRhw", "libSceSaveData", 1, "libSceSaveData", sceSaveDataShutdownStart);
    LIB_FUNCTION("wiT9jeC7xPw", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataSyncSaveDataMemory);
    LIB_FUNCTION("v3vg2+cooYw", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataSetSaveDataLibraryUser);
    LIB_FUNCTION("SN7rTPHS+Cg", "libSceSaveData", 1, "libSceSaveData", sceSaveDataGetSaveDataCount);
    LIB_FUNCTION("msCER7Iibm8", "libSceSaveData", 1, "libSceSaveData", sceSaveDataMountInternal);
    LIB_FUNCTION("ANmSWUiyyGQ", "libSceSaveData", 1, "libSceSaveData", sceSaveDataGetProgress);
    LIB_FUNCTION("Wz-4JZfeO9g", "libSceSaveData", 1, "libSceSaveData", sceSaveDataClearProgress);
    LIB_FUNCTION("PzDtD6eBXIM", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataGetBoundPsnAccountCount);
    LIB_FUNCTION("j8xKtiFj0SY", "libSceSaveData", 1, "libSceSaveData", sceSaveDataGetEventResult);
    LIB_FUNCTION("eBSSNIG6hMk", "libSceSaveData", 1, "libSceSaveData", sceSaveDataGetEventInfo);
    LIB_FUNCTION("52pL2GKkdjA", "libSceSaveData", 1, "libSceSaveData", sceSaveDataSetEventInfo);
    LIB_FUNCTION("aoZKKNjlq3Y", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataRestoreLoadSaveDataMemory);
    LIB_FUNCTION("KuXcrMAQIMQ", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataGetAppLaunchedUser);
    LIB_FUNCTION("bYCnxLexU7M", "libSceSaveData", 1, "libSceSaveData", sceSaveDataDebugCleanMount);
    LIB_FUNCTION("K9gXXlrVLNI", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataDebugCreateSaveDataRoot);
    LIB_FUNCTION("UGTldPVEdB4", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataDebugRemoveSaveDataRoot);
    LIB_FUNCTION("AYBQmnRplrg", "libSceSaveData", 1, "libSceSaveData", sceSaveDataDebugTarget);
    LIB_FUNCTION("hVDqYB8+jkk", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataDebugCompiledSdkVersion);
    LIB_FUNCTION("tu0SDPl+h88", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataGetClientThreadPriority);
    LIB_FUNCTION("5yHFvMwZX2o", "libSceSaveData", 1, "libSceSaveData", sceSaveDataDebugGetThreadId);
    LIB_FUNCTION("gW6G4HxBBXA", "libSceSaveData", 1, "libSceSaveData", sceSaveDataDebug);
    LIB_FUNCTION("4OPOZxfVkHA", "libSceSaveData", 1, "libSceSaveData", sceSaveDataCheckIpmiIfSize);
    LIB_FUNCTION("xJ5NFWC3m+k", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataDirNameSearchInternal);
    LIB_FUNCTION("voAQW45oKuo", "libSceSaveData", 1, "libSceSaveData", sceSaveDataIsDeletingUsbDb);
    LIB_FUNCTION("pc4guaUPVqA", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataGetMountedSaveDataCount);
    LIB_FUNCTION("PL20kjAXZZ4", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataGetAutoUploadRequestInfo);
    LIB_FUNCTION("1i0rfc+mfa8", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataCheckSaveDataBroken);
    LIB_FUNCTION("p6A1adyQi3E", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataCheckSaveDataVersion);
    LIB_FUNCTION("+bRDRotfj0Y", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataGetSaveDataRootDir);
    LIB_FUNCTION("S49B+I96kpk", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataCheckSaveDataVersionLatest);
    LIB_FUNCTION("itZ46iH14Vs", "libSceSaveData", 1, "libSceSaveData",
                 sceSaveDataGetAutoUploadConditions);
    LIB_FUNCTION("wJ6oiu+TqK8", "libSceSaveData", 1, "libSceSaveData", Func_C09EA88AEF93A8AF);
}

} // namespace Libraries::SaveData
