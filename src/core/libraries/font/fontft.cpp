// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/fontft/fontft.h"

namespace Libraries::FontFt {

s32 PS4_SYSV_ABI sceFontFtInitAliases() {
    LOG_ERROR(Lib_FontFt, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontFtSetAliasFont() {
    LOG_ERROR(Lib_FontFt, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontFtSetAliasPath() {
    LOG_ERROR(Lib_FontFt, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontFtSupportBdf() {
    LOG_ERROR(Lib_FontFt, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontFtSupportCid() {
    LOG_ERROR(Lib_FontFt, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontFtSupportFontFormats() {
    LOG_ERROR(Lib_FontFt, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontFtSupportOpenType() {
    LOG_ERROR(Lib_FontFt, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontFtSupportOpenTypeOtf() {
    LOG_ERROR(Lib_FontFt, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontFtSupportOpenTypeTtf() {
    LOG_ERROR(Lib_FontFt, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontFtSupportPcf() {
    LOG_ERROR(Lib_FontFt, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontFtSupportPfr() {
    LOG_ERROR(Lib_FontFt, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontFtSupportSystemFonts() {
    LOG_ERROR(Lib_FontFt, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontFtSupportTrueType() {
    LOG_ERROR(Lib_FontFt, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontFtSupportTrueTypeGx() {
    LOG_ERROR(Lib_FontFt, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontFtSupportType1() {
    LOG_ERROR(Lib_FontFt, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontFtSupportType42() {
    LOG_ERROR(Lib_FontFt, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontFtSupportWinFonts() {
    LOG_ERROR(Lib_FontFt, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontFtTermAliases() {
    LOG_ERROR(Lib_FontFt, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSelectGlyphsFt() {
    LOG_ERROR(Lib_FontFt, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSelectLibraryFt() {
    LOG_ERROR(Lib_FontFt, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSelectRendererFt() {
    LOG_ERROR(Lib_FontFt, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI module_start() {
    LOG_ERROR(Lib_FontFt, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI module_stop() {
    LOG_ERROR(Lib_FontFt, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterlibSceFontFt(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("e60aorDdpB8", "libSceFontFt", 1, "libSceFontFt", 1, 1, sceFontFtInitAliases);
    LIB_FUNCTION("BxcmiMc3UaA", "libSceFontFt", 1, "libSceFontFt", 1, 1, sceFontFtSetAliasFont);
    LIB_FUNCTION("MEWjebIzDEI", "libSceFontFt", 1, "libSceFontFt", 1, 1, sceFontFtSetAliasPath);
    LIB_FUNCTION("ZcQL0iSjvFw", "libSceFontFt", 1, "libSceFontFt", 1, 1, sceFontFtSupportBdf);
    LIB_FUNCTION("LADHEyFTxRQ", "libSceFontFt", 1, "libSceFontFt", 1, 1, sceFontFtSupportCid);
    LIB_FUNCTION("+jqQjsancTs", "libSceFontFt", 1, "libSceFontFt", 1, 1,
                 sceFontFtSupportFontFormats);
    LIB_FUNCTION("oakL15-mBtc", "libSceFontFt", 1, "libSceFontFt", 1, 1, sceFontFtSupportOpenType);
    LIB_FUNCTION("dcQeaDr8UJc", "libSceFontFt", 1, "libSceFontFt", 1, 1,
                 sceFontFtSupportOpenTypeOtf);
    LIB_FUNCTION("2KXS-HkZT3c", "libSceFontFt", 1, "libSceFontFt", 1, 1,
                 sceFontFtSupportOpenTypeTtf);
    LIB_FUNCTION("H0mJnhKwV-s", "libSceFontFt", 1, "libSceFontFt", 1, 1, sceFontFtSupportPcf);
    LIB_FUNCTION("S2mw3sYplAI", "libSceFontFt", 1, "libSceFontFt", 1, 1, sceFontFtSupportPfr);
    LIB_FUNCTION("+ehNXJPUyhk", "libSceFontFt", 1, "libSceFontFt", 1, 1,
                 sceFontFtSupportSystemFonts);
    LIB_FUNCTION("4BAhDLdrzUI", "libSceFontFt", 1, "libSceFontFt", 1, 1, sceFontFtSupportTrueType);
    LIB_FUNCTION("Utlzbdf+g9o", "libSceFontFt", 1, "libSceFontFt", 1, 1,
                 sceFontFtSupportTrueTypeGx);
    LIB_FUNCTION("nAfQ6qaL1fU", "libSceFontFt", 1, "libSceFontFt", 1, 1, sceFontFtSupportType1);
    LIB_FUNCTION("X9+pzrGtBus", "libSceFontFt", 1, "libSceFontFt", 1, 1, sceFontFtSupportType42);
    LIB_FUNCTION("w0hI3xsK-hc", "libSceFontFt", 1, "libSceFontFt", 1, 1, sceFontFtSupportWinFonts);
    LIB_FUNCTION("w5sfH9r8ZJ4", "libSceFontFt", 1, "libSceFontFt", 1, 1, sceFontFtTermAliases);
    LIB_FUNCTION("ojW+VKl4Ehs", "libSceFontFt", 1, "libSceFontFt", 1, 1, sceFontSelectGlyphsFt);
    LIB_FUNCTION("oM+XCzVG3oM", "libSceFontFt", 1, "libSceFontFt", 1, 1, sceFontSelectLibraryFt);
    LIB_FUNCTION("Xx974EW-QFY", "libSceFontFt", 1, "libSceFontFt", 1, 1, sceFontSelectRendererFt);
    LIB_FUNCTION("BaOKcng8g88", "libkernel", 1, "libSceFontFt", 1, 1, module_start);
    LIB_FUNCTION("KpDMrPHvt3Q", "libkernel", 1, "libSceFontFt", 1, 1, module_stop);
};

} // namespace Libraries::FontFt