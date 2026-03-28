//  SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include <magic_enum/magic_enum.hpp>

#include "common/logging/log.h"
#include "core/emulator_settings.h"
#include "core/libraries/network/net_error.h"
#include "core/libraries/np/np_manager.h"
#include "core/shadnet/matching_context.h"
#include "core/shadnet/matching_json.h"

namespace Core::ShadNet {

const char* SHADNET_WS_URL = "ws://127.0.0.1:3000/matching/v1/ws";

using namespace Libraries::Np::NpMatching2;
using json = nlohmann::json;

void to_json(json& j, const WsEnvelope& e) {
    j = json{{"id", e.id}, {"type", e.type}, {"payload", e.payload}};
}

void from_json(const json& j, WsMessage& msg) {
    j.at("id").get_to(msg.request_id);
    if (j.contains("error_code")) {
        j.at("error_code").get_to(msg.error_code);
    }
    if (j.contains("error")) {
        j.at("error").get_to(msg.error);
    }
    if (j.contains("payload")) {
        j.at("payload").get_to(msg.payload);
    }
}

void from_json(const json& j, WsEvent& ev) {
    j.at("type").get_to(ev.type);
    j.at("ev").get_to(ev.ev);
}

struct NpMatching2ContextEvent {
    OrbisNpMatching2ContextId contextId;
    OrbisNpMatching2Event event;
    OrbisNpMatching2EventCause cause;
    int errorCode;
};

std::function<void(const NpMatching2ContextEvent*)> npMatching2ContextCallback = nullptr;

int MatchingContext::Start(OrbisNpMatching2ContextId ctxId, u64 timeout) {
    std::unique_lock lk{this->mutex};

    if (!(EmulatorSettings.IsConnectedToNetwork() && EmulatorSettings.IsPSNSignedIn())) {
        NpMatching2ContextEvent ev{.contextId = this->ctxId,
                                   .event = ORBIS_NP_MATCHING2_CONTEXT_EVENT_START_OVER,
                                   .cause = ORBIS_NP_MATCHING2_EVENT_CAUSE_CONTEXT_ERROR,
                                   .errorCode = ORBIS_NET_ERROR_RESOLVER_ETIMEDOUT};
        npMatching2ContextCallback(&ev);

        return ORBIS_OK;
    }

    if (websocket.isOnMessageCallbackRegistered()) {
        // make it a proper state machine
        return ORBIS_NP_MATCHING2_ERROR_CONTEXT_ALREADY_STARTED;
    }

    LOG_DEBUG(ShadNet, "starting context ctxId = {}", this->ctxId);

    this->ctxId = ctxId;

    ix::initNetSystem();

    websocket.setUrl(SHADNET_WS_URL);
    websocket.setExtraHeaders({
        {"X-NP-TITLE-ID", Libraries::Np::NpManager::g_np_title_id.id},
        {"Authorization", std::format("Bearer {}", "BEARER_TOKEN")} // fix with some auth manager
    });
    websocket.setPingInterval(10);

    auto timeoutSec = 20;
    if (timeout > 10'000'000) {
        timeoutSec = timeout / 1'000'000;
    }
    websocket.setHandshakeTimeout(timeoutSec);

    websocket.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
        switch (msg->type) {
        case ix::WebSocketMessageType::Open: {
            LOG_DEBUG(ShadNet, "ws connection opened for ctxId = {}", this->ctxId);
            connected = true;
            NpMatching2ContextEvent ev{.contextId = this->ctxId,
                                       .event = ORBIS_NP_MATCHING2_CONTEXT_EVENT_STARTED,
                                       .cause = ORBIS_NP_MATCHING2_EVENT_CAUSE_CONTEXT_ACTION,
                                       .errorCode = 0};
            npMatching2ContextCallback(&ev);
            break;
        }
        case ix::WebSocketMessageType::Message: {
            if (msg->binary) {
                LOG_ERROR(ShadNet, "received binary ws message");
                break;
            }
            LOG_DEBUG(ShadNet, "text message: {}", msg->str);
            try {
                this->HandleMessage(msg->str);
            } catch (const json::exception& e) {
                LOG_ERROR(ShadNet, "json error when handling message: {}", e.what());
            } catch (const std::exception& e) {
                LOG_ERROR(ShadNet, "handling message failed: {}", e.what());
            }
            break;
        }
        case ix::WebSocketMessageType::Close: {
            LOG_DEBUG(ShadNet, "close message, code = {}, reason = {}", msg->closeInfo.code,
                      msg->closeInfo.reason);
            break;
        }
        case ix::WebSocketMessageType::Error: {
            LOG_DEBUG(ShadNet, "error message, http_status = {}, reason = {}",
                      msg->errorInfo.http_status, msg->errorInfo.reason);
            break;
        }
        default: {
            LOG_DEBUG(ShadNet, "message type {}", magic_enum::enum_name(msg->type));
            break;
        }
        }
    });

    websocket.start();

    std::thread([this, timeoutSec]() {
        std::this_thread::sleep_for(std::chrono::seconds(timeoutSec));
        if (!this->connected) {
            this->websocket.stop();
            this->websocket.setOnMessageCallback(nullptr);

            NpMatching2ContextEvent ev{.contextId = this->ctxId,
                                       .event = ORBIS_NP_MATCHING2_CONTEXT_EVENT_START_OVER,
                                       .cause = ORBIS_NP_MATCHING2_EVENT_CAUSE_CONTEXT_ACTION,
                                       .errorCode = ORBIS_NP_MATCHING2_ERROR_TIMEDOUT};
            npMatching2ContextCallback(&ev);
        }
    }).detach();

    return ORBIS_OK;
}

