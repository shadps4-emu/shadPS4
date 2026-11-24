// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "font_stack.h"

#include <filesystem>
#include <vector>
#include "core/emulator_settings.h"
#include "font_data.h"

namespace fs = std::filesystem;

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
    0x2122, 0x2122, // TM symbol
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
    0x2000, 0x206F, // General punctuation
    0x20A0, 0x20CF, // Currency symbols
    0x2100, 0x214F, // Letterlike symbols
    0x2190, 0x21FF, // Arrows
    0x2200, 0x22FF, // Math operators
    0x2300, 0x23FF, // Misc technical (includes keyboard symbol)
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
        rb.AddRanges(kSymbolsRanges);
        rb.BuildRanges(&ranges);
    }
    return ranges.Data;
}

const ImWchar* GetCjkCoverageRanges(ImFontAtlas* atlas) {
    static ImVector<ImWchar> ranges{};
    if (ranges.empty()) {
        ImFontGlyphRangesBuilder rb{};
        rb.AddRanges(atlas->GetGlyphRangesJapanese());
        rb.AddRanges(atlas->GetGlyphRangesKorean());
        rb.AddRanges(atlas->GetGlyphRangesChineseFull());
        rb.BuildRanges(&ranges);
    }
    return ranges.Data;
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

static ImFont* s_trophy_title_font = nullptr;
static ImFont* s_trophy_desc_font = nullptr;

ImFont* AddPrimaryUiFont(ImFontAtlas* atlas, const float font_size, const int console_language,
                         const ImFontConfig& base_cfg, const bool include_cjk_fallback) {
    const ImWchar* primary_ranges = GetPrimaryTextRanges(atlas);

    fs::path fonts_dir = EmulatorSettings.GetFontsDir();
    fs::path sst_path;
    const std::vector<fs::path> primary_candidates = {
        fonts_dir / "font" / "SST-Medium.otf", fonts_dir / "SST-Medium.otf",
        fonts_dir / "font" / "SST-Roman.otf",  fonts_dir / "SST-Roman.otf",
        fonts_dir / "font" / "SST-Bold.otf",   fonts_dir / "SST-Bold.otf",
    };
    for (const auto& p : primary_candidates) {
        if (fs::exists(p)) {
            sst_path = p;
            break;
        }
    }

    ImFont* font = nullptr;
    if (!sst_path.empty()) {
        font = atlas->AddFontFromFileTTF(sst_path.string().c_str(), font_size, &base_cfg,
                                         primary_ranges);
    }
    if (!font) {
        font = atlas->AddFontFromMemoryCompressedTTF(kNotoSansBlob.data, kNotoSansBlob.size,
                                                     font_size, &base_cfg, primary_ranges);
    }

    AddMergedFont(atlas, kNotoSansArabicBlob, font_size, kArabicRanges, base_cfg);
    AddMergedFont(atlas, kNotoSansThaiBlob, font_size, atlas->GetGlyphRangesThai(), base_cfg);
    AddMergedFont(atlas, kNotoSansSymbols2Blob, font_size, kSymbolsRanges, base_cfg);

    if (include_cjk_fallback) {
        AddMergedFont(atlas, kNotoSansCjkBlob, font_size, GetCjkCoverageRanges(atlas), base_cfg,
                      GetCjkFontIndex(console_language));
    }

    return font;
}

void InitTrophyFonts(ImFontAtlas* atlas, const int console_language, const ImFontConfig& base_cfg) {
    fs::path fonts_dir = EmulatorSettings.GetFontsDir();
    const ImWchar* primary_ranges = GetPrimaryTextRanges(atlas);

    fs::path bold_path;
    const std::vector<fs::path> bold_candidates = {
        fonts_dir / "font" / "SST-Bold.otf",
        fonts_dir / "SST-Bold.otf",
        fonts_dir / "font" / "SST-Medium.otf",
        fonts_dir / "SST-Medium.otf",
    };
    for (const auto& p : bold_candidates) {
        if (fs::exists(p)) {
            bold_path = p;
            break;
        }
    }

    if (!bold_path.empty()) {
        s_trophy_title_font =
            atlas->AddFontFromFileTTF(bold_path.string().c_str(), 32.0f, &base_cfg, primary_ranges);
    } else {
        s_trophy_title_font = atlas->AddFontFromMemoryCompressedTTF(
            kNotoSansBlob.data, kNotoSansBlob.size, 32.0f, &base_cfg, primary_ranges);
    }

    fs::path medium_path;
    const std::vector<fs::path> medium_candidates = {
        fonts_dir / "font" / "SST-Medium.otf", fonts_dir / "SST-Medium.otf",
        fonts_dir / "font" / "SST-Roman.otf",  fonts_dir / "SST-Roman.otf",
        fonts_dir / "font" / "SST-Bold.otf",   fonts_dir / "SST-Bold.otf",
    };
    for (const auto& p : medium_candidates) {
        if (fs::exists(p)) {
            medium_path = p;
            break;
        }
    }

    if (!medium_path.empty()) {
        s_trophy_desc_font = atlas->AddFontFromFileTTF(medium_path.string().c_str(), 26.0f,
                                                       &base_cfg, primary_ranges);
    } else {
        s_trophy_desc_font = atlas->AddFontFromMemoryCompressedTTF(
            kNotoSansBlob.data, kNotoSansBlob.size, 26.0f, &base_cfg, primary_ranges);
    }
}

ImFont* GetTrophyTitleFont() {
    return s_trophy_title_font;
}

ImFont* GetTrophyDescFont() {
    return s_trophy_desc_font;
}

} // namespace ImGui::FontStack
