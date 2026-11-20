// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <atomic>
#include <bit>
#include <filesystem>
#include <mutex>
#include <optional>
#include <random>
#include <span>
#include <string>
#include <vector>

#include "common/endian.h"
#include "common/io_file.h"
#include "common/logging/log.h"
#include "common/path_util.h"
#include "common/singleton.h"
#include "core/file_format/psf.h"
#include "core/file_format/rif.h"
#include "core/file_sys/fs.h"
#include "core/libraries/libs.h"
#include "core/loader/symbols_resolver.h"
#include "np_entitlement_access.h"
#include "np_entitlement_access_error.h"

namespace Libraries::Np::NpEntitlementAccess {

static std::atomic<bool> is_initialized{false};
static std::atomic<u32> next_request_id{1};

static std::vector<std::string> GetCandidateServiceIds() {
    std::vector<std::string> service_ids;
    auto* param_sfo = Common::Singleton<PSF>::Instance();
    if (!param_sfo) {
        return service_ids;
    }

    const auto& licenses_dir = Common::FS::GetUserPath(Common::FS::PathType::LicensesDir);
    if (!std::filesystem::exists(licenses_dir)) {
        return service_ids;
    }

    if (const auto cid = param_sfo->GetString("CONTENT_ID"); cid.has_value() && !cid->empty()) {
        auto pos = cid->find_last_of('-');
        if (pos != std::string_view::npos) {
            service_ids.emplace_back(cid->substr(0, pos));
        }
        service_ids.emplace_back(*cid);
    }

    if (const auto tid = param_sfo->GetString("TITLE_ID"); tid.has_value() && !tid->empty()) {
        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator(licenses_dir, ec)) {
            const auto stem = entry.path().stem().string();
            if (stem.find(*tid) != std::string::npos && entry.path().extension() == ".idx") {
                service_ids.push_back(stem);
            }
        }
    }

    return service_ids;
}

static std::optional<u32> QueryLicenseSkuFlag() {
    auto* param_sfo = Common::Singleton<PSF>::Instance();
    if (param_sfo) {
        const auto app_type = param_sfo->GetInteger("APP_TYPE").value_or(0);
        const auto category = param_sfo->GetString("CATEGORY").value_or("");
        if (app_type == 2 || category == "gd") {
            return 0;
        }
    }

    const auto& licenses_dir = Common::FS::GetUserPath(Common::FS::PathType::LicensesDir);
    if (!std::filesystem::exists(licenses_dir)) {
        return std::nullopt;
    }

    const auto service_ids = GetCandidateServiceIds();
    for (const auto& service_id : service_ids) {
        std::filesystem::path idx_path = licenses_dir / fmt::format("{}.idx", service_id);
        std::filesystem::path rif_path = licenses_dir / fmt::format("{}.rif", service_id);

        if (std::filesystem::exists(idx_path) && std::filesystem::exists(rif_path)) {
            RIF rif;
            if (rif.Open(idx_path, rif_path)) {
                if (auto sku = rif.GetSkuFlag(); sku.has_value()) {
                    LOG_INFO(Lib_NpCommon, "Found SKU flag 0x{:x} from license container {}", *sku,
                             service_id);
                    return sku;
                }
            }
        }

        if (std::filesystem::exists(rif_path)) {
            Common::FS::IOFile f(rif_path, Common::FS::FileAccessMode::Read);
            if (f.IsOpen() && f.GetSize() >= sizeof(RIFData)) {
                RIFData r{};
                if (f.Read(r) > 0 && r.magic == RIF_MAGIC) {
                    u16 val_le = static_cast<u16>(r.sku_flag);
                    u16 val_be = std::byteswap(val_le);
                    u32 flag = (val_be != 0 && val_be < 0x100) ? val_be : val_le;
                    if (flag != 0) {
                        LOG_INFO(Lib_NpCommon, "Found SKU flag 0x{:x} from single RIF {}", flag,
                                 service_id);
                        return flag;
                    }
                }
            }
        }
    }

    return std::nullopt;
}

