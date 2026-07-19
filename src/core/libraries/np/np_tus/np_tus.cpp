// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <map>
#include <core/libraries/np/np_error.h>
#include <core/libraries/np/np_handler.h>
#include <core/libraries/np/np_types.h>
#include <core/libraries/system/userservice.h>
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np/np_manager.h"
#include "core/libraries/np/np_tus/np_tus.h"
#include "np_tus_ctx.h"

namespace Libraries::Np::NpTus {

struct NpTusRequest {
    int titleCtxId = 0;
    std::shared_ptr<TusRequestCtx> ctx;
};

struct NpTusTitleContext {
    u32 serviceLabel = 0;
    OrbisNpId npId{};
};

constexpr size_t TusMaxTitleCtx = 32;
constexpr size_t TusMaxRequests = 256;

static std::mutex g_mutex;
static std::map<int, NpTusTitleContext> g_title_ctxs;
static std::map<int, NpTusRequest> g_requests;
static int g_next_ctx_id = 1; // 0 stays invalid
static int g_next_req_id = 1;

s32 GetRequest(int requestId, NpTusRequest** out) {
    std::lock_guard lock(g_mutex);
    auto it = g_requests.find(requestId);
    if (it == g_requests.end()) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }
    *out = &it->second;
    return ORBIS_OK;
}
static s32 ResolveTus(int requestId, NpTusRequest** req_out, u32* svc_out, s32* uid_out,
                      std::string* selfNpId_out) {
    OrbisNpId selfNp{};
    {
        std::lock_guard lock(g_mutex);
        auto rit = g_requests.find(requestId);
        if (rit == g_requests.end()) {
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
        }
        auto cit = g_title_ctxs.find(rit->second.titleCtxId);
        if (cit == g_title_ctxs.end()) {
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
        }
        *req_out = &rit->second;
        *svc_out = cit->second.serviceLabel;
        selfNp = cit->second.npId;
    }
    *uid_out = Libraries::Np::NpHandler::GetInstance().GetUserIdByOnlineId(selfNp.handle);
    if (selfNpId_out) {
        *selfNpId_out = std::string(selfNp.handle.data);
    }
    return ORBIS_OK;
}

//***********************************
// Title context management functions
//***********************************
s32 PS4_SYSV_ABI sceNpTusCreateNpTitleCtx(OrbisNpServiceLabel serviceLabel, OrbisNpId* npId) {
    if (!npId) {
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (serviceLabel == ORBIS_NP_INVALID_SERVICE_LABEL) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    LOG_INFO(Lib_NpTus, "serviceLabel = {}, npId->data = {}", serviceLabel, npId->handle.data);

    std::lock_guard lock(g_mutex);
    if (g_title_ctxs.size() >= TusMaxTitleCtx) {
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_OBJECTS;
    }
    const int id = g_next_ctx_id++;
    NpTusTitleContext tc;
    tc.serviceLabel = serviceLabel;
    tc.npId = *npId;
    g_title_ctxs.emplace(id, tc);
    return id;
}

s32 PS4_SYSV_ABI sceNpTusCreateNpTitleCtxA(OrbisNpServiceLabel serviceLabel,
                                           Libraries::UserService::OrbisUserServiceUserId userId) {
    LOG_INFO(Lib_NpTus, "serviceLabel = {}, userId = {}", serviceLabel, userId);
    OrbisNpId npId;
    auto ret = Np::NpManager::sceNpGetNpId(userId, &npId);

    if (ret < 0) {
        return ret;
    }

    return sceNpTusCreateNpTitleCtx(serviceLabel, &npId);
}

s32 PS4_SYSV_ABI sceNpTssCreateNpTitleCtxA(OrbisNpServiceLabel serviceLabel,
                                           Libraries::UserService::OrbisUserServiceUserId userId) {
    LOG_INFO(Lib_NpTus, "redirecting to sceNpTusCreateNpTitleCtxA");
    return sceNpTusCreateNpTitleCtxA(serviceLabel, userId);
}

//***********************************
// Request management functions
//***********************************
s32 PS4_SYSV_ABI sceNpTusCreateRequest(int libCtxId) {
    LOG_INFO(Lib_NpTus, "libCtxId = {}", libCtxId);

    std::lock_guard lock(g_mutex);
    if (!g_title_ctxs.count(libCtxId)) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }
    if (g_requests.size() >= TusMaxRequests) {
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_OBJECTS;
    }
    const int id = g_next_req_id++;
    NpTusRequest r;
    r.titleCtxId = libCtxId;
    g_requests.emplace(id, std::move(r));
    return id;
}

