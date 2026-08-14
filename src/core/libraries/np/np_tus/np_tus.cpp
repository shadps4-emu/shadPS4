// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstring>
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

// Installment state for the TUS blob-data
struct NpTusDataXfer {
    bool active = false;
    s32 slotId = -1;
    s64 accountId = 0;
    std::string virtualUser;
    std::vector<u8> sendBuf; // SetData: accumulated payload
    u64 totalSize = 0;       // SetData: totalSize from the first call
    std::vector<u8> info;    // SetData: info from the first call
    u64 recvOffset = 0;      // GetData: bytes already handed to the game

    bool SameTarget(s32 slot, s64 account, const std::string& vuser) const {
        return active && slotId == slot && accountId == account && virtualUser == vuser;
    }
    void Begin(s32 slot, s64 account, const std::string& vuser) {
        *this = NpTusDataXfer{};
        active = true;
        slotId = slot;
        accountId = account;
        virtualUser = vuser;
    }
};

struct NpTusRequest {
    int titleCtxId = 0;
    std::shared_ptr<TusRequestCtx> ctx;
    NpTusDataXfer xfer;
};

struct NpTusTitleContext {
    u32 serviceLabel = 0;
    OrbisNpId npId{};
};

constexpr size_t TusMaxTitleCtx = 32;
constexpr size_t TusMaxRequests = 256;
constexpr int TusMaxSlotsPerRequest = 64;
constexpr int TusMaxSelectedFriends = 100;
constexpr int TusMaxUsersPerRequest = 101;

static std::mutex g_mutex;
static std::map<int, NpTusTitleContext> g_title_ctxs;
static std::map<int, NpTusRequest> g_requests;

static s32 FakeAsyncComplete(int reqId,
                             s32 result = ORBIS_OK) { // temp function to complete async requests
    std::lock_guard lock(g_mutex);
    auto it = g_requests.find(reqId);
    if (it == g_requests.end()) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }
    auto ctx = std::make_shared<TusRequestCtx>();
    ctx->SetResult(result);
    it->second.ctx = std::move(ctx);
    return ORBIS_OK;
}
static int g_next_ctx_id = 1; // 0 stays invalid
static int g_next_req_id = 1;

