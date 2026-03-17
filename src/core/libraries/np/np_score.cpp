// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#ifdef _WIN32
#ifndef NOGDI
#define NOGDI
#endif
#endif

#include <future>

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_manager.h"
#include "core/libraries/np/np_score.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/np/object_manager.h"
#include "core/libraries/system/userservice.h"
#include "cppcodec/base64_rfc4648.hpp"
#include "externals/cpp-httplib/httplib.h"
#include "nlohmann/json.hpp"

namespace Libraries::Np::NpScore {

using json = nlohmann::json;
using base64 = cppcodec::base64_rfc4648;

const std::string BASE_URL = "http://127.0.0.1:3000";

int PackReqId(int libCtxId, int reqId) {
    return ((libCtxId & 0xFFFF) << 16) | (reqId & 0xFFFF);
}

std::pair<int, int> UnpackReqId(int reqId) {
    return {reqId >> 16, reqId & 0xFFFF};
}

bool IsReqId(int id) {
    return id > (1 << 16);
}

httplib::Client NewClient() {
    httplib::Client client(BASE_URL);

    client.set_default_headers({{"X-NP-TITLE-ID", NpManager::g_np_title_id.id}});
    client.set_bearer_token_auth("BEARER_TOKEN"); // fix when auth manager

    client.set_logger([](const httplib::Request& req, const httplib::Response& res) {
        std::cout << "✓ " << req.method << " " << req.path << " -> " << res.status << " ("
                  << res.body.size() << ") bytes " << res.body << std::endl;
    });
    client.set_error_logger([](const httplib::Error& err, const httplib::Request* req) {
        std::cerr << "✗ ";
        if (req) {
            std::cerr << req->method << " " << req->path << " ";
        }
        std::cerr << "failed: " << httplib::to_string(err);

        // Add specific guidance based on error type
        switch (err) {
        case httplib::Error::Connection:
            std::cerr << " (verify server is running and reachable)";
            break;
        case httplib::Error::SSLConnection:
            std::cerr << " (check SSL certificate and TLS configuration)";
            break;
        case httplib::Error::ConnectionTimeout:
            std::cerr << " (increase timeout or check network latency)";
            break;
        case httplib::Error::Read:
            std::cerr << " (server may have closed connection prematurely)";
            break;
        default:
            break;
        }
        std::cerr << std::endl;
    });

    return client;
}

struct NpScoreRequest {
    u32 pcId;
    std::future<int> requestFuture;

    void GetRankingByRange(OrbisNpScoreBoardId scoreBoardId, OrbisNpScoreRankNumber startRank,
                           OrbisNpScoreRankDataA* ranks, u64 ranksLen,
                           OrbisNpScoreGameInfo* gameInfos, OrbisNpScoreComment* comments,
                           Rtc::OrbisRtcTick* lastUpdate, OrbisNpScoreRankNumber* totalRecords) {
        requestFuture = std::async(std::launch::async, [=]() {
            httplib::Client client = NewClient();
            auto uri = std::format("/score/v1/ranking/{}/by-range?startRank={}&scores={}",
                                   scoreBoardId, startRank, ranksLen);

            auto res = client.Get(uri);

            if (res && res->status == httplib::StatusCode::OK_200) {
                auto json = json::parse(res->body);

                auto card = json["card"].get<u32>();

                if (totalRecords) {
                    *totalRecords = card;
                }

                auto json_ranks = json["ranks"];
                auto ranks_obtained = 0;
                for (auto r : json_ranks) {
                    strcpy(ranks[ranks_obtained].onlineId.data, "shadps4");
                    ranks[ranks_obtained].pcId = r["id"]["pcId"].get<u32>();
                    // most probably not correct but need more RE to figure out what are the
                    // different ranks here
                    ranks[ranks_obtained].rank1 = r["rank"].get<u32>();
                    ranks[ranks_obtained].rank2 = r["rank"].get<u32>();
                    ranks[ranks_obtained].rank3 = r["rank"].get<u32>();
                    ranks[ranks_obtained].hasGameData = false;
                    ranks[ranks_obtained].score = r["score"].get<s64>();
                    Libraries::Rtc::sceRtcGetCurrentTick(&ranks[ranks_obtained].recordTime);
                    ranks[ranks_obtained].accountId = r["id"]["accountId"].get<u64>();
                    if (gameInfos && r.contains("gameInfo")) {
                        std::vector<u8> decoded = base64::decode(r["gameInfo"].get<std::string>());
                        gameInfos[ranks_obtained].dataSize = decoded.size();
                        if (decoded.size() > sizeof(OrbisNpScoreGameInfo::data)) {
                            LOG_WARNING(Lib_NpScore, "gameInfo is too long! {} bytes",
                                        decoded.size());
                        } else {
                            std::memcpy(gameInfos[ranks_obtained].data, decoded.data(),
                                        decoded.size());
                        }
                    }
                    if (comments && r.contains("comment")) {
                        LOG_WARNING(Lib_NpScore, "score comment returned but not handled");
                    }
                    ranks_obtained++;
                }

                if (lastUpdate) {
                    Libraries::Rtc::sceRtcGetCurrentTick(lastUpdate);
                }

                return ranks_obtained;
            }

            return -1;
        });
    }

