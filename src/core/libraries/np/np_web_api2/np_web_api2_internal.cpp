// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_web_api2/np_web_api2_context.h"
#include "core/libraries/np/np_web_api2/np_web_api2_internal.h"

#include <map>
#include <mutex>

namespace Libraries::Np::NpWebApi2 {

std::recursive_mutex g_mutex{};
s32 g_current_lib_context_id{};
std::map<s32, LibraryContext*> g_lib_contexts{};

s32 createLibraryContext(s32 http_ctx_id, const char* name) {
    std::scoped_lock lk{g_mutex};

    if (g_lib_contexts.size() >= 0x8000) {
        LOG_ERROR(Lib_NpWebApi2, "Too many library contexts!");
        return ORBIS_NP_WEBAPI2_ERROR_LIB_CONTEXT_MAX;
    }

    // Find next valid ID
    do {
        g_current_lib_context_id++;
        if (g_current_lib_context_id >= 0x8000) {
            g_current_lib_context_id = 1;
        }
    } while(g_lib_contexts.contains(g_current_lib_context_id));

    // Create new LibraryContext
    if (!name) {
        g_lib_contexts[g_current_lib_context_id] =
            new LibraryContext(g_current_lib_context_id, http_ctx_id);
    } else {
        g_lib_contexts[g_current_lib_context_id] =
            new LibraryContext(g_current_lib_context_id, http_ctx_id, name);
    }
    return g_current_lib_context_id;
};

}; // namespace Libraries::Np::NpWebApi2