static bool QueryLicenseEntitlementKey(std::string_view label, std::span<u8, 16> out_key) {
    const auto& licenses_dir = Common::FS::GetUserPath(Common::FS::PathType::LicensesDir);
    if (!std::filesystem::exists(licenses_dir)) {
        return false;
    }

    const auto service_ids = GetCandidateServiceIds();
    for (const auto& service_id : service_ids) {
        std::filesystem::path idx_path = licenses_dir / fmt::format("{}.idx", service_id);
        std::filesystem::path rif_path = licenses_dir / fmt::format("{}.rif", service_id);

        if (std::filesystem::exists(idx_path) && std::filesystem::exists(rif_path)) {
            RIF rif;
            if (rif.Open(idx_path, rif_path)) {
                if (rif.GetEntitlementKey(label, out_key)) {
                    LOG_INFO(Lib_NpCommon, "Found entitlement key for {} in {}", label, service_id);
                    return true;
                }
            }
        }
    }

    // Fallback: search all .idx files in licenses_dir if candidate service IDs didn't match
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(licenses_dir, ec)) {
        if (entry.path().extension() == ".idx") {
            const auto base_path = entry.path().parent_path() / entry.path().stem();
            const auto rif_path = base_path.string() + ".rif";
            if (std::filesystem::exists(rif_path)) {
                RIF rif;
                if (rif.Open(entry.path(), rif_path)) {
                    if (rif.GetEntitlementKey(label, out_key)) {
                        LOG_INFO(Lib_NpCommon, "Found entitlement key for {} in fallback {}", label,
                                 entry.path().stem().string());
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

static bool QueryLicenseEntitlementExists(std::string_view label) {
    u8 dummy_key[16];
    return QueryLicenseEntitlementKey(label, dummy_key);
}

void PS4_SYSV_ABI dummy() {}

int PS4_SYSV_ABI sceNpEntitlementAccessInitialize(const void* param, const void* reserved) {
    LOG_INFO(Lib_NpCommon, "called");
    if (is_initialized.exchange(true)) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_ALREADY_INITIALIZED;
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpEntitlementAccessGetSkuFlag(u32* flag) {
    if (!is_initialized) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_NOT_INITIALIZED;
    }
    if (!flag) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_INVALID_ARGUMENT;
    }

    auto sku = QueryLicenseSkuFlag();
    if (!sku.has_value()) {
        LOG_WARNING(Lib_NpCommon, "No valid license or disc found for SKU flag");
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_NOT_FOUND;
    }
    *flag = *sku;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpEntitlementAccessGetAddcontEntitlementInfoList(int user_id,
                                                                     const char* service_label,
                                                                     u32 limit, void* out_list) {
    LOG_INFO(Lib_NpCommon, "called (user={}, service={}, limit={})", user_id,
             service_label ? service_label : "null", limit);
    if (!is_initialized) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_NOT_INITIALIZED;
    }
    if (!out_list) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_INVALID_ARGUMENT;
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpEntitlementAccessGetAddcontEntitlementInfo(int user_id,
                                                                 const char* entitlement_label,
                                                                 void* out_info) {
    LOG_INFO(Lib_NpCommon, "called (user={}, label={})", user_id,
             entitlement_label ? entitlement_label : "null");
    if (!is_initialized) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_NOT_INITIALIZED;
    }
    if (!entitlement_label || !out_info) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_INVALID_ARGUMENT;
    }

    if (!QueryLicenseEntitlementExists(entitlement_label)) {
        LOG_WARNING(Lib_NpCommon, "Addcont entitlement {} not found", entitlement_label);
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_NOT_FOUND;
    }

    std::memset(out_info, 0, 0x1c);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpEntitlementAccessGetEntitlementKey(int user_id, const char* entitlement_label,
                                                         void* out_key) {
    LOG_INFO(Lib_NpCommon, "called (user={}, label={})", user_id,
             entitlement_label ? entitlement_label : "null");
    if (!is_initialized) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_NOT_INITIALIZED;
    }
    if (!entitlement_label || !out_key) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_INVALID_ARGUMENT;
    }

    std::span<u8, 16> key_span(reinterpret_cast<u8*>(out_key), 16);
    if (QueryLicenseEntitlementKey(entitlement_label, key_span)) {
        return ORBIS_OK;
    }

    LOG_WARNING(Lib_NpCommon, "Entitlement {} not found in licenses", entitlement_label);
    return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_NOT_FOUND;
}

int PS4_SYSV_ABI sceNpEntitlementAccessGenerateTransactionId(char* out_transaction_id) {
    LOG_INFO(Lib_NpCommon, "called");
    if (!is_initialized) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_NOT_INITIALIZED;
    }
    if (!out_transaction_id) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_INVALID_ARGUMENT;
    }

    static std::mt19937_64 rng(std::random_device{}());
    static std::uniform_int_distribution<u64> dist;
    u64 part1 = dist(rng);
    u64 part2 = dist(rng);

    // Standard UUID v4 format
    std::string uuid =
        fmt::format("{:08x}-{:04x}-4{:03x}-{:04x}-{:012x}", static_cast<u32>(part1 >> 32),
                    static_cast<u16>((part1 >> 16) & 0xffff), static_cast<u16>(part1 & 0x0fff),
                    static_cast<u16>((part2 >> 48) & 0x3fff) | 0x8000, part2 & 0xffffffffffffULL);
    std::memcpy(out_transaction_id, uuid.c_str(), uuid.size() + 1);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpEntitlementAccessRequestConsumeEntitlement(int user_id, u32 type,
                                                                 const char* entitlement_label,
                                                                 const char* transaction_id,
                                                                 u32 use_count, u32* request_id) {
    LOG_INFO(Lib_NpCommon, "called (user={}, type={}, label={}, tx={}, use_count={})", user_id,
             type, entitlement_label ? entitlement_label : "null",
             transaction_id ? transaction_id : "null", use_count);
    if (!is_initialized) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_NOT_INITIALIZED;
    }
    if (!request_id || !entitlement_label || !transaction_id) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_INVALID_ARGUMENT;
    }
    *request_id = next_request_id++;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpEntitlementAccessRequestConsumeUnifiedEntitlement(
    int user_id, u32 type, const char* entitlement_label, const char* transaction_id, u32 use_count,
    u32* request_id) {
    LOG_INFO(Lib_NpCommon, "called (user={}, type={}, label={}, tx={}, use_count={})", user_id,
             type, entitlement_label ? entitlement_label : "null",
             transaction_id ? transaction_id : "null", use_count);
    if (!is_initialized) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_NOT_INITIALIZED;
    }
    if (!request_id || !entitlement_label || !transaction_id) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_INVALID_ARGUMENT;
    }
    *request_id = next_request_id++;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpEntitlementAccessRequestConsumeServiceEntitlement(int user_id, u32 type,
                                                                        const char* service_label,
                                                                        const char* transaction_id,
                                                                        u32 use_count,
                                                                        u32* request_id) {
    LOG_INFO(Lib_NpCommon, "called (user={}, type={}, service={}, tx={}, use_count={})", user_id,
             type, service_label ? service_label : "null", transaction_id ? transaction_id : "null",
             use_count);
    if (!is_initialized) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_NOT_INITIALIZED;
    }
    if (!request_id || !service_label || !transaction_id) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_INVALID_ARGUMENT;
    }
    *request_id = next_request_id++;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpEntitlementAccessPollConsumeEntitlement(u32 request_id, int* out_status,
                                                              u32* out_remaining_count) {
    LOG_TRACE(Lib_NpCommon, "called (request_id={})", request_id);
    if (!is_initialized) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_NOT_INITIALIZED;
    }
    if (out_status) {
        *out_status = 0;
    }
    if (out_remaining_count) {
        *out_remaining_count = 0;
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpEntitlementAccessRequestConsumableEntitlementInfo(
    int user_id, u32 type, const char* entitlement_label, u32* request_id) {
    LOG_INFO(Lib_NpCommon, "called (user={}, type={}, label={})", user_id, type,
             entitlement_label ? entitlement_label : "null");
    if (!is_initialized) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_NOT_INITIALIZED;
    }
    if (!request_id || !entitlement_label) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_INVALID_ARGUMENT;
    }
    *request_id = next_request_id++;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpEntitlementAccessPollConsumableEntitlementInfo(u32 request_id,
                                                                     int* out_status,
                                                                     void* out_info) {
    LOG_TRACE(Lib_NpCommon, "called (request_id={})", request_id);
    if (!is_initialized) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_NOT_INITIALIZED;
    }
    if (out_status) {
        *out_status = 0;
    }
    if (out_info) {
        std::memset(out_info, 0, 0x40);
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpEntitlementAccessRequestUnifiedEntitlementInfo(int user_id, u32 type,
                                                                     const char* entitlement_label,
                                                                     u32* request_id) {
    LOG_INFO(Lib_NpCommon, "called (user={}, type={}, label={})", user_id, type,
             entitlement_label ? entitlement_label : "null");
    if (!is_initialized) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_NOT_INITIALIZED;
    }
    if (!request_id || !entitlement_label) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_INVALID_ARGUMENT;
    }
    *request_id = next_request_id++;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpEntitlementAccessPollUnifiedEntitlementInfo(u32 request_id, int* out_status,
                                                                  void* out_info) {
    LOG_TRACE(Lib_NpCommon, "called (request_id={})", request_id);
    if (!is_initialized) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_NOT_INITIALIZED;
    }
    if (out_status) {
        *out_status = 0;
    }
    if (out_info) {
        std::memset(out_info, 0, 0x40);
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpEntitlementAccessRequestUnifiedEntitlementInfoList(
    int user_id, u32 type, const char** labels, u32 num_labels, const void* opt, u32* request_id) {
    LOG_INFO(Lib_NpCommon, "called (user={}, type={}, num_labels={})", user_id, type, num_labels);
    if (!is_initialized) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_NOT_INITIALIZED;
    }
    if (!request_id) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_INVALID_ARGUMENT;
    }
    *request_id = next_request_id++;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpEntitlementAccessPollUnifiedEntitlementInfoList(
    u32 request_id, int* out_status, void* out_list, u32 limit, u32* out_count,
    u32* out_total_count, const void* opt) {
    LOG_TRACE(Lib_NpCommon, "called (request_id={}, limit={})", request_id, limit);
    if (!is_initialized) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_NOT_INITIALIZED;
    }
    if (out_status) {
        *out_status = 0;
    }
    if (out_count) {
        *out_count = 0;
    }
    if (out_total_count) {
        *out_total_count = 0;
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpEntitlementAccessRequestServiceEntitlementInfo(int user_id, u32 type,
                                                                     const char* service_label,
                                                                     u32* request_id) {
    LOG_INFO(Lib_NpCommon, "called (user={}, type={}, service={})", user_id, type,
             service_label ? service_label : "null");
    if (!is_initialized) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_NOT_INITIALIZED;
    }
    if (!request_id || !service_label) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_INVALID_ARGUMENT;
    }
    *request_id = next_request_id++;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpEntitlementAccessPollServiceEntitlementInfo(u32 request_id, int* out_status,
                                                                  void* out_info) {
    LOG_TRACE(Lib_NpCommon, "called (request_id={})", request_id);
    if (!is_initialized) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_NOT_INITIALIZED;
    }
    if (out_status) {
        *out_status = 0;
    }
    if (out_info) {
        std::memset(out_info, 0, 0x40);
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpEntitlementAccessRequestServiceEntitlementInfoList(
    int user_id, u32 type, const char** labels, u32 num_labels, const void* opt, u32* request_id) {
    LOG_INFO(Lib_NpCommon, "called (user={}, type={}, num_labels={})", user_id, type, num_labels);
    if (!is_initialized) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_NOT_INITIALIZED;
    }
    if (!request_id) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_INVALID_ARGUMENT;
    }
    *request_id = next_request_id++;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpEntitlementAccessPollServiceEntitlementInfoList(
    u32 request_id, int* out_status, void* out_list, u32 limit, u32* out_count,
    u32* out_total_count, const void* opt) {
    LOG_TRACE(Lib_NpCommon, "called (request_id={}, limit={})", request_id, limit);
    if (!is_initialized) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_NOT_INITIALIZED;
    }
    if (out_status) {
        *out_status = 0;
    }
    if (out_count) {
        *out_count = 0;
    }
    if (out_total_count) {
        *out_total_count = 0;
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpEntitlementAccessDeleteRequest(u32 request_id) {
    LOG_TRACE(Lib_NpCommon, "called (request_id={})", request_id);
    if (!is_initialized) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_NOT_INITIALIZED;
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpEntitlementAccessAbortRequest(u32 request_id) {
    LOG_TRACE(Lib_NpCommon, "called (request_id={})", request_id);
    if (!is_initialized) {
        return ORBIS_NP_ENTITLEMENT_ACCESS_ERROR_NOT_INITIALIZED;
    }
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("AS45QoYHjc4", "libSceNpEntitlementAccess", 1, "libSceNpEntitlementAccess", dummy);
    LIB_FUNCTION("jO8DM8oyego", "libSceNpEntitlementAccess", 1, "libSceNpEntitlementAccess",
                 sceNpEntitlementAccessInitialize);
    LIB_FUNCTION("lPDO62PpJIA", "libSceNpEntitlementAccess", 1, "libSceNpEntitlementAccess",
                 sceNpEntitlementAccessGetSkuFlag);
    LIB_FUNCTION("TFyU+KFBv54", "libSceNpEntitlementAccess", 1, "libSceNpEntitlementAccess",
                 sceNpEntitlementAccessGetAddcontEntitlementInfoList);
    LIB_FUNCTION("xddD23+8TfQ", "libSceNpEntitlementAccess", 1, "libSceNpEntitlementAccess",
                 sceNpEntitlementAccessGetAddcontEntitlementInfo);
    LIB_FUNCTION("5LiMEPuW0DQ", "libSceNpEntitlementAccess", 1, "libSceNpEntitlementAccess",
                 sceNpEntitlementAccessGetEntitlementKey);
    LIB_FUNCTION("IQtb-TaIjSM", "libSceNpEntitlementAccess", 1, "libSceNpEntitlementAccess",
                 sceNpEntitlementAccessGenerateTransactionId);
    LIB_FUNCTION("1c2E1ghXt90", "libSceNpEntitlementAccess", 1, "libSceNpEntitlementAccess",
                 sceNpEntitlementAccessRequestConsumeEntitlement);
    LIB_FUNCTION("eOxGbG3sPb0", "libSceNpEntitlementAccess", 1, "libSceNpEntitlementAccess",
                 sceNpEntitlementAccessRequestConsumeUnifiedEntitlement);
    LIB_FUNCTION("Aqae0TjLvQU", "libSceNpEntitlementAccess", 1, "libSceNpEntitlementAccess",
                 sceNpEntitlementAccessRequestConsumeServiceEntitlement);
    LIB_FUNCTION("69u+XqsoNd0", "libSceNpEntitlementAccess", 1, "libSceNpEntitlementAccess",
                 sceNpEntitlementAccessPollConsumeEntitlement);
    LIB_FUNCTION("RU6hHrrzpfc", "libSceNpEntitlementAccess", 1, "libSceNpEntitlementAccess",
                 sceNpEntitlementAccessRequestConsumableEntitlementInfo);
    LIB_FUNCTION("41q4BQrLOyM", "libSceNpEntitlementAccess", 1, "libSceNpEntitlementAccess",
                 sceNpEntitlementAccessPollConsumableEntitlementInfo);
    LIB_FUNCTION("jautSRs4OdQ", "libSceNpEntitlementAccess", 1, "libSceNpEntitlementAccess",
                 sceNpEntitlementAccessRequestUnifiedEntitlementInfo);
    LIB_FUNCTION("M4XlLFnzQaQ", "libSceNpEntitlementAccess", 1, "libSceNpEntitlementAccess",
                 sceNpEntitlementAccessPollUnifiedEntitlementInfo);
    LIB_FUNCTION("uCZf2L27th8", "libSceNpEntitlementAccess", 1, "libSceNpEntitlementAccess",
                 sceNpEntitlementAccessRequestUnifiedEntitlementInfoList);
    LIB_FUNCTION("nAEqawEZG5s", "libSceNpEntitlementAccess", 1, "libSceNpEntitlementAccess",
                 sceNpEntitlementAccessPollUnifiedEntitlementInfoList);
    LIB_FUNCTION("M8icY9OwkKs", "libSceNpEntitlementAccess", 1, "libSceNpEntitlementAccess",
                 sceNpEntitlementAccessRequestServiceEntitlementInfo);
    LIB_FUNCTION("ShGBFuoSSsQ", "libSceNpEntitlementAccess", 1, "libSceNpEntitlementAccess",
                 sceNpEntitlementAccessPollServiceEntitlementInfo);
    LIB_FUNCTION("brbRxzr7qyI", "libSceNpEntitlementAccess", 1, "libSceNpEntitlementAccess",
                 sceNpEntitlementAccessRequestServiceEntitlementInfoList);
    LIB_FUNCTION("aFv8qms6XTM", "libSceNpEntitlementAccess", 1, "libSceNpEntitlementAccess",
                 sceNpEntitlementAccessPollServiceEntitlementInfoList);
    LIB_FUNCTION("Z0eQj8m7XA8", "libSceNpEntitlementAccess", 1, "libSceNpEntitlementAccess",
                 sceNpEntitlementAccessDeleteRequest);
    LIB_FUNCTION("HFcQl9TMcFQ", "libSceNpEntitlementAccess", 1, "libSceNpEntitlementAccess",
                 sceNpEntitlementAccessAbortRequest);
}

} // namespace Libraries::Np::NpEntitlementAccess
