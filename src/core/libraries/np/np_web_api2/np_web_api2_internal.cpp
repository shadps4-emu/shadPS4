// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/network/http2.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_web_api2/np_web_api2_context.h"
#include "core/libraries/np/np_web_api2/np_web_api2_internal.h"
#include "core/libraries/system/userservice.h"

#include <map>
#include <mutex>

namespace Libraries::Np::NpWebApi2 {

std::recursive_mutex g_mutex{};
s32 g_current_lib_context_id{};
std::map<s32, LibraryContext*> g_lib_contexts{};

s32 createLibraryContext(s32 http_ctx_id, u64 pool_size, const char* name) {
    std::scoped_lock lk{g_mutex};

    if (g_lib_contexts.size() >= 0x8000) {
        LOG_ERROR(Lib_NpWebApi2, "Too many library contexts");
        return ORBIS_NP_WEBAPI2_ERROR_LIB_CONTEXT_MAX;
    }

    do {
        g_current_lib_context_id++;
        if (g_current_lib_context_id >= 0x8000) {
            g_current_lib_context_id = 1;
        }
    } while (g_lib_contexts.contains(g_current_lib_context_id));

    if (!name) {
        g_lib_contexts[g_current_lib_context_id] =
            new LibraryContext(g_current_lib_context_id, http_ctx_id, pool_size);
    } else {
        g_lib_contexts[g_current_lib_context_id] =
            new LibraryContext(g_current_lib_context_id, http_ctx_id, pool_size, name);
    }
    return g_current_lib_context_id;
};

LibraryContext* getLibraryContext(s32 lib_ctx_id) {
    std::scoped_lock lk{g_mutex};

    if (!g_lib_contexts.contains(lib_ctx_id)) {
        return nullptr;
    }
    LibraryContext* lib_ctx = g_lib_contexts.at(lib_ctx_id);
    lib_ctx->AddUser();
    return lib_ctx;
}

s32 getMemoryPoolStats(s32 lib_ctx_id, OrbisNpWebApi2MemoryPoolStats* stats) {
    LibraryContext* lib_ctx = getLibraryContext(lib_ctx_id);
    if (!lib_ctx) {
        LOG_ERROR(Lib_NpWebApi2, "No library context with id {:#x}", lib_ctx_id);
        return ORBIS_NP_WEBAPI2_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    if (stats) {
        memset(stats, 0, sizeof(*stats));
        stats->pool_size = lib_ctx->GetPoolSize();
    }
    return ORBIS_OK;
}

s32 createUserContext(s32 lib_ctx_id, Libraries::UserService::OrbisUserServiceUserId user_id) {
    LibraryContext* lib_ctx = getLibraryContext(lib_ctx_id);
    if (!lib_ctx) {
        LOG_ERROR(Lib_NpWebApi2, "No library context with id {:#x}", lib_ctx_id);
        return ORBIS_NP_WEBAPI2_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    UserContext* user_ctx = lib_ctx->GetUserContextByUserId(user_id);
    if (user_ctx) {
        LOG_ERROR(Lib_NpWebApi2, "User context already exists for user id {}", user_id);
        user_ctx->RemoveUser();
        return ORBIS_NP_WEBAPI2_ERROR_USER_CONTEXT_ALREADY_EXIST;
    }

    s32 user_ctx_id = lib_ctx->CreateUserContext(user_id);
    if (user_ctx_id < 0) {
        // user_ctx_id contains an error code, return it.
        return user_ctx_id;
    }

    user_ctx = lib_ctx->GetUserContext(user_ctx_id);
    user_ctx->RemoveUser();

    s32 result = user_ctx->Initialize();
    if (result < 0) {
        // Failed to initialize user_ctx, destroy it.
        lib_ctx->RemoveUserContext(user_ctx_id);
        delete user_ctx;
    }

    lib_ctx->RemoveUser();
    return user_ctx_id;
};

}; // namespace Libraries::Np::NpWebApi2