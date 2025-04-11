// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::FontFt {

s32 PS4_SYSV_ABI sceFontFtInitAliases();
s32 PS4_SYSV_ABI sceFontFtSetAliasFont();
s32 PS4_SYSV_ABI sceFontFtSetAliasPath();
s32 PS4_SYSV_ABI sceFontFtSupportBdf();
s32 PS4_SYSV_ABI sceFontFtSupportCid();
s32 PS4_SYSV_ABI sceFontFtSupportFontFormats();
s32 PS4_SYSV_ABI sceFontFtSupportOpenType();
s32 PS4_SYSV_ABI sceFontFtSupportOpenTypeOtf();
s32 PS4_SYSV_ABI sceFontFtSupportOpenTypeTtf();
s32 PS4_SYSV_ABI sceFontFtSupportPcf();
s32 PS4_SYSV_ABI sceFontFtSupportPfr();
s32 PS4_SYSV_ABI sceFontFtSupportSystemFonts();
s32 PS4_SYSV_ABI sceFontFtSupportTrueType();
s32 PS4_SYSV_ABI sceFontFtSupportTrueTypeGx();
s32 PS4_SYSV_ABI sceFontFtSupportType1();
s32 PS4_SYSV_ABI sceFontFtSupportType42();
s32 PS4_SYSV_ABI sceFontFtSupportWinFonts();
s32 PS4_SYSV_ABI sceFontFtTermAliases();
s32 PS4_SYSV_ABI sceFontSelectGlyphsFt();
s32 PS4_SYSV_ABI sceFontSelectLibraryFt();
s32 PS4_SYSV_ABI sceFontSelectRendererFt();

void RegisterlibSceFontFt(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::FontFt