// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <magic_enum/magic_enum.hpp>

#include "common/logging/log.h"
#include "common/path_util.h"
#include "core/libraries/game_custom_data_dialog/game_custom_data_dialog.h"
#include "core/libraries/libs.h"
#include "core/user_settings.h"

namespace Libraries::GameCustomDataDialog {

using CommonDialog::Error;
using CommonDialog::Result;
using CommonDialog::Status;

namespace {

constexpr u32 kParamSize = sizeof(OrbisGameCustomDataDialogParamA); // 0x90 for both variants
constexpr size_t kMaxMessageBytes = 2048;
constexpr size_t kMaxNameBytes = 256;
constexpr size_t kMaxDetailBytes = 2048;
constexpr u32 kMaxThumbnailSize = 160 * 1024;
constexpr u32 kMaxDataSize = 1024 * 1024;
constexpr u32 kMaxMultiLangStrings = 30;
constexpr size_t kMaxPlatformNameBytes = 16;

Status g_status = Status::NONE;
Result g_result = Result::OK;
void* g_callback_arg = nullptr;
// Set by Close(): the dialog finishes with OK instead of the user cancelling it.
bool g_close_requested = false;

template <typename T>
bool AllZero(const T& bytes) {
    return std::all_of(std::begin(bytes), std::end(bytes), [](u8 b) { return b == 0; });
}

size_t BoundedLength(const char* s, size_t max) {
    return strnlen(s, max);
}

size_t Utf8Length(std::string_view s) {
    return static_cast<size_t>(
        std::count_if(s.begin(), s.end(), [](char c) { return (c & 0xC0) != 0x80; }));
}

bool IsLoggedIn(Libraries::UserService::OrbisUserServiceUserId user_id) {
    if (user_id == Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID) {
        return false;
    }
    for (const auto* user : UserManagement.GetLoggedInUsers()) {
        if (user != nullptr && user->user_id == user_id) {
            return true;
        }
    }
    return false;
}

std::string_view ErrorName(Error e) {
    switch (e) {
    case Error::ARG_NULL:
        return "ARG_NULL";
    case Error::PARAM_INVALID:
        return "PARAM_INVALID";
    default:
        return "error";
    }
}

template <typename... Args>
Error Reject(Error e, fmt::format_string<Args...> why, Args&&... args) {
    LOG_ERROR(Lib_GameCustomDataDialog, "Open rejected: {} ({} {:#x})",
              fmt::format(why, std::forward<Args>(args)...), ErrorName(e), static_cast<u32>(e));
    return e;
}

bool ValidRecipient(const Libraries::Np::OrbisNpAccountId& id) {
    return id != 0;
}
bool ValidRecipient(const Libraries::Np::OrbisNpOnlineId& id) {
    return strnlen(id.data, sizeof(id.data)) != 0;
}

template <typename Send>
Error ValidateCommonSend(const Send& send) {
    if (send.userMessage == nullptr) {
        return Reject(Error::ARG_NULL, "userMessage is NULL");
    }
    const size_t msg_bytes = BoundedLength(send.userMessage, kMaxMessageBytes + 1);
    if (msg_bytes > kMaxMessageBytes) {
        return Reject(Error::PARAM_INVALID, "userMessage longer than {} bytes", kMaxMessageBytes);
    }
    if (const size_t chars = Utf8Length({send.userMessage, msg_bytes});
        chars > ORBIS_GAME_CUSTOM_DATA_DIALOG_MAX_USER_MSG_SIZE) {
        return Reject(Error::PARAM_INVALID, "userMessage has {} characters, max {}", chars,
                      ORBIS_GAME_CUSTOM_DATA_DIALOG_MAX_USER_MSG_SIZE);
    }
    if (send.dataName == nullptr) {
        return Reject(Error::ARG_NULL, "dataName is NULL");
    }
    const size_t name_bytes = BoundedLength(send.dataName, kMaxNameBytes + 1);
    if (name_bytes == 0 || name_bytes > kMaxNameBytes) {
        return Reject(Error::PARAM_INVALID, "dataName is {} bytes, must be 1..{}", name_bytes,
                      kMaxNameBytes);
    }
    if (send.dataDetail == nullptr) {
        return Reject(Error::ARG_NULL, "dataDetail is NULL");
    }
    const size_t detail_bytes = BoundedLength(send.dataDetail, kMaxDetailBytes + 1);
    if (detail_bytes == 0 || detail_bytes > kMaxDetailBytes) {
        return Reject(Error::PARAM_INVALID, "dataDetail is {} bytes, must be 1..{}", detail_bytes,
                      kMaxDetailBytes);
    }
    if (send.thumbnail == nullptr) {
        return Reject(Error::ARG_NULL, "thumbnail is NULL");
    }
    if (send.thumbnailSize == 0 || send.thumbnailSize > kMaxThumbnailSize) {
        return Reject(Error::PARAM_INVALID, "thumbnailSize {} must be 1..{}", send.thumbnailSize,
                      kMaxThumbnailSize);
    }
    return Error::OK;
}

template <typename Send>
Error ValidatePlatformsAndRecipients(const Send& send) {
    const auto& address = send.addressParam;
    if (address.addressType != ORBIS_GAME_CUSTOM_DATA_DIALOG_ADDRESS_TYPE_USERDISABLE &&
        address.addressType != ORBIS_GAME_CUSTOM_DATA_DIALOG_ADDRESS_TYPE_USERENABLE) {
        return Reject(Error::PARAM_INVALID, "unknown addressType {}", address.addressType);
    }
    const auto& platforms = send.availablePlatform;
    if (platforms.count == 0) {
        return Reject(Error::PARAM_INVALID, "availablePlatform.count is 0");
    }
    for (u32 i = 0; i < platforms.count; i++) {
        const char* name = platforms.platformName[i];
        if (name == nullptr) {
            return Reject(Error::ARG_NULL, "availablePlatform.platformName[{}] is NULL", i);
        }
        if (BoundedLength(name, kMaxPlatformNameBytes) == 0) {
            return Reject(Error::PARAM_INVALID, "availablePlatform.platformName[{}] is empty", i);
        }
    }
    if (address.addressType == ORBIS_GAME_CUSTOM_DATA_DIALOG_ADDRESS_TYPE_USERENABLE) {
        const u32 max = address.addressInfo.UserSelectEnableAddress.userMaxCount;
        if (max == 0 || max > ORBIS_GAME_CUSTOM_DATA_DIALOG_ADDRESS_USER_LIST_MAX_SIZE) {
            return Reject(Error::PARAM_INVALID, "userMaxCount {} must be 1..{}", max,
                          ORBIS_GAME_CUSTOM_DATA_DIALOG_ADDRESS_USER_LIST_MAX_SIZE);
        }
    } else {
        const auto& fixed = address.addressInfo.UserSelectDisableAddress;
        u32 count;
        const auto* ids = [&] {
            if constexpr (requires { fixed.accountIds; }) {
                count = fixed.accountIdsCount;
                return fixed.accountIds;
            } else {
                count = fixed.onlineIdsCount;
                return fixed.onlineIds;
            }
        }();
        if (count == 0 || count > ORBIS_GAME_CUSTOM_DATA_DIALOG_ADDRESS_USER_LIST_MAX_SIZE) {
            return Reject(Error::PARAM_INVALID, "recipient count {} must be 1..{}", count,
                          ORBIS_GAME_CUSTOM_DATA_DIALOG_ADDRESS_USER_LIST_MAX_SIZE);
        }
        if (ids == nullptr) {
            return Reject(Error::ARG_NULL, "recipient list is NULL");
        }
        for (u32 i = 0; i < count; i++) {
            if (!ValidRecipient(ids[i])) {
                return Reject(Error::PARAM_INVALID, "recipient[{}] is empty", i);
            }
        }
    }
    if (send.dataNameMultiLangNum > kMaxMultiLangStrings ||
        send.dataDetailMultiLangNum > kMaxMultiLangStrings) {
        return Reject(Error::PARAM_INVALID,
                      "too many translations (dataName {}, dataDetail {}), max {}",
                      send.dataNameMultiLangNum, send.dataDetailMultiLangNum, kMaxMultiLangStrings);
    }
    return Error::OK;
}

template <typename Param>
Error ValidateParam(const Param* param) {
    if (param == nullptr) {
        return Reject(Error::ARG_NULL, "param is NULL");
    }
    if (param->size != kParamSize) {
        return Reject(Error::PARAM_INVALID, "param->size is {:#x}, expected {:#x}", param->size,
                      kParamSize);
    }
    if (!AllZero(param->reserved)) {
        return Reject(Error::PARAM_INVALID, "param->reserved is not zeroed");
    }
    if (param->dataParam == nullptr) {
        return Reject(Error::ARG_NULL, "param->dataParam is NULL");
    }
    if (!IsLoggedIn(param->userId)) {
        return Reject(Error::PARAM_INVALID, "user {} is not signed in", param->userId);
    }
    const auto& data = *param->dataParam;
    switch (param->mode) {
    case ORBIS_GAME_CUSTOM_DATA_DIALOG_MODE_RECV:
        return AllZero(data.RecvInfo.reserved)
                   ? Error::OK
                   : Reject(Error::PARAM_INVALID, "RecvInfo.reserved is not zeroed");
    case ORBIS_GAME_CUSTOM_DATA_DIALOG_MODE_SEND: {
        const auto& send = data.SendInfo;
        if (const Error e = ValidateCommonSend(send); e != Error::OK) {
            return e;
        }
        if (send.data == nullptr) {
            return Reject(Error::ARG_NULL, "data is NULL");
        }
        if (send.dataSize == 0 || send.dataSize > kMaxDataSize) {
            return Reject(Error::PARAM_INVALID, "dataSize {} must be 1..{}", send.dataSize,
                          kMaxDataSize);
        }
        return ValidatePlatformsAndRecipients(send);
    }
    case ORBIS_GAME_CUSTOM_DATA_DIALOG_MODE_SEND_URL: {
        const auto& send = data.SendUrlInfo;
        if (const Error e = ValidateCommonSend(send); e != Error::OK) {
            return e;
        }
        if (send.dataUrl == nullptr) {
            return Reject(Error::ARG_NULL, "dataUrl is NULL");
        }
        const size_t url_bytes =
            BoundedLength(send.dataUrl, ORBIS_GAME_CUSTOM_DATA_DIALOG_MAX_URL_LENGTH + 1);
        if (url_bytes == 0 || url_bytes > ORBIS_GAME_CUSTOM_DATA_DIALOG_MAX_URL_LENGTH) {
            return Reject(Error::PARAM_INVALID, "dataUrl is {} bytes, must be 1..{}", url_bytes,
                          ORBIS_GAME_CUSTOM_DATA_DIALOG_MAX_URL_LENGTH);
        }
        return ValidatePlatformsAndRecipients(send);
    }
    default:
        return Reject(Error::PARAM_INVALID, "unknown mode {}", param->mode);
    }
}

std::string Quote(const char* s, size_t max = kMaxDetailBytes) {
    if (s == nullptr) {
        return "(null)";
    }
    return fmt::format("{:?}", std::string_view{s, BoundedLength(s, max)});
}

std::string HexPreview(const void* bytes, u32 size, u32 max = 32) {
    const auto* p = static_cast<const u8*>(bytes);
    std::string out;
    for (u32 i = 0; i < std::min(size, max); i++) {
        fmt::format_to(std::back_inserter(out), "{}{:02x}", i ? " " : "", p[i]);
    }
    if (size > max) {
        out += " ...";
    }
    return out;
}

u64 Fnv1a64(const void* bytes, u32 size) {
    u64 h = 0xcbf29ce484222325ull;
    for (u32 i = 0; i < size; i++) {
        h = (h ^ static_cast<const u8*>(bytes)[i]) * 0x100000001b3ull;
    }
    return h;
}

std::string DescribeImage(const void* bytes, u32 size, const char** ext) {
    const auto* p = static_cast<const u8*>(bytes);
    static constexpr u8 kPng[] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
    if (size >= 24 && std::memcmp(p, kPng, sizeof(kPng)) == 0) {
        *ext = "png";
        const auto be32 = [p](int o) {
            return u32(p[o]) << 24 | u32(p[o + 1]) << 16 | u32(p[o + 2]) << 8 | u32(p[o + 3]);
        };
        return fmt::format("PNG {}x{}", be32(16), be32(20));
    }
    if (size >= 3 && p[0] == 0xff && p[1] == 0xd8 && p[2] == 0xff) {
        *ext = "jpg";
        return "JPEG";
    }
    *ext = "bin";
    return "unknown format";
}

std::string_view ModeName(s32 mode) {
    switch (mode) {
    case ORBIS_GAME_CUSTOM_DATA_DIALOG_MODE_SEND:
        return "SEND (game data)";
    case ORBIS_GAME_CUSTOM_DATA_DIALOG_MODE_SEND_URL:
        return "SEND_URL";
    case ORBIS_GAME_CUSTOM_DATA_DIALOG_MODE_RECV:
        return "RECV";
    default:
        return "INVALID";
    }
}

void LogTranslations(std::string_view field, const OrbisGameCustomDataMultiLanguageString* list,
                     u32 count) {
    if (count == 0) {
        return;
    }
    if (list == nullptr) {
        LOG_INFO(Lib_GameCustomDataDialog, "  {}: {} translations but the list is NULL", field,
                 count);
        return;
    }
    for (u32 i = 0; i < count; i++) {
        LOG_INFO(Lib_GameCustomDataDialog, "  {}[{}]: {} = {}", field, i,
                 Quote(list[i].language, 16), Quote(list[i].str));
    }
}

void WriteFile(const std::filesystem::path& path, const void* bytes, u32 size) {
    std::ofstream file(path, std::ios::binary);
    file.write(static_cast<const char*>(bytes), size);
    if (!file) {
        LOG_WARNING(Lib_GameCustomDataDialog, "  could not write {}", path.string());
    }
}

void DumpPayload(const void* thumbnail, u32 thumbnail_size, const char* thumbnail_ext,
                 const void* data, u32 data_size) {
    static u32 sequence = 0;
    const std::time_t now = std::time(nullptr);
    char stamp[32];
    std::strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", std::localtime(&now));
    const auto dir = Common::FS::GetUserPath(Common::FS::PathType::LogDir) / "game_custom_data" /
                     fmt::format("{}_{}", stamp, sequence++);
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) {
        LOG_WARNING(Lib_GameCustomDataDialog, "  could not create {}: {}", dir.string(),
                    ec.message());
        return;
    }
    WriteFile(dir / fmt::format("thumbnail.{}", thumbnail_ext), thumbnail, thumbnail_size);
    if (data != nullptr) {
        WriteFile(dir / "data.bin", data, data_size);
    }
    LOG_INFO(Lib_GameCustomDataDialog, "  payload saved to {}", dir.string());
}

template <typename Send>
void LogSendRequest(const Send& send) {
    LOG_INFO(Lib_GameCustomDataDialog, "  userMessage: {}", Quote(send.userMessage));
    LOG_INFO(Lib_GameCustomDataDialog, "  dataName: {}", Quote(send.dataName));
    LogTranslations("dataNameMultiLang", send.dataNameMultiLang, send.dataNameMultiLangNum);
    LOG_INFO(Lib_GameCustomDataDialog, "  dataDetail: {}", Quote(send.dataDetail));
    LogTranslations("dataDetailMultiLang", send.dataDetailMultiLang, send.dataDetailMultiLangNum);

    const char* thumbnail_ext;
    const std::string image = DescribeImage(send.thumbnail, send.thumbnailSize, &thumbnail_ext);
    LOG_INFO(Lib_GameCustomDataDialog, "  thumbnail: {} bytes, {}, fnv1a64 {:016x}",
             send.thumbnailSize, image, Fnv1a64(send.thumbnail, send.thumbnailSize));

    const void* data = nullptr;
    u32 data_size = 0;
    if constexpr (requires { send.dataUrl; }) {
        LOG_INFO(Lib_GameCustomDataDialog, "  dataUrl: {} (withItemId: {})",
                 Quote(send.dataUrl, ORBIS_GAME_CUSTOM_DATA_DIALOG_MAX_URL_LENGTH),
                 send.withItemId);
    } else {
        data = send.data;
        data_size = send.dataSize;
        LOG_INFO(Lib_GameCustomDataDialog, "  data: {} bytes, fnv1a64 {:016x}, starts {}",
                 data_size, Fnv1a64(data, data_size), HexPreview(data, data_size));
    }
    LOG_INFO(Lib_GameCustomDataDialog, "  expireMinutes: {}{}", send.expireMinutes,
             send.expireMinutes == 0 ? " (system default)" : "");

    std::string platforms;
    for (u32 i = 0; i < send.availablePlatform.count; i++) {
        fmt::format_to(std::back_inserter(platforms), "{}{}", i ? ", " : "",
                       std::string_view{send.availablePlatform.platformName[i],
                                        BoundedLength(send.availablePlatform.platformName[i],
                                                      kMaxPlatformNameBytes)});
    }
    LOG_INFO(Lib_GameCustomDataDialog, "  availablePlatform: {}", platforms);

    const auto& address = send.addressParam;
    if (address.addressType == ORBIS_GAME_CUSTOM_DATA_DIALOG_ADDRESS_TYPE_USERENABLE) {
        LOG_INFO(Lib_GameCustomDataDialog, "  recipients: chosen by the player, up to {}",
                 address.addressInfo.UserSelectEnableAddress.userMaxCount);
    } else {
        const auto& fixed = address.addressInfo.UserSelectDisableAddress;
        std::string list;
        if constexpr (requires { fixed.accountIds; }) {
            for (u32 i = 0; i < fixed.accountIdsCount; i++) {
                fmt::format_to(std::back_inserter(list), "{}{}", i ? ", " : "",
                               fixed.accountIds[i]);
            }
            LOG_INFO(Lib_GameCustomDataDialog,
                     "  recipients: fixed by the game, {} account IDs: {}", fixed.accountIdsCount,
                     list);
        } else {
            for (u32 i = 0; i < fixed.onlineIdsCount; i++) {
                const auto& id = fixed.onlineIds[i].data;
                fmt::format_to(std::back_inserter(list), "{}{}", i ? ", " : "",
                               std::string_view{id, strnlen(id, sizeof(id))});
            }
            LOG_INFO(Lib_GameCustomDataDialog, "  recipients: fixed by the game, {} online IDs: {}",
                     fixed.onlineIdsCount, list);
        }
    }

    DumpPayload(send.thumbnail, send.thumbnailSize, thumbnail_ext, data, data_size);
}

template <typename Param>
void LogRequest(const Param* param, const char* entry) {
    LOG_INFO(Lib_GameCustomDataDialog, "{}: mode {}, user {}, callbackArg {}", entry,
             ModeName(param->mode), param->userId, fmt::ptr(param->callbackArg));
    const auto& data = *param->dataParam;
    switch (param->mode) {
    case ORBIS_GAME_CUSTOM_DATA_DIALOG_MODE_SEND:
        LogSendRequest(data.SendInfo);
        break;
    case ORBIS_GAME_CUSTOM_DATA_DIALOG_MODE_SEND_URL:
        LogSendRequest(data.SendUrlInfo);
        break;
    default:
        break;
    }
}

template <typename Param>
Error OpenImpl(const Param* param, const char* entry) {
    if (g_status == Status::NONE) {
        LOG_ERROR(Lib_GameCustomDataDialog, "{}: called before Initialize", entry);
        return Error::NOT_INITIALIZED;
    }
    if (const Error e = ValidateParam(param); e != Error::OK) {
        return e;
    }
    if (g_status != Status::INITIALIZED && g_status != Status::FINISHED) {
        LOG_ERROR(Lib_GameCustomDataDialog, "{}: dialog is already {}", entry,
                  magic_enum::enum_name(g_status));
        return Error::INVALID_STATE;
    }

    LogRequest(param, entry);
    LOG_WARNING(Lib_GameCustomDataDialog,
                "{}: {} is not supported yet, closing as cancelled (nothing {})", entry,
                ModeName(param->mode),
                param->mode == ORBIS_GAME_CUSTOM_DATA_DIALOG_MODE_RECV ? "received" : "sent");

    g_callback_arg = param->callbackArg;
    g_result = Result::USER_CANCELED;
    g_close_requested = false;
    g_status = Status::RUNNING;
    return Error::OK;
}

template <typename ResultT>
s32 GetResultImpl(ResultT* result) {
    if (result == nullptr) {
        return static_cast<s32>(Error::ARG_NULL);
    }
    if (!AllZero(result->reserved)) {
        return static_cast<s32>(Error::PARAM_INVALID);
    }
    if (g_status == Status::NONE) {
        return static_cast<s32>(Error::NOT_INITIALIZED);
    }
    if (g_status != Status::FINISHED) {
        return static_cast<s32>(Error::NOT_FINISHED);
    }
    LOG_INFO(Lib_GameCustomDataDialog, "result: {}, 0 recipients", magic_enum::enum_name(g_result));
    result->callbackArg = g_callback_arg;
    result->errorCode = 0;
    result->result = g_result;
    if constexpr (requires { result->sentUsers; }) {
        if (result->sentUsers != nullptr) {
            std::memset(result->sentUsers, 0, sizeof(*result->sentUsers));
        }
    } else {
        if (result->sentOnlineIds != nullptr) {
            std::memset(result->sentOnlineIds, 0, sizeof(*result->sentOnlineIds));
        }
    }
    return static_cast<s32>(g_result);
}

} // namespace