s32 PS4_SYSV_ABI sceNpTusDeleteRequest(int requestId) {
    LOG_INFO(Lib_NpTus, "requestId = {:#x}", requestId);

    std::lock_guard lock(g_mutex);
    auto it = g_requests.find(requestId);
    if (it == g_requests.end()) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }
    if (it->second.ctx) {
        it->second.ctx->SetResult(ORBIS_NP_COMMUNITY_ERROR_ABORTED);
    }
    g_requests.erase(it);
    return ORBIS_OK;
}
//***********************************
// Async functions
//***********************************
s32 PS4_SYSV_ABI sceNpTusPollAsync(int reqId, int* result) {
    LOG_INFO(Lib_NpTus, "reqId = {:#x}", reqId);

    std::shared_ptr<TusRequestCtx> ctx;
    {
        std::lock_guard glock(g_mutex);
        auto it = g_requests.find(reqId);
        if (it == g_requests.end()) {
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
        }
        ctx = it->second.ctx;
    }

    if (!ctx) {
        LOG_ERROR(Lib_NpTus, "request not started");
        return 1;
    }

    std::lock_guard lock(ctx->mutex);
    if (!ctx->result.has_value()) {
        return 1; // still running
    }
    if (result) {
        *result = *ctx->result;
    }
    LOG_DEBUG(Lib_NpTus, "request finished");
    return 0;
}

