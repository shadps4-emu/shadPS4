// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/network/ssl2.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Http2 {

enum OrbisHttp2HttpVersion : u32 {
    ORBIS_HTTP2_VERSION_1_0 = 1,
    ORBIS_HTTP2_VERSION_1_1 = 2,
    ORBIS_HTTP2_VERSION_2_0 = 3,
};

// TODO: Figure out the whole struct. What's here is based on libSceNpWebApi2 use.
struct OrbisHttp2PreSendCallbackData {
    void* unk;
    void* cert;
};
using OrbisHttp2PreSendCallback = PS4_SYSV_ABI s32 (*)(s32 request_id, s32 ssl_id,
                                                       OrbisHttp2PreSendCallbackData* data,
                                                       void* user_arg);

s32 PS4_SYSV_ABI sceHttp2AbortRequest(s32 req_id);
s32 PS4_SYSV_ABI sceHttp2AddRequestHeader(s32 template_or_req_id, const char* name,
                                          const char* value, u32 mode);
s32 PS4_SYSV_ABI sceHttp2CreateRequestWithURL(s32 tmpl_id, const char* method, const char* url,
                                              u64 content_length);
s32 PS4_SYSV_ABI sceHttp2CreateTemplate(s32 ctx_id, const char* user_agent, s32 http_ver,
                                        s32 auto_proxy_conf);
s32 PS4_SYSV_ABI sceHttp2DeleteRequest(s32 req_id);
s32 PS4_SYSV_ABI sceHttp2DeleteTemplate(s32 tmpl_id);
s32 PS4_SYSV_ABI sceHttp2GetAllResponseHeaders(s32 req_id, char** header, u64* header_size);
s32 PS4_SYSV_ABI sceHttp2GetStatusCode(s32 request_id, s32* status_code);
s32 PS4_SYSV_ABI sceHttp2Init(s32 net_id, s32 ssl_id, u64 pool_size, s32 max_requests);
s32 PS4_SYSV_ABI sceHttp2ReadData(s32 req_id, void* data, u64 size);
s32 PS4_SYSV_ABI sceHttp2SendRequest(s32 req_id, const void* data, u64 size);
s32 PS4_SYSV_ABI sceHttp2SetPreSendCallback(s32 template_id, OrbisHttp2PreSendCallback cb_func,
                                            void* user_arg);
s32 PS4_SYSV_ABI sceHttp2SetRequestContentLength(s32 req_id, u64 content_length);

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Http2