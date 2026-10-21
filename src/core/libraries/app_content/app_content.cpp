// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <ranges>
#include <span>
#include <string>
#include <vector>
#include <fmt/format.h>

#include "app_content.h"
#include "common/assert.h"
#include "common/elf_info.h"
#include "common/logging/log.h"
#include "common/path_util.h"
#include "common/singleton.h"
#include "core/emulator_settings.h"
#include "core/file_format/psf.h"
#include "core/file_format/rif.h"
#include "core/file_sys/fs.h"
#include "core/libraries/app_content/app_content_error.h"
#include "core/libraries/kernel/process.h"
#include "core/libraries/libs.h"
#include "core/libraries/system/systemservice.h"

namespace Libraries::AppContent {

constexpr u32 ORBIS_LICENSE_INFO_ENTITLEMENT_KEY_OFFSET = 0x30;
constexpr u32 ORBIS_LICENSE_DAT_ENTITLEMENT_KEY_OFFSET =
    offsetof(RIFData, encrypted_secret) + RIF_SECRET_ENTITLEMENT_KEY_OFFSET;

struct AddContInfo {
    u32 service_label{};
    char entitlement_label[ORBIS_NP_UNIFIED_ENTITLEMENT_LABEL_SIZE]{};
    OrbisAppContentAddcontDownloadStatus status{};
    OrbisAppContentGetEntitlementKey key{};
};

static std::array<AddContInfo, ORBIS_APP_CONTENT_INFO_LIST_MAX_SIZE> addcont_info{};

static s32 sdk_ver = 0;
static s32 addcont_count = 0;
static std::string title_id;
static bool is_initialized = false;
// Folders searched for additional content: the game's own title ID plus the title IDs of any
// services linked through SERVICE_ID_ADDCONT_ADD_1..7 in the game's param.sfo.
static std::vector<std::filesystem::path> addon_paths;

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
    LOG_INFO(Lib_AppContent, "called");

    if (!is_initialized) {
        return ORBIS_APP_CONTENT_ERROR_NOT_INITIALIZED;
    }

    if (service_label > 7 || entitlement_label == nullptr || mount_point == nullptr) {
        return ORBIS_APP_CONTENT_ERROR_PARAMETER;
    }

    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();

    // Determine which loaded additional content this entitlement label is for.
    s32 i = 0;
    while (i < addcont_count) {
        if (addcont_info[i].service_label == service_label &&
            strncmp(entitlement_label->data, addcont_info[i].entitlement_label,
                    ORBIS_NP_UNIFIED_ENTITLEMENT_LABEL_SIZE - 1) == 0) {
            snprintf(mount_point->data, ORBIS_APP_CONTENT_MOUNTPOINT_DATA_MAXSIZE, "/addcont%d", i);
            break;
        }
        ++i;
    }

    if (i == addcont_count) {
        // None of the loaded additional content match the entitlement label requested.
        return ORBIS_APP_CONTENT_ERROR_NOT_FOUND;
    }

    // Find which directory corresponds to this entitlement
    for (const auto& addon_path : addon_paths) {
        for (const auto& entry : std::filesystem::directory_iterator(addon_path)) {
            if (!entry.is_directory() && entry.path().extension() != ".zar") {
                continue;
            }

            // Create a temporary filesystem backend to view the param.sfo
            std::shared_ptr<Core::FileSys::IBackend> fs = mnt->CreateBackend(entry.path(), true);

            // Look for a param.sfo in the additional content
            auto psf_handle = fs->Open("sce_sys/param.sfo", Common::FS::FileAccessMode::Read);
            if (psf_handle == nullptr) {
                // This content doesn't have a param.sfo
                continue;
            }

            // Open the param.sfo in this content
            std::unique_ptr<PSF> dlc_params = std::make_unique<PSF>();
            if (!dlc_params->Open(psf_handle)) {
                // The param.sfo is invalid
                continue;
            }

            // Validate the available params
            auto category = dlc_params->GetString("CATEGORY");
            auto content_id = dlc_params->GetString("CONTENT_ID");
            if (!category.has_value() || strncmp(category.value().data(), "ac", 2) != 0 ||
                !content_id.has_value() ||
                content_id.value().length() <= ORBIS_APP_CONTENT_ENTITLEMENT_LABEL_OFFSET) {
                // This content fails the error checks performed in sceAppContentInitialize.
                continue;
            }

            auto entitlement_id =
                content_id.value().substr(ORBIS_APP_CONTENT_ENTITLEMENT_LABEL_OFFSET);
            if (strncmp(entitlement_id.data(), entitlement_label->data, entitlement_id.length()) !=
                0) {
                // The entitlement ID is incorrect.
                continue;
            }

            // We've located the correct folder.
            mnt->Mount(entry.path(), mount_point->data, true);
            return ORBIS_OK;
        }
    }

    // Hitting this shouldn't be possible, as it would mean the entitlement was loaded,
    // but the folder it was loaded from doesn't exist.
    UNREACHABLE_MSG("Folder for loaded entitlement label {} doesn't exist.",
                    entitlement_label->data);
}

