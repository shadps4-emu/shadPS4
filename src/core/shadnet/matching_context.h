//  SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>

#include "common/singleton.h"
#include "common/types.h"
#include "core/libraries/np/np_matching2.h"
#include "core/libraries/np/np_matching2_requests.h"
#include "nlohmann/json.hpp"

namespace Core::ShadNet {

using namespace Libraries::Np::NpMatching2;
using json = nlohmann::json;

struct Error {
    u32 code;
    std::string message;
};

struct WsEnvelope {
    u64 id;
    std::string type;
    std::string payload;
};

struct WsMessage {
    u64 request_id;
    u32 error_code;
    std::string error;
    json payload;
};

struct WsEvent {
    std::string type;
    json ev;
};

class MatchingContext {
    ix::WebSocket websocket;
    OrbisNpMatching2ContextId ctxId;
    std::mutex mutex;
    std::unordered_map<u64, std::tuple<std::string, std::optional<OrbisNpMatching2RequestOptParam>>>
        pendingRequests;
    std::atomic<u64> reqId{1};
    std::atomic<bool> connected = false;
    std::optional<OrbisNpMatching2RequestOptParam> optParam;
    std::function<void(OrbisNpMatching2ContextId ctxId, OrbisNpMatching2RoomId roomId,
                       OrbisNpMatching2Event ev, const void* data)>
        roomCallback;
    std::function<void(OrbisNpMatching2ContextId ctxId, OrbisNpMatching2RoomId roomId,
                       OrbisNpMatching2RoomMemberId roomMemberId, OrbisNpMatching2Event ev,
                       int errorCode)>
        signalingCallback;

public:
    static void SetContextCallback(OrbisNpMatching2ContextCallback cb, void* userdata);
    int Start(OrbisNpMatching2ContextId ctxId, u64 timeout);
    int Stop();
    void SetRoomCallback(OrbisNpMatching2RoomCallback cb, void* userdata);
    void SetSignalingCallback(OrbisNpMatching2SignalingCallback cb, void* userdata);

    int CreateJoinRoom(const OrbisNpMatching2CreateJoinRoomRequest& req,
                       const OrbisNpMatching2RequestOptParam* optParam);
    int CreateJoinRoom(const OrbisNpMatching2CreateJoinRoomRequestA& req,
                       const OrbisNpMatching2RequestOptParam* optParam);
    int JoinRoom(const OrbisNpMatching2JoinRoomRequest& req,
                 const OrbisNpMatching2RequestOptParam* optParam);
    int LeaveRoom(const OrbisNpMatching2LeaveRoomRequest& req,
                  const OrbisNpMatching2RequestOptParam* optParam);
    int SearchRoom(const OrbisNpMatching2SearchRoomRequest& req,
                   const OrbisNpMatching2RequestOptParam* optParam);
    int SignalingGetPingInfo(const OrbisNpMatching2SignalingGetPingInfoRequest& req,
                             const OrbisNpMatching2RequestOptParam* optParam);
    void SetDefaultRequestOptParam(const OrbisNpMatching2RequestOptParam& optParam);

private:
    auto GetRequestCallback(std::optional<OrbisNpMatching2RequestOptParam> requestOptParam);

    void HandleResponse(const WsMessage& response);
    void HandleEvent(const WsEvent& event);
    void HandleMessage(const std::string& wsResponse);

    template <typename T>
    int SendRequest(const T&, const OrbisNpMatching2RequestOptParam* optParam);
};

} // namespace Core::ShadNet