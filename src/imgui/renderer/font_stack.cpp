// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "font_stack.h"

#include "font_data.h"

namespace {

struct CompressedFontBlob {
    const unsigned char* data;
    int size;
};

constexpr int kConsoleLanguageJapanese = 0;
constexpr int kConsoleLanguageKorean = 9;
constexpr int kConsoleLanguageTraditionalChinese = 10;
constexpr int kConsoleLanguageSimplifiedChinese = 11;

constexpr int kNotoSansCjkFontIndexJp = 0;
constexpr int kNotoSansCjkFontIndexKr = 1;
constexpr int kNotoSansCjkFontIndexSc = 2;
constexpr int kNotoSansCjkFontIndexTc = 3;

constexpr ImWchar kLatinExtendedRanges[] = {
    0x0100, 0x024F, // Latin Extended-A + Latin Extended-B
    0x1E00, 0x1EFF, // Latin Extended Additional
    0,
};

constexpr ImWchar kArabicRanges[] = {
    0x0600, 0x06FF, // Arabic
    0x0750, 0x077F, // Arabic Supplement
    0x08A0, 0x08FF, // Arabic Extended-A
    0xFB50, 0xFDFF, // Arabic Presentation Forms-A
    0xFE70, 0xFEFF, // Arabic Presentation Forms-B
    0,
};

constexpr ImWchar kSymbolsRanges[] = {
    0x20A0, 0x20CF, // Currency symbols
    0x2100, 0x214F, // Letterlike symbols
    0x2190, 0x21FF, // Arrows
    0x2200, 0x22FF, // Math operators
    0x2460, 0x24FF, // Enclosed alphanumerics
    0x25A0, 0x25FF, // Geometric shapes
    0x2600, 0x26FF, // Misc symbols
    0x2700, 0x27BF, // Dingbats
    0x2B00, 0x2BFF, // Misc symbols and arrows
    0,
};

const CompressedFontBlob kNotoSansBlob{
    imgui_font_notosans_regular_compressed_data,
    static_cast<int>(imgui_font_notosans_regular_compressed_size),
};
const CompressedFontBlob kNotoSansArabicBlob{
    imgui_font_notosansarabic_regular_compressed_data,
    static_cast<int>(imgui_font_notosansarabic_regular_compressed_size),
};
const CompressedFontBlob kNotoSansThaiBlob{
    imgui_font_notosansthai_regular_compressed_data,
    static_cast<int>(imgui_font_notosansthai_regular_compressed_size),
};
const CompressedFontBlob kNotoSansSymbols2Blob{
    imgui_font_notosanssymbols2_regular_compressed_data,
    static_cast<int>(imgui_font_notosanssymbols2_regular_compressed_size),
};
const CompressedFontBlob kNotoSansCjkBlob{
    imgui_font_notosanscjk_regular_compressed_data,
    static_cast<int>(imgui_font_notosanscjk_regular_compressed_size),
};

int GetCjkFontIndex(const int console_language) {
    switch (console_language) {
    case kConsoleLanguageJapanese:
        return kNotoSansCjkFontIndexJp;
    case kConsoleLanguageKorean:
        return kNotoSansCjkFontIndexKr;
    case kConsoleLanguageTraditionalChinese:
        return kNotoSansCjkFontIndexTc;
    case kConsoleLanguageSimplifiedChinese:
        return kNotoSansCjkFontIndexSc;
    default:
        return kNotoSansCjkFontIndexJp;
    }
}

const ImWchar* GetPrimaryTextRanges(ImFontAtlas* atlas) {
    static ImVector<ImWchar> ranges{};
    if (ranges.empty()) {
        ImFontGlyphRangesBuilder rb{};
        rb.AddRanges(atlas->GetGlyphRangesDefault());
        rb.AddRanges(atlas->GetGlyphRangesGreek());
        rb.AddRanges(atlas->GetGlyphRangesCyrillic());
        rb.AddRanges(atlas->GetGlyphRangesVietnamese());
        rb.AddRanges(kLatinExtendedRanges);
        rb.BuildRanges(&ranges);
    }
    return ranges.Data;
}

const ImWchar* GetCjkCoverageRanges(ImFontAtlas* atlas, const int console_language) {
    switch (console_language) {
    case kConsoleLanguageJapanese:
        return atlas->GetGlyphRangesJapanese();
    case kConsoleLanguageKorean:
        return atlas->GetGlyphRangesKorean();
    case kConsoleLanguageSimplifiedChinese:
        return atlas->GetGlyphRangesChineseSimplifiedCommon();
    case kConsoleLanguageTraditionalChinese:
        return atlas->GetGlyphRangesChineseFull();
    default:
        return nullptr;
    }
}

void AddMergedFont(ImFontAtlas* atlas, const CompressedFontBlob blob, const float font_size,
                   const ImWchar* glyph_ranges, const ImFontConfig& base_cfg,
                   const int font_no = 0) {
    ImFontConfig cfg = base_cfg;
    cfg.MergeMode = true;
    cfg.FontNo = font_no;
    atlas->AddFontFromMemoryCompressedTTF(blob.data, blob.size, font_size, &cfg, glyph_ranges);
}

} // namespace

namespace ImGui::FontStack {

ImFont* AddPrimaryUiFont(ImFontAtlas* atlas, const float font_size, const int console_language,
                         const ImFontConfig& base_cfg, const bool include_cjk_fallback) {
    const ImWchar* primary_ranges = GetPrimaryTextRanges(atlas);
    ImFont* font = atlas->AddFontFromMemoryCompressedTTF(kNotoSansBlob.data, kNotoSansBlob.size,
                                                         font_size, &base_cfg, primary_ranges);

    AddMergedFont(atlas, kNotoSansArabicBlob, font_size, kArabicRanges, base_cfg);
    AddMergedFont(atlas, kNotoSansThaiBlob, font_size, atlas->GetGlyphRangesThai(), base_cfg);
    AddMergedFont(atlas, kNotoSansSymbols2Blob, font_size, kSymbolsRanges, base_cfg);

    if (include_cjk_fallback) {
        // Keep the atlas lean by only merging CJK ranges for active CJK console locales.
        const ImWchar* cjk_ranges = GetCjkCoverageRanges(atlas, console_language);
        if (cjk_ranges != nullptr) {
            AddMergedFont(atlas, kNotoSansCjkBlob, font_size, cjk_ranges, base_cfg,
                          GetCjkFontIndex(console_language));
        }
    }

    return font;
}

} // namespace ImGui::FontStack