int MatchingContext::Stop() {
    std::unique_lock lk{this->mutex};

    if (!websocket.isOnMessageCallbackRegistered()) {
        // make it a proper state machine
        return ORBIS_NP_MATCHING2_ERROR_CONTEXT_NOT_STARTED;
    }

    this->websocket.stop();
    this->websocket.setOnMessageCallback(nullptr);

    NpMatching2ContextEvent ev{.contextId = this->ctxId,
                               .event = ORBIS_NP_MATCHING2_CONTEXT_EVENT_STOPPED,
                               .cause = ORBIS_NP_MATCHING2_EVENT_CAUSE_CONTEXT_ACTION,
                               .errorCode = 0};
    npMatching2ContextCallback(&ev);

    return ORBIS_OK;
}

int MatchingContext::CreateJoinRoom(const OrbisNpMatching2CreateJoinRoomRequest& req,
                                    const OrbisNpMatching2RequestOptParam* optParam) {
    return SendRequest(req, optParam);
}

int MatchingContext::CreateJoinRoom(const OrbisNpMatching2CreateJoinRoomRequestA& req,
                                    const OrbisNpMatching2RequestOptParam* optParam) {
    return SendRequest(req, optParam);
}

int MatchingContext::JoinRoom(const OrbisNpMatching2JoinRoomRequest& req,
                              const OrbisNpMatching2RequestOptParam* optParam) {
    return SendRequest(req, optParam);
}

int MatchingContext::LeaveRoom(const OrbisNpMatching2LeaveRoomRequest& req,
                               const OrbisNpMatching2RequestOptParam* optParam) {
    return SendRequest(req, optParam);
}

int MatchingContext::SearchRoom(const OrbisNpMatching2SearchRoomRequest& req,
                                const OrbisNpMatching2RequestOptParam* optParam) {
    return SendRequest(req, optParam);
}

int MatchingContext::SignalingGetPingInfo(const OrbisNpMatching2SignalingGetPingInfoRequest& req,
                                          const OrbisNpMatching2RequestOptParam* optParam) {
    return SendRequest(req, optParam);
}

void MatchingContext::SetDefaultRequestOptParam(const OrbisNpMatching2RequestOptParam& optParam) {
    std::scoped_lock lk{this->mutex};
    this->optParam = optParam;
}

// call under lock as it might access internal state
auto MatchingContext::GetRequestCallback(
    std::optional<OrbisNpMatching2RequestOptParam> requestOptParam) {
    OrbisNpMatching2RequestCallback cb = nullptr;
    void* arg = nullptr;
    if (requestOptParam) {
        cb = requestOptParam->callback;
        arg = requestOptParam->arg;
    } else if (this->optParam) {
        cb = this->optParam->callback;
        arg = this->optParam->arg;
    }

    return [cb, arg](auto ctxId, auto reqId, auto ev, int errorCode, auto data) {
        if (cb) {
            cb(ctxId, reqId, ev, errorCode, data, arg);
        }
    };
}

template <typename T>
int MatchingContext::SendRequest(const T& req, const OrbisNpMatching2RequestOptParam* optParam) {
    json j = req;
    WsEnvelope envelope{this->reqId++, request_tag(req), j.dump()};

    {
        std::scoped_lock lk{this->mutex};
        this->pendingRequests.emplace(
            envelope.id, std::make_tuple(request_tag(req),
                                         optParam ? std::make_optional(*optParam) : std::nullopt));
    }

    json e = envelope;
    websocket.send(e.dump());

    return envelope.id;
}

void MatchingContext::SetContextCallback(OrbisNpMatching2ContextCallback cb, void* userdata) {
    npMatching2ContextCallback = [cb, userdata](auto arg) {
        cb(arg->contextId, arg->event, arg->cause, arg->errorCode, userdata);
    };
}