    void RecordScore(OrbisNpScoreBoardId boardId, OrbisNpScoreValue score,
                     OrbisNpScoreComment* comment, OrbisNpScoreGameInfo* gameInfo) {
        requestFuture = std::async(std::launch::async, [=, this]() {
            httplib::Client client = NewClient();
            auto uri = std::format("/score/v1/ranking/{}", boardId);
            json payload;

            payload["pcId"] = this->pcId;
            payload["value"] = score;

            if (comment) {
                LOG_DEBUG(Lib_NpScore, "score comment {}", comment->comment);
                payload["comment"] = base64::encode(comment->comment);
            }

            if (gameInfo) {
                payload["gameInfo"] = base64::encode(gameInfo->data);
            }

            auto res = client.Post(uri, payload.dump(), "application/json");

            return 0;
        });
    }
};

using NpScoreRequestManager =
    ObjectManager<NpScoreRequest, 32, ORBIS_NP_COMMUNITY_ERROR_INVALID_ID,
                  ORBIS_NP_COMMUNITY_ERROR_INVALID_ID, ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_OBJECTS>;

struct NpScoreTitleContext {
    u32 serviceLabel;
    OrbisNpId npId;
    u32 pcId;
    NpScoreRequestManager requestManager;

    s32 GetRequest(int reqId, NpScoreRequest** out) {
        NpScoreRequest* req = nullptr;
        if (auto ret = requestManager.GetObject(reqId, &req); ret < 0) {
            return ret;
        }

        *out = req;

        return ORBIS_OK;
    }

