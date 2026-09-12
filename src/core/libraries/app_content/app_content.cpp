// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

#include "app_content.h"
#include "common/assert.h"
#include "common/elf_info.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/emulator_settings.h"
#include "core/file_format/psf.h"
#include "core/file_sys/fs.h"
#include "core/libraries/app_content/app_content_error.h"
#include "core/libraries/kernel/process.h"
#include "core/libraries/libs.h"
#include "core/libraries/system/systemservice.h"

namespace Libraries::AppContent {

struct AddContInfo {
    char entitlement_label[ORBIS_NP_UNIFIED_ENTITLEMENT_LABEL_SIZE];
    OrbisAppContentAddcontDownloadStatus status;
    OrbisAppContentGetEntitlementKey key;
    /// Host folder this entitlement was loaded from
    std::filesystem::path path;
    /// NP service label this entitlement belongs to. 0 is the application's own
    /// additional content,1-7 are shared sets named by SERVICE_ID_ADDCONT_ADD_n
    /// in the application's param.sfo.
    u32 service_label = 0;
};

static std::array<AddContInfo, ORBIS_APP_CONTENT_INFO_LIST_MAX_SIZE> addcont_info = {{
    {"0000000000000000",
     OrbisAppContentAddcontDownloadStatus::Installed,
     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00}},
}};

static s32 sdk_ver = 0;
static s32 addcont_count = 0;

// allow at most ORBIS_APP_CONTENT_ADDCONT_MOUNT_MAXNUM simultaneous mounts
struct MountedAddcont {
    bool used = false;
    s32 addcont_index = -1;
};
static std::array<MountedAddcont, ORBIS_APP_CONTENT_ADDCONT_MOUNT_MAXNUM> mounted_addcont{};
static std::string title_id;
static bool is_initialized = false;

s32 CheckEntitlementLabel(const OrbisNpUnifiedEntitlementLabel* label, const char* caller) {
    if (label == nullptr) {
        return ORBIS_APP_CONTENT_ERROR_PARAMETER;
    }
    if (sdk_ver >= Common::ElfInfo::FW_150 &&
        (label->padding[0] != 0 || label->padding[1] != 0 || label->padding[2] != 0)) {
        LOG_ERROR(Lib_AppContent, "{}: CheckEntitlementLabel() parameter error", caller);
        return ORBIS_APP_CONTENT_ERROR_PARAMETER;
    }
    return ORBIS_OK;
}

