// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <cstdint>
#include <mutex>

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/font/font.h"
#include "core/libraries/font/font_internal.h"
#include "core/libraries/font/fontft.h"
#include "core/libraries/font/fontft_internal.h"
#include "core/libraries/libs.h"

namespace Libraries::FontFt {

namespace {

static std::once_flag g_driver_table_once;
alignas(Libraries::Font::Internal::SysDriver) static Libraries::Font::Internal::SysDriver
    g_driver_table{};

static const OrbisFontLibrarySelection* GetDriverTable() {
    std::call_once(g_driver_table_once, [] {
        auto* selection = reinterpret_cast<OrbisFontLibrarySelection*>(&g_driver_table);
        selection->magic = 0;
        selection->reserved = 0;
        selection->reserved_ptr1 = nullptr;

        auto* driver = &g_driver_table;

        driver->pixel_resolution = &Libraries::FontFt::Internal::LibraryGetPixelResolutionStub;
        driver->init = &Libraries::FontFt::Internal::LibraryInitStub;
        driver->term = &Libraries::FontFt::Internal::LibraryTermStub;
        driver->support_formats = &Libraries::FontFt::Internal::LibrarySupportStub;

        driver->open = &Libraries::FontFt::Internal::LibraryOpenFontMemoryStub;
        driver->close = &Libraries::FontFt::Internal::LibraryCloseFontObjStub;
        driver->scale = &Libraries::FontFt::Internal::LibraryGetFaceScaleStub;
        driver->metric = &Libraries::FontFt::Internal::LibraryGetFaceMetricStub;
        driver->glyph_index = &Libraries::FontFt::Internal::LibraryGetGlyphIndexStub;
        driver->set_char_with_dpi = &Libraries::FontFt::Internal::LibrarySetCharSizeWithDpiStub;
        driver->set_char_default_dpi =
            &Libraries::FontFt::Internal::LibrarySetCharSizeDefaultDpiStub;
        driver->compute_layout = &Libraries::FontFt::Internal::LibraryComputeLayoutBlockStub;
        driver->compute_layout_alt = &Libraries::FontFt::Internal::LibraryComputeLayoutAltBlockStub;

        driver->load_glyph_cached = &Libraries::FontFt::Internal::LibraryLoadGlyphCachedStub;
        driver->get_glyph_metrics = &Libraries::FontFt::Internal::LibraryGetGlyphMetricsStub;
        driver->apply_glyph_adjust = &Libraries::FontFt::Internal::LibraryApplyGlyphAdjustStub;
        driver->configure_glyph = &Libraries::FontFt::Internal::LibraryConfigureGlyphStub;
    });

    return reinterpret_cast<const OrbisFontLibrarySelection*>(&g_driver_table);
}

static std::once_flag g_renderer_table_once;
alignas(OrbisFontRendererSelection) static OrbisFontRendererSelection g_renderer_table{};

static const OrbisFontRendererSelection* GetRendererSelectionTable() {
    std::call_once(g_renderer_table_once, [] {
        g_renderer_table.magic = 0;
        g_renderer_table.size =
            static_cast<u32>(sizeof(Libraries::Font::Internal::RendererFtOpaque));

        g_renderer_table.create_fn =
            reinterpret_cast<std::uintptr_t>(&Libraries::FontFt::Internal::FtRendererCreate);
        g_renderer_table.destroy_fn =
            reinterpret_cast<std::uintptr_t>(&Libraries::FontFt::Internal::FtRendererDestroy);
        g_renderer_table.query_fn =
            reinterpret_cast<std::uintptr_t>(&Libraries::FontFt::Internal::FtRendererQuery);
    });
    return &g_renderer_table;
}

} // namespace

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

const OrbisFontLibrarySelection* PS4_SYSV_ABI sceFontSelectLibraryFt(int value) {
    LOG_INFO(Lib_FontFt, "called");
    LOG_DEBUG(Lib_FontFt, "params:\nvalue: {}\n", value);
    if (value == 0) {
        return GetDriverTable();
    }
    return nullptr;
}

const OrbisFontRendererSelection* PS4_SYSV_ABI sceFontSelectRendererFt(int value) {
    LOG_INFO(Lib_FontFt, "called");
    LOG_DEBUG(Lib_FontFt, "params:\nvalue: {}\n", value);
    if (value == 0) {
        return GetRendererSelectionTable();
    }
    return nullptr;
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