Error PS4_SYSV_ABI sceGameCustomDataDialogInitialize() {
    if (!CommonDialog::g_isInitialized) {
        LOG_ERROR(Lib_GameCustomDataDialog, "Initialize: common dialog system not initialized");
        return Error::NOT_SYSTEM_INITIALIZED;
    }
    if (g_status != Status::NONE) {
        LOG_ERROR(Lib_GameCustomDataDialog, "Initialize: already initialized");
        return Error::ALREADY_INITIALIZED;
    }
    if (CommonDialog::g_isUsed) {
        LOG_ERROR(Lib_GameCustomDataDialog, "Initialize: another common dialog is in use");
        return Error::BUSY;
    }
    g_status = Status::INITIALIZED;
    CommonDialog::g_isUsed = true;
    LOG_INFO(Lib_GameCustomDataDialog, "initialized");
    return Error::OK;
}

Error PS4_SYSV_ABI sceGameCustomDataDialogTerminate() {
    if (g_status == Status::NONE) {
        return Error::NOT_INITIALIZED;
    }
    LOG_INFO(Lib_GameCustomDataDialog, "terminated (was {})", magic_enum::enum_name(g_status));
    g_status = Status::NONE;
    CommonDialog::g_isUsed = false;
    return Error::OK;
}

Error PS4_SYSV_ABI sceGameCustomDataDialogOpen(const OrbisGameCustomDataDialogParam* param) {
    return OpenImpl(param, "Open");
}