s32 GetRequest(int requestId, NpTusRequest** out) {
    std::lock_guard lock(g_mutex);
    auto it = g_requests.find(requestId);
    if (it == g_requests.end()) {
        LOG_ERROR(Lib_NpTus, "Invalid requestId {}", requestId);
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
            LOG_ERROR(Lib_NpTus, "Invalid requestId {}", requestId);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
        }
        auto cit = g_title_ctxs.find(rit->second.titleCtxId);
        if (cit == g_title_ctxs.end()) {
            LOG_ERROR(Lib_NpTus, " Invalid titleCtxId {} for request {}", rit->second.titleCtxId,
                      requestId);
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
        LOG_ERROR(Lib_NpTus, "npId is null");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (serviceLabel == ORBIS_NP_INVALID_SERVICE_LABEL) {
        LOG_ERROR(Lib_NpTus, "Invalid serviceLabel {}", serviceLabel);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    LOG_INFO(Lib_NpTus, "serviceLabel = {}, npId->data = {}", serviceLabel, npId->handle.data);

    std::lock_guard lock(g_mutex);
    if (g_title_ctxs.size() >= TusMaxTitleCtx) {
        LOG_ERROR(Lib_NpTus, "Max title contexts reached");
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_OBJECTS;
    }
    const int id = g_next_ctx_id++;
    NpTusTitleContext tc;
    tc.serviceLabel = serviceLabel;
    tc.npId = *npId;
    g_title_ctxs.emplace(id, tc);
    return id;
}

s32 PS4_SYSV_ABI sceNpTssCreateNpTitleCtx(OrbisNpServiceLabel serviceLabel, OrbisNpId* npId) {
    LOG_INFO(Lib_NpTus, "redirecting to sceNpTusCreateNpTitleCtx");
    return sceNpTusCreateNpTitleCtx(serviceLabel, npId);
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

s32 PS4_SYSV_ABI sceNpTusDeleteNpTitleCtx(int ctxId) {
    LOG_INFO(Lib_NpTus, "ctxId = {}", ctxId);

    std::lock_guard lock(g_mutex);
    auto it = g_title_ctxs.find(ctxId);
    if (it == g_title_ctxs.end()) {
        LOG_ERROR(Lib_NpTus, "Invalid ctxId {}", ctxId);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }

    for (auto rit = g_requests.begin(); rit != g_requests.end();) {
        if (rit->second.titleCtxId == ctxId) {
            if (rit->second.ctx) {
                LOG_ERROR(Lib_NpTus, "Cannot delete title context {}: request {} is still active",
                          ctxId, rit->first);
                rit->second.ctx->SetResult(ORBIS_NP_COMMUNITY_ERROR_ABORTED);
            }
            rit = g_requests.erase(rit);
        } else {
            ++rit;
        }
    }
    g_title_ctxs.erase(it);
    return ORBIS_OK;
}

//***********************************
// Request management functions
//***********************************
s32 PS4_SYSV_ABI sceNpTusCreateRequest(int libCtxId) {
    LOG_INFO(Lib_NpTus, "libCtxId = {}", libCtxId);

    std::lock_guard lock(g_mutex);
    if (!g_title_ctxs.count(libCtxId)) {
        LOG_ERROR(Lib_NpTus, "Invalid libCtxId {}", libCtxId);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }
    if (g_requests.size() >= TusMaxRequests) {
        LOG_ERROR(Lib_NpTus, "Max requests reached");
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
        LOG_ERROR(Lib_NpTus, "Invalid requestId {}", requestId);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }
    if (it->second.ctx) {
        LOG_ERROR(Lib_NpTus, "Cannot delete request {}: request is still active", requestId);
        it->second.ctx->SetResult(ORBIS_NP_COMMUNITY_ERROR_ABORTED);
    }
    g_requests.erase(it);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusAbortRequest(int reqId) {
    LOG_INFO(Lib_NpTus, "reqId = {:#x}", reqId);

    std::shared_ptr<TusRequestCtx> ctx;
    {
        std::lock_guard lock(g_mutex);
        auto it = g_requests.find(reqId);
        if (it == g_requests.end()) {
            LOG_ERROR(Lib_NpTus, "Invalid requestId {}", reqId);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
        }
        ctx = it->second.ctx;
    }
    if (ctx) {
        ctx->SetResult(ORBIS_NP_COMMUNITY_ERROR_ABORTED);
    }
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
            LOG_ERROR(Lib_NpTus, "Invalid requestId {}", reqId);
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
    const s32 res = *ctx->result;
    if (result) {
        *result = res;
    }
    if (res < 0) {
        LOG_ERROR(Lib_NpTus, "reqId = {:#x} finished with error {:#x}", reqId,
                  static_cast<u32>(res));
    } else {
        LOG_DEBUG(Lib_NpTus, "reqId = {:#x} finished, result = {}", reqId, res);
    }
    return 0;
}

s32 PS4_SYSV_ABI sceNpTusWaitAsync(int reqId, int* result) {
    LOG_INFO(Lib_NpTus, "reqId = {:#x}", reqId);

    std::shared_ptr<TusRequestCtx> ctx;
    {
        std::lock_guard glock(g_mutex);
        auto it = g_requests.find(reqId);
        if (it == g_requests.end()) {
            LOG_ERROR(Lib_NpTus, "Invalid requestId {}", reqId);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
        }
        ctx = it->second.ctx;
    }

    if (!ctx) {
        LOG_ERROR(Lib_NpTus, "request not started");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    std::unique_lock lock(ctx->mutex);
    ctx->cv.wait(lock, [&] { return ctx->result.has_value(); });
    const s32 res = *ctx->result;
    if (result) {
        *result = res;
    }
    if (res < 0) {
        LOG_ERROR(Lib_NpTus, "reqId = {:#x} finished with error {:#x}", reqId,
                  static_cast<u32>(res));
    } else {
        LOG_DEBUG(Lib_NpTus, "reqId = {:#x} finished, result = {}", reqId, res);
    }
    return ORBIS_OK;
}

//***********************************
// TUS functions
//***********************************
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableAsync(int reqId, OrbisNpId* npId, s32* slotIds,
                                                   OrbisNpTusVariable* variableArray,
                                                   u64 variablesSize, int arrayLen, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, npId = {}, slotIds = {}, variableArray = {}, variablesSize = {}, "
             "arrayLen = {}, option = {}",
             reqId, npId ? npId->handle.data : "", fmt::ptr(slotIds), fmt::ptr(variableArray),
             variablesSize, arrayLen, fmt::ptr(option));
    if (!slotIds || !variableArray) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (arrayLen < 1 || option) {
        LOG_ERROR(Lib_NpTus, "Invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "Too many slots requested: {}", arrayLen);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slotIds, slotIds + arrayLen, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "Invalid slot ID found");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (variablesSize != static_cast<u64>(arrayLen) * sizeof(OrbisNpTusVariable)) {
        LOG_ERROR(Lib_NpTus,
                  "size mismatch: variablesSize={} but arrayLen {} * sizeof(OrbisNpTusVariable)={}",
                  variablesSize, arrayLen, static_cast<u64>(arrayLen) * sizeof(OrbisNpTusVariable));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const std::string owner =
        (npId && npId->handle.data[0]) ? std::string(npId->handle.data) : self;
    std::vector<s32> slots(slotIds, slotIds + arrayLen);
    const int n = arrayLen;
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetMultiSlotVariable(
        uid, static_cast<s32>(svc), owner, std::string(), slots, variableArray, static_cast<u64>(n),
        ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariable(int reqId, OrbisNpId* npId, s32* slotIds,
                                              OrbisNpTusVariable* variableArray, u64 variablesSize,
                                              int arrayLen, void* option) {
    auto ret = sceNpTusGetMultiSlotVariableAsync(reqId, npId, slotIds, variableArray, variablesSize,
                                                 arrayLen, option);
    if (ret < 0) {
        return ret;
    }

    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }

    return ret;
}

s32 PS4_SYSV_ABI sceNpTusSetMultiSlotVariableAsync(int reqId, OrbisNpId* npId, s32* slotIds,
                                                   s64* variables, int arrayLen, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, npId = {}, slotIds = {}, variables = {}, arrayLen = {}, option = {}",
             reqId, npId ? npId->handle.data : "", fmt::ptr(slotIds), fmt::ptr(variables), arrayLen,
             fmt::ptr(option));

    if (!slotIds || !variables) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (arrayLen < 1 || option) {
        LOG_ERROR(Lib_NpTus, "Invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "Too many slots requested: {}", arrayLen);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slotIds, slotIds + arrayLen, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "Invalid slot ID found");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const std::string owner =
        (npId && npId->handle.data[0]) ? std::string(npId->handle.data) : self;
    std::vector<s32> slots(slotIds, slotIds + arrayLen);
    std::vector<s64> vals(variables, variables + arrayLen);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusSetMultiSlotVariable(
        uid, static_cast<s32>(svc), owner, std::string(), slots, vals, ctx);
    if (submit < 0) {
        return submit;
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusSetMultiSlotVariable(int reqId, OrbisNpId* npId, s32* slotIds,
                                              s64* variables, int arrayLen, void* option) {
    auto ret = sceNpTusSetMultiSlotVariableAsync(reqId, npId, slotIds, variables, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableAVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* virtualUserId, s32* slotIds,
    OrbisNpTusVariableA* variableArray, u64 variablesSize, int arrayLen, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, virtualUserId = {}, slotIds = {}, variableArray = {}, "
             "variablesSize = {}, arrayLen = {}, option = {}",
             reqId, virtualUserId ? virtualUserId->data : "", fmt::ptr(slotIds),
             fmt::ptr(variableArray), variablesSize, arrayLen, fmt::ptr(option));
    if (!virtualUserId || !slotIds || !variableArray) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (arrayLen < 1 || option) {
        LOG_ERROR(Lib_NpTus, "Invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "Too many slots requested: {}", arrayLen);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slotIds, slotIds + arrayLen, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "Invalid slot ID found");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (variablesSize != static_cast<u64>(arrayLen) * sizeof(OrbisNpTusVariableA)) {
        LOG_ERROR(
            Lib_NpTus,
            "size mismatch: variablesSize={} but arrayLen {} * sizeof(OrbisNpTusVariableA)={}",
            variablesSize, arrayLen, static_cast<u64>(arrayLen) * sizeof(OrbisNpTusVariableA));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    const std::string virtualUser(virtualUserId->data,
                                  strnlen(virtualUserId->data, sizeof(virtualUserId->data)));
    if (virtualUser.empty()) {
        LOG_ERROR(Lib_NpTus, "Invalid virtual user ID");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slotIds, slotIds + arrayLen);
    const int n = arrayLen;
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetMultiSlotVariable(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), virtualUser, slots,
        /*variablesOut=*/nullptr, static_cast<u64>(n), ctx, /*variablesAOut=*/variableArray);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableAVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32* slotIds,
    OrbisNpTusVariableA* variables, u64 variablesSize, int arrayLen, void* option) {
    auto ret = sceNpTusGetMultiSlotVariableAVUserAsync(reqId, targetVirtualUserId, slotIds,
                                                       variables, variablesSize, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusSetMultiSlotVariableVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* virtualUserId, s32* slotIds, const s64* variables,
    int arrayLen, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, virtualUserId = {}, slotIds = {}, variables = {}, arrayLen = {}, "
             "option = {}",
             reqId, virtualUserId ? virtualUserId->data : "", fmt::ptr(slotIds),
             fmt::ptr(variables), arrayLen, fmt::ptr(option));
    if (!virtualUserId || !slotIds || !variables) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (arrayLen < 1 || option) {
        LOG_ERROR(Lib_NpTus, "Invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "Too many slots requested: {}", arrayLen);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slotIds, slotIds + arrayLen, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "Invalid slot ID found");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const std::string virtualUser(virtualUserId->data,
                                  strnlen(virtualUserId->data, sizeof(virtualUserId->data)));
    if (virtualUser.empty()) {
        LOG_ERROR(Lib_NpTus, "Invalid virtual user ID");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slotIds, slotIds + arrayLen);
    std::vector<s64> values(variables, variables + arrayLen);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusSetMultiSlotVariable(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), virtualUser, slots, values, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
sceNpTusSetMultiSlotVariableVUser(int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId,
                                  s32* slotIds, s64* variables, int arrayLen, void* option) {
    auto ret = sceNpTusSetMultiSlotVariableVUserAsync(reqId, targetVirtualUserId, slotIds,
                                                      variables, arrayLen, option);
    if (ret < 0) {
        return ret;
    }

    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }

    return ret;
}

s32 PS4_SYSV_ABI sceNpTusSetMultiSlotVariableAAsync(int reqId, OrbisNpAccountId targetAccountId,
                                                    s32* slotIds, const s64* variables,
                                                    int arrayLen, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetAccountId = {}, slotIds = {}, variables = {}, arrayLen = {}, "
             "option = {}",
             reqId, targetAccountId, fmt::ptr(slotIds), fmt::ptr(variables), arrayLen,
             fmt::ptr(option));
    if (!slotIds || !variables) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (arrayLen < 1 || option) {
        LOG_ERROR(Lib_NpTus, "Invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "Too many slots requested: {}", arrayLen);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slotIds, slotIds + arrayLen, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "Invalid slot ID found");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slotIds, slotIds + arrayLen);
    std::vector<s64> vals(variables, variables + arrayLen);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusSetMultiSlotVariable(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), /*virtualUser=*/std::string(),
        slots, vals, ctx, static_cast<s64>(targetAccountId));
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableAAsync(int reqId, OrbisNpAccountId targetAccountId,
                                                    s32* slotIds,
                                                    OrbisNpTusVariableA* variableArray,
                                                    u64 variablesSize, int arrayLen, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetAccountId = {}, slotIds = {}, variableArray = {}, "
             "variablesSize = {}, arrayLen = {}, option = {}",
             reqId, targetAccountId, fmt::ptr(slotIds), fmt::ptr(variableArray), variablesSize,
             arrayLen, fmt::ptr(option));
    if (!slotIds || !variableArray) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (arrayLen < 1 || option) {
        LOG_ERROR(Lib_NpTus, "Invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "Too many slots requested: {}", arrayLen);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slotIds, slotIds + arrayLen, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "Invalid slot ID found");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (variablesSize != static_cast<u64>(arrayLen) * sizeof(OrbisNpTusVariableA)) {
        LOG_ERROR(
            Lib_NpTus,
            "size mismatch: variablesSize={} but arrayLen {} * sizeof(OrbisNpTusVariableA)={}",
            variablesSize, arrayLen, static_cast<u64>(arrayLen) * sizeof(OrbisNpTusVariableA));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slotIds, slotIds + arrayLen);
    const int n = arrayLen;
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetMultiSlotVariable(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), /*virtualUser=*/std::string(),
        slots, /*variablesOut=*/nullptr, static_cast<u64>(n), ctx, /*variablesAOut=*/variableArray,
        static_cast<s64>(targetAccountId));
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetDataAAsync(int reqId, OrbisNpAccountId targetAccountId, s32 slotId,
                                       OrbisNpTusDataStatusA* dataStatus, u64 dataStatusSize,
                                       void* data, u64 recvSize, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetAccountId = {}, slotId = {}, dataStatus = {}, "
             "dataStatusSize = {}, data = {}, recvSize = {}, option = {}",
             reqId, targetAccountId, slotId, fmt::ptr(dataStatus), dataStatusSize, fmt::ptr(data),
             recvSize, fmt::ptr(option));
    if (option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (!dataStatus || dataStatusSize == 0) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (slotId < 0) {
        LOG_ERROR(Lib_NpTus, "Invalid slot ID");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }

    const s64 account = static_cast<s64>(targetAccountId);
    u64 offset = 0;
    {
        std::lock_guard lock(g_mutex);
        if (!req->xfer.SameTarget(slotId, account, std::string())) {
            req->xfer.Begin(slotId, account, std::string());
        }
        offset = req->xfer.recvOffset;
        if (data) {
            req->xfer.recvOffset += recvSize;
        }
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetData(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), /*virtualUser=*/std::string(),
        account, slotId, dataStatus, dataStatusSize, data, data ? recvSize : 0, offset, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusSetDataAAsync(int reqId, OrbisNpAccountId targetAccountId, s32 slotId,
                                       u64 totalSize, u64 sendSize, const void* data,
                                       const OrbisNpTusDataInfo* info, u64 infoStructSize,
                                       const OrbisNpAccountId* isLastChangedAuthor,
                                       const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
                                       void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetAccountId = {}, slotId = {}, totalSize = {}, sendSize = {}, "
             "data = {}, info = {}, infoStructSize = {}, option = {}",
             reqId, targetAccountId, slotId, totalSize, sendSize, fmt::ptr(data), fmt::ptr(info),
             infoStructSize, fmt::ptr(option));
    if (option) {
        LOG_ERROR(Lib_NpTus, "Invalid option provided");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (!data || sendSize == 0) {
        LOG_ERROR(Lib_NpTus, "Insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (slotId < 0 || sendSize > totalSize) {
        LOG_ERROR(Lib_NpTus, "Invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }

    const s64 account = static_cast<s64>(targetAccountId);
    std::vector<u8> blob;
    std::vector<u8> infoBytes;
    bool complete = false;
    {
        std::lock_guard lock(g_mutex);
        auto& x = req->xfer;
        if (!x.SameTarget(slotId, account, std::string())) {
            x.Begin(slotId, account, std::string());
            x.totalSize = totalSize;
            x.sendBuf.reserve(totalSize);
            if (info && infoStructSize >= sizeof(OrbisNpTusDataInfo)) {
                const u64 n = std::min<u64>(info->size, sizeof(info->data));
                infoBytes.assign(info->data, info->data + n);
                x.info = infoBytes;
            }
        }
        const u8* src = static_cast<const u8*>(data);
        x.sendBuf.insert(x.sendBuf.end(), src, src + sendSize);
        if (x.sendBuf.size() >= x.totalSize) {
            blob = x.sendBuf;
            infoBytes = x.info;
            complete = true;
            x = NpTusDataXfer{};
        }
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    if (!complete) {
        ctx->SetResult(ORBIS_OK);
        return ORBIS_OK;
    }

    const bool hasAuthorCheck = isLastChangedAuthor != nullptr;
    const bool hasDateCheck = isLastChangedDate != nullptr;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusSetData(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), /*virtualUser=*/std::string(),
        account, slotId, blob, infoBytes, hasAuthorCheck,
        hasAuthorCheck ? static_cast<s64>(*isLastChangedAuthor) : 0,
        /*isLastChangedAuthorNpId=*/std::string(), hasDateCheck,
        hasDateCheck ? isLastChangedDate->tick : 0, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetDataAVUserAsync(int reqId,
                                            const OrbisNpTusVirtualUserId* targetVirtualUserId,
                                            s32 slotId, OrbisNpTusDataStatusA* dataStatus,
                                            u64 dataStatusSize, void* data, u64 recvSize,
                                            void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetVirtualUserId = {}, slotId = {}, dataStatus = {}, "
             "dataStatusSize = {}, data = {}, recvSize = {}, option = {}",
             reqId, targetVirtualUserId ? targetVirtualUserId->data : "", slotId,
             fmt::ptr(dataStatus), dataStatusSize, fmt::ptr(data), recvSize, fmt::ptr(option));
    if (option) {
        LOG_ERROR(Lib_NpTus, "Invalid option provided");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (!targetVirtualUserId || !dataStatus || dataStatusSize == 0) {
        LOG_ERROR(Lib_NpTus, "Insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (slotId < 0) {
        LOG_ERROR(Lib_NpTus, "Invalid slot ID");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const std::string virtualUser(
        targetVirtualUserId->data,
        strnlen(targetVirtualUserId->data, sizeof(targetVirtualUserId->data)));
    if (virtualUser.empty()) {
        LOG_ERROR(Lib_NpTus, "Invalid virtual user ID");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }

    u64 offset = 0;
    {
        std::lock_guard lock(g_mutex);
        if (!req->xfer.SameTarget(slotId, 0, virtualUser)) {
            req->xfer.Begin(slotId, 0, virtualUser);
        }
        offset = req->xfer.recvOffset;
        if (data) {
            req->xfer.recvOffset += recvSize;
        }
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetData(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), virtualUser, /*ownerAccountId=*/0,
        slotId, dataStatus, dataStatusSize, data, data ? recvSize : 0, offset, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusSetDataAVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32 slotId, u64 totalSize,
    u64 sendSize, const void* data, const OrbisNpTusDataInfo* info, u64 infoStructSize,
    const OrbisNpAccountId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetVirtualUserId = {}, slotId = {}, totalSize = {}, sendSize = {}, "
             "data = {}, info = {}, infoStructSize = {}, option = {}",
             reqId, targetVirtualUserId ? targetVirtualUserId->data : "", slotId, totalSize,
             sendSize, fmt::ptr(data), fmt::ptr(info), infoStructSize, fmt::ptr(option));
    if (option) {
        LOG_ERROR(Lib_NpTus, "Invalid option provided");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (!targetVirtualUserId || !data || sendSize == 0) {
        LOG_ERROR(Lib_NpTus, "Insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (slotId < 0 || sendSize > totalSize) {
        LOG_ERROR(Lib_NpTus, "Invalid slot ID");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const std::string virtualUser(
        targetVirtualUserId->data,
        strnlen(targetVirtualUserId->data, sizeof(targetVirtualUserId->data)));
    if (virtualUser.empty()) {
        LOG_ERROR(Lib_NpTus, "Invalid virtual user ID");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }

    std::vector<u8> blob;
    std::vector<u8> infoBytes;
    bool complete = false;
    {
        std::lock_guard lock(g_mutex);
        auto& x = req->xfer;
        if (!x.SameTarget(slotId, 0, virtualUser)) {
            x.Begin(slotId, 0, virtualUser);
            x.totalSize = totalSize;
            x.sendBuf.reserve(totalSize);
            if (info && infoStructSize >= sizeof(OrbisNpTusDataInfo)) {
                const u64 n = std::min<u64>(info->size, sizeof(info->data));
                x.info.assign(info->data, info->data + n);
            }
        }
        const u8* src = static_cast<const u8*>(data);
        x.sendBuf.insert(x.sendBuf.end(), src, src + sendSize);
        if (x.sendBuf.size() >= x.totalSize) {
            blob = x.sendBuf;
            infoBytes = x.info;
            complete = true;
            x = NpTusDataXfer{};
        }
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    if (!complete) {
        ctx->SetResult(ORBIS_OK);
        return ORBIS_OK;
    }

    const bool hasAuthorCheck = isLastChangedAuthor != nullptr;
    const bool hasDateCheck = isLastChangedDate != nullptr;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusSetData(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), virtualUser, /*ownerAccountId=*/0,
        slotId, blob, infoBytes, hasAuthorCheck,
        hasAuthorCheck ? static_cast<s64>(*isLastChangedAuthor) : 0,
        /*isLastChangedAuthorNpId=*/std::string(), hasDateCheck,
        hasDateCheck ? isLastChangedDate->tick : 0, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusAddAndGetVariable(int reqId, const OrbisNpId* targetNpId, s32 slotId,
                                           s64 inVariable, const OrbisNpId* isLastChangedAuthor,
                                           const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
                                           OrbisNpTusVariable* outVariable, u64 outVariableSize,
                                           void* option) {
    auto ret =
        sceNpTusAddAndGetVariableAsync(reqId, targetNpId, slotId, inVariable, isLastChangedAuthor,
                                       isLastChangedDate, outVariable, outVariableSize, option);
    if (ret < 0) {
        return ret;
    }
    // Returns 0 on normal termination; the result value is in outVariable.
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableAsync(
    int reqId, const OrbisNpId* targetNpId, s32 slotId, s64 inVariable,
    const OrbisNpId* isLastChangedAuthor, const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
    OrbisNpTusVariable* outVariable, u64 outVariableSize, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetNpId = {}, slotId = {}, inVariable = {}, outVariable = {}, "
             "outVariableSize = {}, option = {}",
             reqId, targetNpId ? targetNpId->handle.data : "", slotId, inVariable,
             fmt::ptr(outVariable), outVariableSize, fmt::ptr(option));
    if (option) {
        LOG_ERROR(Lib_NpTus, "Invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (!outVariable) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (slotId < 0) {
        LOG_ERROR(Lib_NpTus, "Invalid slot ID");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (outVariableSize != sizeof(OrbisNpTusVariable)) {
        LOG_ERROR(Lib_NpTus, "size mismatch: outVariableSize={} but sizeof(OrbisNpTusVariable)={}",
                  outVariableSize, sizeof(OrbisNpTusVariable));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const std::string owner =
        (targetNpId && targetNpId->handle.data[0]) ? std::string(targetNpId->handle.data) : self;

    const bool hasAuthorCheck = isLastChangedAuthor != nullptr;
    const bool hasDateCheck = isLastChangedDate != nullptr;
    const std::string authorNpId =
        hasAuthorCheck ? std::string(isLastChangedAuthor->handle.data) : std::string();

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusAddAndGetVariable(
        uid, static_cast<s32>(svc), owner, /*virtualUser=*/std::string(), /*ownerAccountId=*/0,
        slotId, inVariable, hasAuthorCheck, /*isLastChangedAuthor=*/0, authorNpId, hasDateCheck,
        hasDateCheck ? isLastChangedDate->tick : 0, /*variableOut=*/outVariable,
        /*variableAOut=*/nullptr, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32 slotId, s64 inVariable,
    const OrbisNpId* isLastChangedAuthor, const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
    OrbisNpTusVariable* outVariable, u64 outVariableSize, void* option) {
    auto ret = sceNpTusAddAndGetVariableVUserAsync(reqId, targetVirtualUserId, slotId, inVariable,
                                                   isLastChangedAuthor, isLastChangedDate,
                                                   outVariable, outVariableSize, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32 slotId, s64 inVariable,
    const OrbisNpId* isLastChangedAuthor, const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
    OrbisNpTusVariable* outVariable, u64 outVariableSize, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetVirtualUserId = {}, slotId = {}, inVariable = {}, "
             "outVariable = {}, outVariableSize = {}, option = {}",
             reqId, targetVirtualUserId ? targetVirtualUserId->data : "", slotId, inVariable,
             fmt::ptr(outVariable), outVariableSize, fmt::ptr(option));
    if (option) {
        LOG_ERROR(Lib_NpTus, "Invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (!targetVirtualUserId || !outVariable) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (slotId < 0) {
        LOG_ERROR(Lib_NpTus, "Invalid slotId");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (outVariableSize != sizeof(OrbisNpTusVariable)) {
        LOG_ERROR(Lib_NpTus, "size mismatch: outVariableSize={} but sizeof(OrbisNpTusVariable)={}",
                  outVariableSize, sizeof(OrbisNpTusVariable));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    const std::string virtualUser(
        targetVirtualUserId->data,
        strnlen(targetVirtualUserId->data, sizeof(targetVirtualUserId->data)));
    if (virtualUser.empty()) {
        LOG_ERROR(Lib_NpTus, "Invalid vuser");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }

    const bool hasAuthorCheck = isLastChangedAuthor != nullptr;
    const bool hasDateCheck = isLastChangedDate != nullptr;
    const std::string authorNpId =
        hasAuthorCheck ? std::string(isLastChangedAuthor->handle.data) : std::string();

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusAddAndGetVariable(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), virtualUser,
        /*ownerAccountId=*/0, slotId, inVariable, hasAuthorCheck, /*isLastChangedAuthor=*/0,
        authorNpId, hasDateCheck, hasDateCheck ? isLastChangedDate->tick : 0,
        /*variableOut=*/outVariable, /*variableAOut=*/nullptr, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetDataAsync(int reqId, const OrbisNpId* targetNpId, s32 slotId,
                                      OrbisNpTusDataStatus* dataStatus, u64 dataStatusSize,
                                      void* data, u64 recvSize, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetNpId = {}, slotId = {}, dataStatus = {}, dataStatusSize = {}, "
             "data = {}, recvSize = {}, option = {}",
             reqId, targetNpId ? targetNpId->handle.data : "", slotId, fmt::ptr(dataStatus),
             dataStatusSize, fmt::ptr(data), recvSize, fmt::ptr(option));
    if (option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (!dataStatus || dataStatusSize == 0) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid slotId");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const std::string owner =
        (targetNpId && targetNpId->handle.data[0]) ? std::string(targetNpId->handle.data) : self;

    u64 offset = 0;
    {
        std::lock_guard lock(g_mutex);
        if (!req->xfer.SameTarget(slotId, 0, owner)) {
            req->xfer.Begin(slotId, 0, owner);
        }
        offset = req->xfer.recvOffset;
        if (data) {
            req->xfer.recvOffset += recvSize;
        }
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetData(
        uid, static_cast<s32>(svc), owner, /*virtualUser=*/std::string(), /*ownerAccountId=*/0,
        slotId, /*statusAOut=*/nullptr, dataStatusSize, data, data ? recvSize : 0, offset, ctx,
        /*statusOut=*/dataStatus);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetDataVUserAsync(int reqId,
                                           const OrbisNpTusVirtualUserId* targetVirtualUserId,
                                           s32 slotId, OrbisNpTusDataStatus* dataStatus,
                                           u64 dataStatusSize, void* data, u64 recvSize,
                                           void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetVirtualUserId = {}, slotId = {}, dataStatus = {}, "
             "dataStatusSize = {}, data = {}, recvSize = {}, option = {}",
             reqId, targetVirtualUserId ? targetVirtualUserId->data : "", slotId,
             fmt::ptr(dataStatus), dataStatusSize, fmt::ptr(data), recvSize, fmt::ptr(option));
    if (option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (!targetVirtualUserId || !dataStatus || dataStatusSize == 0) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid slotId");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const std::string virtualUser(
        targetVirtualUserId->data,
        strnlen(targetVirtualUserId->data, sizeof(targetVirtualUserId->data)));
    if (virtualUser.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid vuser");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }

    u64 offset = 0;
    {
        std::lock_guard lock(g_mutex);
        if (!req->xfer.SameTarget(slotId, 0, virtualUser)) {
            req->xfer.Begin(slotId, 0, virtualUser);
        }
        offset = req->xfer.recvOffset;
        if (data) {
            req->xfer.recvOffset += recvSize;
        }
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetData(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), virtualUser, /*ownerAccountId=*/0,
        slotId, /*statusAOut=*/nullptr, dataStatusSize, data, data ? recvSize : 0, offset, ctx,
        /*statusOut=*/dataStatus);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetFriendsDataStatus(int reqId, s32 slotId, s32 includeSelf, s32 sortType,
                                              OrbisNpTusDataStatus* statusArray,
                                              u64 statusArraySize, int arrayLen, void* option) {
    auto ret = sceNpTusGetFriendsDataStatusAsync(reqId, slotId, includeSelf, sortType, statusArray,
                                                 statusArraySize, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    // Result is the number of friend data statuses stored in statusArray.
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetFriendsDataStatusAsync(int reqId, s32 slotId, s32 includeSelf,
                                                   s32 sortType, OrbisNpTusDataStatus* statusArray,
                                                   u64 statusArraySize, int arrayLen,
                                                   void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, slotId = {}, includeSelf = {}, sortType = {}, statusArray = {}, "
             "statusArraySize = {}, arrayLen = {}, option = {}",
             reqId, slotId, includeSelf, sortType, fmt::ptr(statusArray), statusArraySize, arrayLen,
             fmt::ptr(option));
    if (!statusArray) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || arrayLen < 1 || slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxSelectedFriends) {
        LOG_ERROR(Lib_NpTus, "too many friends");
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    // Data statuses only sort by date (no value ordering).
    if (sortType < ORBIS_NP_TUS_DATASTATUS_SORTTYPE_DESCENDING_DATE ||
        sortType > ORBIS_NP_TUS_DATASTATUS_SORTTYPE_ASCENDING_DATE) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (statusArraySize != static_cast<u64>(arrayLen) * sizeof(OrbisNpTusDataStatus)) {
        LOG_ERROR(
            Lib_NpTus,
            "size mismatch: statusArraySize={} but arrayLen {} * sizeof(OrbisNpTusDataStatus)={}",
            statusArraySize, arrayLen, static_cast<u64>(arrayLen) * sizeof(OrbisNpTusDataStatus));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetFriendsDataStatus(
        uid, static_cast<s32>(svc), slotId, includeSelf != 0, sortType, static_cast<u32>(arrayLen),
        /*statusOut=*/statusArray, /*statusAOut=*/nullptr, static_cast<u64>(arrayLen), ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetFriendsVariable(int reqId, s32 slotId, s32 includeSelf, s32 sortType,
                                            OrbisNpTusVariable* variableArray,
                                            u64 variableArraySize, int arrayLen, void* option) {
    auto ret = sceNpTusGetFriendsVariableAsync(reqId, slotId, includeSelf, sortType, variableArray,
                                               variableArraySize, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    // Result is the number of friend variables stored in variableArray.
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetFriendsVariableAsync(int reqId, s32 slotId, s32 includeSelf,
                                                 s32 sortType, OrbisNpTusVariable* variableArray,
                                                 u64 variableArraySize, int arrayLen,
                                                 void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, slotId = {}, includeSelf = {}, sortType = {}, variableArray = {}, "
             "variableArraySize = {}, arrayLen = {}, option = {}",
             reqId, slotId, includeSelf, sortType, fmt::ptr(variableArray), variableArraySize,
             arrayLen, fmt::ptr(option));
    if (!variableArray) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || arrayLen < 1 || slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxSelectedFriends) {
        LOG_ERROR(Lib_NpTus, "too many friends");
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (sortType < ORBIS_NP_TUS_VARIABLE_SORTTYPE_DESCENDING_DATE ||
        sortType > ORBIS_NP_TUS_VARIABLE_SORTTYPE_ASCENDING_VALUE) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (variableArraySize != static_cast<u64>(arrayLen) * sizeof(OrbisNpTusVariable)) {
        LOG_ERROR(
            Lib_NpTus,
            "size mismatch: variableArraySize={} but arrayLen {} * sizeof(OrbisNpTusVariable)={}",
            variableArraySize, arrayLen, static_cast<u64>(arrayLen) * sizeof(OrbisNpTusVariable));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetFriendsVariable(
        uid, static_cast<s32>(svc), slotId, includeSelf != 0, sortType, static_cast<u32>(arrayLen),
        /*variablesOut=*/variableArray, /*variablesAOut=*/nullptr, static_cast<u64>(arrayLen), ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatus(int reqId, const OrbisNpId* targetNpId,
                                                s32* slotIds, OrbisNpTusDataStatus* statusArray,
                                                u64 statusArraySize, int arrayLen, void* option) {
    auto ret = sceNpTusGetMultiSlotDataStatusAsync(reqId, targetNpId, slotIds, statusArray,
                                                   statusArraySize, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    // Returns the number of statuses obtained (>= 0) on success.
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusAsync(int reqId, const OrbisNpId* targetNpId,
                                                     s32* slotIds,
                                                     OrbisNpTusDataStatus* statusArray,
                                                     u64 statusArraySize, int arrayLen,
                                                     void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetNpId = {}, slotIds = {}, statusArray = {}, statusArraySize = {}, "
             "arrayLen = {}, option = {}",
             reqId, targetNpId ? targetNpId->handle.data : "", fmt::ptr(slotIds),
             fmt::ptr(statusArray), statusArraySize, arrayLen, fmt::ptr(option));
    if (!targetNpId || !slotIds || !statusArray || arrayLen < 1) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots per request");
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (std::any_of(slotIds, slotIds + arrayLen, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (statusArraySize != static_cast<u64>(arrayLen) * sizeof(OrbisNpTusDataStatus)) {
        LOG_ERROR(
            Lib_NpTus,
            "size mismatch: statusArraySize={} but arrayLen {} * sizeof(OrbisNpTusDataStatus)={}",
            statusArraySize, arrayLen, static_cast<u64>(arrayLen) * sizeof(OrbisNpTusDataStatus));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const std::string owner =
        targetNpId->handle.data[0] ? std::string(targetNpId->handle.data) : self;
    std::vector<s32> slots(slotIds, slotIds + arrayLen);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetMultiSlotDataStatus(
        uid, static_cast<s32>(svc), owner, /*virtualUser=*/std::string(), /*ownerAccountId=*/0,
        slots, /*statusOut=*/statusArray, /*statusAOut=*/nullptr, static_cast<u64>(arrayLen), ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32* slotIds,
    OrbisNpTusDataStatus* statusArray, u64 statusArraySize, int arrayLen, void* option) {
    auto ret = sceNpTusGetMultiSlotDataStatusVUserAsync(
        reqId, targetVirtualUserId, slotIds, statusArray, statusArraySize, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32* slotIds,
    OrbisNpTusDataStatus* statusArray, u64 statusArraySize, int arrayLen, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetVirtualUserId = {}, slotIds = {}, statusArray = {}, "
             "statusArraySize = {}, arrayLen = {}, option = {}",
             reqId, targetVirtualUserId ? targetVirtualUserId->data : "", fmt::ptr(slotIds),
             fmt::ptr(statusArray), statusArraySize, arrayLen, fmt::ptr(option));
    if (!targetVirtualUserId || !slotIds || !statusArray || arrayLen < 1) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots per request");
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (std::any_of(slotIds, slotIds + arrayLen, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (statusArraySize != static_cast<u64>(arrayLen) * sizeof(OrbisNpTusDataStatus)) {
        LOG_ERROR(
            Lib_NpTus,
            "size mismatch: statusArraySize={} but arrayLen {} * sizeof(OrbisNpTusDataStatus)={}",
            statusArraySize, arrayLen, static_cast<u64>(arrayLen) * sizeof(OrbisNpTusDataStatus));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    const std::string virtualUser(
        targetVirtualUserId->data,
        strnlen(targetVirtualUserId->data, sizeof(targetVirtualUserId->data)));
    if (virtualUser.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid vuser");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slotIds, slotIds + arrayLen);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetMultiSlotDataStatus(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), virtualUser, /*ownerAccountId=*/0,
        slots, /*statusOut=*/statusArray, /*statusAOut=*/nullptr, static_cast<u64>(arrayLen), ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusSetDataAsync(int reqId, const OrbisNpId* targetNpId, s32 slotId,
                                      u64 totalSize, u64 sendSize, const void* data,
                                      const OrbisNpTusDataInfo* info, u64 infoStructSize,
                                      const OrbisNpId* isLastChangedAuthor,
                                      const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
                                      void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetNpId = {}, slotId = {}, totalSize = {}, sendSize = {}, data = {}, "
             "info = {}, infoStructSize = {}, option = {}",
             reqId, targetNpId ? targetNpId->handle.data : "", slotId, totalSize, sendSize,
             fmt::ptr(data), fmt::ptr(info), infoStructSize, fmt::ptr(option));
    if (option) {
        LOG_ERROR(Lib_NpTus, "Invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (!data || sendSize == 0) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (slotId < 0 || sendSize > totalSize) {
        LOG_ERROR(Lib_NpTus, "Invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const std::string owner =
        (targetNpId && targetNpId->handle.data[0]) ? std::string(targetNpId->handle.data) : self;

    std::vector<u8> blob;
    std::vector<u8> infoBytes;
    bool complete = false;
    {
        std::lock_guard lock(g_mutex);
        auto& x = req->xfer;
        if (!x.SameTarget(slotId, 0, owner)) {
            x.Begin(slotId, 0, owner);
            x.totalSize = totalSize;
            x.sendBuf.reserve(totalSize);
            if (info && infoStructSize >= sizeof(OrbisNpTusDataInfo)) {
                const u64 n = std::min<u64>(info->size, sizeof(info->data));
                x.info.assign(info->data, info->data + n);
            }
        }
        const u8* src = static_cast<const u8*>(data);
        x.sendBuf.insert(x.sendBuf.end(), src, src + sendSize);
        if (x.sendBuf.size() >= x.totalSize) {
            blob = x.sendBuf;
            infoBytes = x.info;
            complete = true;
            x = NpTusDataXfer{};
        }
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    if (!complete) {
        ctx->SetResult(ORBIS_OK);
        return ORBIS_OK;
    }
    const bool hasAuthorCheck = isLastChangedAuthor != nullptr;
    const bool hasDateCheck = isLastChangedDate != nullptr;
    const std::string authorNpId =
        hasAuthorCheck ? std::string(isLastChangedAuthor->handle.data) : std::string();
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusSetData(
        uid, static_cast<s32>(svc), owner, /*virtualUser=*/std::string(), /*ownerAccountId=*/0,
        slotId, blob, infoBytes, hasAuthorCheck, /*isLastChangedAuthor=*/0, authorNpId,
        hasDateCheck, hasDateCheck ? isLastChangedDate->tick : 0, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusSetDataVUserAsync(int reqId,
                                           const OrbisNpTusVirtualUserId* targetVirtualUserId,
                                           s32 slotId, u64 totalSize, u64 sendSize,
                                           const void* data, const OrbisNpTusDataInfo* info,
                                           u64 infoStructSize, const OrbisNpId* isLastChangedAuthor,
                                           const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
                                           void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetVirtualUserId = {}, slotId = {}, totalSize = {}, sendSize = {}, "
             "data = {}, info = {}, infoStructSize = {}, option = {}",
             reqId, targetVirtualUserId ? targetVirtualUserId->data : "", slotId, totalSize,
             sendSize, fmt::ptr(data), fmt::ptr(info), infoStructSize, fmt::ptr(option));
    if (option) {
        LOG_ERROR(Lib_NpTus, "Invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (!targetVirtualUserId || !data || sendSize == 0) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (slotId < 0 || sendSize > totalSize) {
        LOG_ERROR(Lib_NpTus, "Invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const std::string virtualUser(
        targetVirtualUserId->data,
        strnlen(targetVirtualUserId->data, sizeof(targetVirtualUserId->data)));
    if (virtualUser.empty()) {
        LOG_ERROR(Lib_NpTus, "Invalid vuser");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }

    std::vector<u8> blob;
    std::vector<u8> infoBytes;
    bool complete = false;
    {
        std::lock_guard lock(g_mutex);
        auto& x = req->xfer;
        if (!x.SameTarget(slotId, 0, virtualUser)) {
            x.Begin(slotId, 0, virtualUser);
            x.totalSize = totalSize;
            x.sendBuf.reserve(totalSize);
            if (info && infoStructSize >= sizeof(OrbisNpTusDataInfo)) {
                const u64 n = std::min<u64>(info->size, sizeof(info->data));
                x.info.assign(info->data, info->data + n);
            }
        }
        const u8* src = static_cast<const u8*>(data);
        x.sendBuf.insert(x.sendBuf.end(), src, src + sendSize);
        if (x.sendBuf.size() >= x.totalSize) {
            blob = x.sendBuf;
            infoBytes = x.info;
            complete = true;
            x = NpTusDataXfer{};
        }
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    if (!complete) {
        ctx->SetResult(ORBIS_OK);
        return ORBIS_OK;
    }

    const bool hasAuthorCheck = isLastChangedAuthor != nullptr;
    const bool hasDateCheck = isLastChangedDate != nullptr;
    const std::string authorNpId =
        hasAuthorCheck ? std::string(isLastChangedAuthor->handle.data) : std::string();
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusSetData(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), virtualUser, /*ownerAccountId=*/0,
        slotId, blob, infoBytes, hasAuthorCheck, /*isLastChangedAuthor=*/0, authorNpId,
        hasDateCheck, hasDateCheck ? isLastChangedDate->tick : 0, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusTryAndSetVariable(int reqId, const OrbisNpId* targetNpId, s32 slotId,
                                           s32 opeType, s64 variable, const s64* compareValue,
                                           const OrbisNpId* isLastChangedAuthor,
                                           const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
                                           OrbisNpTusVariable* resultVariable,
                                           u64 resultVariableSize, void* option) {
    auto ret = sceNpTusTryAndSetVariableAsync(reqId, targetNpId, slotId, opeType, variable,
                                              compareValue, isLastChangedAuthor, isLastChangedDate,
                                              resultVariable, resultVariableSize, option);
    if (ret < 0) {
        return ret;
    }
    // 0 on normal termination, including when the condition was not met.
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableAsync(
    int reqId, const OrbisNpId* targetNpId, s32 slotId, s32 opeType, s64 variable,
    const s64* compareValue, const OrbisNpId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate, OrbisNpTusVariable* resultVariable,
    u64 resultVariableSize, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetNpId = {}, slotId = {}, opeType = {}, variable = {}, "
             "compareValue = {}, resultVariable = {}, resultVariableSize = {}, option = {}",
             reqId, targetNpId ? targetNpId->handle.data : "", slotId, opeType, variable,
             fmt::ptr(compareValue), fmt::ptr(resultVariable), resultVariableSize,
             fmt::ptr(option));
    if (!targetNpId || !resultVariable) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (opeType < ORBIS_NP_TUS_OPETYPE_EQUAL || opeType > ORBIS_NP_TUS_OPETYPE_LESS_OR_EQUAL) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (resultVariableSize != sizeof(OrbisNpTusVariable)) {
        LOG_ERROR(Lib_NpTus,
                  "size mismatch: resultVariableSize={} but sizeof(OrbisNpTusVariable)={}",
                  resultVariableSize, sizeof(OrbisNpTusVariable));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const std::string owner =
        targetNpId->handle.data[0] ? std::string(targetNpId->handle.data) : self;

    const bool hasAuthorCheck = isLastChangedAuthor != nullptr;
    const bool hasDateCheck = isLastChangedDate != nullptr;
    const std::string authorNpId =
        hasAuthorCheck ? std::string(isLastChangedAuthor->handle.data) : std::string();

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusTryAndSetVariable(
        uid, static_cast<s32>(svc), owner, /*virtualUser=*/std::string(), /*ownerAccountId=*/0,
        slotId, opeType, variable, compareValue != nullptr, compareValue ? *compareValue : variable,
        hasAuthorCheck, /*isLastChangedAuthor=*/0, authorNpId, hasDateCheck,
        hasDateCheck ? isLastChangedDate->tick : 0,
        /*variableOut=*/resultVariable, /*variableAOut=*/nullptr, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32 slotId, s32 opeType,
    s64 variable, const s64* compareValue, const OrbisNpId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate, OrbisNpTusVariable* resultVariable,
    u64 resultVariableSize, void* option) {
    auto ret = sceNpTusTryAndSetVariableVUserAsync(
        reqId, targetVirtualUserId, slotId, opeType, variable, compareValue, isLastChangedAuthor,
        isLastChangedDate, resultVariable, resultVariableSize, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32 slotId, s32 opeType,
    s64 variable, const s64* compareValue, const OrbisNpId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate, OrbisNpTusVariable* resultVariable,
    u64 resultVariableSize, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetVirtualUserId = {}, slotId = {}, opeType = {}, variable = {}, "
             "compareValue = {}, resultVariable = {}, resultVariableSize = {}, option = {}",
             reqId, targetVirtualUserId ? targetVirtualUserId->data : "", slotId, opeType, variable,
             fmt::ptr(compareValue), fmt::ptr(resultVariable), resultVariableSize,
             fmt::ptr(option));
    if (!targetVirtualUserId || !resultVariable) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (opeType < ORBIS_NP_TUS_OPETYPE_EQUAL || opeType > ORBIS_NP_TUS_OPETYPE_LESS_OR_EQUAL) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (resultVariableSize != sizeof(OrbisNpTusVariable)) {
        LOG_ERROR(Lib_NpTus,
                  "size mismatch: resultVariableSize={} but sizeof(OrbisNpTusVariable)={}",
                  resultVariableSize, sizeof(OrbisNpTusVariable));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    const std::string virtualUser(
        targetVirtualUserId->data,
        strnlen(targetVirtualUserId->data, sizeof(targetVirtualUserId->data)));
    if (virtualUser.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid vuser");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }

    const bool hasAuthorCheck = isLastChangedAuthor != nullptr;
    const bool hasDateCheck = isLastChangedDate != nullptr;
    const std::string authorNpId =
        hasAuthorCheck ? std::string(isLastChangedAuthor->handle.data) : std::string();

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusTryAndSetVariable(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), virtualUser,
        /*ownerAccountId=*/0, slotId, opeType, variable, compareValue != nullptr,
        compareValue ? *compareValue : variable, hasAuthorCheck, /*isLastChangedAuthor=*/0,
        authorNpId, hasDateCheck, hasDateCheck ? isLastChangedDate->tick : 0,
        /*variableOut=*/resultVariable, /*variableAOut=*/nullptr, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableA(int reqId, OrbisNpAccountId targetAccountId, s32 slotId,
                                            s64 inVariable,
                                            const OrbisNpAccountId* isLastChangedAuthor,
                                            const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
                                            OrbisNpTusVariableA* outVariable, u64 outVariableSize,
                                            void* option) {
    auto ret = sceNpTusAddAndGetVariableAAsync(reqId, targetAccountId, slotId, inVariable,
                                               isLastChangedAuthor, isLastChangedDate, outVariable,
                                               outVariableSize, option);
    if (ret < 0) {
        return ret;
    }
    // Returns 0 on normal termination; the result value is in outVariable.
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableAAsync(
    int reqId, OrbisNpAccountId targetAccountId, s32 slotId, s64 inVariable,
    const OrbisNpAccountId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate, OrbisNpTusVariableA* outVariable,
    u64 outVariableSize, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetAccountId = {}, slotId = {}, inVariable = {}, outVariable = {}, "
             "outVariableSize = {}, option = {}",
             reqId, targetAccountId, slotId, inVariable, fmt::ptr(outVariable), outVariableSize,
             fmt::ptr(option));
    if (option || slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (!outVariable) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (outVariableSize != sizeof(OrbisNpTusVariableA)) {
        LOG_ERROR(Lib_NpTus, "size mismatch: outVariableSize={} but sizeof(OrbisNpTusVariableA)={}",
                  outVariableSize, sizeof(OrbisNpTusVariableA));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const bool hasAuthorCheck = isLastChangedAuthor != nullptr;
    const bool hasDateCheck = isLastChangedDate != nullptr;

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusAddAndGetVariable(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), /*virtualUser=*/std::string(),
        static_cast<s64>(targetAccountId), slotId, inVariable, hasAuthorCheck,
        hasAuthorCheck ? static_cast<s64>(*isLastChangedAuthor) : 0,
        /*isLastChangedAuthorNpId=*/std::string(), hasDateCheck,
        hasDateCheck ? isLastChangedDate->tick : 0, /*variableOut=*/nullptr,
        /*variableAOut=*/outVariable, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableAVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32 slotId, s64 inVariable,
    const OrbisNpAccountId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate, OrbisNpTusVariableA* outVariable,
    u64 outVariableSize, void* option) {
    auto ret = sceNpTusAddAndGetVariableAVUserAsync(reqId, targetVirtualUserId, slotId, inVariable,
                                                    isLastChangedAuthor, isLastChangedDate,
                                                    outVariable, outVariableSize, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableAVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32 slotId, s64 inVariable,
    const OrbisNpAccountId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate, OrbisNpTusVariableA* outVariable,
    u64 outVariableSize, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetVirtualUserId = {}, slotId = {}, inVariable = {}, "
             "outVariable = {}, outVariableSize = {}, option = {}",
             reqId, targetVirtualUserId ? targetVirtualUserId->data : "", slotId, inVariable,
             fmt::ptr(outVariable), outVariableSize, fmt::ptr(option));
    if (!targetVirtualUserId || !outVariable) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (outVariableSize != sizeof(OrbisNpTusVariableA)) {
        LOG_ERROR(Lib_NpTus, "size mismatch: outVariableSize={} but sizeof(OrbisNpTusVariableA)={}",
                  outVariableSize, sizeof(OrbisNpTusVariableA));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    const std::string virtualUser(
        targetVirtualUserId->data,
        strnlen(targetVirtualUserId->data, sizeof(targetVirtualUserId->data)));
    if (virtualUser.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const bool hasAuthorCheck = isLastChangedAuthor != nullptr;
    const bool hasDateCheck = isLastChangedDate != nullptr;

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusAddAndGetVariable(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), virtualUser,
        /*ownerAccountId=*/0, slotId, inVariable, hasAuthorCheck,
        hasAuthorCheck ? static_cast<s64>(*isLastChangedAuthor) : 0,
        /*isLastChangedAuthorNpId=*/std::string(), hasDateCheck,
        hasDateCheck ? isLastChangedDate->tick : 0, /*variableOut=*/nullptr,
        /*variableAOut=*/outVariable, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotData(int reqId, const OrbisNpId* targetNpId, s32* slotIds,
                                             int arrayLen, void* option) {
    auto ret = sceNpTusDeleteMultiSlotDataAsync(reqId, targetNpId, slotIds, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    // Returns 0 on normal termination.
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotDataAsync(int reqId, const OrbisNpId* targetNpId,
                                                  s32* slotIds, int arrayLen, void* option) {
    LOG_INFO(Lib_NpTus, "reqId = {}, targetNpId = {}, slotIds = {}, arrayLen = {}, option = {}",
             reqId, targetNpId ? targetNpId->handle.data : "", fmt::ptr(slotIds), arrayLen,
             fmt::ptr(option));
    if (!targetNpId || !slotIds || arrayLen < 1) {
        LOG_ERROR(Lib_NpTus, "missing required argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots: arrayLen={} (max {})", arrayLen,
                  TusMaxSlotsPerRequest);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slotIds, slotIds + arrayLen, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const std::string owner =
        targetNpId->handle.data[0] ? std::string(targetNpId->handle.data) : self;
    std::vector<s32> slots(slotIds, slotIds + arrayLen);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusDeleteMultiSlotData(
        uid, static_cast<s32>(svc), owner, /*virtualUser=*/std::string(), /*ownerAccountId=*/0,
        slots, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
sceNpTusDeleteMultiSlotDataVUser(int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId,
                                 s32* slotIds, int arrayLen, void* option) {
    auto ret = sceNpTusDeleteMultiSlotDataVUserAsync(reqId, targetVirtualUserId, slotIds, arrayLen,
                                                     option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI
sceNpTusDeleteMultiSlotDataVUserAsync(int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId,
                                      s32* slotIds, int arrayLen, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetVirtualUserId = {}, slotIds = {}, arrayLen = {}, option = {}",
             reqId, targetVirtualUserId ? targetVirtualUserId->data : "", fmt::ptr(slotIds),
             arrayLen, fmt::ptr(option));
    if (!targetVirtualUserId || !slotIds || arrayLen < 1) {
        LOG_ERROR(Lib_NpTus, "missing required argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots: arrayLen={} (max {})", arrayLen,
                  TusMaxSlotsPerRequest);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slotIds, slotIds + arrayLen, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const std::string virtualUser(
        targetVirtualUserId->data,
        strnlen(targetVirtualUserId->data, sizeof(targetVirtualUserId->data)));
    if (virtualUser.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slotIds, slotIds + arrayLen);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusDeleteMultiSlotData(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), virtualUser,
        /*ownerAccountId=*/0, slots, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotVariable(int reqId, const OrbisNpId* targetNpId,
                                                 s32* slotIds, int arrayLen, void* option) {
    auto ret = sceNpTusDeleteMultiSlotVariableAsync(reqId, targetNpId, slotIds, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    // Returns 0 on normal termination.
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotVariableAsync(int reqId, const OrbisNpId* targetNpId,
                                                      s32* slotIds, int arrayLen, void* option) {
    LOG_INFO(Lib_NpTus, "reqId = {}, targetNpId = {}, slotIds = {}, arrayLen = {}, option = {}",
             reqId, targetNpId ? targetNpId->handle.data : "", fmt::ptr(slotIds), arrayLen,
             fmt::ptr(option));
    if (!targetNpId || !slotIds || arrayLen < 1) {
        LOG_ERROR(Lib_NpTus, "missing required argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots: arrayLen={} (max {})", arrayLen,
                  TusMaxSlotsPerRequest);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slotIds, slotIds + arrayLen, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const std::string owner =
        targetNpId->handle.data[0] ? std::string(targetNpId->handle.data) : self;
    std::vector<s32> slots(slotIds, slotIds + arrayLen);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusDeleteMultiSlotVariable(
        uid, static_cast<s32>(svc), owner, /*virtualUser=*/std::string(), /*ownerAccountId=*/0,
        slots, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
sceNpTusDeleteMultiSlotVariableVUser(int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId,
                                     s32* slotIds, int arrayLen, void* option) {
    auto ret = sceNpTusDeleteMultiSlotVariableVUserAsync(reqId, targetVirtualUserId, slotIds,
                                                         arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotVariableVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32* slotIds, int arrayLen,
    void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetVirtualUserId = {}, slotIds = {}, arrayLen = {}, option = {}",
             reqId, targetVirtualUserId ? targetVirtualUserId->data : "", fmt::ptr(slotIds),
             arrayLen, fmt::ptr(option));
    if (!targetVirtualUserId || !slotIds || arrayLen < 1) {
        LOG_ERROR(Lib_NpTus, "missing required argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots: arrayLen={} (max {})", arrayLen,
                  TusMaxSlotsPerRequest);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slotIds, slotIds + arrayLen, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const std::string virtualUser(
        targetVirtualUserId->data,
        strnlen(targetVirtualUserId->data, sizeof(targetVirtualUserId->data)));
    if (virtualUser.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slotIds, slotIds + arrayLen);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusDeleteMultiSlotVariable(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), virtualUser,
        /*ownerAccountId=*/0, slots, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetData(int reqId, const OrbisNpId* targetNpId, s32 slotId,
                                 OrbisNpTusDataStatus* dataStatus, u64 dataStatusSize, void* data,
                                 u64 recvSize, void* option) {
    auto ret = sceNpTusGetDataAsync(reqId, targetNpId, slotId, dataStatus, dataStatusSize, data,
                                    recvSize, option);
    if (ret < 0) {
        return ret;
    }
    // Returns the size of the received data (>= 0).
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetDataA(int reqId, OrbisNpAccountId targetAccountId, s32 slotId,
                                  OrbisNpTusDataStatusA* dataStatus, u64 dataStatusSize, void* data,
                                  u64 recvSize, void* option) {
    auto ret = sceNpTusGetDataAAsync(reqId, targetAccountId, slotId, dataStatus, dataStatusSize,
                                     data, recvSize, option);
    if (ret < 0) {
        return ret;
    }
    // Returns the size of the received data (>= 0).
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetDataAVUser(int reqId,
                                       const OrbisNpTusVirtualUserId* targetVirtualUserId,
                                       s32 slotId, OrbisNpTusDataStatusA* dataStatus,
                                       u64 dataStatusSize, void* data, u64 recvSize, void* option) {
    auto ret = sceNpTusGetDataAVUserAsync(reqId, targetVirtualUserId, slotId, dataStatus,
                                          dataStatusSize, data, recvSize, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetDataVUser(int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId,
                                      s32 slotId, OrbisNpTusDataStatus* dataStatus,
                                      u64 dataStatusSize, void* data, u64 recvSize, void* option) {
    auto ret = sceNpTusGetDataVUserAsync(reqId, targetVirtualUserId, slotId, dataStatus,
                                         dataStatusSize, data, recvSize, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusA(int reqId, OrbisNpAccountId targetAccountId,
                                                 s32* slotIds, OrbisNpTusDataStatusA* statusArray,
                                                 u64 statusArraySize, int arrayLen, void* option) {
    auto ret = sceNpTusGetMultiSlotDataStatusAAsync(reqId, targetAccountId, slotIds, statusArray,
                                                    statusArraySize, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    // Returns the number of statuses obtained (>= 0).
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusAAsync(int reqId, OrbisNpAccountId targetAccountId,
                                                      s32* slotIds,
                                                      OrbisNpTusDataStatusA* statusArray,
                                                      u64 statusArraySize, int arrayLen,
                                                      void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetAccountId = {}, slotIds = {}, statusArray = {}, "
             "statusArraySize = {}, arrayLen = {}, option = {}",
             reqId, targetAccountId, fmt::ptr(slotIds), fmt::ptr(statusArray), statusArraySize,
             arrayLen, fmt::ptr(option));
    if (!slotIds || !statusArray) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || arrayLen < 1) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots: arrayLen={} (max {})", arrayLen,
                  TusMaxSlotsPerRequest);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slotIds, slotIds + arrayLen, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (statusArraySize != static_cast<u64>(arrayLen) * sizeof(OrbisNpTusDataStatusA)) {
        LOG_ERROR(
            Lib_NpTus,
            "size mismatch: statusArraySize={} but arrayLen {} * sizeof(OrbisNpTusDataStatusA)={}",
            statusArraySize, arrayLen, static_cast<u64>(arrayLen) * sizeof(OrbisNpTusDataStatusA));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slotIds, slotIds + arrayLen);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetMultiSlotDataStatus(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), /*virtualUser=*/std::string(),
        static_cast<s64>(targetAccountId), slots, /*statusOut=*/nullptr,
        /*statusAOut=*/statusArray, static_cast<u64>(arrayLen), ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusAVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32* slotIds,
    OrbisNpTusDataStatusA* statusArray, u64 statusArraySize, int arrayLen, void* option) {
    auto ret = sceNpTusGetMultiSlotDataStatusAVUserAsync(
        reqId, targetVirtualUserId, slotIds, statusArray, statusArraySize, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusAVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32* slotIds,
    OrbisNpTusDataStatusA* statusArray, u64 statusArraySize, int arrayLen, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetVirtualUserId = {}, slotIds = {}, statusArray = {}, "
             "statusArraySize = {}, arrayLen = {}, option = {}",
             reqId, targetVirtualUserId ? targetVirtualUserId->data : "", fmt::ptr(slotIds),
             fmt::ptr(statusArray), statusArraySize, arrayLen, fmt::ptr(option));
    // NULL targetVirtualUserId is explicitly INSUFFICIENT_ARGUMENT for this variant.
    if (!targetVirtualUserId || !slotIds || !statusArray) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || arrayLen < 1) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots: arrayLen={} (max {})", arrayLen,
                  TusMaxSlotsPerRequest);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slotIds, slotIds + arrayLen, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (statusArraySize != static_cast<u64>(arrayLen) * sizeof(OrbisNpTusDataStatusA)) {
        LOG_ERROR(
            Lib_NpTus,
            "size mismatch: statusArraySize={} but arrayLen {} * sizeof(OrbisNpTusDataStatusA)={}",
            statusArraySize, arrayLen, static_cast<u64>(arrayLen) * sizeof(OrbisNpTusDataStatusA));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    const std::string virtualUser(
        targetVirtualUserId->data,
        strnlen(targetVirtualUserId->data, sizeof(targetVirtualUserId->data)));
    if (virtualUser.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slotIds, slotIds + arrayLen);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetMultiSlotDataStatus(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), virtualUser,
        /*ownerAccountId=*/0, slots, /*statusOut=*/nullptr, /*statusAOut=*/statusArray,
        static_cast<u64>(arrayLen), ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableA(int reqId, OrbisNpAccountId targetAccountId,
                                               s32* slotIds, OrbisNpTusVariableA* variableArray,
                                               u64 variablesSize, int arrayLen, void* option) {
    auto ret = sceNpTusGetMultiSlotVariableAAsync(reqId, targetAccountId, slotIds, variableArray,
                                                  variablesSize, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    // Returns the number of variables read (>= 0).
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatus(int reqId, const OrbisNpId* targetNpIdArray,
                                                s32 slotId, OrbisNpTusDataStatus* statusArray,
                                                u64 statusArraySize, int arrayLen, void* option) {
    auto ret = sceNpTusGetMultiUserDataStatusAsync(reqId, targetNpIdArray, slotId, statusArray,
                                                   statusArraySize, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    // Returns the number of statuses obtained (>= 0).
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusAsync(int reqId, const OrbisNpId* targetNpIdArray,
                                                     s32 slotId, OrbisNpTusDataStatus* statusArray,
                                                     u64 statusArraySize, int arrayLen,
                                                     void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetNpIdArray = {}, slotId = {}, statusArray = {}, "
             "statusArraySize = {}, arrayLen = {}, option = {}",
             reqId, fmt::ptr(targetNpIdArray), slotId, fmt::ptr(statusArray), statusArraySize,
             arrayLen, fmt::ptr(option));
    if (!targetNpIdArray || !statusArray || arrayLen < 1) {
        LOG_ERROR(Lib_NpTus, "missing required argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxUsersPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many users: arrayLen={} (max {})", arrayLen,
                  TusMaxUsersPerRequest);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (statusArraySize != static_cast<u64>(arrayLen) * sizeof(OrbisNpTusDataStatus)) {
        LOG_ERROR(
            Lib_NpTus,
            "size mismatch: statusArraySize={} but arrayLen {} * sizeof(OrbisNpTusDataStatus)={}",
            statusArraySize, arrayLen, static_cast<u64>(arrayLen) * sizeof(OrbisNpTusDataStatus));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<std::string> npids;
    npids.reserve(arrayLen);
    for (int i = 0; i < arrayLen; ++i) {
        const char* h = targetNpIdArray[i].handle.data;
        npids.emplace_back(h[0] ? std::string(h, strnlen(h, sizeof(targetNpIdArray[i].handle.data)))
                                : self);
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetMultiUserDataStatus(
        uid, static_cast<s32>(svc), slotId, npids, /*virtualUsers=*/{}, /*ownerAccountIds=*/{},
        /*statusOut=*/statusArray, /*statusAOut=*/nullptr, static_cast<u64>(arrayLen), ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserIdArray, s32 slotId,
    OrbisNpTusDataStatus* statusArray, u64 statusArraySize, int arrayLen, void* option) {
    auto ret = sceNpTusGetMultiUserDataStatusVUserAsync(
        reqId, targetVirtualUserIdArray, slotId, statusArray, statusArraySize, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserIdArray, s32 slotId,
    OrbisNpTusDataStatus* statusArray, u64 statusArraySize, int arrayLen, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetVirtualUserIdArray = {}, slotId = {}, statusArray = {}, "
             "statusArraySize = {}, arrayLen = {}, option = {}",
             reqId, fmt::ptr(targetVirtualUserIdArray), slotId, fmt::ptr(statusArray),
             statusArraySize, arrayLen, fmt::ptr(option));
    if (!targetVirtualUserIdArray || !statusArray || arrayLen < 1) {
        LOG_ERROR(Lib_NpTus, "missing required argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxUsersPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many users: arrayLen={} (max {})", arrayLen,
                  TusMaxUsersPerRequest);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (statusArraySize != static_cast<u64>(arrayLen) * sizeof(OrbisNpTusDataStatus)) {
        LOG_ERROR(
            Lib_NpTus,
            "size mismatch: statusArraySize={} but arrayLen {} * sizeof(OrbisNpTusDataStatus)={}",
            statusArraySize, arrayLen, static_cast<u64>(arrayLen) * sizeof(OrbisNpTusDataStatus));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    std::vector<std::string> vusers;
    vusers.reserve(arrayLen);
    for (int i = 0; i < arrayLen; ++i) {
        const char* d = targetVirtualUserIdArray[i].data;
        std::string name(d, strnlen(d, sizeof(targetVirtualUserIdArray[i].data)));
        if (name.empty()) {
            // An empty entry would silently fall through to account targeting.
            LOG_ERROR(Lib_NpTus, "invalid argument");
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
        }
        vusers.emplace_back(std::move(name));
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetMultiUserDataStatus(
        uid, static_cast<s32>(svc), slotId, /*ownerNpIds=*/{}, vusers, /*ownerAccountIds=*/{},
        /*statusOut=*/statusArray, /*statusAOut=*/nullptr, static_cast<u64>(arrayLen), ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariable(int reqId, const OrbisNpId* targetNpIdArray,
                                              s32 slotId, OrbisNpTusVariable* variableArray,
                                              u64 variableArraySize, int arrayLen, void* option) {
    auto ret = sceNpTusGetMultiUserVariableAsync(reqId, targetNpIdArray, slotId, variableArray,
                                                 variableArraySize, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    // Returns the number of variables read (>= 0).
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableAsync(int reqId, const OrbisNpId* targetNpIdArray,
                                                   s32 slotId, OrbisNpTusVariable* variableArray,
                                                   u64 variableArraySize, int arrayLen,
                                                   void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetNpIdArray = {}, slotId = {}, variableArray = {}, "
             "variableArraySize = {}, arrayLen = {}, option = {}",
             reqId, fmt::ptr(targetNpIdArray), slotId, fmt::ptr(variableArray), variableArraySize,
             arrayLen, fmt::ptr(option));
    if (!targetNpIdArray || !variableArray || arrayLen < 1) {
        LOG_ERROR(Lib_NpTus, "missing required argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxUsersPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many users: arrayLen={} (max {})", arrayLen,
                  TusMaxUsersPerRequest);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (variableArraySize != static_cast<u64>(arrayLen) * sizeof(OrbisNpTusVariable)) {
        LOG_ERROR(
            Lib_NpTus,
            "size mismatch: variableArraySize={} but arrayLen {} * sizeof(OrbisNpTusVariable)={}",
            variableArraySize, arrayLen, static_cast<u64>(arrayLen) * sizeof(OrbisNpTusVariable));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<std::string> npids;
    npids.reserve(arrayLen);
    for (int i = 0; i < arrayLen; ++i) {
        const char* h = targetNpIdArray[i].handle.data;
        npids.emplace_back(h[0] ? std::string(h, strnlen(h, sizeof(targetNpIdArray[i].handle.data)))
                                : self);
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetMultiUserVariable(
        uid, static_cast<s32>(svc), slotId, npids, /*virtualUsers=*/{}, /*ownerAccountIds=*/{},
        /*variablesOut=*/variableArray, /*variablesAOut=*/nullptr, static_cast<u64>(arrayLen), ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserIdArray, s32 slotId,
    OrbisNpTusVariable* variableArray, u64 variableArraySize, int arrayLen, void* option) {
    auto ret =
        sceNpTusGetMultiUserVariableVUserAsync(reqId, targetVirtualUserIdArray, slotId,
                                               variableArray, variableArraySize, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserIdArray, s32 slotId,
    OrbisNpTusVariable* variableArray, u64 variableArraySize, int arrayLen, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetVirtualUserIdArray = {}, slotId = {}, variableArray = {}, "
             "variableArraySize = {}, arrayLen = {}, option = {}",
             reqId, fmt::ptr(targetVirtualUserIdArray), slotId, fmt::ptr(variableArray),
             variableArraySize, arrayLen, fmt::ptr(option));
    if (!targetVirtualUserIdArray || !variableArray || arrayLen < 1) {
        LOG_ERROR(Lib_NpTus, "missing required argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxUsersPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many users: arrayLen={} (max {})", arrayLen,
                  TusMaxUsersPerRequest);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (variableArraySize != static_cast<u64>(arrayLen) * sizeof(OrbisNpTusVariable)) {
        LOG_ERROR(
            Lib_NpTus,
            "size mismatch: variableArraySize={} but arrayLen {} * sizeof(OrbisNpTusVariable)={}",
            variableArraySize, arrayLen, static_cast<u64>(arrayLen) * sizeof(OrbisNpTusVariable));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    std::vector<std::string> vusers;
    vusers.reserve(arrayLen);
    for (int i = 0; i < arrayLen; ++i) {
        const char* d = targetVirtualUserIdArray[i].data;
        std::string name(d, strnlen(d, sizeof(targetVirtualUserIdArray[i].data)));
        if (name.empty()) {
            LOG_ERROR(Lib_NpTus, "invalid argument");
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
        }
        vusers.emplace_back(std::move(name));
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetMultiUserVariable(
        uid, static_cast<s32>(svc), slotId, /*ownerNpIds=*/{}, vusers, /*ownerAccountIds=*/{},
        /*variablesOut=*/variableArray, /*variablesAOut=*/nullptr, static_cast<u64>(arrayLen), ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusSetData(int reqId, const OrbisNpId* targetNpId, s32 slotId, u64 totalSize,
                                 u64 sendSize, const void* data, const OrbisNpTusDataInfo* info,
                                 u64 infoStructSize, const OrbisNpId* isLastChangedAuthor,
                                 const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
                                 void* option) {
    auto ret = sceNpTusSetDataAsync(reqId, targetNpId, slotId, totalSize, sendSize, data, info,
                                    infoStructSize, isLastChangedAuthor, isLastChangedDate, option);
    if (ret < 0) {
        return ret;
    }
    // Returns 0 on normal termination.
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusSetDataA(int reqId, OrbisNpAccountId targetAccountId, s32 slotId,
                                  u64 totalSize, u64 sendSize, const void* data,
                                  const OrbisNpTusDataInfo* info, u64 infoStructSize,
                                  const OrbisNpAccountId* isLastChangedAuthor,
                                  const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
                                  void* option) {
    auto ret =
        sceNpTusSetDataAAsync(reqId, targetAccountId, slotId, totalSize, sendSize, data, info,
                              infoStructSize, isLastChangedAuthor, isLastChangedDate, option);
    if (ret < 0) {
        return ret;
    }
    // Returns 0 on normal termination.
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusSetDataAVUser(int reqId,
                                       const OrbisNpTusVirtualUserId* targetVirtualUserId,
                                       s32 slotId, u64 totalSize, u64 sendSize, const void* data,
                                       const OrbisNpTusDataInfo* info, u64 infoStructSize,
                                       const OrbisNpAccountId* isLastChangedAuthor,
                                       const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
                                       void* option) {
    auto ret = sceNpTusSetDataAVUserAsync(reqId, targetVirtualUserId, slotId, totalSize, sendSize,
                                          data, info, infoStructSize, isLastChangedAuthor,
                                          isLastChangedDate, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusSetDataVUser(int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId,
                                      s32 slotId, u64 totalSize, u64 sendSize, const void* data,
                                      const OrbisNpTusDataInfo* info, u64 infoStructSize,
                                      const OrbisNpId* isLastChangedAuthor,
                                      const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
                                      void* option) {
    auto ret = sceNpTusSetDataVUserAsync(reqId, targetVirtualUserId, slotId, totalSize, sendSize,
                                         data, info, infoStructSize, isLastChangedAuthor,
                                         isLastChangedDate, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusSetMultiSlotVariableA(int reqId, OrbisNpAccountId targetAccountId,
                                               s32* slotIds, const s64* variables, int arrayLen,
                                               void* option) {
    auto ret = sceNpTusSetMultiSlotVariableAAsync(reqId, targetAccountId, slotIds, variables,
                                                  arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    // Returns 0 on normal termination.
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableA(int reqId, OrbisNpAccountId targetAccountId, s32 slotId,
                                            s32 opeType, s64 variable, const s64* compareValue,
                                            const OrbisNpAccountId* isLastChangedAuthor,
                                            const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
                                            OrbisNpTusVariableA* resultVariable,
                                            u64 resultVariableSize, void* option) {
    auto ret = sceNpTusTryAndSetVariableAAsync(reqId, targetAccountId, slotId, opeType, variable,
                                               compareValue, isLastChangedAuthor, isLastChangedDate,
                                               resultVariable, resultVariableSize, option);
    if (ret < 0) {
        return ret;
    }
    // 0 on normal termination, including when the condition was not met.
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableAAsync(
    int reqId, OrbisNpAccountId targetAccountId, s32 slotId, s32 opeType, s64 variable,
    const s64* compareValue, const OrbisNpAccountId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate, OrbisNpTusVariableA* resultVariable,
    u64 resultVariableSize, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetAccountId = {}, slotId = {}, opeType = {}, variable = {}, "
             "compareValue = {}, resultVariable = {}, resultVariableSize = {}, option = {}",
             reqId, targetAccountId, slotId, opeType, variable, fmt::ptr(compareValue),
             fmt::ptr(resultVariable), resultVariableSize, fmt::ptr(option));
    if (!resultVariable) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (opeType < ORBIS_NP_TUS_OPETYPE_EQUAL || opeType > ORBIS_NP_TUS_OPETYPE_LESS_OR_EQUAL) {
        LOG_ERROR(Lib_NpTus, "undefined opeType {}", opeType);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_TYPE;
    }
    if (resultVariableSize != sizeof(OrbisNpTusVariableA)) {
        LOG_ERROR(Lib_NpTus,
                  "size mismatch: resultVariableSize={} but sizeof(OrbisNpTusVariableA)={}",
                  resultVariableSize, sizeof(OrbisNpTusVariableA));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const bool hasAuthorCheck = isLastChangedAuthor != nullptr;
    const bool hasDateCheck = isLastChangedDate != nullptr;

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusTryAndSetVariable(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), /*virtualUser=*/std::string(),
        static_cast<s64>(targetAccountId), slotId, opeType, variable, compareValue != nullptr,
        compareValue ? *compareValue : variable, hasAuthorCheck,
        hasAuthorCheck ? static_cast<s64>(*isLastChangedAuthor) : 0,
        /*isLastChangedAuthorNpId=*/std::string(), hasDateCheck,
        hasDateCheck ? isLastChangedDate->tick : 0, /*variableOut=*/nullptr,
        /*variableAOut=*/resultVariable, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableAVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32 slotId, s32 opeType,
    s64 variable, const s64* compareValue, const OrbisNpAccountId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate, OrbisNpTusVariableA* resultVariable,
    u64 resultVariableSize, void* option) {
    auto ret = sceNpTusTryAndSetVariableAVUserAsync(
        reqId, targetVirtualUserId, slotId, opeType, variable, compareValue, isLastChangedAuthor,
        isLastChangedDate, resultVariable, resultVariableSize, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableAVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32 slotId, s32 opeType,
    s64 variable, const s64* compareValue, const OrbisNpAccountId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate, OrbisNpTusVariableA* resultVariable,
    u64 resultVariableSize, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetVirtualUserId = {}, slotId = {}, opeType = {}, variable = {}, "
             "compareValue = {}, resultVariable = {}, resultVariableSize = {}, option = {}",
             reqId, targetVirtualUserId ? targetVirtualUserId->data : "", slotId, opeType, variable,
             fmt::ptr(compareValue), fmt::ptr(resultVariable), resultVariableSize,
             fmt::ptr(option));
    if (!targetVirtualUserId || !resultVariable) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (opeType < ORBIS_NP_TUS_OPETYPE_EQUAL || opeType > ORBIS_NP_TUS_OPETYPE_LESS_OR_EQUAL) {
        LOG_ERROR(Lib_NpTus, "undefined opeType {}", opeType);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_TYPE;
    }
    if (resultVariableSize != sizeof(OrbisNpTusVariableA)) {
        LOG_ERROR(Lib_NpTus,
                  "size mismatch: resultVariableSize={} but sizeof(OrbisNpTusVariableA)={}",
                  resultVariableSize, sizeof(OrbisNpTusVariableA));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    const std::string virtualUser(
        targetVirtualUserId->data,
        strnlen(targetVirtualUserId->data, sizeof(targetVirtualUserId->data)));
    if (virtualUser.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const bool hasAuthorCheck = isLastChangedAuthor != nullptr;
    const bool hasDateCheck = isLastChangedDate != nullptr;

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusTryAndSetVariable(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), virtualUser,
        /*ownerAccountId=*/0, slotId, opeType, variable, compareValue != nullptr,
        compareValue ? *compareValue : variable, hasAuthorCheck,
        hasAuthorCheck ? static_cast<s64>(*isLastChangedAuthor) : 0,
        /*isLastChangedAuthorNpId=*/std::string(), hasDateCheck,
        hasDateCheck ? isLastChangedDate->tick : 0, /*variableOut=*/nullptr,
        /*variableAOut=*/resultVariable, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotDataAAsync(int reqId, OrbisNpAccountId targetAccountId,
                                                   s32* slotIds, int arrayLen, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetAccountId = {}, slotIds = {}, arrayLen = {}, option = {}", reqId,
             targetAccountId, fmt::ptr(slotIds), arrayLen, fmt::ptr(option));
    if (!slotIds) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || arrayLen < 1) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots: arrayLen={} (max {})", arrayLen,
                  TusMaxSlotsPerRequest);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slotIds, slotIds + arrayLen, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slotIds, slotIds + arrayLen);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusDeleteMultiSlotData(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), /*virtualUser=*/std::string(),
        static_cast<s64>(targetAccountId), slots, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotDataA(int reqId, OrbisNpAccountId targetAccountId,
                                              s32* slotIds, int arrayLen, void* option) {
    auto ret = sceNpTusDeleteMultiSlotDataAAsync(reqId, targetAccountId, slotIds, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    // Returns 0 on normal termination.
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotVariableA(int reqId, OrbisNpAccountId targetAccountId,
                                                  s32* slotIds, int arrayLen, void* option) {
    auto ret =
        sceNpTusDeleteMultiSlotVariableAAsync(reqId, targetAccountId, slotIds, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    // Returns 0 on normal termination.
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotVariableAAsync(int reqId, OrbisNpAccountId targetAccountId,
                                                       s32* slotIds, int arrayLen, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetAccountId = {}, slotIds = {}, arrayLen = {}, option = {}", reqId,
             targetAccountId, fmt::ptr(slotIds), arrayLen, fmt::ptr(option));
    if (!slotIds) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || arrayLen < 1) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots: arrayLen={} (max {})", arrayLen,
                  TusMaxSlotsPerRequest);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slotIds, slotIds + arrayLen, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slotIds, slotIds + arrayLen);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusDeleteMultiSlotVariable(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), /*virtualUser=*/std::string(),
        static_cast<s64>(targetAccountId), slots, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetFriendsDataStatusA(int reqId, s32 slotId, s32 includeSelf, s32 sortType,
                                               OrbisNpTusDataStatusA* statusArray,
                                               u64 statusArraySize, int arrayLen,
                                               OrbisNpTusGetFriendsDataStatusOptParam* option) {
    auto ret = sceNpTusGetFriendsDataStatusAAsync(reqId, slotId, includeSelf, sortType, statusArray,
                                                  statusArraySize, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    // Returns the number of data statuses read (>= 0).
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetFriendsDataStatusAAsync(
    int reqId, s32 slotId, s32 includeSelf, s32 sortType, OrbisNpTusDataStatusA* statusArray,
    u64 statusArraySize, int arrayLen, OrbisNpTusGetFriendsDataStatusOptParam* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, slotId = {}, includeSelf = {}, sortType = {}, statusArray = {}, "
             "statusArraySize = {}, arrayLen = {}, option = {}",
             reqId, slotId, includeSelf, sortType, fmt::ptr(statusArray), statusArraySize, arrayLen,
             fmt::ptr(option));
    if (!statusArray) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (arrayLen < 1 || slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxSelectedFriends) {
        LOG_ERROR(Lib_NpTus, "too many entries: arrayLen={} (max {})", arrayLen,
                  TusMaxSelectedFriends);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    // Data statuses only sort by date (no value ordering).
    if (sortType < ORBIS_NP_TUS_DATASTATUS_SORTTYPE_DESCENDING_DATE ||
        sortType > ORBIS_NP_TUS_DATASTATUS_SORTTYPE_ASCENDING_DATE) {
        LOG_ERROR(Lib_NpTus, "invalid sortType {}", sortType);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (statusArraySize != static_cast<u64>(arrayLen) * sizeof(OrbisNpTusDataStatusA)) {
        LOG_ERROR(
            Lib_NpTus,
            "size mismatch: statusArraySize={} but arrayLen {} * sizeof(OrbisNpTusDataStatusA)={}",
            statusArraySize, arrayLen, static_cast<u64>(arrayLen) * sizeof(OrbisNpTusDataStatusA));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    u32 startOffset = 0;
    u32* hitsOut = nullptr;
    if (option) {
        if (option->size != sizeof(OrbisNpTusGetFriendsDataStatusOptParam)) {
            LOG_ERROR(Lib_NpTus,
                      "size mismatch: option->size={} but "
                      "sizeof(OrbisNpTusGetFriendsDataStatusOptParam)={}",
                      option->size, sizeof(OrbisNpTusGetFriendsDataStatusOptParam));
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
        }
        // startSerialRank is a 1-based rank
        if (option->startSerialRank) {
            const u32 rank = *option->startSerialRank;
            if (rank == 0) {
                LOG_ERROR(Lib_NpTus, "startSerialRank must be 1-based");
                return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
            }
            startOffset = rank - 1;
        }
        hitsOut = option->hits;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetFriendsDataStatus(
        uid, static_cast<s32>(svc), slotId, includeSelf != 0, sortType, static_cast<u32>(arrayLen),
        /*statusOut=*/nullptr, /*statusAOut=*/statusArray, static_cast<u64>(arrayLen), ctx,
        startOffset, hitsOut);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetFriendsVariableA(int reqId, s32 slotId, s32 includeSelf, s32 sortType,
                                             OrbisNpTusVariableA* variableArray,
                                             u64 variableArraySize, int arrayLen,
                                             OrbisNpTusGetFriendsVariableOptParam* option) {
    auto ret = sceNpTusGetFriendsVariableAAsync(reqId, slotId, includeSelf, sortType, variableArray,
                                                variableArraySize, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    // Returns the number of variables read (>= 0).
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetFriendsVariableAAsync(int reqId, s32 slotId, s32 includeSelf,
                                                  s32 sortType, OrbisNpTusVariableA* variableArray,
                                                  u64 variableArraySize, int arrayLen,
                                                  OrbisNpTusGetFriendsVariableOptParam* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, slotId = {}, includeSelf = {}, sortType = {}, variableArray = {}, "
             "variableArraySize = {}, arrayLen = {}, option = {}",
             reqId, slotId, includeSelf, sortType, fmt::ptr(variableArray), variableArraySize,
             arrayLen, fmt::ptr(option));
    if (!variableArray) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (arrayLen < 1 || slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxSelectedFriends) {
        LOG_ERROR(Lib_NpTus, "too many entries: arrayLen={} (max {})", arrayLen,
                  TusMaxSelectedFriends);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (sortType < ORBIS_NP_TUS_VARIABLE_SORTTYPE_DESCENDING_DATE ||
        sortType > ORBIS_NP_TUS_VARIABLE_SORTTYPE_ASCENDING_VALUE) {
        LOG_ERROR(Lib_NpTus, "invalid sortType {}", sortType);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (variableArraySize != static_cast<u64>(arrayLen) * sizeof(OrbisNpTusVariableA)) {
        LOG_ERROR(
            Lib_NpTus,
            "size mismatch: variableArraySize={} but arrayLen {} * sizeof(OrbisNpTusVariableA)={}",
            variableArraySize, arrayLen, static_cast<u64>(arrayLen) * sizeof(OrbisNpTusVariableA));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    u32 startOffset = 0;
    u32* hitsOut = nullptr;
    if (option) {
        if (option->size != sizeof(OrbisNpTusGetFriendsVariableOptParam)) {
            LOG_ERROR(Lib_NpTus,
                      "size mismatch: option->size={} but "
                      "sizeof(OrbisNpTusGetFriendsVariableOptParam)={}",
                      option->size, sizeof(OrbisNpTusGetFriendsVariableOptParam));
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
        }
        // startSerialRank is a 1-based rank
        if (option->startSerialRank) {
            const u32 rank = *option->startSerialRank;
            if (rank == 0) {
                LOG_ERROR(Lib_NpTus, "startSerialRank must be 1-based");
                return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
            }
            startOffset = rank - 1;
        }
        hitsOut = option->hits;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetFriendsVariable(
        uid, static_cast<s32>(svc), slotId, includeSelf != 0, sortType, static_cast<u32>(arrayLen),
        /*variablesOut=*/nullptr, /*variablesAOut=*/variableArray, static_cast<u64>(arrayLen), ctx,
        startOffset, hitsOut);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32* slotIds,
    OrbisNpTusVariable* variableArray, u64 variableArraySize, int arrayLen, void* option) {
    auto ret = sceNpTusGetMultiSlotVariableVUserAsync(
        reqId, targetVirtualUserId, slotIds, variableArray, variableArraySize, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    // Returns the number of variables read (>= 0).
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32* slotIds,
    OrbisNpTusVariable* variableArray, u64 variableArraySize, int arrayLen, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetVirtualUserId = {}, slotIds = {}, variableArray = {}, "
             "variableArraySize = {}, arrayLen = {}, option = {}",
             reqId, targetVirtualUserId ? targetVirtualUserId->data : "", fmt::ptr(slotIds),
             fmt::ptr(variableArray), variableArraySize, arrayLen, fmt::ptr(option));
    if (!targetVirtualUserId || !slotIds || !variableArray || arrayLen < 1) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots: arrayLen={} (max {})", arrayLen,
                  TusMaxSlotsPerRequest);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slotIds, slotIds + arrayLen, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (variableArraySize != static_cast<u64>(arrayLen) * sizeof(OrbisNpTusVariable)) {
        LOG_ERROR(
            Lib_NpTus,
            "size mismatch: variableArraySize={} but arrayLen {} * sizeof(OrbisNpTusVariable)={}",
            variableArraySize, arrayLen, static_cast<u64>(arrayLen) * sizeof(OrbisNpTusVariable));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    const std::string virtualUser(
        targetVirtualUserId->data,
        strnlen(targetVirtualUserId->data, sizeof(targetVirtualUserId->data)));
    if (virtualUser.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slotIds, slotIds + arrayLen);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetMultiSlotVariable(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), virtualUser, slots,
        /*variablesOut=*/variableArray, static_cast<u64>(arrayLen), ctx,
        /*variablesAOut=*/nullptr, /*ownerAccountId=*/0);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusA(int reqId,
                                                 const OrbisNpAccountId* targetAccountIdArray,
                                                 s32 slotId, OrbisNpTusDataStatusA* statusArray,
                                                 u64 statusArraySize, int arrayLen, void* option) {
    auto ret = sceNpTusGetMultiUserDataStatusAAsync(reqId, targetAccountIdArray, slotId,
                                                    statusArray, statusArraySize, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    // Returns the number of statuses obtained (>= 0).
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusAAsync(
    int reqId, const OrbisNpAccountId* targetAccountIdArray, s32 slotId,
    OrbisNpTusDataStatusA* statusArray, u64 statusArraySize, int arrayLen, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetAccountIdArray = {}, slotId = {}, statusArray = {}, "
             "statusArraySize = {}, arrayLen = {}, option = {}",
             reqId, fmt::ptr(targetAccountIdArray), slotId, fmt::ptr(statusArray), statusArraySize,
             arrayLen, fmt::ptr(option));
    if (!targetAccountIdArray || !statusArray) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || arrayLen < 1 || slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxUsersPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many users: arrayLen={} (max {})", arrayLen,
                  TusMaxUsersPerRequest);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (statusArraySize != static_cast<u64>(arrayLen) * sizeof(OrbisNpTusDataStatusA)) {
        LOG_ERROR(
            Lib_NpTus,
            "size mismatch: statusArraySize={} but arrayLen {} * sizeof(OrbisNpTusDataStatusA)={}",
            statusArraySize, arrayLen, static_cast<u64>(arrayLen) * sizeof(OrbisNpTusDataStatusA));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    // Account ids map straight to the server's owner key, no npid resolution.
    std::vector<s64> accounts;
    accounts.reserve(arrayLen);
    for (int i = 0; i < arrayLen; ++i) {
        accounts.push_back(static_cast<s64>(targetAccountIdArray[i]));
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetMultiUserDataStatus(
        uid, static_cast<s32>(svc), slotId, /*ownerNpIds=*/{}, /*virtualUsers=*/{}, accounts,
        /*statusOut=*/nullptr, /*statusAOut=*/statusArray, static_cast<u64>(arrayLen), ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusAVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserIdArray, s32 slotId,
    OrbisNpTusDataStatusA* statusArray, u64 statusArraySize, int arrayLen, void* option) {
    auto ret = sceNpTusGetMultiUserDataStatusAVUserAsync(
        reqId, targetVirtualUserIdArray, slotId, statusArray, statusArraySize, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusAVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserIdArray, s32 slotId,
    OrbisNpTusDataStatusA* statusArray, u64 statusArraySize, int arrayLen, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetVirtualUserIdArray = {}, slotId = {}, statusArray = {}, "
             "statusArraySize = {}, arrayLen = {}, option = {}",
             reqId, fmt::ptr(targetVirtualUserIdArray), slotId, fmt::ptr(statusArray),
             statusArraySize, arrayLen, fmt::ptr(option));
    if (!targetVirtualUserIdArray || !statusArray) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || arrayLen < 1 || slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxUsersPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many users: arrayLen={} (max {})", arrayLen,
                  TusMaxUsersPerRequest);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (statusArraySize != static_cast<u64>(arrayLen) * sizeof(OrbisNpTusDataStatusA)) {
        LOG_ERROR(
            Lib_NpTus,
            "size mismatch: statusArraySize={} but arrayLen {} * sizeof(OrbisNpTusDataStatusA)={}",
            statusArraySize, arrayLen, static_cast<u64>(arrayLen) * sizeof(OrbisNpTusDataStatusA));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    std::vector<std::string> vusers;
    vusers.reserve(arrayLen);
    for (int i = 0; i < arrayLen; ++i) {
        const char* d = targetVirtualUserIdArray[i].data;
        std::string name(d, strnlen(d, sizeof(targetVirtualUserIdArray[i].data)));
        if (name.empty()) {
            // An empty entry would silently fall through to account targeting.
            LOG_ERROR(Lib_NpTus, "empty virtual user at index {}", i);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
        }
        vusers.emplace_back(std::move(name));
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetMultiUserDataStatus(
        uid, static_cast<s32>(svc), slotId, /*ownerNpIds=*/{}, vusers, /*ownerAccountIds=*/{},
        /*statusOut=*/nullptr, /*statusAOut=*/statusArray, static_cast<u64>(arrayLen), ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableA(int reqId,
                                               const OrbisNpAccountId* targetAccountIdArray,
                                               s32 slotId, OrbisNpTusVariableA* variableArray,
                                               u64 variableArraySize, int arrayLen, void* option) {
    auto ret = sceNpTusGetMultiUserVariableAAsync(
        reqId, targetAccountIdArray, slotId, variableArray, variableArraySize, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    // Returns the number of variables read (>= 0).
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableAAsync(int reqId,
                                                    const OrbisNpAccountId* targetAccountIdArray,
                                                    s32 slotId, OrbisNpTusVariableA* variableArray,
                                                    u64 variableArraySize, int arrayLen,
                                                    void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetAccountIdArray = {}, slotId = {}, variableArray = {}, "
             "variableArraySize = {}, arrayLen = {}, option = {}",
             reqId, fmt::ptr(targetAccountIdArray), slotId, fmt::ptr(variableArray),
             variableArraySize, arrayLen, fmt::ptr(option));
    if (!targetAccountIdArray || !variableArray) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || arrayLen < 1 || slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxUsersPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many users: arrayLen={} (max {})", arrayLen,
                  TusMaxUsersPerRequest);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (variableArraySize != static_cast<u64>(arrayLen) * sizeof(OrbisNpTusVariableA)) {
        LOG_ERROR(
            Lib_NpTus,
            "size mismatch: variableArraySize={} but arrayLen {} * sizeof(OrbisNpTusVariableA)={}",
            variableArraySize, arrayLen, static_cast<u64>(arrayLen) * sizeof(OrbisNpTusVariableA));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    // Account ids map straight to the server's owner key, no npid resolution.
    std::vector<s64> accounts;
    accounts.reserve(arrayLen);
    for (int i = 0; i < arrayLen; ++i) {
        accounts.push_back(static_cast<s64>(targetAccountIdArray[i]));
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetMultiUserVariable(
        uid, static_cast<s32>(svc), slotId, /*ownerNpIds=*/{}, /*virtualUsers=*/{}, accounts,
        /*variablesOut=*/nullptr, /*variablesAOut=*/variableArray, static_cast<u64>(arrayLen), ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableAVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserIdArray, s32 slotId,
    OrbisNpTusVariableA* variableArray, u64 variableArraySize, int arrayLen, void* option) {
    auto ret =
        sceNpTusGetMultiUserVariableAVUserAsync(reqId, targetVirtualUserIdArray, slotId,
                                                variableArray, variableArraySize, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableAVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserIdArray, s32 slotId,
    OrbisNpTusVariableA* variableArray, u64 variableArraySize, int arrayLen, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetVirtualUserIdArray = {}, slotId = {}, variableArray = {}, "
             "variableArraySize = {}, arrayLen = {}, option = {}",
             reqId, fmt::ptr(targetVirtualUserIdArray), slotId, fmt::ptr(variableArray),
             variableArraySize, arrayLen, fmt::ptr(option));
    if (!targetVirtualUserIdArray || !variableArray) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || arrayLen < 1 || slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxUsersPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many users: arrayLen={} (max {})", arrayLen,
                  TusMaxUsersPerRequest);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (variableArraySize != static_cast<u64>(arrayLen) * sizeof(OrbisNpTusVariableA)) {
        LOG_ERROR(
            Lib_NpTus,
            "size mismatch: variableArraySize={} but arrayLen {} * sizeof(OrbisNpTusVariableA)={}",
            variableArraySize, arrayLen, static_cast<u64>(arrayLen) * sizeof(OrbisNpTusVariableA));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    std::vector<std::string> vusers;
    vusers.reserve(arrayLen);
    for (int i = 0; i < arrayLen; ++i) {
        const char* d = targetVirtualUserIdArray[i].data;
        std::string name(d, strnlen(d, sizeof(targetVirtualUserIdArray[i].data)));
        if (name.empty()) {
            // An empty entry would silently fall through to account targeting.
            LOG_ERROR(Lib_NpTus, "empty virtual user at index {}", i);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
        }
        vusers.emplace_back(std::move(name));
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetMultiUserVariable(
        uid, static_cast<s32>(svc), slotId, /*ownerNpIds=*/{}, vusers, /*ownerAccountIds=*/{},
        /*variablesOut=*/nullptr, /*variablesAOut=*/variableArray, static_cast<u64>(arrayLen), ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableForCrossSave(
    int reqId, OrbisNpAccountId targetAccountId, s32 slotId, s64 inVariable,
    const OrbisNpAccountId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
    OrbisNpTusVariableForCrossSave* outVariable, u64 outVariableSize, void* option) {
    auto ret = sceNpTusAddAndGetVariableForCrossSaveAsync(
        reqId, targetAccountId, slotId, inVariable, isLastChangedAuthor, isLastChangedDate,
        outVariable, outVariableSize, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableForCrossSaveAsync(
    int reqId, OrbisNpAccountId targetAccountId, s32 slotId, s64 inVariable,
    const OrbisNpAccountId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
    OrbisNpTusVariableForCrossSave* outVariable, u64 outVariableSize, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetAccountId = {}, slotId = {}, inVariable = {}, outVariable = {}, "
             "outVariableSize = {}, option = {}",
             reqId, targetAccountId, slotId, inVariable, fmt::ptr(outVariable), outVariableSize,
             fmt::ptr(option));
    if (option || slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (!outVariable) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (outVariableSize != sizeof(OrbisNpTusVariableForCrossSave)) {
        LOG_ERROR(Lib_NpTus,
                  "size mismatch: outVariableSize={} but sizeof(OrbisNpTusVariableForCrossSave)={}",
                  outVariableSize, sizeof(OrbisNpTusVariableForCrossSave));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const bool hasAuthorCheck = isLastChangedAuthor != nullptr;
    const bool hasDateCheck = isLastChangedDate != nullptr;

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusAddAndGetVariable(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), /*virtualUser=*/std::string(),
        static_cast<s64>(targetAccountId), slotId, inVariable, hasAuthorCheck,
        hasAuthorCheck ? static_cast<s64>(*isLastChangedAuthor) : 0,
        /*isLastChangedAuthorNpId=*/std::string(), hasDateCheck,
        hasDateCheck ? isLastChangedDate->tick : 0, /*variableOut=*/nullptr,
        /*variableAOut=*/nullptr, ctx, /*variableCSOut=*/outVariable);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableForCrossSaveVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32 slotId, s64 inVariable,
    const OrbisNpAccountId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
    OrbisNpTusVariableForCrossSave* outVariable, u64 outVariableSize, void* option) {
    auto ret = sceNpTusAddAndGetVariableForCrossSaveVUserAsync(
        reqId, targetVirtualUserId, slotId, inVariable, isLastChangedAuthor, isLastChangedDate,
        outVariable, outVariableSize, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableForCrossSaveVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32 slotId, s64 inVariable,
    const OrbisNpAccountId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
    OrbisNpTusVariableForCrossSave* outVariable, u64 outVariableSize, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetVirtualUserId = {}, slotId = {}, inVariable = {}, "
             "outVariable = {}, outVariableSize = {}, option = {}",
             reqId, targetVirtualUserId ? targetVirtualUserId->data : "", slotId, inVariable,
             fmt::ptr(outVariable), outVariableSize, fmt::ptr(option));
    if (!targetVirtualUserId || !outVariable) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (outVariableSize != sizeof(OrbisNpTusVariableForCrossSave)) {
        LOG_ERROR(Lib_NpTus,
                  "size mismatch: outVariableSize={} but sizeof(OrbisNpTusVariableForCrossSave)={}",
                  outVariableSize, sizeof(OrbisNpTusVariableForCrossSave));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    const std::string virtualUser(
        targetVirtualUserId->data,
        strnlen(targetVirtualUserId->data, sizeof(targetVirtualUserId->data)));
    if (virtualUser.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const bool hasAuthorCheck = isLastChangedAuthor != nullptr;
    const bool hasDateCheck = isLastChangedDate != nullptr;

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusAddAndGetVariable(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), virtualUser,
        /*ownerAccountId=*/0, slotId, inVariable, hasAuthorCheck,
        hasAuthorCheck ? static_cast<s64>(*isLastChangedAuthor) : 0,
        /*isLastChangedAuthorNpId=*/std::string(), hasDateCheck,
        hasDateCheck ? isLastChangedDate->tick : 0, /*variableOut=*/nullptr,
        /*variableAOut=*/nullptr, ctx, /*variableCSOut=*/outVariable);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetDataForCrossSave(int reqId, OrbisNpAccountId targetAccountId,
                                             s32 slotId,
                                             OrbisNpTusDataStatusForCrossSave* dataStatus,
                                             u64 dataStatusSize, void* data, u64 recvSize,
                                             void* option) {
    auto ret = sceNpTusGetDataForCrossSaveAsync(reqId, targetAccountId, slotId, dataStatus,
                                                dataStatusSize, data, recvSize, option);
    if (ret < 0) {
        return ret;
    }
    // Returns the size of the received data (>= 0).
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetDataForCrossSaveAsync(int reqId, OrbisNpAccountId targetAccountId,
                                                  s32 slotId,
                                                  OrbisNpTusDataStatusForCrossSave* dataStatus,
                                                  u64 dataStatusSize, void* data, u64 recvSize,
                                                  void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetAccountId = {}, slotId = {}, dataStatus = {}, "
             "dataStatusSize = {}, data = {}, recvSize = {}, option = {}",
             reqId, targetAccountId, slotId, fmt::ptr(dataStatus), dataStatusSize, fmt::ptr(data),
             recvSize, fmt::ptr(option));
    if (option || slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (!dataStatus || dataStatusSize == 0) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (dataStatusSize != sizeof(OrbisNpTusDataStatusForCrossSave)) {
        LOG_ERROR(Lib_NpTus,
                  "size mismatch: dataStatusSize={} but "
                  "sizeof(OrbisNpTusDataStatusForCrossSave)={}",
                  dataStatusSize, sizeof(OrbisNpTusDataStatusForCrossSave));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }

    const s64 account = static_cast<s64>(targetAccountId);
    u64 offset = 0;
    {
        std::lock_guard lock(g_mutex);
        if (!req->xfer.SameTarget(slotId, account, std::string())) {
            req->xfer.Begin(slotId, account, std::string());
        }
        offset = req->xfer.recvOffset;
        if (data) {
            req->xfer.recvOffset += recvSize;
        }
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetData(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), /*virtualUser=*/std::string(),
        account, slotId, /*statusAOut=*/nullptr, dataStatusSize, data, data ? recvSize : 0, offset,
        ctx, /*statusOut=*/nullptr, /*statusCSOut=*/dataStatus);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
sceNpTusGetDataForCrossSaveVUser(int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId,
                                 s32 slotId, OrbisNpTusDataStatusForCrossSave* dataStatus,
                                 u64 dataStatusSize, void* data, u64 recvSize, void* option) {
    auto ret = sceNpTusGetDataForCrossSaveVUserAsync(reqId, targetVirtualUserId, slotId, dataStatus,
                                                     dataStatusSize, data, recvSize, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI
sceNpTusGetDataForCrossSaveVUserAsync(int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId,
                                      s32 slotId, OrbisNpTusDataStatusForCrossSave* dataStatus,
                                      u64 dataStatusSize, void* data, u64 recvSize, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetVirtualUserId = {}, slotId = {}, dataStatus = {}, "
             "dataStatusSize = {}, data = {}, recvSize = {}, option = {}",
             reqId, targetVirtualUserId ? targetVirtualUserId->data : "", slotId,
             fmt::ptr(dataStatus), dataStatusSize, fmt::ptr(data), recvSize, fmt::ptr(option));
    if (option || slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    // NULL targetVirtualUserId is explicitly INSUFFICIENT_ARGUMENT.
    if (!targetVirtualUserId || !dataStatus || dataStatusSize == 0) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (dataStatusSize != sizeof(OrbisNpTusDataStatusForCrossSave)) {
        LOG_ERROR(Lib_NpTus,
                  "size mismatch: dataStatusSize={} but "
                  "sizeof(OrbisNpTusDataStatusForCrossSave)={}",
                  dataStatusSize, sizeof(OrbisNpTusDataStatusForCrossSave));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    const std::string virtualUser(
        targetVirtualUserId->data,
        strnlen(targetVirtualUserId->data, sizeof(targetVirtualUserId->data)));
    if (virtualUser.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }

    u64 offset = 0;
    {
        std::lock_guard lock(g_mutex);
        if (!req->xfer.SameTarget(slotId, 0, virtualUser)) {
            req->xfer.Begin(slotId, 0, virtualUser);
        }
        offset = req->xfer.recvOffset;
        if (data) {
            req->xfer.recvOffset += recvSize;
        }
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetData(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), virtualUser, /*ownerAccountId=*/0,
        slotId, /*statusAOut=*/nullptr, dataStatusSize, data, data ? recvSize : 0, offset, ctx,
        /*statusOut=*/nullptr, /*statusCSOut=*/dataStatus);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetFriendsDataStatusForCrossSave(
    int reqId, s32 slotId, s32 includeSelf, s32 sortType,
    OrbisNpTusDataStatusForCrossSave* statusArray, u64 statusArraySize, int arrayLen,
    OrbisNpTusGetFriendsDataStatusOptParam* option) {
    auto ret = sceNpTusGetFriendsDataStatusForCrossSaveAsync(
        reqId, slotId, includeSelf, sortType, statusArray, statusArraySize, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetFriendsDataStatusForCrossSaveAsync(
    int reqId, s32 slotId, s32 includeSelf, s32 sortType,
    OrbisNpTusDataStatusForCrossSave* statusArray, u64 statusArraySize, int arrayLen,
    OrbisNpTusGetFriendsDataStatusOptParam* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, slotId = {}, includeSelf = {}, sortType = {}, statusArray = {}, "
             "statusArraySize = {}, arrayLen = {}, option = {}",
             reqId, slotId, includeSelf, sortType, fmt::ptr(statusArray), statusArraySize, arrayLen,
             fmt::ptr(option));
    if (!statusArray) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (arrayLen < 1 || slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxSelectedFriends) {
        LOG_ERROR(Lib_NpTus, "too many entries: arrayLen={} (max {})", arrayLen,
                  TusMaxSelectedFriends);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    // Data statuses only sort by date (no value ordering).
    if (sortType < ORBIS_NP_TUS_DATASTATUS_SORTTYPE_DESCENDING_DATE ||
        sortType > ORBIS_NP_TUS_DATASTATUS_SORTTYPE_ASCENDING_DATE) {
        LOG_ERROR(Lib_NpTus, "invalid sortType {}", sortType);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (statusArraySize != static_cast<u64>(arrayLen) * sizeof(OrbisNpTusDataStatusForCrossSave)) {
        LOG_ERROR(Lib_NpTus,
                  "size mismatch: statusArraySize={} but arrayLen {} * "
                  "sizeof(OrbisNpTusDataStatusForCrossSave)={}",
                  statusArraySize, arrayLen,
                  static_cast<u64>(arrayLen) * sizeof(OrbisNpTusDataStatusForCrossSave));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    u32 startOffset = 0;
    u32* hitsOut = nullptr;
    if (option) {
        if (option->size != sizeof(OrbisNpTusGetFriendsDataStatusOptParam)) {
            LOG_ERROR(Lib_NpTus,
                      "size mismatch: option->size={} but "
                      "sizeof(OrbisNpTusGetFriendsDataStatusOptParam)={}",
                      option->size, sizeof(OrbisNpTusGetFriendsDataStatusOptParam));
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
        }
        // startSerialRank is a 1-based rank
        if (option->startSerialRank) {
            const u32 rank = *option->startSerialRank;
            if (rank == 0) {
                LOG_ERROR(Lib_NpTus, "startSerialRank must be 1-based");
                return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
            }
            startOffset = rank - 1;
        }
        hitsOut = option->hits;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetFriendsDataStatus(
        uid, static_cast<s32>(svc), slotId, includeSelf != 0, sortType, static_cast<u32>(arrayLen),
        /*statusOut=*/nullptr, /*statusAOut=*/nullptr, static_cast<u64>(arrayLen), ctx, startOffset,
        hitsOut, /*statusCSOut=*/statusArray);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetFriendsVariableForCrossSave(
    int reqId, s32 slotId, s32 includeSelf, s32 sortType,
    OrbisNpTusVariableForCrossSave* variableArray, u64 variableArraySize, int arrayLen,
    OrbisNpTusGetFriendsVariableOptParam* option) {
    auto ret = sceNpTusGetFriendsVariableForCrossSaveAsync(
        reqId, slotId, includeSelf, sortType, variableArray, variableArraySize, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetFriendsVariableForCrossSaveAsync(
    int reqId, s32 slotId, s32 includeSelf, s32 sortType,
    OrbisNpTusVariableForCrossSave* variableArray, u64 variableArraySize, int arrayLen,
    OrbisNpTusGetFriendsVariableOptParam* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, slotId = {}, includeSelf = {}, sortType = {}, variableArray = {}, "
             "variableArraySize = {}, arrayLen = {}, option = {}",
             reqId, slotId, includeSelf, sortType, fmt::ptr(variableArray), variableArraySize,
             arrayLen, fmt::ptr(option));
    if (!variableArray) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (arrayLen < 1 || slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxSelectedFriends) {
        LOG_ERROR(Lib_NpTus, "too many entries: arrayLen={} (max {})", arrayLen,
                  TusMaxSelectedFriends);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (sortType < ORBIS_NP_TUS_VARIABLE_SORTTYPE_DESCENDING_DATE ||
        sortType > ORBIS_NP_TUS_VARIABLE_SORTTYPE_ASCENDING_VALUE) {
        LOG_ERROR(Lib_NpTus, "invalid sortType {}", sortType);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (variableArraySize != static_cast<u64>(arrayLen) * sizeof(OrbisNpTusVariableForCrossSave)) {
        LOG_ERROR(Lib_NpTus,
                  "size mismatch: variableArraySize={} but arrayLen {} * "
                  "sizeof(OrbisNpTusVariableForCrossSave)={}",
                  variableArraySize, arrayLen,
                  static_cast<u64>(arrayLen) * sizeof(OrbisNpTusVariableForCrossSave));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    u32 startOffset = 0;
    u32* hitsOut = nullptr;
    if (option) {
        if (option->size != sizeof(OrbisNpTusGetFriendsVariableOptParam)) {
            LOG_ERROR(Lib_NpTus,
                      "size mismatch: option->size={} but "
                      "sizeof(OrbisNpTusGetFriendsVariableOptParam)={}",
                      option->size, sizeof(OrbisNpTusGetFriendsVariableOptParam));
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
        }
        // startSerialRank is a 1-based rank
        if (option->startSerialRank) {
            const u32 rank = *option->startSerialRank;
            if (rank == 0) {
                LOG_ERROR(Lib_NpTus, "startSerialRank must be 1-based");
                return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
            }
            startOffset = rank - 1;
        }
        hitsOut = option->hits;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetFriendsVariable(
        uid, static_cast<s32>(svc), slotId, includeSelf != 0, sortType, static_cast<u32>(arrayLen),
        /*variablesOut=*/nullptr, /*variablesAOut=*/nullptr, static_cast<u64>(arrayLen), ctx,
        startOffset, hitsOut, /*variablesCSOut=*/variableArray);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusForCrossSave(
    int reqId, OrbisNpAccountId targetAccountId, s32* slotIds,
    OrbisNpTusDataStatusForCrossSave* statusArray, u64 statusArraySize, int arrayLen,
    void* option) {
    auto ret = sceNpTusGetMultiSlotDataStatusForCrossSaveAsync(
        reqId, targetAccountId, slotIds, statusArray, statusArraySize, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusForCrossSaveAsync(
    int reqId, OrbisNpAccountId targetAccountId, s32* slotIds,
    OrbisNpTusDataStatusForCrossSave* statusArray, u64 statusArraySize, int arrayLen,
    void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetAccountId = {}, slotIds = {}, statusArray = {}, "
             "statusArraySize = {}, arrayLen = {}, option = {}",
             reqId, targetAccountId, fmt::ptr(slotIds), fmt::ptr(statusArray), statusArraySize,
             arrayLen, fmt::ptr(option));
    if (!slotIds || !statusArray) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || arrayLen < 1) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots: arrayLen={} (max {})", arrayLen,
                  TusMaxSlotsPerRequest);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slotIds, slotIds + arrayLen, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (statusArraySize != static_cast<u64>(arrayLen) * sizeof(OrbisNpTusDataStatusForCrossSave)) {
        LOG_ERROR(Lib_NpTus,
                  "size mismatch: statusArraySize={} but arrayLen {} * "
                  "sizeof(OrbisNpTusDataStatusForCrossSave)={}",
                  statusArraySize, arrayLen,
                  static_cast<u64>(arrayLen) * sizeof(OrbisNpTusDataStatusForCrossSave));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slotIds, slotIds + arrayLen);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetMultiSlotDataStatus(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), /*virtualUser=*/std::string(),
        static_cast<s64>(targetAccountId), slots, /*statusOut=*/nullptr,
        /*statusAOut=*/nullptr, static_cast<u64>(arrayLen), ctx, /*statusCSOut=*/statusArray);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusForCrossSaveVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32* slotIds,
    OrbisNpTusDataStatusForCrossSave* statusArray, u64 statusArraySize, int arrayLen,
    void* option) {
    auto ret = sceNpTusGetMultiSlotDataStatusForCrossSaveVUserAsync(
        reqId, targetVirtualUserId, slotIds, statusArray, statusArraySize, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusForCrossSaveVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32* slotIds,
    OrbisNpTusDataStatusForCrossSave* statusArray, u64 statusArraySize, int arrayLen,
    void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetVirtualUserId = {}, slotIds = {}, statusArray = {}, "
             "statusArraySize = {}, arrayLen = {}, option = {}",
             reqId, targetVirtualUserId ? targetVirtualUserId->data : "", fmt::ptr(slotIds),
             fmt::ptr(statusArray), statusArraySize, arrayLen, fmt::ptr(option));
    if (!targetVirtualUserId || !slotIds || !statusArray) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || arrayLen < 1) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots: arrayLen={} (max {})", arrayLen,
                  TusMaxSlotsPerRequest);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slotIds, slotIds + arrayLen, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (statusArraySize != static_cast<u64>(arrayLen) * sizeof(OrbisNpTusDataStatusForCrossSave)) {
        LOG_ERROR(Lib_NpTus,
                  "size mismatch: statusArraySize={} but arrayLen {} * "
                  "sizeof(OrbisNpTusDataStatusForCrossSave)={}",
                  statusArraySize, arrayLen,
                  static_cast<u64>(arrayLen) * sizeof(OrbisNpTusDataStatusForCrossSave));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    const std::string virtualUser(
        targetVirtualUserId->data,
        strnlen(targetVirtualUserId->data, sizeof(targetVirtualUserId->data)));
    if (virtualUser.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slotIds, slotIds + arrayLen);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetMultiSlotDataStatus(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), virtualUser,
        /*ownerAccountId=*/0, slots, /*statusOut=*/nullptr, /*statusAOut=*/nullptr,
        static_cast<u64>(arrayLen), ctx, /*statusCSOut=*/statusArray);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableForCrossSave(
    int reqId, OrbisNpAccountId targetAccountId, s32* slotIds,
    OrbisNpTusVariableForCrossSave* variableArray, u64 variablesSize, int arrayLen, void* option) {
    auto ret = sceNpTusGetMultiSlotVariableForCrossSaveAsync(
        reqId, targetAccountId, slotIds, variableArray, variablesSize, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableForCrossSaveAsync(
    int reqId, OrbisNpAccountId targetAccountId, s32* slotIds,
    OrbisNpTusVariableForCrossSave* variableArray, u64 variablesSize, int arrayLen, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetAccountId = {}, slotIds = {}, variableArray = {}, "
             "variablesSize = {}, arrayLen = {}, option = {}",
             reqId, targetAccountId, fmt::ptr(slotIds), fmt::ptr(variableArray), variablesSize,
             arrayLen, fmt::ptr(option));
    if (!slotIds || !variableArray) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (arrayLen < 1 || option) {
        LOG_ERROR(Lib_NpTus, "Invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "Too many slots requested: {}", arrayLen);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slotIds, slotIds + arrayLen, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "Invalid slot ID found");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (variablesSize != static_cast<u64>(arrayLen) * sizeof(OrbisNpTusVariableForCrossSave)) {
        LOG_ERROR(Lib_NpTus,
                  "size mismatch: variablesSize={} but arrayLen {} * "
                  "sizeof(OrbisNpTusVariableForCrossSave)={}",
                  variablesSize, arrayLen,
                  static_cast<u64>(arrayLen) * sizeof(OrbisNpTusVariableForCrossSave));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slotIds, slotIds + arrayLen);
    const int n = arrayLen;
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetMultiSlotVariable(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), /*virtualUser=*/std::string(),
        slots, /*variablesOut=*/nullptr, static_cast<u64>(n), ctx, /*variablesAOut=*/nullptr,
        static_cast<s64>(targetAccountId), /*variablesCSOut=*/variableArray);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableForCrossSaveVUser(
    int reqId, const OrbisNpTusVirtualUserId* virtualUserId, s32* slotIds,
    OrbisNpTusVariableForCrossSave* variableArray, u64 variablesSize, int arrayLen, void* option) {
    auto ret = sceNpTusGetMultiSlotVariableForCrossSaveVUserAsync(
        reqId, virtualUserId, slotIds, variableArray, variablesSize, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableForCrossSaveVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* virtualUserId, s32* slotIds,
    OrbisNpTusVariableForCrossSave* variableArray, u64 variablesSize, int arrayLen, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, virtualUserId = {}, slotIds = {}, variableArray = {}, "
             "variablesSize = {}, arrayLen = {}, option = {}",
             reqId, virtualUserId ? virtualUserId->data : "", fmt::ptr(slotIds),
             fmt::ptr(variableArray), variablesSize, arrayLen, fmt::ptr(option));
    if (!virtualUserId || !slotIds || !variableArray) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (arrayLen < 1 || option) {
        LOG_ERROR(Lib_NpTus, "Invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "Too many slots requested: {}", arrayLen);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slotIds, slotIds + arrayLen, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "Invalid slot ID found");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (variablesSize != static_cast<u64>(arrayLen) * sizeof(OrbisNpTusVariableForCrossSave)) {
        LOG_ERROR(Lib_NpTus,
                  "size mismatch: variablesSize={} but arrayLen {} * "
                  "sizeof(OrbisNpTusVariableForCrossSave)={}",
                  variablesSize, arrayLen,
                  static_cast<u64>(arrayLen) * sizeof(OrbisNpTusVariableForCrossSave));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    const std::string virtualUser(virtualUserId->data,
                                  strnlen(virtualUserId->data, sizeof(virtualUserId->data)));
    if (virtualUser.empty()) {
        LOG_ERROR(Lib_NpTus, "Invalid virtual user ID");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slotIds, slotIds + arrayLen);
    const int n = arrayLen;
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetMultiSlotVariable(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), virtualUser, slots,
        /*variablesOut=*/nullptr, static_cast<u64>(n), ctx, /*variablesAOut=*/nullptr,
        /*ownerAccountId=*/0, /*variablesCSOut=*/variableArray);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusForCrossSave(
    int reqId, const OrbisNpAccountId* targetAccountIdArray, s32 slotId,
    OrbisNpTusDataStatusForCrossSave* statusArray, u64 statusArraySize, int arrayLen,
    void* option) {
    auto ret = sceNpTusGetMultiUserDataStatusForCrossSaveAsync(
        reqId, targetAccountIdArray, slotId, statusArray, statusArraySize, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusForCrossSaveAsync(
    int reqId, const OrbisNpAccountId* targetAccountIdArray, s32 slotId,
    OrbisNpTusDataStatusForCrossSave* statusArray, u64 statusArraySize, int arrayLen,
    void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetAccountIdArray = {}, slotId = {}, statusArray = {}, "
             "statusArraySize = {}, arrayLen = {}, option = {}",
             reqId, fmt::ptr(targetAccountIdArray), slotId, fmt::ptr(statusArray), statusArraySize,
             arrayLen, fmt::ptr(option));
    if (!targetAccountIdArray || !statusArray) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || arrayLen < 1 || slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxUsersPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many users: arrayLen={} (max {})", arrayLen,
                  TusMaxUsersPerRequest);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (statusArraySize != static_cast<u64>(arrayLen) * sizeof(OrbisNpTusDataStatusForCrossSave)) {
        LOG_ERROR(Lib_NpTus,
                  "size mismatch: statusArraySize={} but arrayLen {} * "
                  "sizeof(OrbisNpTusDataStatusForCrossSave)={}",
                  statusArraySize, arrayLen,
                  static_cast<u64>(arrayLen) * sizeof(OrbisNpTusDataStatusForCrossSave));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    // Account ids map straight to the server's owner key, no npid resolution.
    std::vector<s64> accounts;
    accounts.reserve(arrayLen);
    for (int i = 0; i < arrayLen; ++i) {
        accounts.push_back(static_cast<s64>(targetAccountIdArray[i]));
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetMultiUserDataStatus(
        uid, static_cast<s32>(svc), slotId, /*ownerNpIds=*/{}, /*virtualUsers=*/{}, accounts,
        /*statusOut=*/nullptr, /*statusAOut=*/nullptr, static_cast<u64>(arrayLen), ctx,
        /*statusCSOut=*/statusArray);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusForCrossSaveVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserIdArray, s32 slotId,
    OrbisNpTusDataStatusForCrossSave* statusArray, u64 statusArraySize, int arrayLen,
    void* option) {
    auto ret = sceNpTusGetMultiUserDataStatusForCrossSaveVUserAsync(
        reqId, targetVirtualUserIdArray, slotId, statusArray, statusArraySize, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusForCrossSaveVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserIdArray, s32 slotId,
    OrbisNpTusDataStatusForCrossSave* statusArray, u64 statusArraySize, int arrayLen,
    void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetVirtualUserIdArray = {}, slotId = {}, statusArray = {}, "
             "statusArraySize = {}, arrayLen = {}, option = {}",
             reqId, fmt::ptr(targetVirtualUserIdArray), slotId, fmt::ptr(statusArray),
             statusArraySize, arrayLen, fmt::ptr(option));
    if (!targetVirtualUserIdArray || !statusArray) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || arrayLen < 1 || slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxUsersPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many users: arrayLen={} (max {})", arrayLen,
                  TusMaxUsersPerRequest);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (statusArraySize != static_cast<u64>(arrayLen) * sizeof(OrbisNpTusDataStatusForCrossSave)) {
        LOG_ERROR(Lib_NpTus,
                  "size mismatch: statusArraySize={} but arrayLen {} * "
                  "sizeof(OrbisNpTusDataStatusForCrossSave)={}",
                  statusArraySize, arrayLen,
                  static_cast<u64>(arrayLen) * sizeof(OrbisNpTusDataStatusForCrossSave));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    std::vector<std::string> vusers;
    vusers.reserve(arrayLen);
    for (int i = 0; i < arrayLen; ++i) {
        const char* d = targetVirtualUserIdArray[i].data;
        std::string name(d, strnlen(d, sizeof(targetVirtualUserIdArray[i].data)));
        if (name.empty()) {
            // An empty entry would silently fall through to account targeting.
            LOG_ERROR(Lib_NpTus, "empty virtual user at index {}", i);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
        }
        vusers.emplace_back(std::move(name));
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetMultiUserDataStatus(
        uid, static_cast<s32>(svc), slotId, /*ownerNpIds=*/{}, vusers, /*ownerAccountIds=*/{},
        /*statusOut=*/nullptr, /*statusAOut=*/nullptr, static_cast<u64>(arrayLen), ctx,
        /*statusCSOut=*/statusArray);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
sceNpTusGetMultiUserVariableForCrossSave(int reqId, const OrbisNpAccountId* targetAccountIdArray,
                                         s32 slotId, OrbisNpTusVariableForCrossSave* variableArray,
                                         u64 variableArraySize, int arrayLen, void* option) {
    auto ret = sceNpTusGetMultiUserVariableForCrossSaveAsync(
        reqId, targetAccountIdArray, slotId, variableArray, variableArraySize, arrayLen, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableForCrossSaveAsync(
    int reqId, const OrbisNpAccountId* targetAccountIdArray, s32 slotId,
    OrbisNpTusVariableForCrossSave* variableArray, u64 variableArraySize, int arrayLen,
    void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetAccountIdArray = {}, slotId = {}, variableArray = {}, "
             "variableArraySize = {}, arrayLen = {}, option = {}",
             reqId, fmt::ptr(targetAccountIdArray), slotId, fmt::ptr(variableArray),
             variableArraySize, arrayLen, fmt::ptr(option));
    if (!targetAccountIdArray || !variableArray) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || arrayLen < 1 || slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxUsersPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many users: arrayLen={} (max {})", arrayLen,
                  TusMaxUsersPerRequest);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (variableArraySize != static_cast<u64>(arrayLen) * sizeof(OrbisNpTusVariableForCrossSave)) {
        LOG_ERROR(Lib_NpTus,
                  "size mismatch: variableArraySize={} but arrayLen {} * "
                  "sizeof(OrbisNpTusVariableForCrossSave)={}",
                  variableArraySize, arrayLen,
                  static_cast<u64>(arrayLen) * sizeof(OrbisNpTusVariableForCrossSave));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s64> accounts;
    accounts.reserve(arrayLen);
    for (int i = 0; i < arrayLen; ++i) {
        accounts.push_back(static_cast<s64>(targetAccountIdArray[i]));
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetMultiUserVariable(
        uid, static_cast<s32>(svc), slotId, /*ownerNpIds=*/{}, /*virtualUsers=*/{}, accounts,
        /*variablesOut=*/nullptr, /*variablesAOut=*/nullptr, static_cast<u64>(arrayLen), ctx,
        /*variablesCSOut=*/variableArray);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableForCrossSaveVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserIdArray, s32 slotId,
    OrbisNpTusVariableForCrossSave* variableArray, u64 variableArraySize, int arrayLen,
    void* option) {
    auto ret = sceNpTusGetMultiUserVariableForCrossSaveVUserAsync(
        reqId, targetVirtualUserIdArray, slotId, variableArray, variableArraySize, arrayLen,
        option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableForCrossSaveVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserIdArray, s32 slotId,
    OrbisNpTusVariableForCrossSave* variableArray, u64 variableArraySize, int arrayLen,
    void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetVirtualUserIdArray = {}, slotId = {}, variableArray = {}, "
             "variableArraySize = {}, arrayLen = {}, option = {}",
             reqId, fmt::ptr(targetVirtualUserIdArray), slotId, fmt::ptr(variableArray),
             variableArraySize, arrayLen, fmt::ptr(option));
    if (!targetVirtualUserIdArray || !variableArray) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || arrayLen < 1 || slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (arrayLen > TusMaxUsersPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many users: arrayLen={} (max {})", arrayLen,
                  TusMaxUsersPerRequest);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (variableArraySize != static_cast<u64>(arrayLen) * sizeof(OrbisNpTusVariableForCrossSave)) {
        LOG_ERROR(Lib_NpTus,
                  "size mismatch: variableArraySize={} but arrayLen {} * "
                  "sizeof(OrbisNpTusVariableForCrossSave)={}",
                  variableArraySize, arrayLen,
                  static_cast<u64>(arrayLen) * sizeof(OrbisNpTusVariableForCrossSave));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    std::vector<std::string> vusers;
    vusers.reserve(arrayLen);
    for (int i = 0; i < arrayLen; ++i) {
        const char* d = targetVirtualUserIdArray[i].data;
        std::string name(d, strnlen(d, sizeof(targetVirtualUserIdArray[i].data)));
        if (name.empty()) {
            LOG_ERROR(Lib_NpTus, "empty virtual user at index {}", i);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
        }
        vusers.emplace_back(std::move(name));
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusGetMultiUserVariable(
        uid, static_cast<s32>(svc), slotId, /*ownerNpIds=*/{}, vusers, /*ownerAccountIds=*/{},
        /*variablesOut=*/nullptr, /*variablesAOut=*/nullptr, static_cast<u64>(arrayLen), ctx,
        /*variablesCSOut=*/variableArray);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableForCrossSave(
    int reqId, OrbisNpAccountId targetAccountId, s32 slotId, s32 opeType, s64 variable,
    const s64* compareValue, const OrbisNpAccountId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
    OrbisNpTusVariableForCrossSave* resultVariable, u64 resultVariableSize, void* option) {
    auto ret = sceNpTusTryAndSetVariableForCrossSaveAsync(
        reqId, targetAccountId, slotId, opeType, variable, compareValue, isLastChangedAuthor,
        isLastChangedDate, resultVariable, resultVariableSize, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableForCrossSaveAsync(
    int reqId, OrbisNpAccountId targetAccountId, s32 slotId, s32 opeType, s64 variable,
    const s64* compareValue, const OrbisNpAccountId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
    OrbisNpTusVariableForCrossSave* resultVariable, u64 resultVariableSize, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetAccountId = {}, slotId = {}, opeType = {}, variable = {}, "
             "compareValue = {}, resultVariable = {}, resultVariableSize = {}, option = {}",
             reqId, targetAccountId, slotId, opeType, variable, fmt::ptr(compareValue),
             fmt::ptr(resultVariable), resultVariableSize, fmt::ptr(option));
    if (!resultVariable) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (opeType < ORBIS_NP_TUS_OPETYPE_EQUAL || opeType > ORBIS_NP_TUS_OPETYPE_LESS_OR_EQUAL) {
        LOG_ERROR(Lib_NpTus, "undefined opeType {}", opeType);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_TYPE;
    }
    if (resultVariableSize != sizeof(OrbisNpTusVariableForCrossSave)) {
        LOG_ERROR(
            Lib_NpTus,
            "size mismatch: resultVariableSize={} but sizeof(OrbisNpTusVariableForCrossSave)={}",
            resultVariableSize, sizeof(OrbisNpTusVariableForCrossSave));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const bool hasAuthorCheck = isLastChangedAuthor != nullptr;
    const bool hasDateCheck = isLastChangedDate != nullptr;

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusTryAndSetVariable(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), /*virtualUser=*/std::string(),
        static_cast<s64>(targetAccountId), slotId, opeType, variable, compareValue != nullptr,
        compareValue ? *compareValue : variable, hasAuthorCheck,
        hasAuthorCheck ? static_cast<s64>(*isLastChangedAuthor) : 0,
        /*isLastChangedAuthorNpId=*/std::string(), hasDateCheck,
        hasDateCheck ? isLastChangedDate->tick : 0, /*variableOut=*/nullptr,
        /*variableAOut=*/nullptr, ctx, /*variableCSOut=*/resultVariable);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableForCrossSaveVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32 slotId, s32 opeType,
    s64 variable, const s64* compareValue, const OrbisNpAccountId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
    OrbisNpTusVariableForCrossSave* resultVariable, u64 resultVariableSize, void* option) {
    auto ret = sceNpTusTryAndSetVariableForCrossSaveVUserAsync(
        reqId, targetVirtualUserId, slotId, opeType, variable, compareValue, isLastChangedAuthor,
        isLastChangedDate, resultVariable, resultVariableSize, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(reqId, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableForCrossSaveVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32 slotId, s32 opeType,
    s64 variable, const s64* compareValue, const OrbisNpAccountId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
    OrbisNpTusVariableForCrossSave* resultVariable, u64 resultVariableSize, void* option) {
    LOG_INFO(Lib_NpTus,
             "reqId = {}, targetVirtualUserId = {}, slotId = {}, opeType = {}, variable = {}, "
             "compareValue = {}, resultVariable = {}, resultVariableSize = {}, option = {}",
             reqId, targetVirtualUserId ? targetVirtualUserId->data : "", slotId, opeType, variable,
             fmt::ptr(compareValue), fmt::ptr(resultVariable), resultVariableSize,
             fmt::ptr(option));
    if (!targetVirtualUserId || !resultVariable) {
        LOG_ERROR(Lib_NpTus, "insufficient argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || slotId < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (opeType < ORBIS_NP_TUS_OPETYPE_EQUAL || opeType > ORBIS_NP_TUS_OPETYPE_LESS_OR_EQUAL) {
        LOG_ERROR(Lib_NpTus, "undefined opeType {}", opeType);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_TYPE;
    }
    if (resultVariableSize != sizeof(OrbisNpTusVariableForCrossSave)) {
        LOG_ERROR(
            Lib_NpTus,
            "size mismatch: resultVariableSize={} but sizeof(OrbisNpTusVariableForCrossSave)={}",
            resultVariableSize, sizeof(OrbisNpTusVariableForCrossSave));
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    const std::string virtualUser(
        targetVirtualUserId->data,
        strnlen(targetVirtualUserId->data, sizeof(targetVirtualUserId->data)));
    if (virtualUser.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(reqId, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const bool hasAuthorCheck = isLastChangedAuthor != nullptr;
    const bool hasDateCheck = isLastChangedDate != nullptr;

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = Libraries::Np::NpHandler::GetInstance().TusTryAndSetVariable(
        uid, static_cast<s32>(svc), /*ownerNpId=*/std::string(), virtualUser,
        /*ownerAccountId=*/0, slotId, opeType, variable, compareValue != nullptr,
        compareValue ? *compareValue : variable, hasAuthorCheck,
        hasAuthorCheck ? static_cast<s64>(*isLastChangedAuthor) : 0,
        /*isLastChangedAuthorNpId=*/std::string(), hasDateCheck,
        hasDateCheck ? isLastChangedDate->tick : 0, /*variableOut=*/nullptr,
        /*variableAOut=*/nullptr, ctx, /*variableCSOut=*/resultVariable);
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
        LOG_ERROR(Lib_NpTus, "Invalid option provided");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    if (dataStatus && dataStatusSize != 0x18) {
        LOG_ERROR(Lib_NpTus, "Invalid data status size");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    if (slotId < 0 || slotId > 15) {
        LOG_ERROR(Lib_NpTus, "Invalid slot ID");
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

s32 PS4_SYSV_ABI sceNpTssGetStorageAsync(int reqId) {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called, faking async completion");
    return FakeAsyncComplete(reqId);
}

s32 PS4_SYSV_ABI sceNpTssGetData() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTssGetSmallStorage() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTssGetSmallStorageAsync(int reqId) {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called, faking async completion");
    return FakeAsyncComplete(reqId);
}

s32 PS4_SYSV_ABI sceNpTssGetStorage() {
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

s32 PS4_SYSV_ABI sceNpTusSetThreadParam() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTusSetTimeout() {
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