    s32 DeleteRequest(int reqId) {
        return requestManager.DeleteObject(reqId);
    }
};

using NpScoreContextManager =
    ObjectManager<NpScoreTitleContext, 32, ORBIS_NP_COMMUNITY_ERROR_INVALID_ID,
                  ORBIS_NP_COMMUNITY_ERROR_INVALID_ID, ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_OBJECTS>;

static NpScoreContextManager ctxManager;

s32 GetRequest(int requestId, NpScoreRequest** out) {
    auto [ctxId, reqId] = UnpackReqId(requestId);

    NpScoreTitleContext* ctx = nullptr;
    if (auto ret = ctxManager.GetObject(ctxId, &ctx); ret < 0) {
        return ret;
    }

    NpScoreRequest* req = nullptr;
    if (auto ret = ctx->GetRequest(reqId, &req); ret < 0) {
        return ret;
    }

    *out = req;

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreAbortRequest() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreCensorComment() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreCensorCommentAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreChangeModeForOtherSaveDataOwners() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreCreateNpTitleCtx(Libraries::Np::OrbisNpServiceLabel serviceLabel,
                                            OrbisNpId* npId) {
    if (!npId) {
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (serviceLabel == ORBIS_NP_INVALID_SERVICE_LABEL) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    LOG_INFO(Lib_NpScore, "serviceLabel = {}, npId->data = {}", serviceLabel, npId->handle.data);

    return ctxManager.CreateObject(serviceLabel, *npId);
}

int PS4_SYSV_ABI sceNpScoreCreateNpTitleCtxA(
    OrbisNpServiceLabel serviceLabel, Libraries::UserService::OrbisUserServiceUserId userId) {
    LOG_INFO(Lib_NpScore, "serviceLabel = {}, userId = {}", serviceLabel, userId);
    OrbisNpId npId;
    auto ret = NpManager::sceNpGetNpId(userId, &npId);

    if (ret < 0) {
        return ret;
    }

    return sceNpScoreCreateNpTitleCtx(serviceLabel, &npId);
}

int PS4_SYSV_ABI sceNpScoreCreateRequest(int libCtxId) {
    LOG_INFO(Lib_NpScore, "libCtxId = {}", libCtxId);

    NpScoreTitleContext* ctx = nullptr;
    if (auto ret = ctxManager.GetObject(libCtxId, &ctx); ret < 0) {
        return ret;
    }

    auto req = ctx->requestManager.CreateObject(ctx->pcId);
    if (req < 0) {
        return req;
    }

    return PackReqId(libCtxId, req);
}

int PS4_SYSV_ABI sceNpScoreCreateTitleCtx() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreDeleteNpTitleCtx(int libCtxId) {
    LOG_INFO(Lib_NpScore, "libCtxId = {}", libCtxId);

    return ctxManager.DeleteObject(libCtxId);
}

int PS4_SYSV_ABI sceNpScoreDeleteRequest(int requestId) {
    LOG_INFO(Lib_NpScore, "requestId = {:#x}", requestId);

    auto [ctxId, reqId] = UnpackReqId(requestId);

    NpScoreTitleContext* ctx = nullptr;
    if (auto ret = ctxManager.GetObject(ctxId, &ctx); ret < 0) {
        return ret;
    }

    return ctx->DeleteRequest(reqId);
}

int PS4_SYSV_ABI sceNpScoreGetBoardInfo() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreGetBoardInfoAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreGetFriendsRankingAsync(
    int reqId, int scoreBoardId, int includeMe, OrbisNpScoreRankData* rankArray, u64 rankArraySize,
    OrbisNpScoreComment* comments, u64 commentsBytes, OrbisNpScoreGameInfo* gameInfos,
    u64 gameInfosBytes, u64 arrayLen, Libraries::Rtc::OrbisRtcTick* lastUpdate, u64* totalRecords,
    void* option) {
    LOG_ERROR(Lib_NpScore,
              "(STUBBED) reqId = {:#x}, scoreBoardId = {}, includeMe = {}, rankArray = {}, "
              "rankArraySize = {}, comments = {}, commentsBytes = {}, gameInfos = {}, "
              "gameInfosBytes = {}, arrayLen = {}",
              reqId, scoreBoardId, includeMe, fmt::ptr(rankArray), rankArraySize,
              fmt::ptr(comments), commentsBytes, fmt::ptr(gameInfos), gameInfosBytes, arrayLen);

    NpScoreRequest* req = nullptr;
    if (auto ret = GetRequest(reqId, &req); ret < 0) {
        return ret;
    }

    req->requestFuture = std::async([=]() {
        // fake an empty response
        if (totalRecords) {
            *totalRecords = 0;
        }
        if (lastUpdate) {
            Libraries::Rtc::sceRtcGetCurrentTick(lastUpdate);
        }

        return 0;
    });

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreGetFriendsRanking(int reqId, int scoreBoardId, int includeMe,
                                             OrbisNpScoreRankData* rankArray, u64 rankArraySize,
                                             OrbisNpScoreComment* comments, u64 commentsBytes,
                                             OrbisNpScoreGameInfo* gameInfos, u64 gameInfosBytes,
                                             u64 arrayLen, Libraries::Rtc::OrbisRtcTick* lastUpdate,
                                             u64* totalRecords, void* option) {
    LOG_ERROR(Lib_NpScore,
              "(STUBBED) reqId = {:#x}, scoreBoardId = {}, includeMe = {}, rankArray = {}, "
              "rankArraySize = {}, comments = {}, commentsBytes = {}, gameInfos = {}, "
              "gameInfosBytes = {}, arrayLen = {}",
              reqId, scoreBoardId, includeMe, fmt::ptr(rankArray), rankArraySize,
              fmt::ptr(comments), commentsBytes, fmt::ptr(gameInfos), gameInfosBytes, arrayLen);

    auto ret = sceNpScoreGetFriendsRankingAsync(
        reqId, scoreBoardId, includeMe, rankArray, rankArraySize, comments, commentsBytes,
        gameInfos, gameInfosBytes, arrayLen, lastUpdate, totalRecords, option);

    if (ret < 0) {
        return ret;
    }

    int result = 0;
    if (sceNpScoreWaitAsync(reqId, &result) == 0) {
        return result;
    }

    return -1; //?
}

int PS4_SYSV_ABI sceNpScoreGetFriendsRankingA() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreGetFriendsRankingAAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreGetFriendsRankingForCrossSave() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreGetFriendsRankingForCrossSaveAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreGetGameData() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreGetGameDataAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreGetGameDataByAccountId() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreGetGameDataByAccountIdAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreGetRankingByAccountId() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreGetRankingByAccountIdAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreGetRankingByAccountIdForCrossSave() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreGetRankingByAccountIdForCrossSaveAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreGetRankingByAccountIdPcId() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreGetRankingByAccountIdPcIdAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreGetRankingByAccountIdPcIdForCrossSave() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreGetRankingByAccountIdPcIdForCrossSaveAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreGetRankingByNpId() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreGetRankingByNpIdPcIdAsync(
    int reqId, OrbisNpScoreBoardId scoreBoardId, OrbisNpScoreNpIdPcId* accountIds,
    u64 accountIdsBytes, OrbisNpScorePlayerRankData* ranks, u64 ranksBytes,
    OrbisNpScoreComment* comments, u64 commentsBytes, OrbisNpScoreGameInfo* gameInfos,
    u64 gameInfosBytes, u64 accountsLen, Rtc::OrbisRtcTick* lastUpdate,
    OrbisNpScoreRankNumber* totalRecords, void* option) {
    LOG_ERROR(
        Lib_NpScore,
        "(STUBBED) called, reqId = {:#x}, scoreBoardId = {}, accountIds = {}, accountIdsBytes = "
        "{}, ranks = {}, "
        "comments = {}, gameInfos = {}, accountsLen = {}",
        reqId, scoreBoardId, fmt::ptr(accountIds), accountIdsBytes, fmt::ptr(ranks),
        fmt::ptr(comments), fmt::ptr(gameInfos), accountsLen);

    if (option) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (!accountIds || !ranks) {
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (accountsLen > 101) {
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (accountsLen * sizeof(*accountIds) != accountIdsBytes ||
        accountsLen * sizeof(*ranks) != ranksBytes) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    if (comments && accountsLen * sizeof(*comments) != commentsBytes) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    if (gameInfos && accountsLen * sizeof(*gameInfos) != gameInfosBytes) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    // if sdk version > 5.49 zero all of the provided buffers

    NpScoreRequest* req = nullptr;
    if (auto ret = GetRequest(reqId, &req); ret < 0) {
        return ret;
    }

    req->requestFuture = std::async([=]() {
        // fake an empty response
        if (totalRecords) {
            *totalRecords = 0;
        }
        if (lastUpdate) {
            Libraries::Rtc::sceRtcGetCurrentTick(lastUpdate);
        }

        return 0;
    });

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreGetRankingByNpIdAsync(
    int reqId, OrbisNpScoreBoardId scoreBoardId, OrbisNpId* npIds, u64 npIdsBytes,
    OrbisNpScorePlayerRankData* ranks, u64 ranksBytes, OrbisNpScoreComment* comments,
    u64 commentsBytes, OrbisNpScoreGameInfo* gameInfos, u64 gameInfosBytes, u64 accountsLen,
    Rtc::OrbisRtcTick* lastUpdate, OrbisNpScoreRankNumber* totalRecords, void* option) {
    LOG_ERROR(Lib_NpScore,
              "(STUBBED) called, reqId = {:#x}, scoreBoardId = {}, npIds = {}, npIdsBytes = "
              "{}, ranks = {}, "
              "comments = {}, gameInfos = {}, accountsLen = {}",
              reqId, scoreBoardId, fmt::ptr(npIds), npIdsBytes, fmt::ptr(ranks), fmt::ptr(comments),
              fmt::ptr(gameInfos), accountsLen);

    std::vector<OrbisNpScoreNpIdPcId> npIdsPcIds;
    std::ranges::transform(std::span(npIds, accountsLen), std::back_inserter(npIdsPcIds),
                           [](OrbisNpId npId) -> OrbisNpScoreNpIdPcId { return {npId, 0, {}}; });

    return sceNpScoreGetRankingByNpIdPcIdAsync(
        reqId, scoreBoardId, npIdsPcIds.data(), accountsLen * sizeof(OrbisNpScoreNpIdPcId), ranks,
        ranksBytes, comments, commentsBytes, gameInfos, gameInfosBytes, accountsLen, lastUpdate,
        totalRecords, option);
}

int PS4_SYSV_ABI sceNpScoreGetRankingByNpIdPcId(
    int reqId, OrbisNpScoreBoardId scoreBoardId, OrbisNpScoreNpIdPcId* accountIds,
    u64 accountIdsBytes, OrbisNpScorePlayerRankData* ranks, u64 ranksBytes,
    OrbisNpScoreComment* comments, u64 commentsBytes, OrbisNpScoreGameInfo* gameInfos,
    u64 gameInfosBytes, u64 accountsLen, Rtc::OrbisRtcTick* lastUpdate,
    OrbisNpScoreRankNumber* totalRecords, void* option) {
    LOG_ERROR(Lib_NpScore,
              "(STUBBED) called, reqId = {:#x}, scoreBoardId = {}, accountIds = {}, ranks = {}, "
              "comments = {}, gameInfos = {}, accountsLen = {}",
              reqId, scoreBoardId, fmt::ptr(accountIds), fmt::ptr(ranks), fmt::ptr(comments),
              fmt::ptr(gameInfos), accountsLen);

    int ret = sceNpScoreGetRankingByNpIdPcIdAsync(
        reqId, scoreBoardId, accountIds, accountIdsBytes, ranks, ranksBytes, comments,
        commentsBytes, gameInfos, gameInfosBytes, accountsLen, lastUpdate, totalRecords, option);

    if (ret < 0) {
        return ret;
    }

    int result = 0;
    if (sceNpScoreWaitAsync(reqId, &result) == 0) {
        return result;
    }

    return -1; //?
}

int PS4_SYSV_ABI sceNpScoreGetRankingByRange() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreGetRankingByRangeA() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreGetRankingByRangeAsync(
    int reqId, OrbisNpScoreBoardId scoreBoardId, OrbisNpScoreRankNumber startRank,
    OrbisNpScoreRankDataA* ranks, u64 ranksBytes, OrbisNpScoreComment* comments, u64 commentsBytes,
    OrbisNpScoreGameInfo* gameInfos, u64 gameInfosBytes, u64 ranksLen,
    Rtc::OrbisRtcTick* lastUpdate, OrbisNpScoreRankNumber* totalRecords, void* option) {
    LOG_ERROR(Lib_NpScore,
              "(STUBBED) called, reqId = {:#x}, scoreBoardId = {}, startRank = {}, ranks = {}, "
              "comments = {}, gameInfos = {}, ranksLen = {}",
              reqId, scoreBoardId, startRank, fmt::ptr(ranks), fmt::ptr(comments),
              fmt::ptr(gameInfos), ranksLen);

    if (ranksLen * sizeof(*ranks) != ranksBytes) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    if (startRank == 0 || option != nullptr || ranksLen > 100) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    // if sdk version > 1.70 zero all of the provided buffers
    if (ranks == nullptr || ranksLen == 0) {
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (comments) {
        if (ranksLen * sizeof(*comments) != commentsBytes) {
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
        }
    }
    if (gameInfos) {
        if (ranksLen * sizeof(*gameInfos) != gameInfosBytes) {
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
        }
    }

    NpScoreRequest* req = nullptr;
    if (auto ret = GetRequest(reqId, &req); ret < 0) {
        return ret;
    }

    req->GetRankingByRange(scoreBoardId, startRank, ranks, ranksLen, gameInfos, comments,
                           lastUpdate, totalRecords);

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreGetRankingByRangeAAsync(
    int reqId, OrbisNpScoreBoardId scoreBoardId, OrbisNpScoreRankNumber startRank,
    OrbisNpScoreRankDataA* ranks, u64 ranksBytes, OrbisNpScoreComment* comments, u64 commentsBytes,
    OrbisNpScoreGameInfo* gameInfos, u64 gameInfosBytes, u64 ranksLen,
    Rtc::OrbisRtcTick* lastUpdate, OrbisNpScoreRankNumber* totalRecords, void* option) {
    // at least in 9.00 it's identical A vs non-A
    return sceNpScoreGetRankingByRangeAsync(reqId, scoreBoardId, startRank, ranks, ranksBytes,
                                            comments, commentsBytes, gameInfos, gameInfosBytes,
                                            ranksLen, lastUpdate, totalRecords, option);
}

int PS4_SYSV_ABI sceNpScoreGetRankingByRangeForCrossSave() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreGetRankingByRangeForCrossSaveAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScorePollAsync(int reqId, int* result) {
    LOG_INFO(Lib_NpScore, "reqId = {:#x}", reqId);

    NpScoreRequest* req = nullptr;
    if (auto ret = GetRequest(reqId, &req); ret < 0) {
        return ret;
    }

    if (!req->requestFuture.valid()) {
        LOG_ERROR(Lib_NpScore, "request not started");
        return 1;
    }
    if (req->requestFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        LOG_DEBUG(Lib_NpScore, "request finished");
        if (result) {
            *result = req->requestFuture.get();
        }
        return 0;
    }

    return 1;
}

int PS4_SYSV_ABI sceNpScoreRecordGameData() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreRecordGameDataAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreRecordScoreAsync(int reqId, OrbisNpScoreBoardId boardId,
                                            OrbisNpScoreValue score, OrbisNpScoreComment* comment,
                                            OrbisNpScoreGameInfo* gameInfo, void* unk,
                                            Libraries::Rtc::OrbisRtcTick* date, void* option) {
    LOG_ERROR(Lib_NpScore, "reqId = {:#x}, boardId = {}, score = {}", reqId, boardId, score);

    if (option) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpScoreRequest* req = nullptr;
    if (auto ret = GetRequest(reqId, &req); ret < 0) {
        return ret;
    }

    req->RecordScore(boardId, score, comment, gameInfo);

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreRecordScore(int reqId, OrbisNpScoreBoardId boardId,
                                       OrbisNpScoreValue score, OrbisNpScoreComment* comment,
                                       OrbisNpScoreGameInfo* gameInfo, void* unk,
                                       Libraries::Rtc::OrbisRtcTick* date, void* option) {
    LOG_ERROR(Lib_NpScore, "reqId = {:#x}, boardId = {}, score = {}", reqId, boardId, score);

    int ret =
        sceNpScoreRecordScoreAsync(reqId, boardId, score, comment, gameInfo, unk, date, option);

    if (ret < 0) {
        return ret;
    }

    int result = 0;
    if (sceNpScoreWaitAsync(reqId, &result) == 0) {
        return result;
    }

    return -1; //?
}

int PS4_SYSV_ABI sceNpScoreSanitizeComment() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreSanitizeCommentAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreSetPlayerCharacterId(int id, int pcId) {
    LOG_INFO(Lib_NpScore, "id = {:#x}, pcId = {}", id, pcId);

    if (IsReqId(id)) {
        NpScoreRequest* req = nullptr;
        if (auto ret = GetRequest(id, &req); ret < 0) {
            return ret;
        }

        req->pcId = pcId;
    } else {
        NpScoreTitleContext* ctx = nullptr;
        if (auto ret = ctxManager.GetObject(id, &ctx); ret < 0) {
            return ret;
        }

        ctx->pcId = pcId;
    }

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreSetThreadParam() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreSetTimeout() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpScoreWaitAsync(int reqId, int* result) {
    LOG_INFO(Lib_NpScore, "reqId = {:#x}", reqId);

    NpScoreRequest* req = nullptr;
    if (auto ret = GetRequest(reqId, &req); ret < 0) {
        return ret;
    }

    if (!req->requestFuture.valid()) {
        LOG_ERROR(Lib_NpScore, "request not started");
        return 1;
    }

    req->requestFuture.wait();

    LOG_DEBUG(Lib_NpScore, "request finished");
    if (result) {
        *result = req->requestFuture.get();
    }
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("1i7kmKbX6hk", "libSceNpScore", 1, "libSceNpScore", sceNpScoreAbortRequest);
    LIB_FUNCTION("2b3TI0mDYiI", "libSceNpScore", 1, "libSceNpScore", sceNpScoreCensorComment);
    LIB_FUNCTION("4eOvDyN-aZc", "libSceNpScore", 1, "libSceNpScore", sceNpScoreCensorCommentAsync);
    LIB_FUNCTION("dTXC+YcePtM", "libSceNpScore", 1, "libSceNpScore",
                 sceNpScoreChangeModeForOtherSaveDataOwners);
    LIB_FUNCTION("KnNA1TEgtBI", "libSceNpScore", 1, "libSceNpScore", sceNpScoreCreateNpTitleCtx);
    LIB_FUNCTION("GWnWQNXZH5M", "libSceNpScore", 1, "libSceNpScore", sceNpScoreCreateNpTitleCtxA);
    LIB_FUNCTION("gW8qyjYrUbk", "libSceNpScore", 1, "libSceNpScore", sceNpScoreCreateRequest);
    LIB_FUNCTION("qW9M0bQ-Zx0", "libSceNpScore", 1, "libSceNpScore", sceNpScoreCreateTitleCtx);
    LIB_FUNCTION("G0pE+RNCwfk", "libSceNpScore", 1, "libSceNpScore", sceNpScoreDeleteNpTitleCtx);
    LIB_FUNCTION("dK8-SgYf6r4", "libSceNpScore", 1, "libSceNpScore", sceNpScoreDeleteRequest);
    LIB_FUNCTION("LoVMVrijVOk", "libSceNpScore", 1, "libSceNpScore", sceNpScoreGetBoardInfo);
    LIB_FUNCTION("Q0Avi9kebsY", "libSceNpScore", 1, "libSceNpScore", sceNpScoreGetBoardInfoAsync);
    LIB_FUNCTION("8kuIzUw6utQ", "libSceNpScore", 1, "libSceNpScore", sceNpScoreGetFriendsRanking);
    LIB_FUNCTION("gMbOn+-6eXA", "libSceNpScore", 1, "libSceNpScore", sceNpScoreGetFriendsRankingA);
    LIB_FUNCTION("6-G9OxL5DKg", "libSceNpScore", 1, "libSceNpScore",
                 sceNpScoreGetFriendsRankingAAsync);
    LIB_FUNCTION("7SuMUlN7Q6I", "libSceNpScore", 1, "libSceNpScore",
                 sceNpScoreGetFriendsRankingAsync);
    LIB_FUNCTION("AgcxgceaH8k", "libSceNpScore", 1, "libSceNpScore",
                 sceNpScoreGetFriendsRankingForCrossSave);
    LIB_FUNCTION("m6F7sE1HQZU", "libSceNpScore", 1, "libSceNpScore",
                 sceNpScoreGetFriendsRankingForCrossSaveAsync);
    LIB_FUNCTION("zKoVok6FFEI", "libSceNpScore", 1, "libSceNpScore", sceNpScoreGetGameData);
    LIB_FUNCTION("JjOFRVPdQWc", "libSceNpScore", 1, "libSceNpScore", sceNpScoreGetGameDataAsync);
    LIB_FUNCTION("Lmtc9GljeUA", "libSceNpScore", 1, "libSceNpScore",
                 sceNpScoreGetGameDataByAccountId);
    LIB_FUNCTION("PP9jx8s0574", "libSceNpScore", 1, "libSceNpScore",
                 sceNpScoreGetGameDataByAccountIdAsync);
    LIB_FUNCTION("K9tlODTQx3c", "libSceNpScore", 1, "libSceNpScore",
                 sceNpScoreGetRankingByAccountId);
    LIB_FUNCTION("dRszNNyGWkw", "libSceNpScore", 1, "libSceNpScore",
                 sceNpScoreGetRankingByAccountIdAsync);
    LIB_FUNCTION("3Ybj4E1qNtY", "libSceNpScore", 1, "libSceNpScore",
                 sceNpScoreGetRankingByAccountIdForCrossSave);
    LIB_FUNCTION("Kc+3QK84AKM", "libSceNpScore", 1, "libSceNpScore",
                 sceNpScoreGetRankingByAccountIdForCrossSaveAsync);
    LIB_FUNCTION("wJPWycVGzrs", "libSceNpScore", 1, "libSceNpScore",
                 sceNpScoreGetRankingByAccountIdPcId);
    LIB_FUNCTION("bFVjDgxFapc", "libSceNpScore", 1, "libSceNpScore",
                 sceNpScoreGetRankingByAccountIdPcIdAsync);
    LIB_FUNCTION("oXjVieH6ZGQ", "libSceNpScore", 1, "libSceNpScore",
                 sceNpScoreGetRankingByAccountIdPcIdForCrossSave);
    LIB_FUNCTION("nXaF1Bxb-Nw", "libSceNpScore", 1, "libSceNpScore",
                 sceNpScoreGetRankingByAccountIdPcIdForCrossSaveAsync);
    LIB_FUNCTION("9mZEgoiEq6Y", "libSceNpScore", 1, "libSceNpScore", sceNpScoreGetRankingByNpId);
    LIB_FUNCTION("Rd27dqUFZV8", "libSceNpScore", 1, "libSceNpScore",
                 sceNpScoreGetRankingByNpIdAsync);
    LIB_FUNCTION("ETS-uM-vH9Q", "libSceNpScore", 1, "libSceNpScore",
                 sceNpScoreGetRankingByNpIdPcId);
    LIB_FUNCTION("FsouSN0ykN8", "libSceNpScore", 1, "libSceNpScore",
                 sceNpScoreGetRankingByNpIdPcIdAsync);
    LIB_FUNCTION("KBHxDjyk-jA", "libSceNpScore", 1, "libSceNpScore", sceNpScoreGetRankingByRange);
    LIB_FUNCTION("MA9vSt7JImY", "libSceNpScore", 1, "libSceNpScore", sceNpScoreGetRankingByRangeA);
    LIB_FUNCTION("y5ja7WI05rs", "libSceNpScore", 1, "libSceNpScore",
                 sceNpScoreGetRankingByRangeAAsync);
    LIB_FUNCTION("rShmqXHwoQE", "libSceNpScore", 1, "libSceNpScore",
                 sceNpScoreGetRankingByRangeAsync);
    LIB_FUNCTION("nRoYV2yeUuw", "libSceNpScore", 1, "libSceNpScore",
                 sceNpScoreGetRankingByRangeForCrossSave);
    LIB_FUNCTION("AZ4eAlGDy-Q", "libSceNpScore", 1, "libSceNpScore",
                 sceNpScoreGetRankingByRangeForCrossSaveAsync);
    LIB_FUNCTION("m1DfNRstkSQ", "libSceNpScore", 1, "libSceNpScore", sceNpScorePollAsync);
    LIB_FUNCTION("bcoVwcBjQ9E", "libSceNpScore", 1, "libSceNpScore", sceNpScoreRecordGameData);
    LIB_FUNCTION("1gL5PwYzrrw", "libSceNpScore", 1, "libSceNpScore", sceNpScoreRecordGameDataAsync);
    LIB_FUNCTION("zT0XBtgtOSI", "libSceNpScore", 1, "libSceNpScore", sceNpScoreRecordScore);
    LIB_FUNCTION("ANJssPz3mY0", "libSceNpScore", 1, "libSceNpScore", sceNpScoreRecordScoreAsync);
    LIB_FUNCTION("r4oAo9in0TA", "libSceNpScore", 1, "libSceNpScore", sceNpScoreSanitizeComment);
    LIB_FUNCTION("3UVqGJeDf30", "libSceNpScore", 1, "libSceNpScore",
                 sceNpScoreSanitizeCommentAsync);
    LIB_FUNCTION("bygbKdHmjn4", "libSceNpScore", 1, "libSceNpScore",
                 sceNpScoreSetPlayerCharacterId);
    LIB_FUNCTION("yxK68584JAU", "libSceNpScore", 1, "libSceNpScore", sceNpScoreSetThreadParam);
    LIB_FUNCTION("S3xZj35v8Z8", "libSceNpScore", 1, "libSceNpScore", sceNpScoreSetTimeout);
    LIB_FUNCTION("fqk8SC63p1U", "libSceNpScore", 1, "libSceNpScore", sceNpScoreWaitAsync);
    LIB_FUNCTION("KnNA1TEgtBI", "libSceNpScoreCompat", 1, "libSceNpScore",
                 sceNpScoreCreateNpTitleCtx);
    LIB_FUNCTION("8kuIzUw6utQ", "libSceNpScoreCompat", 1, "libSceNpScore",
                 sceNpScoreGetFriendsRanking);
    LIB_FUNCTION("7SuMUlN7Q6I", "libSceNpScoreCompat", 1, "libSceNpScore",
                 sceNpScoreGetFriendsRankingAsync);
    LIB_FUNCTION("zKoVok6FFEI", "libSceNpScoreCompat", 1, "libSceNpScore", sceNpScoreGetGameData);
    LIB_FUNCTION("JjOFRVPdQWc", "libSceNpScoreCompat", 1, "libSceNpScore",
                 sceNpScoreGetGameDataAsync);
    LIB_FUNCTION("9mZEgoiEq6Y", "libSceNpScoreCompat", 1, "libSceNpScore",
                 sceNpScoreGetRankingByNpId);
    LIB_FUNCTION("Rd27dqUFZV8", "libSceNpScoreCompat", 1, "libSceNpScore",
                 sceNpScoreGetRankingByNpIdAsync);
    LIB_FUNCTION("ETS-uM-vH9Q", "libSceNpScoreCompat", 1, "libSceNpScore",
                 sceNpScoreGetRankingByNpIdPcId);
    LIB_FUNCTION("FsouSN0ykN8", "libSceNpScoreCompat", 1, "libSceNpScore",
                 sceNpScoreGetRankingByNpIdPcIdAsync);
    LIB_FUNCTION("KBHxDjyk-jA", "libSceNpScoreCompat", 1, "libSceNpScore",
                 sceNpScoreGetRankingByRange);
    LIB_FUNCTION("rShmqXHwoQE", "libSceNpScoreCompat", 1, "libSceNpScore",
                 sceNpScoreGetRankingByRangeAsync);
};

} // namespace Libraries::Np::NpScore