Error PS4_SYSV_ABI sceGameCustomDataDialogOpenA(const OrbisGameCustomDataDialogParamA* param) {
    return OpenImpl(param, "OpenA");
}

Error PS4_SYSV_ABI sceGameCustomDataDialogClose() {
    if (g_status == Status::NONE) {
        return Error::NOT_INITIALIZED;
    }
    if (g_status != Status::RUNNING) {
        return Error::NOT_RUNNING;
    }
    LOG_INFO(Lib_GameCustomDataDialog, "closed by the game");
    g_close_requested = true;
    return Error::OK;
}

Status PS4_SYSV_ABI sceGameCustomDataDialogUpdateStatus() {
    if (g_status == Status::RUNNING) {
        g_result = g_close_requested ? Result::OK : Result::USER_CANCELED;
        g_status = Status::FINISHED;
        LOG_INFO(Lib_GameCustomDataDialog, "finished: {}", magic_enum::enum_name(g_result));
    }
    return g_status;
}

Status PS4_SYSV_ABI sceGameCustomDataDialogGetStatus() {
    return g_status;
}

s32 PS4_SYSV_ABI sceGameCustomDataDialogGetResult(OrbisGameCustomDataDialogResult* result) {
    return GetResultImpl(result);
}

s32 PS4_SYSV_ABI sceGameCustomDataDialogGetResultA(OrbisGameCustomDataDialogResultA* result) {
    return GetResultImpl(result);
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("xtbb-2f703A", "libSceGameCustomDataDialog", 1, "libSceGameCustomDataDialog",
                 sceGameCustomDataDialogInitialize);
    LIB_FUNCTION("HwEtHFCpU5U", "libSceGameCustomDataDialog", 1, "libSceGameCustomDataDialog",
                 sceGameCustomDataDialogTerminate);
    LIB_FUNCTION("5TvttyRuU84", "libSceGameCustomDataDialog", 1, "libSceGameCustomDataDialog",
                 sceGameCustomDataDialogOpen);
    LIB_FUNCTION("QHFJB2F0p3k", "libSceGameCustomDataDialog", 1, "libSceGameCustomDataDialog",
                 sceGameCustomDataDialogOpenA);
    LIB_FUNCTION("3vyRmRU3CL8", "libSceGameCustomDataDialog", 1, "libSceGameCustomDataDialog",
                 sceGameCustomDataDialogClose);
    LIB_FUNCTION("PkdLsRA4ON0", "libSceGameCustomDataDialog", 1, "libSceGameCustomDataDialog",
                 sceGameCustomDataDialogUpdateStatus);
    LIB_FUNCTION("tcm8nRXZwNU", "libSceGameCustomDataDialog", 1, "libSceGameCustomDataDialog",
                 sceGameCustomDataDialogGetStatus);
    LIB_FUNCTION("sJptZwvs1is", "libSceGameCustomDataDialog", 1, "libSceGameCustomDataDialog",
                 sceGameCustomDataDialogGetResult);
    LIB_FUNCTION("TWi-wZVp5Pc", "libSceGameCustomDataDialog", 1, "libSceGameCustomDataDialog",
                 sceGameCustomDataDialogGetResultA);
    // Older games link the non-A entry points through the compat library.
    LIB_FUNCTION("5TvttyRuU84", "libSceGameCustomDataDialogCompat", 1, "libSceGameCustomDataDialog",
                 sceGameCustomDataDialogOpen);
    LIB_FUNCTION("sJptZwvs1is", "libSceGameCustomDataDialogCompat", 1, "libSceGameCustomDataDialog",
                 sceGameCustomDataDialogGetResult);
}

} // namespace Libraries::GameCustomDataDialog