int PS4_SYSV_ABI sceAppContentAddcontShrink() {
    LOG_ERROR(Lib_AppContent, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentAddcontUnmount(OrbisAppContentMountPoint* mount_point) {
    if (!is_initialized) {
        LOG_ERROR(Lib_AppContent, "called before initialization");
        return ORBIS_APP_CONTENT_ERROR_NOT_INITIALIZED;
    }

    if (!mount_point) {
        LOG_ERROR(Lib_AppContent, "mount_point is null");
        return ORBIS_APP_CONTENT_ERROR_PARAMETER;
    }

    const std::string_view mp{mount_point->data,
                              strnlen(mount_point->data, sizeof(mount_point->data))};
    LOG_INFO(Lib_AppContent, "called, mount_point = {}", mp);

    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    mnt->Unmount(std::string{mp});
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
    LOG_INFO(Lib_AppContent, "called");

    if (!is_initialized) {
        return ORBIS_APP_CONTENT_ERROR_NOT_INITIALIZED;
    }

    if (service_label > 7 || entitlementLabel == nullptr || info == nullptr) {
        return ORBIS_APP_CONTENT_ERROR_PARAMETER;
    }

    for (auto i = 0; i < addcont_count; i++) {
        if (addcont_info[i].service_label != service_label ||
            strncmp(entitlementLabel->data, addcont_info[i].entitlement_label,
                    ORBIS_NP_UNIFIED_ENTITLEMENT_LABEL_SIZE - 1) != 0) {
            continue;
        }

        LOG_INFO(Lib_AppContent, "found DLC {}", entitlementLabel->data);

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
    LOG_INFO(Lib_AppContent, "called");

    if (!is_initialized) {
        return ORBIS_APP_CONTENT_ERROR_NOT_INITIALIZED;
    }

    if (service_label > 7) {
        return ORBIS_APP_CONTENT_ERROR_PARAMETER;
    }

    const auto matching_count = static_cast<u32>(std::ranges::count_if(
        std::span(addcont_info.data(), addcont_count),
        [service_label](const auto& info) { return info.service_label == service_label; }));

    if (list_num == 0 || list == nullptr) {
        if (hit_num == nullptr) {
            return ORBIS_APP_CONTENT_ERROR_PARAMETER;
        }

        *hit_num = matching_count;
        return ORBIS_OK;
    }

    u32 dlcs_to_list = std::min(matching_count, list_num);
    u32 written = 0;
    for (int i = 0; i < addcont_count && written < dlcs_to_list; i++) {
        if (addcont_info[i].service_label == service_label) {
            strncpy(list[written].entitlement_label.data, addcont_info[i].entitlement_label,
                    ORBIS_NP_UNIFIED_ENTITLEMENT_LABEL_SIZE);
            list[written].status = addcont_info[i].status;
            written++;
        }
    }

    if (hit_num != nullptr) {
        *hit_num = dlcs_to_list;
    }

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentGetEntitlementKey(
    u32 service_label, const OrbisNpUnifiedEntitlementLabel* entitlement_label,
    OrbisAppContentGetEntitlementKey* key) {
    LOG_INFO(Lib_AppContent, "called");

    if (!is_initialized) {
        return ORBIS_APP_CONTENT_ERROR_NOT_INITIALIZED;
    }

    if (service_label > 7) {
        return ORBIS_APP_CONTENT_ERROR_PARAMETER;
    }

    if (entitlement_label == nullptr || key == nullptr) {
        return ORBIS_APP_CONTENT_ERROR_PARAMETER;
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

static bool CanAddEntitlement(u32 service_label, std::string_view entitlement_id) {
    if (addcont_count >= ORBIS_APP_CONTENT_INFO_LIST_MAX_SIZE) {
        LOG_ERROR(Lib_AppContent, "Too many entitlements, ignoring {}", entitlement_id);
        return false;
    }
    for (s32 i = 0; i < addcont_count; i++) {
        if (addcont_info[i].service_label != service_label) {
            continue;
        }
        const std::string_view label{
            addcont_info[i].entitlement_label,
            strnlen(addcont_info[i].entitlement_label, ORBIS_NP_UNIFIED_ENTITLEMENT_LABEL_SIZE)};
        if (label == entitlement_id) {
            return false;
        }
    }
    return true;
}

static void LoadAddonFolder(const std::filesystem::path& addon_path, u32 service_label) {
    if (!std::filesystem::exists(addon_path)) {
        return;
    }
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();

    for (const auto& entry : std::filesystem::directory_iterator(addon_path)) {
        if (!entry.is_directory() && entry.path().extension() != ".zar") {
            continue;
        }

        // Create a temporary filesystem backend to view the param.sfo
        std::shared_ptr<Core::FileSys::IBackend> fs = mnt->CreateBackend(entry.path(), true);

        // Look for a param.sfo in the additional content.
        auto psf_handle = fs->Open("sce_sys/param.sfo", Common::FS::FileAccessMode::Read);
        if (psf_handle == nullptr) {
            // This content doesn't have a param.sfo
            LOG_WARNING(Lib_AppContent, "Additional content {} has no param.sfo",
                        entry.path().filename().string());
            continue;
        }

        // Open the param.sfo in this content
        std::unique_ptr<PSF> dlc_params = std::make_unique<PSF>();
        if (!dlc_params->Open(psf_handle)) {
            // The param.sfo is invalid
            LOG_WARNING(Lib_AppContent, "Additional content {} has an invalid param.sfo",
                        entry.path().filename().string());
            continue;
        }

        auto category = dlc_params->GetString("CATEGORY");
        if (category.has_value() && strncmp(category.value().data(), "ac", 2) != 0) {
            LOG_WARNING(Lib_AppContent, "Additonal content {} is not additional content",
                        entry.path().filename().string());
            continue;
        }

        // Find the entitlement id from the content id.
        auto content_id = dlc_params->GetString("CONTENT_ID");
        if (!content_id.has_value()) {
            LOG_WARNING(Lib_AppContent, "Additonal content {} param.sfo is missing CONTENT_ID",
                        entry.path().filename().string());
            continue;
        }

        // Content IDs have consistent formatting, so this will always work.
        // They follow the format UPXXXX-CUSAXXXXX_XX-entitlement
        if (content_id.value().length() <= ORBIS_APP_CONTENT_ENTITLEMENT_LABEL_OFFSET) {
            LOG_WARNING(Lib_AppContent, "Additonal content {} param.sfo has malformed CONTENT_ID",
                        entry.path().filename().string());
            continue;
        }

        auto entitlement_id = content_id.value().substr(ORBIS_APP_CONTENT_ENTITLEMENT_LABEL_OFFSET);
        LOG_INFO(Lib_AppContent, "Entitlement {} found for service_label {}", entitlement_id,
                 service_label);

        // Save the additional content info in addcont_info.
        if (!CanAddEntitlement(service_label, entitlement_id)) {
            continue;
        }
        auto& info = addcont_info[addcont_count++];
        info.service_label = service_label;
        entitlement_id.copy(info.entitlement_label, entitlement_id.length());
        info.status = OrbisAppContentAddcontDownloadStatus::Installed;

        // Attempt 1: Use license.info for entitlement key
        auto license_handle = fs->Open("sce_sys/license.info", Common::FS::FileAccessMode::Read);
        auto key_offset = ORBIS_LICENSE_INFO_ENTITLEMENT_KEY_OFFSET;
        if (!license_handle) {
            // Attempt 2: Use license.dat for entitlement key
            license_handle = fs->Open("sce_sys/license.dat", Common::FS::FileAccessMode::Read);
            key_offset = ORBIS_LICENSE_DAT_ENTITLEMENT_KEY_OFFSET;
        }
        // Read the entitlement key if available
        if (!license_handle ||
            !license_handle->Seek(key_offset, Common::FS::SeekOrigin::SetOrigin) ||
            !license_handle->Read(info.key.data, sizeof(info.key.data))) {
            LOG_WARNING(Lib_AppContent, "Could not load entitlement key for {}", entitlement_id);
        }
    }
}

static void LoadLicenses(std::string_view service_id, u32 service_label) {
    const std::filesystem::path& licenses_dir =
        Common::FS::GetUserPath(Common::FS::PathType::LicensesDir);
    std::filesystem::path ridx_path = licenses_dir / fmt::format("{}.idx", service_id);
    std::filesystem::path rifa_path = licenses_dir / fmt::format("{}.rif", service_id);

    if (!std::filesystem::exists(ridx_path) || !std::filesystem::exists(rifa_path)) {
        return;
    }

    RIF rif;
    if (!rif.Open(ridx_path, rifa_path)) {
        LOG_ERROR(Lib_AppContent, "Could not open license file for {}", service_id);
        return;
    }

    LOG_INFO(Lib_AppContent, "Checking license {} for additional entitlements", service_id);
    const auto& entitlements = rif.GetLicenseOnlyEntitlements();
    for (const auto& entitlement : entitlements) {
        LOG_INFO(Lib_AppContent, "License-only entitlement {} found for service_label {}",
                 entitlement.first, service_label);
        if (!CanAddEntitlement(service_label, entitlement.first)) {
            continue;
        }
        auto& info = addcont_info[addcont_count++];
        info.service_label = service_label;
        entitlement.first.copy(info.entitlement_label, entitlement.first.length());
        info.status = OrbisAppContentAddcontDownloadStatus::NoExtraData;
        std::memcpy(info.key.data, entitlement.second.data, sizeof(info.key.data));
    }
}

int PS4_SYSV_ABI sceAppContentInitialize(const OrbisAppContentInitParam* initParam,
                                         OrbisAppContentBootParam* bootParam) {
    if (sdk_ver >= Common::ElfInfo::FW_150 && is_initialized) {
        LOG_ERROR(Lib_AppContent, "Already initialized");
        return ORBIS_APP_CONTENT_ERROR_BUSY;
    }

    LOG_WARNING(Lib_AppContent, "(DUMMY) called");
    is_initialized = true;
    auto* param_sfo = Common::Singleton<PSF>::Instance();

    const auto addons_dir = EmulatorSettings.GetAddonInstallDir();
    if (const auto value = param_sfo->GetString("TITLE_ID"); value.has_value()) {
        title_id = *value;
    } else {
        UNREACHABLE_MSG("Failed to get TITLE_ID");
    }

    struct ServiceEntry {
        u32 service_label{};
        std::string service_id{};
    };
    std::vector<ServiceEntry> services;

    if (const auto content_id = param_sfo->GetString("CONTENT_ID"); content_id.has_value()) {
        services.push_back(
            ServiceEntry{0, std::string(content_id->substr(0, content_id->find_last_of('-')))});
    }

    for (u32 i = 1; i <= 7; i++) {
        const auto key = fmt::format("SERVICE_ID_ADDCONT_ADD_{}", i);
        const auto service_id = param_sfo->GetString(key);
        if (service_id.has_value() && !service_id->empty()) {
            LOG_INFO(Lib_AppContent, "Linked additional content service {} at slot {}", *service_id,
                     i);
            services.push_back(ServiceEntry{i, std::string(*service_id)});
        }
    }

    // Additional content folders: the game's own title ID first, then the title ID of each
    // linked service (service IDs look like UPXXXX-CUSAXXXXX_XX).
    addon_paths.clear();
    addcont_count = 0;
    for (const auto& srv : services) {
        std::filesystem::path path;
        if (srv.service_label == 0) {
            path = addons_dir / title_id;
        } else if (srv.service_id.length() >= 16) {
            path = addons_dir / srv.service_id.substr(7, 9);
        }
        if (!path.empty() && std::filesystem::exists(path)) {
            if (std::ranges::find(addon_paths, path) == addon_paths.end()) {
                addon_paths.push_back(path);
            }
            LoadAddonFolder(path, srv.service_label);
        }
        LoadLicenses(srv.service_id, srv.service_label);
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

int PS4_SYSV_ABI sceAppContentTemporaryDataMount(OrbisAppContentMountPoint* mountPoint) {
    return sceAppContentTemporaryDataMount2(ORBIS_APP_CONTENT_TEMPORARY_DATA_OPTION_FORMAT,
                                            mountPoint);
}

int PS4_SYSV_ABI sceAppContentTemporaryDataMount2(OrbisAppContentTemporaryDataOption option,
                                                  OrbisAppContentMountPoint* mountPoint) {
    if (!is_initialized) {
        return ORBIS_APP_CONTENT_ERROR_NOT_INITIALIZED;
    }

    if (mountPoint == nullptr) {
        return ORBIS_APP_CONTENT_ERROR_PARAMETER;
    }

    auto* param_sfo = Common::Singleton<PSF>::Instance();
    const auto tid = !title_id.empty() ? title_id : param_sfo->GetString("TITLE_ID").value_or("");
    const auto mount_temp_dir = Common::FS::GetUserPath(Common::FS::PathType::TempDataDir) / tid;

    std::error_code ec;
    if (option == ORBIS_APP_CONTENT_TEMPORARY_DATA_OPTION_FORMAT) {
        std::filesystem::remove_all(mount_temp_dir, ec);
    }
    std::filesystem::create_directories(mount_temp_dir, ec);

    static constexpr std::string_view TmpMount = "/temp0";
    TmpMount.copy(mountPoint->data, TmpMount.size());
    mountPoint->data[TmpMount.size()] = '\0';

    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    mnt->Mount(mount_temp_dir, std::string{TmpMount});

    LOG_INFO(Lib_AppContent, "option = {}, mountPoint = {}", option, mountPoint->data);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppContentTemporaryDataUnmount(const OrbisAppContentMountPoint* mountPoint) {
    if (!is_initialized) {
        return ORBIS_APP_CONTENT_ERROR_NOT_INITIALIZED;
    }

    if (mountPoint == nullptr) {
        return ORBIS_APP_CONTENT_ERROR_PARAMETER;
    }
    const std::string_view mp{mountPoint->data,
                              strnlen(mountPoint->data, sizeof(mountPoint->data))};
    LOG_INFO(Lib_AppContent, "mountPoint = {}", mp);
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    mnt->Unmount(std::string{mp});
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