void MatchingContext::SetRoomCallback(OrbisNpMatching2RoomCallback cb, void* userdata) {
    this->roomCallback = [cb, userdata](auto ctxId, auto roomId, auto event, const void* data) {
        cb(ctxId, roomId, event, data, userdata);
    };
}

void MatchingContext::SetSignalingCallback(OrbisNpMatching2SignalingCallback cb, void* userdata) {
    this->signalingCallback = [cb, userdata](auto ctxId, auto roomId, auto roomMemberId, auto event,
                                             auto errorCode) {
        cb(ctxId, roomId, roomMemberId, event, errorCode, userdata);
    };
}

class Finalizer {
    std::function<void()> f;

public:
    explicit Finalizer(std::function<void()> f) : f(f) {}
    ~Finalizer() {
        f();
    }
};

void MatchingContext::HandleResponse(const WsMessage& response) {
    Finalizer f([this, response] {
        std::scoped_lock lk{this->mutex};
        this->pendingRequests.erase(response.request_id);
    });

    std::unique_lock lk{this->mutex};
    auto [type, optParam] = this->pendingRequests.at(response.request_id);
    auto cb = GetRequestCallback(optParam);
    lk.unlock();

    if (response.error_code) {
        LOG_ERROR(ShadNet, "matching request {} failed with {} (code {})", response.request_id,
                  response.error, response.error_code);

    } else {
        LOG_DEBUG(ShadNet, "matching request {} response received", response.request_id);
        if (type == request_tag_t<OrbisNpMatching2CreateJoinRoomRequest>()) {
            auto resp = response.payload.get<OrbisNpMatching2CreateJoinRoomResponseOwned>();
            auto view = resp.view();
            cb(this->ctxId, response.request_id, ORBIS_NP_MATCHING2_REQUEST_EVENT_CREATE_JOIN_ROOM,
               ORBIS_OK, &view);
        } else if (type == request_tag_t<OrbisNpMatching2SearchRoomRequest>()) {
            auto resp = response.payload.get<OrbisNpMatching2SearchRoomResponseOwned>();
            auto view = resp.view();
            cb(this->ctxId, response.request_id, ORBIS_NP_MATCHING2_REQUEST_EVENT_SEARCH_ROOM,
               ORBIS_OK, &view);
        } else if (type == request_tag_t<OrbisNpMatching2JoinRoomRequest>()) {
            // it's the same response as createjoin
            auto resp = response.payload.get<OrbisNpMatching2CreateJoinRoomResponseOwned>();
            auto view = resp.view();
            cb(this->ctxId, response.request_id, ORBIS_NP_MATCHING2_REQUEST_EVENT_JOIN_ROOM,
               ORBIS_OK, &view);
        } else if (type == request_tag_t<OrbisNpMatching2LeaveRoomRequest>()) {
            auto resp = response.payload.get<OrbisNpMatching2LeaveRoomResponse>();
            cb(this->ctxId, response.request_id, ORBIS_NP_MATCHING2_REQUEST_EVENT_LEAVE_ROOM,
               ORBIS_OK, &resp);
        }
        ///
        else {
            LOG_ERROR(ShadNet, "unhandled response type: {}", type);
        }
    }
}

void MatchingContext::HandleEvent(const WsEvent& event) {
    LOG_DEBUG(ShadNet, "handling event {}", event.type);

    if (event.type == "member_joined") {
        auto evData = event.ev.get<OrbisNpMatching2RoomMemberUpdateInfoOwned>();
        auto view = evData.view();
        this->roomCallback(this->ctxId, evData.roomId, ORBIS_NP_MATCHING2_ROOM_EVENT_MEMBER_JOINED,
                           &view);
        LOG_DEBUG(ShadNet, "room callback called");
    } else if (event.type == "signaling_established") {
        auto evData = event.ev.get<SignalingEstablishedInfo>();
        this->signalingCallback(this->ctxId, evData.roomId, evData.roomMemberId,
                                ORBIS_NP_MATCHING2_SIGNALING_EVENT_ESTABLISHED, 0);
        LOG_DEBUG(ShadNet, "signaling callback called");
    } else {
        LOG_ERROR(ShadNet, "unhandled event type: {}", event.type);
    }
}

void MatchingContext::HandleMessage(const std::string& wsMessage) {
    auto j = json::parse(wsMessage);

    if (j.contains("id")) {
        auto message = j.get<WsMessage>();
        HandleResponse(message);
    } else {
        auto ev = j.get<WsEvent>();
        HandleEvent(ev);
    }
}

} // namespace Core::ShadNet
