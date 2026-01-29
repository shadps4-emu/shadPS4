// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <mutex>
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np/np_partner.h"
#include "core/libraries/np/np_partner_error.h"
#include "core/libraries/system/userservice.h"

namespace Libraries::Np::NpPartner {

static bool g_library_init = false;
std::mutex g_library_mutex{};

/**
 * Terminates the library
 */
s32 PS4_SYSV_ABI Func_A4CC5784DA33517F() {
    LOG_ERROR(Lib_NpPartner, "(STUBBED) called");
    if (!g_library_init) {
        return ORBIS_NP_PARTNER_ERROR_NOT_INITIALIZED;
    }
    std::scoped_lock lk{g_library_mutex};
    g_library_init = false;
    return ORBIS_OK;
}

/**
 * Aborts requests started by Func_F8E9DB52CD425743
 */
s32 PS4_SYSV_ABI Func_A507D84D91F39CC7() {
    LOG_ERROR(Lib_NpPartner, "(STUBBED) called");
    if (!g_library_init) {
        return ORBIS_NP_PARTNER_ERROR_NOT_INITIALIZED;
    }
    // Request logic is unimplemented, so this does nothing.
    return ORBIS_OK;
}

/**
 * Initializes the library
 */
s32 PS4_SYSV_ABI Func_EC2C48E74FF19429() {
    LOG_ERROR(Lib_NpPartner, "(STUBBED) called");
    g_library_init = true;
    // Also retrieves and sends compiled SDK version to server.
    return ORBIS_OK;
}

/**
 * Creates an NP request to determine if the user has a subscription to EA's services.
 */
s32 PS4_SYSV_ABI Func_F8E9DB52CD425743(UserService::OrbisUserServiceUserId user_id, bool* result) {
    LOG_ERROR(Lib_NpPartner, "(STUBBED) called");
    if (!g_library_init) {
        return ORBIS_NP_PARTNER_ERROR_NOT_INITIALIZED;
    }
    if (result == nullptr) {
        return ORBIS_NP_PARTNER_ERROR_INVALID_ARGUMENT;
    }
    std::scoped_lock lk{g_library_mutex};
    // In the real library, this creates and sends a request that checks for EA subscription,
    // then waits for the request to return a response, and returns that response.
    // NP signed out likely returns an error, but I haven't figured out the error code yet.
    // For now, stub having no subscription.
    *result = false;
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("pMxXhNozUX8", "libSceNpPartner001", 1, "libSceNpPartner001",
                 Func_A4CC5784DA33517F);
    LIB_FUNCTION("pQfYTZHznMc", "libSceNpPartner001", 1, "libSceNpPartner001",
                 Func_A507D84D91F39CC7);
    LIB_FUNCTION("7CxI50-xlCk", "libSceNpPartner001", 1, "libSceNpPartner001",
                 Func_EC2C48E74FF19429);
    LIB_FUNCTION("+OnbUs1CV0M", "libSceNpPartner001", 1, "libSceNpPartner001",
                 Func_F8E9DB52CD425743);
};

} // namespace Libraries::Np::NpPartner