s32 ScanAddcontDir(const std::filesystem::path& addon_path, u32 service_label) {
    if (addon_path.empty()) {
        return 0;
    }
    if (!std::filesystem::exists(addon_path)) {
        LOG_INFO(Lib_AppContent, "No additional content directory at '{}' (service label {})",
                 addon_path.string(), service_label);
        return 0;
    }

    LOG_INFO(Lib_AppContent, "Scanning '{}' for service label {}", addon_path.string(),
             service_label);

    s32 found = 0;
    s32 examined = 0;
    for (const auto& entry : std::filesystem::directory_iterator(addon_path)) {
        if (!entry.is_directory()) {
            continue;
        }
        examined++;

        // Look for a param.sfo in the additional content directory.
        const auto param_sfo_path = entry.path() / "sce_sys/param.sfo";
        if (!std::filesystem::exists(param_sfo_path)) {
            const bool has_nested =
                std::filesystem::exists(entry.path() / "sce_sys") == false &&
                std::any_of(std::filesystem::directory_iterator(entry.path()),
                            std::filesystem::directory_iterator{}, [](const auto& sub) {
                                return sub.is_directory() &&
                                       std::filesystem::exists(sub.path() / "sce_sys/param.sfo");
                            });
            if (has_nested) {
                LOG_WARNING(Lib_AppContent,
                            "'{}' has no param.sfo but its subfolders do the entitlement "
                            "folders are nested one level too deep. Move them up into '{}'.",
                            entry.path().string(), addon_path.string());
            } else {
                LOG_WARNING(Lib_AppContent,
                            "Additional content folder '{}' has no sce_sys/param.sfo",
                            entry.path().string());
            }
            continue;
        }
        PSF dlc_params;
        if (!dlc_params.Open(param_sfo_path)) {
            LOG_WARNING(Lib_AppContent, "Additional content folder {} has an unreadable param.sfo",
                        entry.path().filename().string());
            continue;
        }

        const auto category = dlc_params.GetString("CATEGORY");
        if (!category.has_value() || strncmp(category.value().data(), "ac", 2) != 0) {
            LOG_WARNING(Lib_AppContent, "Additional content folder {} is not additional content",
                        entry.path().filename().string());
            continue;
        }

        // We've located additional content. Find the entitlement id from the content id.
        const auto content_id = dlc_params.GetString("CONTENT_ID");
        if (!content_id.has_value()) {
            LOG_WARNING(Lib_AppContent, "Additional content {} param.sfo is missing CONTENT_ID",
                        entry.path().filename().string());
            continue;
        }

        // content id's have consistent formatting, so this will always work.
        // They follow the format UPXXXX-CUSAXXXXX_XX-entitlement
        if (content_id.value().length() <= ORBIS_APP_CONTENT_ENTITLEMENT_LABEL_OFFSET) {
            LOG_WARNING(Lib_AppContent, "Additional content {} param.sfo has malformed CONTENT_ID",
                        entry.path().filename().string());
            continue;
        }

        if (addcont_count >= ORBIS_APP_CONTENT_INFO_LIST_MAX_SIZE) {
            LOG_ERROR(Lib_AppContent, "More than {} additional content entries, ignoring the rest",
                      ORBIS_APP_CONTENT_INFO_LIST_MAX_SIZE);
            break;
        }

        const auto entitlement_id =
            content_id.value().substr(ORBIS_APP_CONTENT_ENTITLEMENT_LABEL_OFFSET);
        LOG_INFO(Lib_AppContent, "Entitlement {} found", entitlement_id);
        auto& info = addcont_info[addcont_count++];
        std::memset(info.entitlement_label, 0, sizeof(info.entitlement_label));
        entitlement_id.copy(info.entitlement_label, sizeof(info.entitlement_label) - 1);
        const bool has_content =
            std::any_of(std::filesystem::directory_iterator(entry.path()),
                        std::filesystem::directory_iterator{},
                        [](const auto& sub) { return sub.path().filename() != "sce_sys"; });
        if (!has_content) {
            LOG_WARNING(Lib_AppContent,
                        "'{}' holds no content besides sce_sys reporting NoExtraData. If this "
                        "DLC is meant to have data, the folder was not extracted correctly.",
                        entry.path().string());
        }
        info.status = has_content ? OrbisAppContentAddcontDownloadStatus::Installed
                                  : OrbisAppContentAddcontDownloadStatus::NoExtraData;
        info.path = entry.path();
        info.service_label = service_label;
        found++;
    }

    LOG_INFO(Lib_AppContent, "'{}': {} of {} subfolders were valid additional content",
             addon_path.string(), found, examined);
    return found;
}

/// Pull the title id out of a Service ID of the form "UP4108-CUSA01665_00".
std::string TitleIdFromServiceId(std::string_view service_id) {
    const auto dash = service_id.find('-');
    const auto underscore = service_id.rfind('_');
    if (dash == std::string_view::npos || underscore == std::string_view::npos ||
        underscore <= dash + 1) {
        return {};
    }
    return std::string{service_id.substr(dash + 1, underscore - dash - 1)};
}

