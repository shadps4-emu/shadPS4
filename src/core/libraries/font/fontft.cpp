// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/font/fontft.h"
#include "core/libraries/libs.h"

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

void RegisterlibSceFontFt(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("e60aorDdpB8", "libSceFontFt", 1, "libSceFontFt", sceFontFtInitAliases);
    LIB_FUNCTION("BxcmiMc3UaA", "libSceFontFt", 1, "libSceFontFt", sceFontFtSetAliasFont);
    LIB_FUNCTION("MEWjebIzDEI", "libSceFontFt", 1, "libSceFontFt", sceFontFtSetAliasPath);
    LIB_FUNCTION("ZcQL0iSjvFw", "libSceFontFt", 1, "libSceFontFt", sceFontFtSupportBdf);
    LIB_FUNCTION("LADHEyFTxRQ", "libSceFontFt", 1, "libSceFontFt", sceFontFtSupportCid);
    LIB_FUNCTION("+jqQjsancTs", "libSceFontFt", 1, "libSceFontFt", sceFontFtSupportFontFormats);
    LIB_FUNCTION("oakL15-mBtc", "libSceFontFt", 1, "libSceFontFt", sceFontFtSupportOpenType);
    LIB_FUNCTION("dcQeaDr8UJc", "libSceFontFt", 1, "libSceFontFt", sceFontFtSupportOpenTypeOtf);
    LIB_FUNCTION("2KXS-HkZT3c", "libSceFontFt", 1, "libSceFontFt", sceFontFtSupportOpenTypeTtf);
    LIB_FUNCTION("H0mJnhKwV-s", "libSceFontFt", 1, "libSceFontFt", sceFontFtSupportPcf);
    LIB_FUNCTION("S2mw3sYplAI", "libSceFontFt", 1, "libSceFontFt", sceFontFtSupportPfr);
    LIB_FUNCTION("+ehNXJPUyhk", "libSceFontFt", 1, "libSceFontFt", sceFontFtSupportSystemFonts);
    LIB_FUNCTION("4BAhDLdrzUI", "libSceFontFt", 1, "libSceFontFt", sceFontFtSupportTrueType);
    LIB_FUNCTION("Utlzbdf+g9o", "libSceFontFt", 1, "libSceFontFt", sceFontFtSupportTrueTypeGx);
    LIB_FUNCTION("nAfQ6qaL1fU", "libSceFontFt", 1, "libSceFontFt", sceFontFtSupportType1);
    LIB_FUNCTION("X9+pzrGtBus", "libSceFontFt", 1, "libSceFontFt", sceFontFtSupportType42);
    LIB_FUNCTION("w0hI3xsK-hc", "libSceFontFt", 1, "libSceFontFt", sceFontFtSupportWinFonts);
    LIB_FUNCTION("w5sfH9r8ZJ4", "libSceFontFt", 1, "libSceFontFt", sceFontFtTermAliases);
    LIB_FUNCTION("ojW+VKl4Ehs", "libSceFontFt", 1, "libSceFontFt", sceFontSelectGlyphsFt);
    LIB_FUNCTION("oM+XCzVG3oM", "libSceFontFt", 1, "libSceFontFt", sceFontSelectLibraryFt);
    LIB_FUNCTION("Xx974EW-QFY", "libSceFontFt", 1, "libSceFontFt", sceFontSelectRendererFt);
};

} // namespace Libraries::FontFt