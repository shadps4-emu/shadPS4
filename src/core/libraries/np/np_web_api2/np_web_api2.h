// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/np/np_types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpWebApi2 {

struct OrbisNpWebApi2IntInitializeArgs {
    s32 lib_http_ctx_id;
    s32 reserved;
    u64 pool_size;
    const char* name;
    u64 struct_size;
};

struct OrbisNpWebApi2IntInitialize2Args {
    s32 lib_http_ctx_id;
    s32 reserved;
    u64 pool_size;
    const char* name;
    u32 push_config_group;
    s32 reserved2;
    u64 struct_size;
};

struct OrbisNpWebApi2ContentParameter {
    u64 content_length;
    const char* content_type;
    u8 reserved[16];
};

struct OrbisNpWebApi2MemoryPoolStats {
    u64 pool_size;
    u64 max_inuse_size;
    u64 current_inuse_size;
    s32 reserved;
};

struct OrbisNpWebApi2ResponseInformationOption {
    s32 http_status;
    s32 reserved;
    char* error_object;
    u64 error_object_size;
    u64 response_data_size;
};

struct OrbisNpWebApi2PushEventDataType {
    char val[65];
};

struct OrbisNpWebApi2PushEventExtdDataKey {
    char val[33];
};

struct OrbisNpWebApi2PushEventFilterParameter {
    OrbisNpWebApi2PushEventDataType data_type;
    OrbisNpWebApi2PushEventExtdDataKey* extd_data_key;
    u64 extd_data_key_num;
};

struct OrbisNpWebApi2PushEventExtdData {
    OrbisNpWebApi2PushEventExtdDataKey extd_data_key;
    char* data;
    u64 data_len;
};

using OrbisNpWebApi2PushEventCallback = void PS4_SYSV_ABI (*)(
    s32 user_ctx_id, s32 callback_id, const char* np_service_name,
    OrbisNpServiceLabel np_service_label, const OrbisNpPeerAddressA* to,
    const OrbisNpOnlineId* to_online_id, const OrbisNpPeerAddressA* from,
    const OrbisNpOnlineId* from_online_id, const OrbisNpWebApi2PushEventDataType* data_type,
    const char* data, u64 data_len, const OrbisNpWebApi2PushEventExtdData* extd_data,
    u64 extd_data_num, void* user_arg);

enum OrbisNpWebApi2PushEventPushContextCallbackType {
    Unknown = -1,
    Received = 0,
    Dropped = 1,
};

struct OrbisNpWebApi2PushEventPushContextId {
    char uuid[37];
};

using OrbisNpWebApi2PushEventPushContextCallback = void PS4_SYSV_ABI (*)(
    s32 user_ctx_id, s32 callback_id, const OrbisNpWebApi2PushEventPushContextId* push_ctx_id,
    OrbisNpWebApi2PushEventPushContextCallbackType cb_type, const char* np_service_name,
    OrbisNpServiceLabel np_service_label, const OrbisNpPeerAddressA* to,
    const OrbisNpOnlineId* to_online_id, const OrbisNpPeerAddressA* from,
    const OrbisNpOnlineId* from_online_id, const OrbisNpWebApi2PushEventDataType* data_type,
    const char* data, u64 data_len, const OrbisNpWebApi2PushEventExtdData* extd_data,
    u64 extd_data_num, void* user_arg);

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpWebApi2