// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <unordered_map>
#include <pugixml.hpp>

#include "common/logging/log.h"
#include "common/path_util.h"
#include "common/slot_vector.h"
#include "core/libraries/libs.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_trophy.h"
#include "core/libraries/np/np_trophy_error.h"
#include "core/libraries/np/trophy_ui.h"
#include "core/memory.h"

namespace Libraries::Np::NpTrophy {

std::string game_serial;

static constexpr auto MaxTrophyHandles = 4u;
static constexpr auto MaxTrophyContexts = 8u;

using ContextKey = std::pair<u32, u32>; // <user_id, service label>
struct ContextKeyHash {
    size_t operator()(const ContextKey& key) const {
        return key.first + (u64(key.second) << 32u);
    }
};

struct TrophyContext {
    u32 context_id;
};
static Common::SlotVector<OrbisNpTrophyHandle> trophy_handles{};
static Common::SlotVector<ContextKey> trophy_contexts{};
static std::unordered_map<ContextKey, TrophyContext, ContextKeyHash> contexts_internal{};

void ORBIS_NP_TROPHY_FLAG_ZERO(OrbisNpTrophyFlagArray* p) {
    for (int i = 0; i < ORBIS_NP_TROPHY_NUM_MAX; i++) {
        uint32_t array_index = i / 32;
        uint32_t bit_position = i % 32;

        p->flag_bits[array_index] &= ~(1U << bit_position);
    }
}

void ORBIS_NP_TROPHY_FLAG_SET(int32_t trophyId, OrbisNpTrophyFlagArray* p) {
    uint32_t array_index = trophyId / 32;
    uint32_t bit_position = trophyId % 32;

    p->flag_bits[array_index] |= (1U << bit_position);
}

void ORBIS_NP_TROPHY_FLAG_SET_ALL(OrbisNpTrophyFlagArray* p) {
    for (int i = 0; i < ORBIS_NP_TROPHY_NUM_MAX; i++) {
        uint32_t array_index = i / 32;
        uint32_t bit_position = i % 32;

        p->flag_bits[array_index] |= (1U << bit_position);
    }
}

void ORBIS_NP_TROPHY_FLAG_CLR(int32_t trophyId, OrbisNpTrophyFlagArray* p) {
    uint32_t array_index = trophyId / 32;
    uint32_t bit_position = trophyId % 32;

    p->flag_bits[array_index] &= ~(1U << bit_position);
}

bool ORBIS_NP_TROPHY_FLAG_ISSET(int32_t trophyId, OrbisNpTrophyFlagArray* p) {
    uint32_t array_index = trophyId / 32;
    uint32_t bit_position = trophyId % 32;

    return (p->flag_bits[array_index] & (1U << bit_position)) ? 1 : 0;
}

OrbisNpTrophyGrade GetTrophyGradeFromChar(char trophyType) {
    switch (trophyType) {
    default:
        return ORBIS_NP_TROPHY_GRADE_UNKNOWN;
        break;
    case 'B':
        return ORBIS_NP_TROPHY_GRADE_BRONZE;
        break;
    case 'S':
        return ORBIS_NP_TROPHY_GRADE_SILVER;
        break;
    case 'G':
        return ORBIS_NP_TROPHY_GRADE_GOLD;
        break;
    case 'P':
        return ORBIS_NP_TROPHY_GRADE_PLATINUM;
        break;
    }
}

int PS4_SYSV_ABI sceNpTrophyAbortHandle(OrbisNpTrophyHandle handle) {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophyCaptureScreenshot() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophyConfigGetTrophyDetails() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophyConfigGetTrophyFlagArray() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophyConfigGetTrophyGroupArray() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophyConfigGetTrophyGroupDetails() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophyConfigGetTrophySetInfo() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophyConfigGetTrophySetInfoInGroup() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophyConfigGetTrophySetVersion() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophyConfigGetTrophyTitleDetails() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophyConfigHasGroupFeature() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTrophyCreateContext(OrbisNpTrophyContext* context, s32 user_id,
                                          uint32_t service_label, u64 options) {
    ASSERT(options == 0ull);
    if (!context) {
        return ORBIS_NP_TROPHY_ERROR_INVALID_ARGUMENT;
    }

    if (trophy_contexts.size() >= MaxTrophyContexts) {
        return ORBIS_NP_TROPHY_ERROR_CONTEXT_EXCEEDS_MAX;
    }

    const auto& key = ContextKey{user_id, service_label};
    if (contexts_internal.contains(key)) {
        return ORBIS_NP_TROPHY_ERROR_CONTEXT_ALREADY_EXISTS;
    }

    const auto ctx_id = trophy_contexts.insert(user_id, service_label);

    *context = ctx_id.index + 1;
    contexts_internal[key].context_id = *context;
    LOG_INFO(Lib_NpTrophy, "New context = {}, user_id = {} service label = {}", *context, user_id,
             service_label);

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTrophyCreateHandle(OrbisNpTrophyHandle* handle) {
    if (!handle) {
        return ORBIS_NP_TROPHY_ERROR_INVALID_ARGUMENT;
    }

    if (trophy_handles.size() >= MaxTrophyHandles) {
        return ORBIS_NP_TROPHY_ERROR_HANDLE_EXCEEDS_MAX;
    }

    const auto handle_id = trophy_handles.insert();

    *handle = handle_id.index + 1;
    LOG_INFO(Lib_NpTrophy, "New handle = {}", *handle);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophyDestroyContext(OrbisNpTrophyContext context) {
    LOG_INFO(Lib_NpTrophy, "Destroyed Context {}", context);

    if (context == ORBIS_NP_TROPHY_INVALID_CONTEXT) {
        return ORBIS_NP_TROPHY_ERROR_INVALID_CONTEXT;
    }

    Common::SlotId contextId;
    contextId.index = context - 1;

    if (contextId.index >= trophy_contexts.size()) {
        return ORBIS_NP_TROPHY_ERROR_INVALID_CONTEXT;
    }

    ContextKey contextkey = trophy_contexts[contextId];
    trophy_contexts.erase(contextId);
    contexts_internal.erase(contextkey);

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTrophyDestroyHandle(OrbisNpTrophyHandle handle) {
    if (handle == ORBIS_NP_TROPHY_INVALID_HANDLE)
        return ORBIS_NP_TROPHY_ERROR_INVALID_HANDLE;

    s32 handle_index = handle - 1;
    if (handle_index >= trophy_handles.size()) {
        LOG_ERROR(Lib_NpTrophy, "Invalid handle {}", handle);
        return ORBIS_NP_TROPHY_ERROR_INVALID_HANDLE;
    }

    if (!trophy_handles.is_allocated({static_cast<u32>(handle_index)})) {
        return ORBIS_NP_TROPHY_ERROR_INVALID_HANDLE;
    }

    trophy_handles.erase({static_cast<u32>(handle_index)});
    LOG_INFO(Lib_NpTrophy, "Handle {} destroyed", handle);
    return ORBIS_OK;
}

u64 ReadFile(Common::FS::IOFile& file, void* buf, u64 nbytes) {
    const auto* memory = Core::Memory::Instance();
    // Invalidate up to the actual number of bytes that could be read.
    const auto remaining = file.GetSize() - file.Tell();
    memory->InvalidateMemory(reinterpret_cast<VAddr>(buf), std::min<u64>(nbytes, remaining));

    return file.ReadRaw<u8>(buf, nbytes);
}

int PS4_SYSV_ABI sceNpTrophyGetGameIcon(OrbisNpTrophyContext context, OrbisNpTrophyHandle handle,
                                        void* buffer, u64* size) {
    ASSERT(size != nullptr);
    const auto trophy_dir =
        Common::FS::GetUserPath(Common::FS::PathType::MetaDataDir) / game_serial / "TrophyFiles";
    auto icon_file = trophy_dir / "trophy00" / "Icons" / "ICON0.PNG";

    Common::FS::IOFile icon(icon_file, Common::FS::FileAccessMode::Read);
    if (!icon.IsOpen()) {
        LOG_ERROR(Lib_NpTrophy, "Failed to open trophy icon file: {}", icon_file.string());
        return ORBIS_NP_TROPHY_ERROR_ICON_FILE_NOT_FOUND;
    }
    u64 icon_size = icon.GetSize();

    if (buffer != nullptr) {
        ReadFile(icon, buffer, *size);
    } else {
        *size = icon_size;
    }
    return ORBIS_OK;
}

struct GameTrophyInfo {
    uint32_t num_groups;
    uint32_t num_trophies;
    uint32_t num_trophies_by_rarity[5];
    uint32_t unlocked_trophies;
    uint32_t unlocked_trophies_by_rarity[5];
};

int PS4_SYSV_ABI sceNpTrophyGetGameInfo(OrbisNpTrophyContext context, OrbisNpTrophyHandle handle,
                                        OrbisNpTrophyGameDetails* details,
                                        OrbisNpTrophyGameData* data) {
    LOG_INFO(Lib_NpTrophy, "Getting Game Trophy");

    if (context == ORBIS_NP_TROPHY_INVALID_CONTEXT)
        return ORBIS_NP_TROPHY_ERROR_INVALID_CONTEXT;

    if (handle == ORBIS_NP_TROPHY_INVALID_HANDLE)
        return ORBIS_NP_TROPHY_ERROR_INVALID_HANDLE;

    if (details == nullptr || data == nullptr)
        return ORBIS_NP_TROPHY_ERROR_INVALID_ARGUMENT;

    if (details->size != 0x4A0 || data->size != 0x20)
        return ORBIS_NP_TROPHY_ERROR_INVALID_ARGUMENT;

    const auto trophy_dir =
        Common::FS::GetUserPath(Common::FS::PathType::MetaDataDir) / game_serial / "TrophyFiles";
    auto trophy_file = trophy_dir / "trophy00" / "Xml" / "TROP.XML";

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(trophy_file.native().c_str());

    if (!result) {
        LOG_ERROR(Lib_NpTrophy, "Failed to parse trophy xml : {}", result.description());
        return ORBIS_OK;
    }

    GameTrophyInfo game_info{};

    auto trophyconf = doc.child("trophyconf");
    for (const pugi::xml_node& node : trophyconf.children()) {
        std::string_view node_name = node.name();

        if (node_name == "title-name") {
            strncpy(details->title, node.text().as_string(), ORBIS_NP_TROPHY_GAME_TITLE_MAX_SIZE);
        }

        if (node_name == "title-detail") {
            strncpy(details->description, node.text().as_string(),
                    ORBIS_NP_TROPHY_GAME_DESCR_MAX_SIZE);
        }

        if (node_name == "group")
            game_info.num_groups++;

        if (node_name == "trophy") {
            bool current_trophy_unlockstate = node.attribute("unlockstate").as_bool();
            std::string_view current_trophy_grade = node.attribute("ttype").value();

            if (current_trophy_grade.empty()) {
                continue;
            }

            game_info.num_trophies++;
            int trophy_grade = GetTrophyGradeFromChar(current_trophy_grade.at(0));
            game_info.num_trophies_by_rarity[trophy_grade]++;

            if (current_trophy_unlockstate) {
                game_info.unlocked_trophies++;
                game_info.unlocked_trophies_by_rarity[trophy_grade]++;
            }
        }
    }

    details->num_groups = game_info.num_groups;
    details->num_trophies = game_info.num_trophies;
    details->num_platinum = game_info.num_trophies_by_rarity[ORBIS_NP_TROPHY_GRADE_PLATINUM];
    details->num_gold = game_info.num_trophies_by_rarity[ORBIS_NP_TROPHY_GRADE_GOLD];
    details->num_silver = game_info.num_trophies_by_rarity[ORBIS_NP_TROPHY_GRADE_SILVER];
    details->num_bronze = game_info.num_trophies_by_rarity[ORBIS_NP_TROPHY_GRADE_BRONZE];
    data->unlocked_trophies = game_info.unlocked_trophies;
    data->unlocked_platinum = game_info.unlocked_trophies_by_rarity[ORBIS_NP_TROPHY_GRADE_PLATINUM];
    data->unlocked_gold = game_info.unlocked_trophies_by_rarity[ORBIS_NP_TROPHY_GRADE_GOLD];
    data->unlocked_silver = game_info.unlocked_trophies_by_rarity[ORBIS_NP_TROPHY_GRADE_SILVER];
    data->unlocked_bronze = game_info.unlocked_trophies_by_rarity[ORBIS_NP_TROPHY_GRADE_BRONZE];

    // maybe this should be 1 instead of 100?
    data->progress_percentage = 100;

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophyGetGroupIcon(OrbisNpTrophyContext context, OrbisNpTrophyHandle handle,
                                         OrbisNpTrophyGroupId groupId, void* buffer, u64* size) {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

struct GroupTrophyInfo {
    uint32_t num_trophies;
    uint32_t num_trophies_by_rarity[5];
    uint32_t unlocked_trophies;
    uint32_t unlocked_trophies_by_rarity[5];
};

int PS4_SYSV_ABI sceNpTrophyGetGroupInfo(OrbisNpTrophyContext context, OrbisNpTrophyHandle handle,
                                         OrbisNpTrophyGroupId groupId,
                                         OrbisNpTrophyGroupDetails* details,
                                         OrbisNpTrophyGroupData* data) {
    LOG_INFO(Lib_NpTrophy, "Getting Trophy Group Info for id {}", groupId);

    if (context == ORBIS_NP_TROPHY_INVALID_CONTEXT)
        return ORBIS_NP_TROPHY_ERROR_INVALID_CONTEXT;

    if (handle == ORBIS_NP_TROPHY_INVALID_HANDLE)
        return ORBIS_NP_TROPHY_ERROR_INVALID_HANDLE;

    if (details == nullptr || data == nullptr)
        return ORBIS_NP_TROPHY_ERROR_INVALID_ARGUMENT;

    if (details->size != 0x4A0 || data->size != 0x28)
        return ORBIS_NP_TROPHY_ERROR_INVALID_ARGUMENT;

    const auto trophy_dir =
        Common::FS::GetUserPath(Common::FS::PathType::MetaDataDir) / game_serial / "TrophyFiles";
    auto trophy_file = trophy_dir / "trophy00" / "Xml" / "TROP.XML";

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(trophy_file.native().c_str());

    if (!result) {
        LOG_ERROR(Lib_NpTrophy, "Failed to open trophy xml : {}", result.description());
        return ORBIS_OK;
    }

    GroupTrophyInfo group_info{};

    auto trophyconf = doc.child("trophyconf");
    for (const pugi::xml_node& node : trophyconf.children()) {
        std::string_view node_name = node.name();

        if (node_name == "group") {
            int current_group_id = node.attribute("id").as_int(ORBIS_NP_TROPHY_INVALID_GROUP_ID);
            if (current_group_id != ORBIS_NP_TROPHY_INVALID_GROUP_ID) {
                if (current_group_id == groupId) {
                    std::string_view current_group_name = node.child("name").text().as_string();
                    std::string_view current_group_description =
                        node.child("detail").text().as_string();

                    strncpy(details->title, current_group_name.data(),
                            ORBIS_NP_TROPHY_GROUP_TITLE_MAX_SIZE);
                    strncpy(details->description, current_group_description.data(),
                            ORBIS_NP_TROPHY_GAME_DESCR_MAX_SIZE);
                }
            }
        }

        details->group_id = groupId;
        data->group_id = groupId;

        if (node_name == "trophy") {
            bool current_trophy_unlockstate = node.attribute("unlockstate").as_bool();
            std::string_view current_trophy_grade = node.attribute("ttype").value();
            int current_trophy_group_id = node.attribute("gid").as_int(-1);

            if (current_trophy_grade.empty()) {
                continue;
            }

            if (current_trophy_group_id == groupId) {
                group_info.num_trophies++;
                int trophyGrade = GetTrophyGradeFromChar(current_trophy_grade.at(0));
                group_info.num_trophies_by_rarity[trophyGrade]++;
                if (current_trophy_unlockstate) {
                    group_info.unlocked_trophies++;
                    group_info.unlocked_trophies_by_rarity[trophyGrade]++;
                }
            }
        }
    }

    details->num_trophies = group_info.num_trophies;
    details->num_platinum = group_info.num_trophies_by_rarity[ORBIS_NP_TROPHY_GRADE_PLATINUM];
    details->num_gold = group_info.num_trophies_by_rarity[ORBIS_NP_TROPHY_GRADE_GOLD];
    details->num_silver = group_info.num_trophies_by_rarity[ORBIS_NP_TROPHY_GRADE_SILVER];
    details->num_bronze = group_info.num_trophies_by_rarity[ORBIS_NP_TROPHY_GRADE_BRONZE];
    data->unlocked_trophies = group_info.unlocked_trophies;
    data->unlocked_platinum =
        group_info.unlocked_trophies_by_rarity[ORBIS_NP_TROPHY_GRADE_PLATINUM];
    data->unlocked_gold = group_info.unlocked_trophies_by_rarity[ORBIS_NP_TROPHY_GRADE_GOLD];
    data->unlocked_silver = group_info.unlocked_trophies_by_rarity[ORBIS_NP_TROPHY_GRADE_SILVER];
    data->unlocked_bronze = group_info.unlocked_trophies_by_rarity[ORBIS_NP_TROPHY_GRADE_BRONZE];

    // maybe this should be 1 instead of 100?
    data->progress_percentage = 100;

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophyGetTrophyIcon(OrbisNpTrophyContext context, OrbisNpTrophyHandle handle,
                                          OrbisNpTrophyId trophyId, void* buffer, u64* size) {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophyGetTrophyInfo(OrbisNpTrophyContext context, OrbisNpTrophyHandle handle,
                                          OrbisNpTrophyId trophyId, OrbisNpTrophyDetails* details,
                                          OrbisNpTrophyData* data) {
    LOG_INFO(Lib_NpTrophy, "Getting trophy info for id {}", trophyId);

    if (context == ORBIS_NP_TROPHY_INVALID_CONTEXT)
        return ORBIS_NP_TROPHY_ERROR_INVALID_CONTEXT;

    if (handle == ORBIS_NP_TROPHY_INVALID_HANDLE)
        return ORBIS_NP_TROPHY_ERROR_INVALID_HANDLE;

    if (trophyId >= 127)
        return ORBIS_NP_TROPHY_ERROR_INVALID_TROPHY_ID;

    if (details == nullptr || data == nullptr)
        return ORBIS_NP_TROPHY_ERROR_INVALID_ARGUMENT;

    if (details->size != 0x498 || data->size != 0x18)
        return ORBIS_NP_TROPHY_ERROR_INVALID_ARGUMENT;

    const auto trophy_dir =
        Common::FS::GetUserPath(Common::FS::PathType::MetaDataDir) / game_serial / "TrophyFiles";
    auto trophy_file = trophy_dir / "trophy00" / "Xml" / "TROP.XML";

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(trophy_file.native().c_str());

    if (!result) {
        LOG_ERROR(Lib_NpTrophy, "Failed to open trophy xml : {}", result.description());
        return ORBIS_OK;
    }

    auto trophyconf = doc.child("trophyconf");

    for (const pugi::xml_node& node : trophyconf.children()) {
        std::string_view node_name = node.name();

        if (node_name == "trophy") {
            int current_trophy_id = node.attribute("id").as_int(ORBIS_NP_TROPHY_INVALID_TROPHY_ID);
            if (current_trophy_id == trophyId) {
                bool current_trophy_unlockstate = node.attribute("unlockstate").as_bool();
                std::string_view current_trophy_grade = node.attribute("ttype").value();
                std::string_view current_trophy_name = node.child("name").text().as_string();
                std::string_view current_trophy_description =
                    node.child("detail").text().as_string();

                uint64_t current_trophy_timestamp = node.attribute("timestamp").as_ullong();
                int current_trophy_groupid = node.attribute("gid").as_int(-1);
                bool current_trophy_hidden = node.attribute("hidden").as_bool();

                details->trophy_id = trophyId;
                details->trophy_grade = GetTrophyGradeFromChar(current_trophy_grade.at(0));
                details->group_id = current_trophy_groupid;
                details->hidden = current_trophy_hidden;

                strncpy(details->name, current_trophy_name.data(), ORBIS_NP_TROPHY_NAME_MAX_SIZE);
                strncpy(details->description, current_trophy_description.data(),
                        ORBIS_NP_TROPHY_DESCR_MAX_SIZE);

                data->trophy_id = trophyId;
                data->unlocked = current_trophy_unlockstate;
                data->timestamp.tick = current_trophy_timestamp;
            }
        }
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTrophyGetTrophyUnlockState(OrbisNpTrophyContext context,
                                                 OrbisNpTrophyHandle handle,
                                                 OrbisNpTrophyFlagArray* flags, u32* count) {
    LOG_INFO(Lib_NpTrophy, "called");

    if (context == ORBIS_NP_TROPHY_INVALID_CONTEXT)
        return ORBIS_NP_TROPHY_ERROR_INVALID_CONTEXT;

    if (handle == ORBIS_NP_TROPHY_INVALID_HANDLE)
        return ORBIS_NP_TROPHY_ERROR_INVALID_HANDLE;

    if (flags == nullptr || count == nullptr)
        return ORBIS_NP_TROPHY_ERROR_INVALID_ARGUMENT;

    ORBIS_NP_TROPHY_FLAG_ZERO(flags);

    const auto trophy_dir =
        Common::FS::GetUserPath(Common::FS::PathType::MetaDataDir) / game_serial / "TrophyFiles";
    auto trophy_file = trophy_dir / "trophy00" / "Xml" / "TROP.XML";

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(trophy_file.native().c_str());

    if (!result) {
        LOG_ERROR(Lib_NpTrophy, "Failed to open trophy XML: {}", result.description());
        *count = 0;
        return ORBIS_OK;
    }

    int num_trophies = 0;
    auto trophyconf = doc.child("trophyconf");

    for (const pugi::xml_node& node : trophyconf.children()) {
        std::string_view node_name = node.name();
        int current_trophy_id = node.attribute("id").as_int(ORBIS_NP_TROPHY_INVALID_TROPHY_ID);
        bool current_trophy_unlockstate = node.attribute("unlockstate").as_bool();

        if (node_name == "trophy") {
            num_trophies++;
        }

        if (current_trophy_unlockstate) {
            ORBIS_NP_TROPHY_FLAG_SET(current_trophy_id, flags);
        }
    }

    *count = num_trophies;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophyGroupArrayGetNum() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophyIntAbortHandle() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophyIntCheckNetSyncTitles() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophyIntCreateHandle() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophyIntDestroyHandle() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophyIntGetLocalTrophySummary() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophyIntGetProgress() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophyIntGetRunningTitle() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophyIntGetRunningTitles() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophyIntGetTrpIconByUri() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophyIntNetSyncTitle() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophyIntNetSyncTitles() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophyNumInfoGetTotal() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophyRegisterContext(OrbisNpTrophyContext context,
                                            OrbisNpTrophyHandle handle, uint64_t options) {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");

    if (context == ORBIS_NP_TROPHY_INVALID_CONTEXT)
        return ORBIS_NP_TROPHY_ERROR_INVALID_CONTEXT;

    if (handle == ORBIS_NP_TROPHY_INVALID_HANDLE)
        return ORBIS_NP_TROPHY_ERROR_INVALID_HANDLE;

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySetInfoGetTrophyFlagArray() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySetInfoGetTrophyNum() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophyShowTrophyList(OrbisNpTrophyContext context,
                                           OrbisNpTrophyHandle handle) {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemAbortHandle() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemBuildGroupIconUri() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemBuildNetTrophyIconUri() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemBuildTitleIconUri() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemBuildTrophyIconUri() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemCheckNetSyncTitles() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemCheckRecoveryRequired() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemCloseStorage() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemCreateContext() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemCreateHandle() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemDbgCtl() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemDebugLockTrophy() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemDebugUnlockTrophy() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemDestroyContext() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemDestroyHandle() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemDestroyTrophyConfig() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemGetDbgParam() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemGetDbgParamInt() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemGetGroupIcon() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemGetLocalTrophySummary() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemGetNextTitleFileEntryStatus() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemGetProgress() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemGetTitleFileStatus() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemGetTitleIcon() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemGetTitleSyncStatus() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemGetTrophyConfig() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemGetTrophyData() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemGetTrophyGroupData() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemGetTrophyIcon() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemGetTrophyTitleData() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemGetTrophyTitleIds() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemGetUserFileInfo() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemGetUserFileStatus() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemIsServerAvailable() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemNetSyncTitle() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemNetSyncTitles() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemOpenStorage() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemPerformRecovery() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemRemoveAll() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemRemoveTitleData() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemRemoveUserData() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemSetDbgParam() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophySystemSetDbgParamInt() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpTrophyUnlockTrophy(OrbisNpTrophyContext context, OrbisNpTrophyHandle handle,
                                         OrbisNpTrophyId trophyId, OrbisNpTrophyId* platinumId) {
    LOG_INFO(Lib_NpTrophy, "Unlocking trophy id {}", trophyId);

    if (context == ORBIS_NP_TROPHY_INVALID_CONTEXT)
        return ORBIS_NP_TROPHY_ERROR_INVALID_CONTEXT;

    if (handle == ORBIS_NP_TROPHY_INVALID_HANDLE)
        return ORBIS_NP_TROPHY_ERROR_INVALID_HANDLE;

    if (trophyId >= 127)
        return ORBIS_NP_TROPHY_ERROR_INVALID_TROPHY_ID;

    if (platinumId == nullptr)
        return ORBIS_NP_TROPHY_ERROR_INVALID_ARGUMENT;

    const auto trophy_dir =
        Common::FS::GetUserPath(Common::FS::PathType::MetaDataDir) / game_serial / "TrophyFiles";
    auto trophy_file = trophy_dir / "trophy00" / "Xml" / "TROP.XML";

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(trophy_file.native().c_str());

    if (!result) {
        LOG_ERROR(Lib_NpTrophy, "Failed to parse trophy xml : {}", result.description());
        return ORBIS_OK;
    }

    *platinumId = ORBIS_NP_TROPHY_INVALID_TROPHY_ID;

    int num_trophies = 0;
    int num_trophies_unlocked = 0;
    pugi::xml_node platinum_node;

    auto trophyconf = doc.child("trophyconf");

    for (pugi::xml_node& node : trophyconf.children()) {
        int current_trophy_id = node.attribute("id").as_int(ORBIS_NP_TROPHY_INVALID_TROPHY_ID);
        bool current_trophy_unlockstate = node.attribute("unlockstate").as_bool();
        const char* current_trophy_name = node.child("name").text().as_string();
        std::string_view current_trophy_description = node.child("detail").text().as_string();
        std::string_view current_trophy_type = node.attribute("ttype").value();

        if (current_trophy_type == "P") {
            platinum_node = node;
            if (trophyId == current_trophy_id) {
                return ORBIS_NP_TROPHY_ERROR_PLATINUM_CANNOT_UNLOCK;
            }
        }

        if (std::string_view(node.name()) == "trophy") {
            if (node.attribute("pid").as_int(-1) != ORBIS_NP_TROPHY_INVALID_TROPHY_ID) {
                num_trophies++;
                if (current_trophy_unlockstate) {
                    num_trophies_unlocked++;
                }
            }

            if (current_trophy_id == trophyId) {
                if (current_trophy_unlockstate) {
                    LOG_INFO(Lib_NpTrophy, "Trophy already unlocked");
                    return ORBIS_NP_TROPHY_ERROR_TROPHY_ALREADY_UNLOCKED;
                } else {
                    if (node.attribute("unlockstate").empty()) {
                        node.append_attribute("unlockstate") = "true";
                    } else {
                        node.attribute("unlockstate").set_value("true");
                    }

                    auto trophyTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
                                               std::chrono::system_clock::now().time_since_epoch())
                                               .count();

                    if (node.attribute("timestamp").empty()) {
                        node.append_attribute("timestamp") =
                            std::to_string(trophyTimestamp).c_str();
                    } else {
                        node.attribute("timestamp")
                            .set_value(std::to_string(trophyTimestamp).c_str());
                    }

                    std::string trophy_icon_file = "TROP";
                    trophy_icon_file.append(node.attribute("id").value());
                    trophy_icon_file.append(".PNG");

                    std::filesystem::path current_icon_path =
                        trophy_dir / "trophy00" / "Icons" / trophy_icon_file;

                    AddTrophyToQueue(current_icon_path, current_trophy_name, current_trophy_type);
                }
            }
        }
    }

    if (!platinum_node.attribute("unlockstate").as_bool()) {
        if ((num_trophies - 1) == num_trophies_unlocked) {
            if (platinum_node.attribute("unlockstate").empty()) {
                platinum_node.append_attribute("unlockstate") = "true";
            } else {
                platinum_node.attribute("unlockstate").set_value("true");
            }

            auto trophyTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
                                       std::chrono::system_clock::now().time_since_epoch())
                                       .count();

            if (platinum_node.attribute("timestamp").empty()) {
                platinum_node.append_attribute("timestamp") =
                    std::to_string(trophyTimestamp).c_str();
            } else {
                platinum_node.attribute("timestamp")
                    .set_value(std::to_string(trophyTimestamp).c_str());
            }

            int platinum_trophy_id =
                platinum_node.attribute("id").as_int(ORBIS_NP_TROPHY_INVALID_TROPHY_ID);
            const char* platinum_trophy_name = platinum_node.child("name").text().as_string();

            std::string platinum_icon_file = "TROP";
            platinum_icon_file.append(platinum_node.attribute("id").value());
            platinum_icon_file.append(".PNG");

            std::filesystem::path platinum_icon_path =
                trophy_dir / "trophy00" / "Icons" / platinum_icon_file;

            *platinumId = platinum_trophy_id;
            AddTrophyToQueue(platinum_icon_path, platinum_trophy_name, "P");
        }
    }

    doc.save_file((trophy_dir / "trophy00" / "Xml" / "TROP.XML").native().c_str());

    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_149656DA81D41C59() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_9F80071876FFA5F6() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_F8EF6F5350A91990() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_FA7A2DD770447552() {
    LOG_ERROR(Lib_NpTrophy, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("aTnHs7W-9Uk", "libSceNpTrophy", 1, "libSceNpTrophy", sceNpTrophyAbortHandle);
    LIB_FUNCTION("cqGkYAN-gRw", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophyCaptureScreenshot);
    LIB_FUNCTION("lhE4XS9OJXs", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophyConfigGetTrophyDetails);
    LIB_FUNCTION("qJ3IvrOoXg0", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophyConfigGetTrophyFlagArray);
    LIB_FUNCTION("zDjF2G+6tI0", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophyConfigGetTrophyGroupArray);
    LIB_FUNCTION("7Kh86vJqtxw", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophyConfigGetTrophyGroupDetails);
    LIB_FUNCTION("ndLeNWExeZE", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophyConfigGetTrophySetInfo);
    LIB_FUNCTION("6EOfS5SDgoo", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophyConfigGetTrophySetInfoInGroup);
    LIB_FUNCTION("MW5ygoZqEBs", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophyConfigGetTrophySetVersion);
    LIB_FUNCTION("3tWKpNKn5+I", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophyConfigGetTrophyTitleDetails);
    LIB_FUNCTION("iqYfxC12sak", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophyConfigHasGroupFeature);
    LIB_FUNCTION("XbkjbobZlCY", "libSceNpTrophy", 1, "libSceNpTrophy", sceNpTrophyCreateContext);
    LIB_FUNCTION("q7U6tEAQf7c", "libSceNpTrophy", 1, "libSceNpTrophy", sceNpTrophyCreateHandle);
    LIB_FUNCTION("E1Wrwd07Lr8", "libSceNpTrophy", 1, "libSceNpTrophy", sceNpTrophyDestroyContext);
    LIB_FUNCTION("GNcF4oidY0Y", "libSceNpTrophy", 1, "libSceNpTrophy", sceNpTrophyDestroyHandle);
    LIB_FUNCTION("HLwz1fRIycA", "libSceNpTrophy", 1, "libSceNpTrophy", sceNpTrophyGetGameIcon);
    LIB_FUNCTION("YYP3f2W09og", "libSceNpTrophy", 1, "libSceNpTrophy", sceNpTrophyGetGameInfo);
    LIB_FUNCTION("w4uMPmErD4I", "libSceNpTrophy", 1, "libSceNpTrophy", sceNpTrophyGetGroupIcon);
    LIB_FUNCTION("wTUwGfspKic", "libSceNpTrophy", 1, "libSceNpTrophy", sceNpTrophyGetGroupInfo);
    LIB_FUNCTION("eBL+l6HG9xk", "libSceNpTrophy", 1, "libSceNpTrophy", sceNpTrophyGetTrophyIcon);
    LIB_FUNCTION("qqUVGDgQBm0", "libSceNpTrophy", 1, "libSceNpTrophy", sceNpTrophyGetTrophyInfo);
    LIB_FUNCTION("LHuSmO3SLd8", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophyGetTrophyUnlockState);
    LIB_FUNCTION("Ht6MNTl-je4", "libSceNpTrophy", 1, "libSceNpTrophy", sceNpTrophyGroupArrayGetNum);
    LIB_FUNCTION("u9plkqa2e0k", "libSceNpTrophy", 1, "libSceNpTrophy", sceNpTrophyIntAbortHandle);
    LIB_FUNCTION("pE5yhroy9m0", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophyIntCheckNetSyncTitles);
    LIB_FUNCTION("edPIOFpEAvU", "libSceNpTrophy", 1, "libSceNpTrophy", sceNpTrophyIntCreateHandle);
    LIB_FUNCTION("DSh3EXpqAQ4", "libSceNpTrophy", 1, "libSceNpTrophy", sceNpTrophyIntDestroyHandle);
    LIB_FUNCTION("sng98qULzPA", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophyIntGetLocalTrophySummary);
    LIB_FUNCTION("t3CQzag7-zs", "libSceNpTrophy", 1, "libSceNpTrophy", sceNpTrophyIntGetProgress);
    LIB_FUNCTION("jF-mCgGuvbQ", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophyIntGetRunningTitle);
    LIB_FUNCTION("PeAyBjC5kp8", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophyIntGetRunningTitles);
    LIB_FUNCTION("PEo09Dkqv0o", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophyIntGetTrpIconByUri);
    LIB_FUNCTION("kF9zjnlAzIA", "libSceNpTrophy", 1, "libSceNpTrophy", sceNpTrophyIntNetSyncTitle);
    LIB_FUNCTION("UXiyfabxFNQ", "libSceNpTrophy", 1, "libSceNpTrophy", sceNpTrophyIntNetSyncTitles);
    LIB_FUNCTION("hvdThnVvwdY", "libSceNpTrophy", 1, "libSceNpTrophy", sceNpTrophyNumInfoGetTotal);
    LIB_FUNCTION("TJCAxto9SEU", "libSceNpTrophy", 1, "libSceNpTrophy", sceNpTrophyRegisterContext);
    LIB_FUNCTION("ITUmvpBPaG0", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySetInfoGetTrophyFlagArray);
    LIB_FUNCTION("BSoSgiMVHnY", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySetInfoGetTrophyNum);
    LIB_FUNCTION("d9jpdPz5f-8", "libSceNpTrophy", 1, "libSceNpTrophy", sceNpTrophyShowTrophyList);
    LIB_FUNCTION("JzJdh-JLtu0", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemAbortHandle);
    LIB_FUNCTION("z8RCP536GOM", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemBuildGroupIconUri);
    LIB_FUNCTION("Rd2FBOQE094", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemBuildNetTrophyIconUri);
    LIB_FUNCTION("Q182x0rT75I", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemBuildTitleIconUri);
    LIB_FUNCTION("lGnm5Kg-zpA", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemBuildTrophyIconUri);
    LIB_FUNCTION("20wAMbXP-u0", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemCheckNetSyncTitles);
    LIB_FUNCTION("sKGFFY59ksY", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemCheckRecoveryRequired);
    LIB_FUNCTION("JMSapEtDH9Q", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemCloseStorage);
    LIB_FUNCTION("dk27olS4CEE", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemCreateContext);
    LIB_FUNCTION("cBzXEdzVzvs", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemCreateHandle);
    LIB_FUNCTION("8aLlLHKP+No", "libSceNpTrophy", 1, "libSceNpTrophy", sceNpTrophySystemDbgCtl);
    LIB_FUNCTION("NobVwD8qcQY", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemDebugLockTrophy);
    LIB_FUNCTION("yXJlgXljItk", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemDebugUnlockTrophy);
    LIB_FUNCTION("U0TOSinfuvw", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemDestroyContext);
    LIB_FUNCTION("-LC9hudmD+Y", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemDestroyHandle);
    LIB_FUNCTION("q6eAMucXIEM", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemDestroyTrophyConfig);
    LIB_FUNCTION("WdCUUJLQodM", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemGetDbgParam);
    LIB_FUNCTION("4QYFwC7tn4U", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemGetDbgParamInt);
    LIB_FUNCTION("OcllHFFcQkI", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemGetGroupIcon);
    LIB_FUNCTION("tQ3tXfVZreU", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemGetLocalTrophySummary);
    LIB_FUNCTION("g0dxBNTspC0", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemGetNextTitleFileEntryStatus);
    LIB_FUNCTION("sJSDnJRJHhI", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemGetProgress);
    LIB_FUNCTION("X47s4AamPGg", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemGetTitleFileStatus);
    LIB_FUNCTION("7WPj4KCF3D8", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemGetTitleIcon);
    LIB_FUNCTION("pzL+aAk0tQA", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemGetTitleSyncStatus);
    LIB_FUNCTION("Ro4sI9xgYl4", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemGetTrophyConfig);
    LIB_FUNCTION("7+OR1TU5QOA", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemGetTrophyData);
    LIB_FUNCTION("aXhvf2OmbiE", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemGetTrophyGroupData);
    LIB_FUNCTION("Rkt0bVyaa4Y", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemGetTrophyIcon);
    LIB_FUNCTION("nXr5Rho8Bqk", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemGetTrophyTitleData);
    LIB_FUNCTION("eV1rtLr+eys", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemGetTrophyTitleIds);
    LIB_FUNCTION("SsGLKTfWfm0", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemGetUserFileInfo);
    LIB_FUNCTION("XqLLsvl48kA", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemGetUserFileStatus);
    LIB_FUNCTION("-qjm2fFE64M", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemIsServerAvailable);
    LIB_FUNCTION("50BvYYzPTsY", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemNetSyncTitle);
    LIB_FUNCTION("yDJ-r-8f4S4", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemNetSyncTitles);
    LIB_FUNCTION("mWtsnHY8JZg", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemOpenStorage);
    LIB_FUNCTION("tAxnXpzDgFw", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemPerformRecovery);
    LIB_FUNCTION("tV18n8OcheI", "libSceNpTrophy", 1, "libSceNpTrophy", sceNpTrophySystemRemoveAll);
    LIB_FUNCTION("kV4DP0OTMNo", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemRemoveTitleData);
    LIB_FUNCTION("lZSZoN8BstI", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemRemoveUserData);
    LIB_FUNCTION("nytN-3-pdvI", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemSetDbgParam);
    LIB_FUNCTION("JsRnDKRzvRw", "libSceNpTrophy", 1, "libSceNpTrophy",
                 sceNpTrophySystemSetDbgParamInt);
    LIB_FUNCTION("28xmRUFao68", "libSceNpTrophy", 1, "libSceNpTrophy", sceNpTrophyUnlockTrophy);
    LIB_FUNCTION("FJZW2oHUHFk", "libSceNpTrophy", 1, "libSceNpTrophy", Func_149656DA81D41C59);
    LIB_FUNCTION("n4AHGHb-pfY", "libSceNpTrophy", 1, "libSceNpTrophy", Func_9F80071876FFA5F6);
    LIB_FUNCTION("+O9vU1CpGZA", "libSceNpTrophy", 1, "libSceNpTrophy", Func_F8EF6F5350A91990);
    LIB_FUNCTION("+not13BEdVI", "libSceNpTrophy", 1, "libSceNpTrophy", Func_FA7A2DD770447552);
};

} // namespace Libraries::Np::NpTrophy
