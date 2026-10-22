// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "core/libraries/np/np_types.h"
#include "shadnet.pb.h"

namespace Libraries::Np {

namespace {

constexpr s32 NpAuthIssuerId = 0x100;
// The auth server's numeric error codes reach the game as this base | code.
constexpr s32 NpAuthServerErrorBase = static_cast<s32>(0x82E00000);

s32 AuthErrorFromReply(ShadNet::ErrorType error) {
    switch (error) {
    case ShadNet::ErrorType::InvalidInput:
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    case ShadNet::ErrorType::Unauthorized:
        return ORBIS_NP_ERROR_SIGNED_OUT;
    case ShadNet::ErrorType::TooSoon:
        return ORBIS_NP_AUTH_ERROR_SERVICE_BUSY;
    default:
        return ORBIS_NP_AUTH_ERROR_SERVICE_DOWN;
    }
}

// OAuth error from the auth server, for an ID token request it refused.
s32 AuthErrorFromServer(const std::string& error, u32 code) {
    if (code > 0 && code <= 0xffff) {
        return NpAuthServerErrorBase | static_cast<s32>(code);
    }
    if (error == "invalid_client" || error == "unauthorized_client") {
        return ORBIS_NP_AUTH_ERROR_S_INVALID_CREDENTIAL;
    }
    if (error == "invalid_request" || error == "invalid_scope") {
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }
    return ORBIS_NP_AUTH_ERROR_SERVICE_DOWN;
}

// Reply body: u32LE blob size + proto.
template <typename Message>
bool ParseReply(const std::vector<u8>& body, Message& pb) {
    if (body.size() < 4) {
        return false;
    }
    const u32 blob_sz = static_cast<u32>(body[0]) | (static_cast<u32>(body[1]) << 8) |
                        (static_cast<u32>(body[2]) << 16) | (static_cast<u32>(body[3]) << 24);
    return body.size() - 4 >= blob_sz &&
           pb.ParseFromArray(body.data() + 4, static_cast<int>(blob_sz));
}

} // namespace

std::shared_ptr<ShadNet::ShadNetClient> NpHandler::AuthenticatedClient(s32 user_id) {
    std::lock_guard lock(m_mutex_clients);
    auto it = m_clients.find(user_id);
    if (it == m_clients.end() || !it->second || !it->second->IsAuthenticated()) {
        return nullptr;
    }
    return it->second;
}

s32 NpHandler::RequestAuthorizationCode(s32 user_id, const std::string& client_id,
                                        const std::string& scope,
                                        std::shared_ptr<NpAuth::AuthRequestCtx> ctx) {
    const auto client = AuthenticatedClient(user_id);
    if (!client) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    std::lock_guard lock(m_mutex_pending_auth);
    const u64 pkt_id = client->GetAuthorizationCode(client_id, scope);
    PendingAuthRequest p;
    p.req = std::move(ctx);
    p.user_id = user_id;
    m_pending_auth.emplace(pkt_id, std::move(p));
    LOG_INFO(NpHandler, "user_id={} client_id='{}' pkt_id={}", user_id, client_id, pkt_id);
    return ORBIS_OK;
}

s32 NpHandler::RequestIdToken(s32 user_id, const std::string& client_id,
                              const std::string& client_secret, const std::string& scope,
                              std::shared_ptr<NpAuth::AuthRequestCtx> ctx) {
    const auto client = AuthenticatedClient(user_id);
    if (!client) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    std::lock_guard lock(m_mutex_pending_auth);
    const u64 pkt_id = client->GetIdToken(client_id, client_secret, scope);
    PendingAuthRequest p;
    p.req = std::move(ctx);
    p.user_id = user_id;
    m_pending_auth.emplace(pkt_id, std::move(p));
    LOG_INFO(NpHandler, "user_id={} client_id='{}' scope='{}' pkt_id={}", user_id, client_id, scope,
             pkt_id);
    return ORBIS_OK;
}

void NpHandler::OnAuthReply(s32 user_id, ShadNet::CommandType cmd, u64 pkt_id,
                            ShadNet::ErrorType error, const std::vector<u8>& body) {
    std::shared_ptr<NpAuth::AuthRequestCtx> req;
    {
        std::lock_guard lock(m_mutex_pending_auth);
        auto it = m_pending_auth.find(pkt_id);
        if (it == m_pending_auth.end()) {
            return;
        }
        req = std::move(it->second.req);
        m_pending_auth.erase(it);
    }
    if (!req) {
        return;
    }

    if (error != ShadNet::ErrorType::NoError) {
        LOG_WARNING(NpHandler, "user_id={} cmd={} error={}", user_id, static_cast<int>(cmd),
                    static_cast<int>(error));
        req->SetResult(AuthErrorFromReply(error));
        return;
    }

    if (cmd == ShadNet::CommandType::GetIdToken) {
        shadnet::GetIdTokenReply pb;
        if (!ParseReply(body, pb)) {
            LOG_WARNING(NpHandler, "could not parse ID token reply");
            req->SetResult(ORBIS_NP_AUTH_ERROR_NO_TOKEN_RECEIVED);
            return;
        }
        if (pb.id_token().empty()) {
            const s32 result = pb.error().empty()
                                   ? ORBIS_NP_AUTH_ERROR_NO_TOKEN_RECEIVED
                                   : AuthErrorFromServer(pb.error(), pb.error_code());
            LOG_WARNING(NpHandler, "user_id={} ID token refused: {} ({}) {} -> {:#x}", user_id,
                        pb.error(), pb.error_code(), pb.error_description(),
                        static_cast<u32>(result));
            req->SetResult(result);
            return;
        }
        if (pb.id_token().size() >= sizeof(OrbisNpIdToken::token)) {
            LOG_WARNING(NpHandler, "ID token has invalid length {}", pb.id_token().size());
            req->SetResult(ORBIS_NP_AUTH_ERROR_NO_TOKEN_RECEIVED);
            return;
        }
        LOG_INFO(NpHandler, "user_id={} pkt_id={} received ID token", user_id, pkt_id);
        req->SetIdToken(pb.id_token());
        return;
    }

    shadnet::GetAuthorizationCodeReply pb;
    if (!ParseReply(body, pb)) {
        LOG_WARNING(NpHandler, "could not parse reply");
        req->SetResult(ORBIS_NP_AUTH_ERROR_NO_TOKEN_RECEIVED);
        return;
    }
    if (pb.code().empty() || pb.code().size() >= sizeof(OrbisNpAuthorizationCode::code)) {
        LOG_WARNING(NpHandler, "code has invalid length {}", pb.code().size());
        req->SetResult(ORBIS_NP_AUTH_ERROR_NO_TOKEN_RECEIVED);
        return;
    }
    LOG_INFO(NpHandler, "user_id={} pkt_id={} received code", user_id, pkt_id);
    req->SetAuthorizationCode(pb.code(), NpAuthIssuerId);
}

} // namespace Libraries::Np
