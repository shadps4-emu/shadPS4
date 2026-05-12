// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/content_export/content_export.h"
#include "core/libraries/content_export/content_export_error.h"
#include "core/libraries/libs.h"

namespace Libraries::ContentExport {

static bool g_is_initialized = false;

s32 _sceContentExportInit(OrbisContentExportInitParam* init_param, u8 version) {
    if (g_is_initialized) {
        return ORBIS_CONTENT_EXPORT_ERROR_MULTIPLEINIT;
    }
    if (!init_param || !init_param->mallocfunc || !init_param->freefunc) {
        return ORBIS_CONTENT_EXPORT_ERROR_INVALDPARAM;
    }
    if (version == 1 && (init_param->reserved0 != 0 || init_param->reserved1 != 0 ||
                         (init_param->bufsize != 0 && init_param->bufsize < 0x100))) {
        return ORBIS_CONTENT_EXPORT_ERROR_INVALDPARAM;
    }
    g_is_initialized = true;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceContentExportInit(OrbisContentExportInitParam* init_param) {
    LOG_ERROR(Lib_ContentExport, "(STUBBED) called");
    return _sceContentExportInit(init_param, 0);
}

s32 PS4_SYSV_ABI sceContentExportInit2(OrbisContentExportInitParam* init_param) {
    LOG_ERROR(Lib_ContentExport, "(STUBBED) called");
    return _sceContentExportInit(init_param, 1);
}

s32 PS4_SYSV_ABI sceContentExportTerm() {
    LOG_ERROR(Lib_ContentExport, "(STUBBED) called");
    if (!g_is_initialized) {
        return ORBIS_CONTENT_EXPORT_ERROR_NOINIT;
    }
    g_is_initialized = false;
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("FzEWeYnAFlI", "libSceContentExport", 1, "libSceContentExport",
                 sceContentExportInit);
    LIB_FUNCTION("0GnN4QCgIfs", "libSceContentExport", 1, "libSceContentExport",
                 sceContentExportInit2);
    LIB_FUNCTION("+KDWny9Y-6k", "libSceContentExport", 1, "libSceContentExport",
                 sceContentExportTerm);
}

} // namespace Libraries::ContentExport