int PS4_SYSV_ABI _Z5dummyv() {
    LOG_ERROR(Lib_AppContent, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentAddcontDelete() {
    LOG_ERROR(Lib_AppContent, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentAddcontEnqueueDownload() {
    LOG_ERROR(Lib_AppContent, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentAddcontEnqueueDownloadSp() {
    LOG_ERROR(Lib_AppContent, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentAddcontMount(u32 service_label,
                                           const OrbisNpUnifiedEntitlementLabel* entitlement_label,
                                           OrbisAppContentMountPoint* mount_point) {
    LOG_INFO(Lib_AppContent, "called, service_label = {}", service_label);

    if (!is_initialized) {
        return ORBIS_APP_CONTENT_ERROR_NOT_INITIALIZED;
    }
    if (entitlement_label == nullptr || mount_point == nullptr ||
        service_label >= ORBIS_NP_SERVICE_LABEL_MAX) {
        return ORBIS_APP_CONTENT_ERROR_PARAMETER;
    }
    if (const s32 err = CheckEntitlementLabel(entitlement_label, "AddcontMount"); err != ORBIS_OK) {
        return err;
    }

    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();

    for (s32 i = 0; i < addcont_count; i++) {
        if (addcont_info[i].service_label != service_label ||
            strncmp(entitlement_label->data, addcont_info[i].entitlement_label,
                    ORBIS_NP_UNIFIED_ENTITLEMENT_LABEL_SIZE - 1) != 0) {
            continue;
        }

        if (addcont_info[i].status == OrbisAppContentAddcontDownloadStatus::NoExtraData) {
            LOG_ERROR(Lib_AppContent, "Entitlement {} has no data to mount",
                      addcont_info[i].entitlement_label);
            return ORBIS_APP_CONTENT_ERROR_NOT_FOUND;
        }

        if (addcont_info[i].path.empty() || !std::filesystem::exists(addcont_info[i].path)) {
            LOG_ERROR(Lib_AppContent, "Folder for entitlement {} is gone",
                      addcont_info[i].entitlement_label);
            return ORBIS_APP_CONTENT_ERROR_NOT_FOUND;
        }

        // Mounting the same additional content twice is an error, not a second mount.
        for (const auto& m : mounted_addcont) {
            if (m.used && m.addcont_index == i) {
                LOG_ERROR(Lib_AppContent, "Entitlement {} is already mounted",
                          addcont_info[i].entitlement_label);
                return ORBIS_APP_CONTENT_ERROR_BUSY;
            }
        }

        // Take the lowest free mount slot
        s32 slot = -1;
        for (s32 s = 0; s < ORBIS_APP_CONTENT_ADDCONT_MOUNT_MAXNUM; s++) {
            if (!mounted_addcont[s].used) {
                slot = s;
                break;
            }
        }
        if (slot < 0) {
            LOG_ERROR(Lib_AppContent, "All {} mount slots are in use",
                      ORBIS_APP_CONTENT_ADDCONT_MOUNT_MAXNUM);
            return ORBIS_APP_CONTENT_ERROR_MOUNT_FULL;
        }

        snprintf(mount_point->data, ORBIS_APP_CONTENT_MOUNTPOINT_DATA_MAXSIZE, "/addcont%d", slot);
        mnt->Mount(addcont_info[i].path, mount_point->data);
        mounted_addcont[slot].used = true;
        mounted_addcont[slot].addcont_index = i;
        LOG_INFO(Lib_AppContent, "Mounted entitlement {} at {}", addcont_info[i].entitlement_label,
                 mount_point->data);
        return ORBIS_OK;
    }

    // Not a loaded entitlement.
    LOG_ERROR(Lib_AppContent, "Entitlement {} is not installed", entitlement_label->data);
    return ORBIS_APP_CONTENT_ERROR_NOT_FOUND;
}

int PS4_SYSV_ABI sceAppContentAddcontShrink() {
    LOG_ERROR(Lib_AppContent, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentAddcontUnmount(OrbisAppContentMountPoint* mount_point) {
    LOG_INFO(Lib_AppContent, "called");

    if (!is_initialized) {
        return ORBIS_APP_CONTENT_ERROR_NOT_INITIALIZED;
    }
    if (mount_point == nullptr) {
        return ORBIS_APP_CONTENT_ERROR_PARAMETER;
    }

    // Mount points are named "/addcont<slot>", so recover the slot and look up what
    // was mounted into it.
    s32 slot = -1;
    if (sscanf(mount_point->data, "/addcont%d", &slot) != 1 || slot < 0 ||
        slot >= ORBIS_APP_CONTENT_ADDCONT_MOUNT_MAXNUM || !mounted_addcont[slot].used) {
        LOG_ERROR(Lib_AppContent, "Mount point {} is not mounted", mount_point->data);
        return ORBIS_APP_CONTENT_ERROR_NOT_MOUNTED;
    }

    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    mnt->Unmount(addcont_info[mounted_addcont[slot].addcont_index].path, mount_point->data);
    mounted_addcont[slot] = {};
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentAppParamGetInt(OrbisAppContentAppParamId paramId, s32* out_value) {
    if (out_value == nullptr)
        return ORBIS_APP_CONTENT_ERROR_PARAMETER;
    auto* param_sfo = Common::Singleton<PSF>::Instance();
    std::optional<s32> value;
    switch (paramId) {
    case ORBIS_APP_CONTENT_APPPARAM_ID_SKU_FLAG:
        value = ORBIS_APP_CONTENT_APPPARAM_SKU_FLAG_FULL;
        break;
    case ORBIS_APP_CONTENT_APPPARAM_ID_USER_DEFINED_PARAM_1:
        value = param_sfo->GetInteger("USER_DEFINED_PARAM_1");
        break;
    case ORBIS_APP_CONTENT_APPPARAM_ID_USER_DEFINED_PARAM_2:
        value = param_sfo->GetInteger("USER_DEFINED_PARAM_2");
        break;
    case ORBIS_APP_CONTENT_APPPARAM_ID_USER_DEFINED_PARAM_3:
        value = param_sfo->GetInteger("USER_DEFINED_PARAM_3");
        break;
    case ORBIS_APP_CONTENT_APPPARAM_ID_USER_DEFINED_PARAM_4:
        value = param_sfo->GetInteger("USER_DEFINED_PARAM_4");
        break;
    default:
        LOG_ERROR(Lib_AppContent, " paramId = {} paramId is not valid", paramId);
        return ORBIS_APP_CONTENT_ERROR_PARAMETER;
    }
    *out_value = value.value_or(0);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentAppParamGetString() {
    LOG_ERROR(Lib_AppContent, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentDownload0Expand() {
    LOG_ERROR(Lib_AppContent, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentDownload0Shrink() {
    LOG_ERROR(Lib_AppContent, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentDownload1Expand() {
    LOG_ERROR(Lib_AppContent, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentDownload1Shrink() {
    LOG_ERROR(Lib_AppContent, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentDownloadDataFormat() {
    LOG_ERROR(Lib_AppContent, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentDownloadDataGetAvailableSpaceKb(OrbisAppContentMountPoint* mountPoint,
                                                              u64* availableSpaceKb) {
    LOG_ERROR(Lib_AppContent, "(STUBBED) called");
    *availableSpaceKb = 1048576;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentGetAddcontDownloadProgress() {
    LOG_ERROR(Lib_AppContent, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentGetAddcontInfo(u32 service_label,
                                             const OrbisNpUnifiedEntitlementLabel* entitlementLabel,
                                             OrbisAppContentAddcontInfo* info) {
    LOG_INFO(Lib_AppContent, "called, service_label = {}", service_label);

    if (!is_initialized) {
        return ORBIS_APP_CONTENT_ERROR_NOT_INITIALIZED;
    }
    if (entitlementLabel == nullptr || info == nullptr ||
        service_label >= ORBIS_NP_SERVICE_LABEL_MAX) {
        return ORBIS_APP_CONTENT_ERROR_PARAMETER;
    }
    if (const s32 err = CheckEntitlementLabel(entitlementLabel, "GetAddcontInfo");
        err != ORBIS_OK) {
        return err;
    }

    for (auto i = 0; i < addcont_count; i++) {
        if (addcont_info[i].service_label != service_label ||
            strncmp(entitlementLabel->data, addcont_info[i].entitlement_label,
                    ORBIS_NP_UNIFIED_ENTITLEMENT_LABEL_SIZE - 1) != 0) {
            continue;
        }

        LOG_INFO(Lib_AppContent, "found DLC {}", entitlementLabel->data);

        std::memset(info, 0, sizeof(*info));
        strncpy(info->entitlement_label.data, addcont_info[i].entitlement_label,
                ORBIS_NP_UNIFIED_ENTITLEMENT_LABEL_SIZE);
        info->status = addcont_info[i].status;
        return ORBIS_OK;
    }

    return ORBIS_APP_CONTENT_ERROR_DRM_NO_ENTITLEMENT;
}

int PS4_SYSV_ABI sceAppContentGetAddcontInfoList(u32 service_label,
                                                 OrbisAppContentAddcontInfo* list, u32 list_num,
                                                 u32* hit_num) {
    LOG_INFO(Lib_AppContent, "called, service_label = {}, list_num = {}", service_label, list_num);

    if (!is_initialized) {
        return ORBIS_APP_CONTENT_ERROR_NOT_INITIALIZED;
    }
    if (hit_num == nullptr || service_label >= ORBIS_NP_SERVICE_LABEL_MAX) {
        return ORBIS_APP_CONTENT_ERROR_PARAMETER;
    }
    u32 total = 0;
    u32 written = 0;
    for (s32 i = 0; i < addcont_count; i++) {
        if (addcont_info[i].service_label != service_label) {
            continue;
        }
        total++;

        if (list == nullptr || written >= list_num) {
            continue;
        }
        std::memset(&list[written], 0, sizeof(list[written]));
        strncpy(list[written].entitlement_label.data, addcont_info[i].entitlement_label,
                ORBIS_NP_UNIFIED_ENTITLEMENT_LABEL_SIZE);
        list[written].status = addcont_info[i].status;
        written++;
    }

    *hit_num = total;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentGetEntitlementKey(
    u32 service_label, const OrbisNpUnifiedEntitlementLabel* entitlement_label,
    OrbisAppContentGetEntitlementKey* key) {
    LOG_INFO(Lib_AppContent, "called, service_label = {}", service_label);

    if (!is_initialized) {
        return ORBIS_APP_CONTENT_ERROR_NOT_INITIALIZED;
    }
    if (entitlement_label == nullptr || key == nullptr ||
        service_label >= ORBIS_NP_SERVICE_LABEL_MAX) {
        return ORBIS_APP_CONTENT_ERROR_PARAMETER;
    }
    if (const s32 err = CheckEntitlementLabel(entitlement_label, "GetEntitlementKey");
        err != ORBIS_OK) {
        return err;
    }

    for (int i = 0; i < addcont_count; i++) {
        if (addcont_info[i].service_label != service_label ||
            strncmp(entitlement_label->data, addcont_info[i].entitlement_label,
                    ORBIS_NP_UNIFIED_ENTITLEMENT_LABEL_SIZE - 1) != 0) {
            continue;
        }

        memcpy(key->data, addcont_info[i].key.data, ORBIS_APP_CONTENT_ENTITLEMENT_KEY_SIZE);
        return ORBIS_OK;
    }

    return ORBIS_APP_CONTENT_ERROR_DRM_NO_ENTITLEMENT;
}

int PS4_SYSV_ABI sceAppContentGetRegion() {
    LOG_ERROR(Lib_AppContent, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentInitialize(const OrbisAppContentInitParam* initParam,
                                         OrbisAppContentBootParam* bootParam) {
    if (sdk_ver >= Common::ElfInfo::FW_150 && is_initialized) {
        LOG_ERROR(Lib_AppContent, "Already initialized");
        return ORBIS_APP_CONTENT_ERROR_BUSY;
    }

    LOG_WARNING(Lib_AppContent, "(DUMMY) called");
    is_initialized = true;

    if (bootParam != nullptr) {
        std::memset(bootParam, 0, sizeof(*bootParam));
    }

    auto* param_sfo = Common::Singleton<PSF>::Instance();

    const auto addons_dir = EmulatorSettings.GetAddonInstallDir();
    LOG_INFO(Lib_AppContent, "Additional content root is '{}'", addons_dir.string());
    if (const auto value = param_sfo->GetString("TITLE_ID"); value.has_value()) {
        title_id = *value;
    } else {
        UNREACHABLE_MSG("Failed to get TITLE_ID");
    }
    addcont_count = 0;
    mounted_addcont.fill({});

    // Service label 0 is the application's own additional content.
    ScanAddcontDir(addons_dir / title_id, 0);

    // Labels 1-7 are shared additional content belonging to other applications. The
    // base game's param.sfo names them in SERVICE_ID_ADDCONT_ADD_n as Service IDs
    for (u32 label = 1; label < ORBIS_NP_SERVICE_LABEL_MAX; label++) {
        const auto key = "SERVICE_ID_ADDCONT_ADD_" + std::to_string(label);
        const auto service_id = param_sfo->GetString(key);
        if (!service_id.has_value() || service_id->empty()) {
            continue;
        }

        const auto shared_title_id = TitleIdFromServiceId(*service_id);
        if (shared_title_id.empty()) {
            LOG_WARNING(Lib_AppContent, "{} = '{}' is not a usable Service ID", key, *service_id);
            continue;
        }

        s32 found = ScanAddcontDir(addons_dir / shared_title_id, label);
        if (found == 0) {
            found = ScanAddcontDir(addons_dir / title_id, label);
            if (found > 0) {
                LOG_WARNING(Lib_AppContent,
                            "Shared DLC for label {} found under the base title {} rather "
                            "than its own title {}",
                            label, title_id, shared_title_id);
            }
        }

        LOG_INFO(Lib_AppContent, "Service label {} -> {} ({} entitlements)", label, shared_title_id,
                 found);
    }

    if (addcont_count > 0) {
        SystemService::OrbisSystemServiceEvent event{};
        event.event_type = SystemService::OrbisSystemServiceEventType::EntitlementUpdate;
        event.service_entitlement_update.userId = 0;
        event.service_entitlement_update.np_service_label = 0;
        SystemService::PushSystemServiceEvent(event);
    }

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentRequestPatchInstall() {
    LOG_ERROR(Lib_AppContent, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentSmallSharedDataFormat() {
    LOG_ERROR(Lib_AppContent, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentSmallSharedDataGetAvailableSpaceKb() {
    LOG_ERROR(Lib_AppContent, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentSmallSharedDataMount() {
    LOG_ERROR(Lib_AppContent, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentSmallSharedDataUnmount() {
    LOG_ERROR(Lib_AppContent, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentTemporaryDataFormat() {
    LOG_ERROR(Lib_AppContent, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentTemporaryDataGetAvailableSpaceKb(
    const OrbisAppContentMountPoint* mountPoint, u64* availableSpaceKb) {
    LOG_ERROR(Lib_AppContent, "(STUBBED) called");
    *availableSpaceKb = 1048576;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentTemporaryDataMount() {
    LOG_ERROR(Lib_AppContent, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentTemporaryDataMount2(OrbisAppContentTemporaryDataOption option,
                                                  OrbisAppContentMountPoint* mountPoint) {
    if (mountPoint == nullptr) {
        return ORBIS_APP_CONTENT_ERROR_PARAMETER;
    }
    static constexpr std::string_view TmpMount = "/temp0";
    TmpMount.copy(mountPoint->data, TmpMount.size());
    mountPoint->data[TmpMount.size()] = '\0';
    LOG_INFO(Lib_AppContent, "sceAppContentTemporaryDataMount2: option = {}, mountPoint = {}",
             option, mountPoint->data);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentTemporaryDataUnmount() {
    LOG_ERROR(Lib_AppContent, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentGetPftFlag() {
    LOG_ERROR(Lib_AppContent, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C59A36FF8D7C59DA() {
    LOG_ERROR(Lib_AppContent, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentAddcontEnqueueDownloadByEntitlementId() {
    LOG_ERROR(Lib_AppContent, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentAddcontMountByEntitlementId() {
    LOG_ERROR(Lib_AppContent, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentGetAddcontInfoByEntitlementId() {
    LOG_ERROR(Lib_AppContent, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentGetAddcontInfoListByIroTag() {
    LOG_ERROR(Lib_AppContent, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentGetDownloadedStoreCountry() {
    LOG_ERROR(Lib_AppContent, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    ASSERT_MSG(Libraries::Kernel::sceKernelGetCompiledSdkVersion(&sdk_ver) == 0,
               "Failed to get SDK version");

    LIB_FUNCTION("AS45QoYHjc4", "libSceAppContent", 1, "libSceAppContentUtil", _Z5dummyv);
    LIB_FUNCTION("ZiATpP9gEkA", "libSceAppContent", 1, "libSceAppContentUtil",
                 sceAppContentAddcontDelete);
    LIB_FUNCTION("7gxh+5QubhY", "libSceAppContent", 1, "libSceAppContentUtil",
                 sceAppContentAddcontEnqueueDownload);
    LIB_FUNCTION("TVM-aYIsG9k", "libSceAppContent", 1, "libSceAppContentUtil",
                 sceAppContentAddcontEnqueueDownloadSp);
    LIB_FUNCTION("VANhIWcqYak", "libSceAppContent", 1, "libSceAppContentUtil",
                 sceAppContentAddcontMount);
    LIB_FUNCTION("D3H+cjfzzFY", "libSceAppContent", 1, "libSceAppContentUtil",
                 sceAppContentAddcontShrink);
    LIB_FUNCTION("3rHWaV-1KC4", "libSceAppContent", 1, "libSceAppContentUtil",
                 sceAppContentAddcontUnmount);
    LIB_FUNCTION("99b82IKXpH4", "libSceAppContent", 1, "libSceAppContentUtil",
                 sceAppContentAppParamGetInt);
    LIB_FUNCTION("+OlXCu8qxUk", "libSceAppContent", 1, "libSceAppContentUtil",
                 sceAppContentAppParamGetString);
    LIB_FUNCTION("gpGZDB4ZlrI", "libSceAppContent", 1, "libSceAppContentUtil",
                 sceAppContentDownload0Expand);
    LIB_FUNCTION("S5eMvWnbbXg", "libSceAppContent", 1, "libSceAppContentUtil",
                 sceAppContentDownload0Shrink);
    LIB_FUNCTION("B5gVeVurdUA", "libSceAppContent", 1, "libSceAppContentUtil",
                 sceAppContentDownload1Expand);
    LIB_FUNCTION("kUeYucqnb7o", "libSceAppContent", 1, "libSceAppContentUtil",
                 sceAppContentDownload1Shrink);
    LIB_FUNCTION("CN7EbEV7MFU", "libSceAppContent", 1, "libSceAppContentUtil",
                 sceAppContentDownloadDataFormat);
    LIB_FUNCTION("Gl6w5i0JokY", "libSceAppContent", 1, "libSceAppContentUtil",
                 sceAppContentDownloadDataGetAvailableSpaceKb);
    LIB_FUNCTION("5bvvbUSiFs4", "libSceAppContent", 1, "libSceAppContentUtil",
                 sceAppContentGetAddcontDownloadProgress);
    LIB_FUNCTION("m47juOmH0VE", "libSceAppContent", 1, "libSceAppContentUtil",
                 sceAppContentGetAddcontInfo);
    LIB_FUNCTION("xnd8BJzAxmk", "libSceAppContent", 1, "libSceAppContentUtil",
                 sceAppContentGetAddcontInfoList);
    LIB_FUNCTION("XTWR0UXvcgs", "libSceAppContent", 1, "libSceAppContentUtil",
                 sceAppContentGetEntitlementKey);
    LIB_FUNCTION("74-1x3lyZK8", "libSceAppContent", 1, "libSceAppContentUtil",
                 sceAppContentGetRegion);
    LIB_FUNCTION("R9lA82OraNs", "libSceAppContent", 1, "libSceAppContentUtil",
                 sceAppContentInitialize);
    LIB_FUNCTION("bVtF7v2uqT0", "libSceAppContent", 1, "libSceAppContentUtil",
                 sceAppContentRequestPatchInstall);
    LIB_FUNCTION("9Gq5rOkWzNU", "libSceAppContent", 1, "libSceAppContentUtil",
                 sceAppContentSmallSharedDataFormat);
    LIB_FUNCTION("xhb-r8etmAA", "libSceAppContent", 1, "libSceAppContentUtil",
                 sceAppContentSmallSharedDataGetAvailableSpaceKb);
    LIB_FUNCTION("QuApZnMo9MM", "libSceAppContent", 1, "libSceAppContentUtil",
                 sceAppContentSmallSharedDataMount);
    LIB_FUNCTION("EqMtBHWu-5M", "libSceAppContent", 1, "libSceAppContentUtil",
                 sceAppContentSmallSharedDataUnmount);
    LIB_FUNCTION("a5N7lAG0y2Q", "libSceAppContent", 1, "libSceAppContentUtil",
                 sceAppContentTemporaryDataFormat);
    LIB_FUNCTION("SaKib2Ug0yI", "libSceAppContent", 1, "libSceAppContentUtil",
                 sceAppContentTemporaryDataGetAvailableSpaceKb);
    LIB_FUNCTION("7bOLX66Iz-U", "libSceAppContent", 1, "libSceAppContentUtil",
                 sceAppContentTemporaryDataMount);
    LIB_FUNCTION("buYbeLOGWmA", "libSceAppContent", 1, "libSceAppContentUtil",
                 sceAppContentTemporaryDataMount2);
    LIB_FUNCTION("bcolXMmp6qQ", "libSceAppContent", 1, "libSceAppContentUtil",
                 sceAppContentTemporaryDataUnmount);
    LIB_FUNCTION("xmhnAoxN3Wk", "libSceAppContentPft", 1, "libSceAppContent",
                 sceAppContentGetPftFlag);
    LIB_FUNCTION("xZo2-418Wdo", "libSceAppContentBundle", 1, "libSceAppContent",
                 Func_C59A36FF8D7C59DA);
    LIB_FUNCTION("kJmjt81mXKQ", "libSceAppContentIro", 1, "libSceAppContent",
                 sceAppContentAddcontEnqueueDownloadByEntitlementId);
    LIB_FUNCTION("efX3lrPwdKA", "libSceAppContentIro", 1, "libSceAppContent",
                 sceAppContentAddcontMountByEntitlementId);
    LIB_FUNCTION("z9hgjLd1SGA", "libSceAppContentIro", 1, "libSceAppContent",
                 sceAppContentGetAddcontInfoByEntitlementId);
    LIB_FUNCTION("3wUaDTGmjcQ", "libSceAppContentIro", 1, "libSceAppContent",
                 sceAppContentGetAddcontInfoListByIroTag);
    LIB_FUNCTION("TCqT7kPuGx0", "libSceAppContentSc", 1, "libSceAppContent",
                 sceAppContentGetDownloadedStoreCountry);
};

} // namespace Libraries::AppContent