//***********************************
// TUS functions - WIP TODO
//***********************************
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableAsync(int reqId, OrbisNpId* npId, s32* slotIds,
                                                   s64* variables, u64 variablesSize, int arrayLen,
                                                   void* option) {
    LOG_INFO(Lib_NpTus,
              "reqId = {}, npId = {}, slotIds = {}, variables = {}, variablesSize = {}, arrayLen = "
              "{}, option = {}",
              reqId, npId ? npId->handle.data : "", fmt::ptr(slotIds), fmt::ptr(variables),
              variablesSize, arrayLen, fmt::ptr(option));
    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    if (!slotIds || !variables || arrayLen < 1) {
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    const std::string owner =
        (npId && npId->handle.data[0]) ? std::string(npId->handle.data) : self;
    std::vector<s32> slots(slotIds, slotIds + arrayLen);
    s64* out = variables;
    const int n = arrayLen;
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetMultiSlotVariable(
        uid, static_cast<s32>(svc), owner, std::string(), slots, /*variablesOut=*/nullptr,
        static_cast<u64>(n), ctx, out);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

//***********************************
// TSS functions - WIP TODO
//***********************************
s32 PS4_SYSV_ABI sceNpTssGetDataAsync(int reqId, s32 slotId, OrbisNpTssDataStatus* dataStatus,
                                      u64 dataStatusSize, void* data, u64 dataSize,
                                      OrbisNpTssGetDataOptParam* option) { // dummy atm TODO
    LOG_INFO(Lib_NpTus, "reqId = {:#x}, slotId = {}, dataStatusSize = {}, dataSize = {}", reqId,
             slotId, dataStatusSize, dataSize);

    if (option && option->size != 0x20) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    if (dataStatus && dataStatusSize != 0x18) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    if (slotId < 0 || slotId > 15) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    if (auto ret = GetRequest(reqId, &req); ret < 0) {
        return ret;
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    if (dataStatus) {
        dataStatus->status = OrbisNpTssStatus::Ok;
        dataStatus->contentLength = 0;
    }
    ctx->SetResult(0);

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTssCreateNpTitleCtx() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTssGetData() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTssGetSmallStorage() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTssGetSmallStorageAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTssGetStorage() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTssGetStorageAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusAbortRequest() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusAddAndGetVariable() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableA() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableAAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableAVUser() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableAVUserAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableForCrossSave() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableForCrossSaveAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableForCrossSaveVUser() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableForCrossSaveVUserAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableVUser() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableVUserAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusChangeModeForOtherSaveDataOwners() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusCreateTitleCtx() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotData() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotDataA() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotDataAAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotDataAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotDataVUser() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotDataVUserAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotVariable() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotVariableA() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotVariableAAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotVariableAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotVariableVUser() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotVariableVUserAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusDeleteNpTitleCtx() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetData() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetDataA() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetDataAAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetDataAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetDataAVUser() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetDataAVUserAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetDataForCrossSave() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetDataForCrossSaveAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetDataForCrossSaveVUser() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetDataForCrossSaveVUserAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetDataVUser() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetDataVUserAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetFriendsDataStatus() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetFriendsDataStatusA() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetFriendsDataStatusAAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetFriendsDataStatusAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetFriendsDataStatusForCrossSave() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetFriendsDataStatusForCrossSaveAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetFriendsVariable() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetFriendsVariableA() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetFriendsVariableAAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetFriendsVariableAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetFriendsVariableForCrossSave() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetFriendsVariableForCrossSaveAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatus() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusA() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusAAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusAVUser() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusAVUserAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusForCrossSave() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusForCrossSaveAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusForCrossSaveVUser() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusForCrossSaveVUserAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusVUser() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusVUserAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariable() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableA() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableAAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableAVUser() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableAVUserAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableForCrossSave() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableForCrossSaveAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableForCrossSaveVUser() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableForCrossSaveVUserAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableVUser() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableVUserAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatus() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusA() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusAAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusAVUser() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusAVUserAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusForCrossSave() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusForCrossSaveAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusForCrossSaveVUser() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusForCrossSaveVUserAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusVUser() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusVUserAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariable() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableA() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableAAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableAVUser() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableAVUserAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableForCrossSave() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableForCrossSaveAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableForCrossSaveVUser() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableForCrossSaveVUserAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableVUser() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableVUserAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusSetData() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusSetDataA() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusSetDataAAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusSetDataAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusSetDataAVUser() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusSetDataAVUserAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusSetDataVUser() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusSetDataVUserAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusSetMultiSlotVariable() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusSetMultiSlotVariableA() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusSetMultiSlotVariableAAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusSetMultiSlotVariableAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusSetMultiSlotVariableVUser() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusSetMultiSlotVariableVUserAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusSetThreadParam() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusSetTimeout() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusTryAndSetVariable() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableA() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableAAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableAVUser() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableAVUserAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableForCrossSave() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableForCrossSaveAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableForCrossSaveVUser() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableForCrossSaveVUserAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableVUser() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableVUserAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusWaitAsync() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("sRVb2Cf0GHg", "libSceNpTus", 1, "libSceNpTus", sceNpTssCreateNpTitleCtx);
    LIB_FUNCTION("lBtrk+7lk14", "libSceNpTus", 1, "libSceNpTus", sceNpTssCreateNpTitleCtxA);
    LIB_FUNCTION("-SUR+UoLS6c", "libSceNpTus", 1, "libSceNpTus", sceNpTssGetData);
    LIB_FUNCTION("DS2yu3Sjj1o", "libSceNpTus", 1, "libSceNpTus", sceNpTssGetDataAsync);
    LIB_FUNCTION("lL+Z3zCKNTs", "libSceNpTus", 1, "libSceNpTus", sceNpTssGetSmallStorage);
    LIB_FUNCTION("f2Pe4LGS2II", "libSceNpTus", 1, "libSceNpTus", sceNpTssGetSmallStorageAsync);
    LIB_FUNCTION("IVSbAEOxJ6I", "libSceNpTus", 1, "libSceNpTus", sceNpTssGetStorage);
    LIB_FUNCTION("k5NZIzggbuk", "libSceNpTus", 1, "libSceNpTus", sceNpTssGetStorageAsync);
    LIB_FUNCTION("2eq1bMwgZYo", "libSceNpTus", 1, "libSceNpTus", sceNpTusAbortRequest);
    LIB_FUNCTION("cRVmNrJDbG8", "libSceNpTus", 1, "libSceNpTus", sceNpTusAddAndGetVariable);
    LIB_FUNCTION("wPFah4-5Xec", "libSceNpTus", 1, "libSceNpTus", sceNpTusAddAndGetVariableA);
    LIB_FUNCTION("2dB427dT3Iw", "libSceNpTus", 1, "libSceNpTus", sceNpTusAddAndGetVariableAAsync);
    LIB_FUNCTION("Q2UmHdK04c8", "libSceNpTus", 1, "libSceNpTus", sceNpTusAddAndGetVariableAsync);
    LIB_FUNCTION("Nt1runsPVJc", "libSceNpTus", 1, "libSceNpTus", sceNpTusAddAndGetVariableAVUser);
    LIB_FUNCTION("GjlEgLCh4DY", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusAddAndGetVariableAVUserAsync);
    LIB_FUNCTION("EPeq43CQKxY", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusAddAndGetVariableForCrossSave);
    LIB_FUNCTION("mXZi1D2xwZE", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusAddAndGetVariableForCrossSaveAsync);
    LIB_FUNCTION("4VLlu7EIjzk", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusAddAndGetVariableForCrossSaveVUser);
    LIB_FUNCTION("6Lu9geO5TiA", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusAddAndGetVariableForCrossSaveVUserAsync);
    LIB_FUNCTION("ukr6FBSrkJw", "libSceNpTus", 1, "libSceNpTus", sceNpTusAddAndGetVariableVUser);
    LIB_FUNCTION("lliK9T6ylJg", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusAddAndGetVariableVUserAsync);
    LIB_FUNCTION("wjNhItL2wzg", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusChangeModeForOtherSaveDataOwners);
    LIB_FUNCTION("BIkMmUfNKWM", "libSceNpTus", 1, "libSceNpTus", sceNpTusCreateNpTitleCtx);
    LIB_FUNCTION("1n-dGukBgnY", "libSceNpTus", 1, "libSceNpTus", sceNpTusCreateNpTitleCtxA);
    LIB_FUNCTION("3bh2aBvvmvM", "libSceNpTus", 1, "libSceNpTus", sceNpTusCreateRequest);
    LIB_FUNCTION("hhy8+oecGac", "libSceNpTus", 1, "libSceNpTus", sceNpTusCreateTitleCtx);
    LIB_FUNCTION("0DT5bP6YzBo", "libSceNpTus", 1, "libSceNpTus", sceNpTusDeleteMultiSlotData);
    LIB_FUNCTION("iXzUOM9sXU0", "libSceNpTus", 1, "libSceNpTus", sceNpTusDeleteMultiSlotDataA);
    LIB_FUNCTION("6-+Yqc-NppQ", "libSceNpTus", 1, "libSceNpTus", sceNpTusDeleteMultiSlotDataAAsync);
    LIB_FUNCTION("OCozl1ZtxRY", "libSceNpTus", 1, "libSceNpTus", sceNpTusDeleteMultiSlotDataAsync);
    LIB_FUNCTION("xutwCvsydkk", "libSceNpTus", 1, "libSceNpTus", sceNpTusDeleteMultiSlotDataVUser);
    LIB_FUNCTION("zDeH4tr+0cQ", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusDeleteMultiSlotDataVUserAsync);
    LIB_FUNCTION("mYhbiRtkE1Y", "libSceNpTus", 1, "libSceNpTus", sceNpTusDeleteMultiSlotVariable);
    LIB_FUNCTION("pwnE9Oa1uF8", "libSceNpTus", 1, "libSceNpTus", sceNpTusDeleteMultiSlotVariableA);
    LIB_FUNCTION("NQIw7tzo0Ow", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusDeleteMultiSlotVariableAAsync);
    LIB_FUNCTION("0nDVqcYECoM", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusDeleteMultiSlotVariableAsync);
    LIB_FUNCTION("o02Mtf8G6V0", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusDeleteMultiSlotVariableVUser);
    LIB_FUNCTION("WCzd3cxhubo", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusDeleteMultiSlotVariableVUserAsync);
    LIB_FUNCTION("H3uq7x0sZOI", "libSceNpTus", 1, "libSceNpTus", sceNpTusDeleteNpTitleCtx);
    LIB_FUNCTION("CcIH40dYS88", "libSceNpTus", 1, "libSceNpTus", sceNpTusDeleteRequest);
    LIB_FUNCTION("XOzszO4ONWU", "libSceNpTus", 1, "libSceNpTus", sceNpTusGetData);
    LIB_FUNCTION("yWEHUFkY1qI", "libSceNpTus", 1, "libSceNpTus", sceNpTusGetDataA);
    LIB_FUNCTION("xzG8mG9YlKY", "libSceNpTus", 1, "libSceNpTus", sceNpTusGetDataAAsync);
    LIB_FUNCTION("uHtKS5V1T5k", "libSceNpTus", 1, "libSceNpTus", sceNpTusGetDataAsync);
    LIB_FUNCTION("iaH+Sxlw32k", "libSceNpTus", 1, "libSceNpTus", sceNpTusGetDataAVUser);
    LIB_FUNCTION("uoFvgzwawAY", "libSceNpTus", 1, "libSceNpTus", sceNpTusGetDataAVUserAsync);
    LIB_FUNCTION("1TE3OvH61qo", "libSceNpTus", 1, "libSceNpTus", sceNpTusGetDataForCrossSave);
    LIB_FUNCTION("CFPx3eyaT34", "libSceNpTus", 1, "libSceNpTus", sceNpTusGetDataForCrossSaveAsync);
    LIB_FUNCTION("-LxFGYCJwww", "libSceNpTus", 1, "libSceNpTus", sceNpTusGetDataForCrossSaveVUser);
    LIB_FUNCTION("B7rBR0CoYLI", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetDataForCrossSaveVUserAsync);
    LIB_FUNCTION("GQHCksS7aLs", "libSceNpTus", 1, "libSceNpTus", sceNpTusGetDataVUser);
    LIB_FUNCTION("5R6kI-8f+Hk", "libSceNpTus", 1, "libSceNpTus", sceNpTusGetDataVUserAsync);
    LIB_FUNCTION("DXigwIBTjWE", "libSceNpTus", 1, "libSceNpTus", sceNpTusGetFriendsDataStatus);
    LIB_FUNCTION("yixh7HDKWfk", "libSceNpTus", 1, "libSceNpTus", sceNpTusGetFriendsDataStatusA);
    LIB_FUNCTION("OheijxY5RYE", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetFriendsDataStatusAAsync);
    LIB_FUNCTION("LUwvy0MOSqw", "libSceNpTus", 1, "libSceNpTus", sceNpTusGetFriendsDataStatusAsync);
    LIB_FUNCTION("TDoqRD+CE+M", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetFriendsDataStatusForCrossSave);
    LIB_FUNCTION("68B6XDgSANk", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetFriendsDataStatusForCrossSaveAsync);
    LIB_FUNCTION("cy+pAALkHp8", "libSceNpTus", 1, "libSceNpTus", sceNpTusGetFriendsVariable);
    LIB_FUNCTION("C8TY-UnQoXg", "libSceNpTus", 1, "libSceNpTus", sceNpTusGetFriendsVariableA);
    LIB_FUNCTION("wrImtTqUSGM", "libSceNpTus", 1, "libSceNpTus", sceNpTusGetFriendsVariableAAsync);
    LIB_FUNCTION("YFYWOwYI6DY", "libSceNpTus", 1, "libSceNpTus", sceNpTusGetFriendsVariableAsync);
    LIB_FUNCTION("mD6s8HtMdpk", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetFriendsVariableForCrossSave);
    LIB_FUNCTION("FabW3QpY3gQ", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetFriendsVariableForCrossSaveAsync);
    LIB_FUNCTION("pgcNwFHoOL4", "libSceNpTus", 1, "libSceNpTus", sceNpTusGetMultiSlotDataStatus);
    LIB_FUNCTION("833Y2TnyonE", "libSceNpTus", 1, "libSceNpTus", sceNpTusGetMultiSlotDataStatusA);
    LIB_FUNCTION("7uLPqiNvNLc", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiSlotDataStatusAAsync);
    LIB_FUNCTION("Qyek420uZmM", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiSlotDataStatusAsync);
    LIB_FUNCTION("azmjx3jBAZA", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiSlotDataStatusAVUser);
    LIB_FUNCTION("668Ij9MYKEU", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiSlotDataStatusAVUserAsync);
    LIB_FUNCTION("DgpRToHWN40", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiSlotDataStatusForCrossSave);
    LIB_FUNCTION("LQ6CoHcp+ug", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiSlotDataStatusForCrossSaveAsync);
    LIB_FUNCTION("KBfBmtxCdmI", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiSlotDataStatusForCrossSaveVUser);
    LIB_FUNCTION("4UF2uu2eDCo", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiSlotDataStatusForCrossSaveVUserAsync);
    LIB_FUNCTION("NGCeFUl5ckM", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiSlotDataStatusVUser);
    LIB_FUNCTION("bHWFSg6jvXc", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiSlotDataStatusVUserAsync);
    LIB_FUNCTION("F+eQlfcka98", "libSceNpTus", 1, "libSceNpTus", sceNpTusGetMultiSlotVariable);
    LIB_FUNCTION("GDXlRTxgd+M", "libSceNpTus", 1, "libSceNpTus", sceNpTusGetMultiSlotVariableA);
    LIB_FUNCTION("2BnPSY1Oxd8", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiSlotVariableAAsync);
    LIB_FUNCTION("bcPB2rnhQqo", "libSceNpTus", 1, "libSceNpTus", sceNpTusGetMultiSlotVariableAsync);
    LIB_FUNCTION("AsziNQ9X2uk", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiSlotVariableAVUser);
    LIB_FUNCTION("y-DJK+d+leg", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiSlotVariableAVUserAsync);
    LIB_FUNCTION("m9XZnxw9AmE", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiSlotVariableForCrossSave);
    LIB_FUNCTION("DFlBYT+Lm2I", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiSlotVariableForCrossSaveAsync);
    LIB_FUNCTION("wTuuw4-6HI8", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiSlotVariableForCrossSaveVUser);
    LIB_FUNCTION("DPcu0qWsd7Q", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiSlotVariableForCrossSaveVUserAsync);
    LIB_FUNCTION("uFxVYJEkcmc", "libSceNpTus", 1, "libSceNpTus", sceNpTusGetMultiSlotVariableVUser);
    LIB_FUNCTION("qp-rTrq1klk", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiSlotVariableVUserAsync);
    LIB_FUNCTION("NvHjFkx2rnU", "libSceNpTus", 1, "libSceNpTus", sceNpTusGetMultiUserDataStatus);
    LIB_FUNCTION("lxNDPDnWfMc", "libSceNpTus", 1, "libSceNpTus", sceNpTusGetMultiUserDataStatusA);
    LIB_FUNCTION("kt+k6jegYZ8", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiUserDataStatusAAsync);
    LIB_FUNCTION("0zkr0T+NYvI", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiUserDataStatusAsync);
    LIB_FUNCTION("fJU2TZId210", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiUserDataStatusAVUser);
    LIB_FUNCTION("WBh3zfrjS38", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiUserDataStatusAVUserAsync);
    LIB_FUNCTION("cVeBif6zdZ4", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiUserDataStatusForCrossSave);
    LIB_FUNCTION("lq0Anwhj0wY", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiUserDataStatusForCrossSaveAsync);
    LIB_FUNCTION("w-c7U0MW2KY", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiUserDataStatusForCrossSaveVUser);
    LIB_FUNCTION("H6sQJ99usfE", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiUserDataStatusForCrossSaveVUserAsync);
    LIB_FUNCTION("xwJIlK0bHgA", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiUserDataStatusVUser);
    LIB_FUNCTION("I5dlIKkHNkQ", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiUserDataStatusVUserAsync);
    LIB_FUNCTION("6G9+4eIb+cY", "libSceNpTus", 1, "libSceNpTus", sceNpTusGetMultiUserVariable);
    LIB_FUNCTION("Gjixv5hqRVY", "libSceNpTus", 1, "libSceNpTus", sceNpTusGetMultiUserVariableA);
    LIB_FUNCTION("eGunerNP9n0", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiUserVariableAAsync);
    LIB_FUNCTION("YRje5yEXS0U", "libSceNpTus", 1, "libSceNpTus", sceNpTusGetMultiUserVariableAsync);
    LIB_FUNCTION("fVvocpq4mG4", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiUserVariableAVUser);
    LIB_FUNCTION("V8ZA3hHrAbw", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiUserVariableAVUserAsync);
    LIB_FUNCTION("Q5uQeScvTPE", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiUserVariableForCrossSave);
    LIB_FUNCTION("oZ8DMeTU-50", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiUserVariableForCrossSaveAsync);
    LIB_FUNCTION("Djuj2+1VNL0", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiUserVariableForCrossSaveVUser);
    LIB_FUNCTION("82RP7itI-zI", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiUserVariableForCrossSaveVUserAsync);
    LIB_FUNCTION("zB0vaHTzA6g", "libSceNpTus", 1, "libSceNpTus", sceNpTusGetMultiUserVariableVUser);
    LIB_FUNCTION("xZXQuNSTC6o", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusGetMultiUserVariableVUserAsync);
    LIB_FUNCTION("t7b6dmpQNiI", "libSceNpTus", 1, "libSceNpTus", sceNpTusPollAsync);
    LIB_FUNCTION("4NrufkNCkiE", "libSceNpTus", 1, "libSceNpTus", sceNpTusSetData);
    LIB_FUNCTION("VzxN3tOouj8", "libSceNpTus", 1, "libSceNpTus", sceNpTusSetDataA);
    LIB_FUNCTION("4u58d6g6uwU", "libSceNpTus", 1, "libSceNpTus", sceNpTusSetDataAAsync);
    LIB_FUNCTION("G68xdfQuiyU", "libSceNpTus", 1, "libSceNpTus", sceNpTusSetDataAsync);
    LIB_FUNCTION("kbWqOt3QjKU", "libSceNpTus", 1, "libSceNpTus", sceNpTusSetDataAVUser);
    LIB_FUNCTION("Fmx4tapJGzo", "libSceNpTus", 1, "libSceNpTus", sceNpTusSetDataAVUserAsync);
    LIB_FUNCTION("+RhzSuuXwxo", "libSceNpTus", 1, "libSceNpTus", sceNpTusSetDataVUser);
    LIB_FUNCTION("E4BCVfx-YfM", "libSceNpTus", 1, "libSceNpTus", sceNpTusSetDataVUserAsync);
    LIB_FUNCTION("c6aYoa47YgI", "libSceNpTus", 1, "libSceNpTus", sceNpTusSetMultiSlotVariable);
    LIB_FUNCTION("cf-WMA0jYCc", "libSceNpTus", 1, "libSceNpTus", sceNpTusSetMultiSlotVariableA);
    LIB_FUNCTION("ypMObSwfcns", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusSetMultiSlotVariableAAsync);
    LIB_FUNCTION("5J9GGMludxY", "libSceNpTus", 1, "libSceNpTus", sceNpTusSetMultiSlotVariableAsync);
    LIB_FUNCTION("1Cz0hTJFyh4", "libSceNpTus", 1, "libSceNpTus", sceNpTusSetMultiSlotVariableVUser);
    LIB_FUNCTION("CJAxTxQdwHM", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusSetMultiSlotVariableVUserAsync);
    LIB_FUNCTION("6GKDdRCFx8c", "libSceNpTus", 1, "libSceNpTus", sceNpTusSetThreadParam);
    LIB_FUNCTION("KMlHj+tgfdQ", "libSceNpTus", 1, "libSceNpTus", sceNpTusSetTimeout);
    LIB_FUNCTION("ukC55HsotJ4", "libSceNpTus", 1, "libSceNpTus", sceNpTusTryAndSetVariable);
    LIB_FUNCTION("0up4MP1wNtc", "libSceNpTus", 1, "libSceNpTus", sceNpTusTryAndSetVariableA);
    LIB_FUNCTION("bGTjTkHPHTE", "libSceNpTus", 1, "libSceNpTus", sceNpTusTryAndSetVariableAAsync);
    LIB_FUNCTION("xQfR51i4kck", "libSceNpTus", 1, "libSceNpTus", sceNpTusTryAndSetVariableAsync);
    LIB_FUNCTION("oGIcxlUabSA", "libSceNpTus", 1, "libSceNpTus", sceNpTusTryAndSetVariableAVUser);
    LIB_FUNCTION("uf77muc5Bog", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusTryAndSetVariableAVUserAsync);
    LIB_FUNCTION("MGvSJEHwyL8", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusTryAndSetVariableForCrossSave);
    LIB_FUNCTION("JKGYZ2F1yT8", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusTryAndSetVariableForCrossSaveAsync);
    LIB_FUNCTION("fcCwKpi4CbU", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusTryAndSetVariableForCrossSaveVUser);
    LIB_FUNCTION("CjVIpztpTNc", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusTryAndSetVariableForCrossSaveVUserAsync);
    LIB_FUNCTION("ZbitD262GhY", "libSceNpTus", 1, "libSceNpTus", sceNpTusTryAndSetVariableVUser);
    LIB_FUNCTION("trZ6QGW6jHs", "libSceNpTus", 1, "libSceNpTus",
                 sceNpTusTryAndSetVariableVUserAsync);
    LIB_FUNCTION("hYPJFWzFPjA", "libSceNpTus", 1, "libSceNpTus", sceNpTusWaitAsync);
    LIB_FUNCTION("sRVb2Cf0GHg", "libSceNpTusCompat", 1, "libSceNpTus", sceNpTssCreateNpTitleCtx);
    LIB_FUNCTION("cRVmNrJDbG8", "libSceNpTusCompat", 1, "libSceNpTus", sceNpTusAddAndGetVariable);
    LIB_FUNCTION("Q2UmHdK04c8", "libSceNpTusCompat", 1, "libSceNpTus",
                 sceNpTusAddAndGetVariableAsync);
    LIB_FUNCTION("ukr6FBSrkJw", "libSceNpTusCompat", 1, "libSceNpTus",
                 sceNpTusAddAndGetVariableVUser);
    LIB_FUNCTION("lliK9T6ylJg", "libSceNpTusCompat", 1, "libSceNpTus",
                 sceNpTusAddAndGetVariableVUserAsync);
    LIB_FUNCTION("BIkMmUfNKWM", "libSceNpTusCompat", 1, "libSceNpTus", sceNpTusCreateNpTitleCtx);
    LIB_FUNCTION("0DT5bP6YzBo", "libSceNpTusCompat", 1, "libSceNpTus", sceNpTusDeleteMultiSlotData);
    LIB_FUNCTION("OCozl1ZtxRY", "libSceNpTusCompat", 1, "libSceNpTus",
                 sceNpTusDeleteMultiSlotDataAsync);
    LIB_FUNCTION("mYhbiRtkE1Y", "libSceNpTusCompat", 1, "libSceNpTus",
                 sceNpTusDeleteMultiSlotVariable);
    LIB_FUNCTION("0nDVqcYECoM", "libSceNpTusCompat", 1, "libSceNpTus",
                 sceNpTusDeleteMultiSlotVariableAsync);
    LIB_FUNCTION("XOzszO4ONWU", "libSceNpTusCompat", 1, "libSceNpTus", sceNpTusGetData);
    LIB_FUNCTION("uHtKS5V1T5k", "libSceNpTusCompat", 1, "libSceNpTus", sceNpTusGetDataAsync);
    LIB_FUNCTION("GQHCksS7aLs", "libSceNpTusCompat", 1, "libSceNpTus", sceNpTusGetDataVUser);
    LIB_FUNCTION("5R6kI-8f+Hk", "libSceNpTusCompat", 1, "libSceNpTus", sceNpTusGetDataVUserAsync);
    LIB_FUNCTION("DXigwIBTjWE", "libSceNpTusCompat", 1, "libSceNpTus",
                 sceNpTusGetFriendsDataStatus);
    LIB_FUNCTION("LUwvy0MOSqw", "libSceNpTusCompat", 1, "libSceNpTus",
                 sceNpTusGetFriendsDataStatusAsync);
    LIB_FUNCTION("cy+pAALkHp8", "libSceNpTusCompat", 1, "libSceNpTus", sceNpTusGetFriendsVariable);
    LIB_FUNCTION("YFYWOwYI6DY", "libSceNpTusCompat", 1, "libSceNpTus",
                 sceNpTusGetFriendsVariableAsync);
    LIB_FUNCTION("pgcNwFHoOL4", "libSceNpTusCompat", 1, "libSceNpTus",
                 sceNpTusGetMultiSlotDataStatus);
    LIB_FUNCTION("Qyek420uZmM", "libSceNpTusCompat", 1, "libSceNpTus",
                 sceNpTusGetMultiSlotDataStatusAsync);
    LIB_FUNCTION("NGCeFUl5ckM", "libSceNpTusCompat", 1, "libSceNpTus",
                 sceNpTusGetMultiSlotDataStatusVUser);
    LIB_FUNCTION("bHWFSg6jvXc", "libSceNpTusCompat", 1, "libSceNpTus",
                 sceNpTusGetMultiSlotDataStatusVUserAsync);
    LIB_FUNCTION("F+eQlfcka98", "libSceNpTusCompat", 1, "libSceNpTus",
                 sceNpTusGetMultiSlotVariable);
    LIB_FUNCTION("bcPB2rnhQqo", "libSceNpTusCompat", 1, "libSceNpTus",
                 sceNpTusGetMultiSlotVariableAsync);
    LIB_FUNCTION("uFxVYJEkcmc", "libSceNpTusCompat", 1, "libSceNpTus",
                 sceNpTusGetMultiSlotVariableVUser);
    LIB_FUNCTION("qp-rTrq1klk", "libSceNpTusCompat", 1, "libSceNpTus",
                 sceNpTusGetMultiSlotVariableVUserAsync);
    LIB_FUNCTION("NvHjFkx2rnU", "libSceNpTusCompat", 1, "libSceNpTus",
                 sceNpTusGetMultiUserDataStatus);
    LIB_FUNCTION("0zkr0T+NYvI", "libSceNpTusCompat", 1, "libSceNpTus",
                 sceNpTusGetMultiUserDataStatusAsync);
    LIB_FUNCTION("xwJIlK0bHgA", "libSceNpTusCompat", 1, "libSceNpTus",
                 sceNpTusGetMultiUserDataStatusVUser);
    LIB_FUNCTION("I5dlIKkHNkQ", "libSceNpTusCompat", 1, "libSceNpTus",
                 sceNpTusGetMultiUserDataStatusVUserAsync);
    LIB_FUNCTION("6G9+4eIb+cY", "libSceNpTusCompat", 1, "libSceNpTus",
                 sceNpTusGetMultiUserVariable);
    LIB_FUNCTION("YRje5yEXS0U", "libSceNpTusCompat", 1, "libSceNpTus",
                 sceNpTusGetMultiUserVariableAsync);
    LIB_FUNCTION("zB0vaHTzA6g", "libSceNpTusCompat", 1, "libSceNpTus",
                 sceNpTusGetMultiUserVariableVUser);
    LIB_FUNCTION("xZXQuNSTC6o", "libSceNpTusCompat", 1, "libSceNpTus",
                 sceNpTusGetMultiUserVariableVUserAsync);
    LIB_FUNCTION("4NrufkNCkiE", "libSceNpTusCompat", 1, "libSceNpTus", sceNpTusSetData);
    LIB_FUNCTION("G68xdfQuiyU", "libSceNpTusCompat", 1, "libSceNpTus", sceNpTusSetDataAsync);
    LIB_FUNCTION("+RhzSuuXwxo", "libSceNpTusCompat", 1, "libSceNpTus", sceNpTusSetDataVUser);
    LIB_FUNCTION("E4BCVfx-YfM", "libSceNpTusCompat", 1, "libSceNpTus", sceNpTusSetDataVUserAsync);
    LIB_FUNCTION("c6aYoa47YgI", "libSceNpTusCompat", 1, "libSceNpTus",
                 sceNpTusSetMultiSlotVariable);
    LIB_FUNCTION("5J9GGMludxY", "libSceNpTusCompat", 1, "libSceNpTus",
                 sceNpTusSetMultiSlotVariableAsync);
    LIB_FUNCTION("ukC55HsotJ4", "libSceNpTusCompat", 1, "libSceNpTus", sceNpTusTryAndSetVariable);
    LIB_FUNCTION("xQfR51i4kck", "libSceNpTusCompat", 1, "libSceNpTus",
                 sceNpTusTryAndSetVariableAsync);
    LIB_FUNCTION("ZbitD262GhY", "libSceNpTusCompat", 1, "libSceNpTus",
                 sceNpTusTryAndSetVariableVUser);
    LIB_FUNCTION("trZ6QGW6jHs", "libSceNpTusCompat", 1, "libSceNpTus",
                 sceNpTusTryAndSetVariableVUserAsync);
};

} // namespace Libraries::Np::NpTus