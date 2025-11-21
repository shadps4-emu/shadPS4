// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <optional>
#include <string>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <fmt/format.h>
#include "common/config.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/file_sys/fs.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/font/font.h"
#include "core/libraries/libs.h"
#include "font_error.h"

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "externals/dear_imgui/imstb_truetype.h"

namespace Libraries::Font {
struct FontLibOpaque;
}

namespace {
Core::FileSys::MntPoints* g_mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();

using Libraries::Font::OrbisFontGenerateGlyphParams;
using Libraries::Font::OrbisFontGlyph;
using Libraries::Font::OrbisFontGlyphMetrics;
using Libraries::Font::OrbisFontGlyphOpaque;
using Libraries::Font::OrbisFontGlyphOutline;
using Libraries::Font::OrbisFontGlyphOutlinePoint;
using Libraries::Font::OrbisFontMem;
using Libraries::Font::OrbisFontStyleFrame;

struct GlyphEntry {
    std::vector<std::uint8_t> bitmap;
    int w = 0;
    int h = 0;
    int x0 = 0;
    int y0 = 0;
    int x1 = 0;
    int y1 = 0;
    float advance = 0.0f;
    float bearingX = 0.0f;
};

struct FontState {
    float scale_w = 16.0f;
    float scale_h = 16.0f;
    float scale_point_w = 12.0f;
    float scale_point_h = 12.0f;
    bool scale_point_active = false;
    u32 dpi_x = 72;
    u32 dpi_y = 72;
    u32 font_set_type = 0;
    std::filesystem::path system_font_path;
    Libraries::Font::OrbisFontLib library = nullptr;
    bool ext_face_ready = false;
    std::vector<unsigned char> ext_face_data;
    stbtt_fontinfo ext_face{};
    float ext_scale_for_height = 0.0f;
    int ext_ascent = 0, ext_descent = 0, ext_lineGap = 0;
    std::unordered_map<std::uint64_t, GlyphEntry> ext_cache;
    std::vector<std::uint8_t> scratch;
    bool logged_ext_use = false;
    // Cached layout (PS4-style) for GetHorizontalLayout
    float cached_baseline_y = 0.0f;
    bool layout_cached = false;
    // Renderer binding state (PS4 requires a bound renderer for render-scale queries)
    Libraries::Font::OrbisFontRenderer bound_renderer = nullptr;
    // Mark when the game asked for PS4 internal/system fonts (via OpenFontSet)
    bool system_requested = false;
};

static std::unordered_map<Libraries::Font::OrbisFontHandle, FontState> g_font_state;

struct LibraryState {
    bool support_system = false;
    bool support_external = false;
    u32 external_formats = 0;
    u32 external_fontMax = 0;
    const Libraries::Font::OrbisFontMem* backing_memory = nullptr;
    Libraries::Font::OrbisFontLibCreateParams create_params = nullptr;
    u64 edition = 0;
    Libraries::Font::FontLibOpaque* native = nullptr;
    void* owned_device_cache = nullptr;
    u32 owned_device_cache_size = 0;
};
static std::unordered_map<Libraries::Font::OrbisFontLib, LibraryState> g_library_state;

static std::unordered_map<Libraries::Font::OrbisFontRenderSurface*,
                          const Libraries::Font::OrbisFontStyleFrame*>
    g_style_for_surface;

static std::once_flag g_system_font_once;
static bool g_system_font_available = false;
static std::vector<unsigned char> g_system_font_bytes;
static std::filesystem::path g_system_font_path;

static bool EnsureSystemFontBlob();
static bool HasTrueTypeGlyf(const std::vector<unsigned char>& bytes);
static FontState* TryGetState(Libraries::Font::OrbisFontHandle h);

struct GeneratedGlyph {
    OrbisFontGlyphOpaque glyph{};
    OrbisFontGlyphMetrics metrics{};
    u32 codepoint = 0;
    Libraries::Font::OrbisFontHandle owner_handle{};
    OrbisFontGlyphOutline outline{};
    std::vector<OrbisFontGlyphOutlinePoint> outline_points;
    std::vector<u8> outline_tags;
    std::vector<u16> outline_contours;
    bool outline_initialized = false;
};

static std::mutex g_generated_glyph_mutex;
static std::unordered_set<OrbisFontGlyph> g_generated_glyphs;

static void TrackGeneratedGlyph(OrbisFontGlyph glyph) {
    std::scoped_lock lock(g_generated_glyph_mutex);
    g_generated_glyphs.insert(glyph);
}

static bool ForgetGeneratedGlyph(OrbisFontGlyph glyph) {
    std::scoped_lock lock(g_generated_glyph_mutex);
    return g_generated_glyphs.erase(glyph) > 0;
}

static void BuildBoundingOutline(GeneratedGlyph& gg) {
    const float left = gg.metrics.h_bearing_x;
    const float top = gg.metrics.h_bearing_y;
    const float right = gg.metrics.h_bearing_x + gg.metrics.w;
    const float bottom = gg.metrics.h_bearing_y - gg.metrics.h;

    gg.outline_points.clear();
    gg.outline_tags.clear();
    gg.outline_contours.clear();

    gg.outline_points.push_back({left, top});
    gg.outline_points.push_back({right, top});
    gg.outline_points.push_back({right, bottom});
    gg.outline_points.push_back({left, bottom});

    gg.outline_tags.resize(gg.outline_points.size(), 1); // on-curve flags
    gg.outline_contours.push_back(static_cast<u16>(gg.outline_points.size() - 1));

    gg.outline.points_ptr = gg.outline_points.data();
    gg.outline.tags_ptr = gg.outline_tags.data();
    gg.outline.contour_end_idx = gg.outline_contours.data();
    gg.outline.points_cnt = static_cast<s16>(gg.outline_points.size());
    gg.outline.contours_cnt = static_cast<s16>(gg.outline_contours.size());
    gg.outline_initialized = true;
}

static bool BuildTrueOutline(GeneratedGlyph& gg) {
    const auto* st = TryGetState(gg.owner_handle);
    if (!st || !st->ext_face_ready) {
        return false;
    }
    auto* face = &st->ext_face;
    const int glyph_index = stbtt_FindGlyphIndex(face, static_cast<int>(gg.codepoint));
    if (glyph_index <= 0) {
        return false;
    }

    stbtt_vertex* verts = nullptr;
    const int count = stbtt_GetGlyphShape(face, glyph_index, &verts);
    if (count <= 0 || !verts) {
        return false;
    }

    gg.outline_points.clear();
    gg.outline_tags.clear();
    gg.outline_contours.clear();

    const float scale = st->ext_scale_for_height == 0.0f
                            ? stbtt_ScaleForPixelHeight(face, st->scale_h)
                            : st->ext_scale_for_height;

    auto push_point = [&](float x, float y, u8 tag) {
        gg.outline_points.push_back({x * scale, y * scale});
        gg.outline_tags.push_back(tag);
    };

    int last_start = -1;
    for (int i = 0; i < count; ++i) {
        const stbtt_vertex& v = verts[i];
        switch (v.type) {
        case STBTT_vmove:
            if (last_start >= 0 && !gg.outline_points.empty()) {
                gg.outline_contours.push_back(static_cast<u16>(gg.outline_points.size() - 1));
            }
            last_start = static_cast<int>(gg.outline_points.size());
            push_point(static_cast<float>(v.x), static_cast<float>(v.y), 1);
            break;
        case STBTT_vline:
            push_point(static_cast<float>(v.x), static_cast<float>(v.y), 1);
            break;
        case STBTT_vcurve:
            push_point(static_cast<float>(v.cx), static_cast<float>(v.cy), 0);
            push_point(static_cast<float>(v.x), static_cast<float>(v.y), 1);
            break;
        case STBTT_vcubic:
            // Approximate cubic with two off-curve controls then on-curve end.
            push_point(static_cast<float>(v.cx), static_cast<float>(v.cy), 0);
            push_point(static_cast<float>(v.cx1), static_cast<float>(v.cy1), 0);
            push_point(static_cast<float>(v.x), static_cast<float>(v.y), 1);
            break;
        default:
            break;
        }
    }

    if (last_start >= 0 && !gg.outline_points.empty()) {
        gg.outline_contours.push_back(static_cast<u16>(gg.outline_points.size() - 1));
    }

    const bool ok = !gg.outline_points.empty() && !gg.outline_contours.empty();
    if (ok) {
        gg.outline.points_ptr = gg.outline_points.data();
        gg.outline.tags_ptr = gg.outline_tags.data();
        gg.outline.contour_end_idx = gg.outline_contours.data();
        gg.outline.points_cnt = static_cast<s16>(gg.outline_points.size());
        gg.outline.contours_cnt = static_cast<s16>(gg.outline_contours.size());
        gg.outline_initialized = true;
    }

    stbtt_FreeShape(face, verts);
    return ok;
}

static u16 ClampToU16(float value) {
    if (value <= 0.0f) {
        return 0;
    }
    const float clamped = std::min(value, static_cast<float>(std::numeric_limits<u16>::max()));
    return static_cast<u16>(std::lround(clamped));
}

static FontState& GetState(Libraries::Font::OrbisFontHandle h) {
    return g_font_state[h];
}

static FontState* TryGetState(Libraries::Font::OrbisFontHandle h) {
    if (!h)
        return nullptr;
    auto it = g_font_state.find(h);
    if (it == g_font_state.end())
        return nullptr;
    return &it->second;
}

static LibraryState& GetLibState(Libraries::Font::OrbisFontLib lib) {
    return g_library_state[lib];
}

static void RemoveLibState(Libraries::Font::OrbisFontLib lib) {
    if (auto it = g_library_state.find(lib); it != g_library_state.end()) {
        if (it->second.owned_device_cache) {
            delete[] static_cast<std::uint8_t*>(it->second.owned_device_cache);
        }
        g_library_state.erase(it);
    }
}

static void LogExternalFormatSupport(u32 formats_mask) {
    LOG_INFO(Lib_Font, "ExternalFormatsMask=0x{:X}", formats_mask);
}

static bool LoadFontFile(const std::filesystem::path& path, std::vector<unsigned char>& out_bytes);

static void LogFontOpenParams(const Libraries::Font::OrbisFontOpenParams* params) {
    if (!params) {
        LOG_INFO(Lib_Font, "OpenFontSetParams: <null>");
        return;
    }
    LOG_INFO(
        Lib_Font,
        "OpenFontSetParams: tag=0x{:04X} flags=0x{:X} subfont={} unique_id={} reserved_ptrs=[{}, "
        "{}]",
        params->tag, params->flags, params->subfont_index, params->unique_id, params->reserved_ptr1,
        params->reserved_ptr2);
}

static std::filesystem::path ResolveGuestPath(const char* guest_path) {
    if (!guest_path) {
        return {};
    }
    if (guest_path[0] != '/') {
        return std::filesystem::path(guest_path);
    }
    if (!g_mnt) {
        return {};
    }
    return g_mnt->GetHostPath(guest_path);
}

static bool LoadGuestFileBytes(const std::filesystem::path& host_path,
                               std::vector<unsigned char>& out_bytes) {
    std::ifstream file(host_path, std::ios::binary | std::ios::ate);
    if (!file) {
        return false;
    }
    const std::streamoff size = file.tellg();
    if (size < 0) {
        return false;
    }
    if (size == 0) {
        out_bytes.clear();
        return true;
    }
    if (static_cast<std::uint64_t>(size) > std::numeric_limits<std::size_t>::max()) {
        return false;
    }
    out_bytes.resize(static_cast<std::size_t>(size));
    file.seekg(0, std::ios::beg);
    if (!file.read(reinterpret_cast<char*>(out_bytes.data()),
                   static_cast<std::streamsize>(out_bytes.size()))) {
        out_bytes.clear();
        return false;
    }
    return true;
}

enum class ScaleMode {
    AscenderHeight,
    EmSquare,
};

static ScaleMode GetScaleMode() {
    static int inited = 0;
    static ScaleMode mode = ScaleMode::AscenderHeight;
    if (!inited) {
        inited = 1;
        if (const char* env = std::getenv("SHADPS4_FONT_SCALE_MODE")) {
            std::string v = env;
            for (auto& c : v)
                c = static_cast<char>(std::tolower(c));
            if (v == "em" || v == "emsquare" || v == "em_square") {
                mode = ScaleMode::EmSquare;
            }
        }
        LOG_INFO(Lib_Font, "scale mode initialized");
        LOG_DEBUG(Lib_Font,
                  "scale mode params:\n"
                  " mode={}\n",
                  mode == ScaleMode::EmSquare ? "em-square" : "ascender-height");
    }
    return mode;
}

static float GetScaleBias() {
    static int inited = 0;
    static float bias = 1.0f;
    if (!inited) {
        inited = 1;
        if (const char* env = std::getenv("SHADPS4_FONT_SCALE_BIAS")) {
            try {
                bias = std::max(0.01f, std::min(10.0f, std::stof(env)));
            } catch (...) {
                bias = 1.0f;
            }
        }
        if (bias != 1.0f) {
            LOG_INFO(Lib_Font, "scale bias initialized");
            LOG_DEBUG(Lib_Font,
                      "scale bias params:\n"
                      " bias={}\n",
                      bias);
        }
    }
    return bias;
}

static float ComputeScale(const stbtt_fontinfo* face, float pixel_h) {
    if (!face)
        return 0.0f;
    float s = 0.0f;
    if (GetScaleMode() == ScaleMode::EmSquare)
        s = stbtt_ScaleForMappingEmToPixels(face, pixel_h);
    else
        s = stbtt_ScaleForPixelHeight(face, pixel_h);
    return s * GetScaleBias();
}

// External face scale: keep legacy ascender-height mapping to avoid regressions
// in titles that ship their own fonts (e.g., CUSA47038). No EM toggle, no bias.
static float ComputeScaleExt(const stbtt_fontinfo* face, float pixel_h) {
    if (!face)
        return 0.0f;
    return stbtt_ScaleForPixelHeight(face, pixel_h);
}

static constexpr float kPointsPerInch = 72.0f;
static constexpr float kScaleEpsilon = 1e-4f;

static float SafeDpiToFloat(u32 dpi) {
    return dpi == 0 ? kPointsPerInch : static_cast<float>(dpi);
}

static float PointsToPixels(float pt, u32 dpi) {
    return pt * SafeDpiToFloat(dpi) / kPointsPerInch;
}

static float PixelsToPoints(float px, u32 dpi) {
    const float dpi_f = SafeDpiToFloat(dpi);
    return px * kPointsPerInch / dpi_f;
}

constexpr u16 kStyleFrameMagic = 0x0F09;
constexpr u16 kStyleFrameFlagScale = 1 << 0;
constexpr u16 kStyleFrameFlagWeight = 1 << 1;
constexpr u16 kStyleFrameFlagSlant = 1 << 2;
constexpr u16 kStyleFrameFlagDpi = 1 << 3;

enum class StyleFrameScalingMode : s32 {
    None = 0,
    Point = 1,
    Pixel = 2,
};

struct StyleFrameScaleState {
    float scale_w;
    float scale_h;
    u32 dpi_x;
    u32 dpi_y;
    bool scale_overridden;
    bool dpi_overridden;
};

static StyleFrameScaleState ResolveStyleFrameScale(const OrbisFontStyleFrame* style,
                                                   const FontState& st) {
    StyleFrameScaleState resolved{
        .scale_w = st.scale_w,
        .scale_h = st.scale_h,
        .dpi_x = st.dpi_x,
        .dpi_y = st.dpi_y,
        .scale_overridden = false,
        .dpi_overridden = false,
    };
    if (!style || style->magic != kStyleFrameMagic) {
        return resolved;
    }
    if ((style->flags & kStyleFrameFlagDpi) != 0) {
        if (style->dpiX > 0) {
            resolved.dpi_x = static_cast<u32>(style->dpiX);
            resolved.dpi_overridden = true;
        }
        if (style->dpiY > 0) {
            resolved.dpi_y = static_cast<u32>(style->dpiY);
            resolved.dpi_overridden = true;
        }
    }
    if ((style->flags & kStyleFrameFlagScale) != 0 && style->scaleWidth > 0.0f &&
        style->scaleHeight > 0.0f) {
        const auto mode = static_cast<StyleFrameScalingMode>(style->scalingFlag);
        if (mode == StyleFrameScalingMode::Point) {
            resolved.scale_w = PointsToPixels(style->scaleWidth, resolved.dpi_x);
            resolved.scale_h = PointsToPixels(style->scaleHeight, resolved.dpi_y);
            resolved.scale_overridden = true;
        } else if (mode == StyleFrameScalingMode::Pixel) {
            resolved.scale_w = style->scaleWidth;
            resolved.scale_h = style->scaleHeight;
            resolved.scale_overridden = true;
        }
    }
    return resolved;
}

static void InitializeStyleFrame(OrbisFontStyleFrame& frame) {
    frame.magic = kStyleFrameMagic;
    frame.flags = 0;
    frame.dpiX = 72;
    frame.dpiY = 72;
    frame.scalingFlag = static_cast<s32>(StyleFrameScalingMode::None);
    frame.scaleWidth = 0.0f;
    frame.scaleHeight = 0.0f;
    frame.weightXScale = 1.0f;
    frame.weightYScale = 1.0f;
    frame.slantRatio = 0.0f;
}

static bool EnsureStyleFrameInitialized(OrbisFontStyleFrame* frame) {
    if (!frame)
        return false;
    if (frame->magic != kStyleFrameMagic) {
        InitializeStyleFrame(*frame);
    }
    return true;
}

static bool ValidateStyleFramePtr(const OrbisFontStyleFrame* frame) {
    return frame && frame->magic == kStyleFrameMagic;
}

static void UpdateCachedLayout(FontState& st) {
    if (!st.ext_face_ready) {
        return;
    }
    if (st.ext_scale_for_height == 0.0f)
        st.ext_scale_for_height = ComputeScaleExt(&st.ext_face, st.scale_h);
    int asc = 0, desc = 0, gap = 0;
    stbtt_GetFontVMetrics(&st.ext_face, &asc, &desc, &gap);
    const float scale = st.ext_scale_for_height;
    st.cached_baseline_y = static_cast<float>(asc) * scale;
    st.layout_cached = true;
}

static std::unordered_set<u32> g_logged_pua;

static inline std::uint64_t MakeGlyphKey(u32 code, int pixel_h) {
    return (static_cast<std::uint64_t>(code) << 32) | static_cast<std::uint32_t>(pixel_h);
}
static std::unordered_set<const void*> g_stride_logged;
static inline void LogStrideOnce(const Libraries::Font::OrbisFontRenderSurface* surf) {
    if (!surf)
        return;
    const void* key = static_cast<const void*>(surf);
    if (g_stride_logged.insert(key).second) {
        const int bpp = std::max(1, static_cast<int>(surf->pixelSizeByte));
        const long expected = static_cast<long>(surf->width) * bpp;
        const bool match = (expected == surf->widthByte);
        LOG_INFO(Lib_Font,
                 "StrideCheck: surf={} buf={} width={} height={} pixelSizeByte={} widthByte={} "
                 "expected={} match={}",
                 key, surf->buffer, surf->width, surf->height, bpp, surf->widthByte, expected,
                 match);
    }
}

// Font-set ID bit layout:
// bits [31:28] family       (0x18 = SST Standard UI)
// bits [27:24] region       (0x0 Latin, 0x8 Japanese, 0xC Chinese, 0xD Korean, 0xE Thai, 0xF
// Asian-mix) bits [23:20] variant      (0x0 European, 0x2 Typewriter, 0x4 JP, 0x7 EU JPCJK, 0x8 JP
// UH, 0xC GB UH) bits [19:16] coverage     (0x0 base, 0x4 Arabic, 0x5 Vietnamese, 0x6 Thai, 0xA CJK
// TH, 0xB full-set) bits [15:8]  language tag (0x44 JP, 0x53 VI, 0x54 TH, 0x80 GB, 0xA CJK TH, 0xA4
// JP CJK, 0xC4 JP AR) bits [7:0]   weight/style (0x43 light, 0x44 regular, 0x45 medium, 0x47 bold,
// 0x53 light VI, 0x57 bold VI, 0xC7 Arabic bold)
struct SystemFontDefinition {
    u32 font_set_type;
    const char* config_key;   // legacy UPPER_SNAKE (e.g., FONTSET_SST_STD_EUROPEAN_LIGHT)
    const char* default_file; // default filename within sys font dir
};

static constexpr SystemFontDefinition kSystemFontDefinitions[] = {
    {0x18070043, "FONTSET_SST_STD_EUROPEAN_LIGHT", "SST-Light.otf"},
    {0x18070044, "FONTSET_SST_STD_EUROPEAN", "SST-Roman.otf"},
    {0x18070045, "FONTSET_SST_STD_EUROPEAN_MEDIUM", "SST-Medium.otf"},
    {0x18070047, "FONTSET_SST_STD_EUROPEAN_BOLD", "SST-Bold.otf"},
    {0x18070053, "FONTSET_SST_STD_VIETNAMESE_LIGHT", "SSTVietnamese-Light.otf"},
    {0x18070054, "FONTSET_SST_STD_VIETNAMESE", "SSTVietnamese-Roman.otf"},
    {0x18070055, "FONTSET_SST_STD_VIETNAMESE_MEDIUM", "SSTVietnamese-Medium.otf"},
    {0x18070057, "FONTSET_SST_STD_VIETNAMESE_BOLD", "SSTVietnamese-Bold.otf"},
    {0x180700C3, "FONTSET_SST_STD_EUROPEAN_AR_LIGHT", "SSTArabic-Light.otf"},
    {0x180700C4, "FONTSET_SST_STD_EUROPEAN_AR", "SSTArabic-Roman.otf"},
    {0x180700C5, "FONTSET_SST_STD_EUROPEAN_AR_MEDIUM", "SSTArabic-Medium.otf"},
    {0x180700C7, "FONTSET_SST_STD_EUROPEAN_AR_BOLD", "SSTArabic-Bold.otf"},
    {0x18070444, "FONTSET_SST_STD_EUROPEAN_JP", "SSTVietnamese-Roman.otf"},
    {0x18070447, "FONTSET_SST_STD_EUROPEAN_JP_BOLD", "SSTVietnamese-Bold.otf"},
    {0x18070454, "FONTSET_SST_STD_EUROPEAN_JP", "SSTVietnamese-Roman.otf"},
    {0x18070457, "FONTSET_SST_STD_EUROPEAN_JP_BOLD", "SSTVietnamese-Bold.otf"},
    {0x180704C4, "FONTSET_SST_STD_EUROPEAN_JP_AR", "SSTArabic-Roman.otf"},
    {0x180704C7, "FONTSET_SST_STD_EUROPEAN_JP_AR_BOLD", "SSTArabic-Bold.otf"},
    {0x18071053, "FONTSET_SST_STD_THAI_LIGHT", "SSTThai-Light.otf"},
    {0x18071054, "FONTSET_SST_STD_THAI", "SSTThai-Roman.otf"},
    {0x18071055, "FONTSET_SST_STD_THAI_MEDIUM", "SSTThai-Medium.otf"},
    {0x18071057, "FONTSET_SST_STD_THAI_BOLD", "SSTThai-Bold.otf"},
    {0x18071454, "FONTSET_SST_STD_EUROPEAN_JP_TH", "SSTThai-Roman.otf"},
    {0x18071457, "FONTSET_SST_STD_EUROPEAN_JP_TH_BOLD", "SSTThai-Bold.otf"},
    {0x18072444, "FONTSET_SST_STD_EUROPEAN_JPUH", "SSTAribStdB24-Regular.ttf"},
    {0x18072447, "FONTSET_SST_STD_EUROPEAN_JPUH_BOLD", "SSTJpPro-Bold.otf"},
    {0x180724C4, "FONTSET_SST_STD_EUROPEAN_JPUH_AR", "SSTArabic-Roman.otf"},
    {0x180724C7, "FONTSET_SST_STD_EUROPEAN_JPUH_AR_BOLD", "SSTArabic-Bold.otf"},
    {0x18073454, "FONTSET_SST_STD_EUROPEAN_JPUH_TH", "SSTThai-Roman.otf"},
    {0x18073457, "FONTSET_SST_STD_EUROPEAN_JPUH_TH_BOLD", "SSTThai-Bold.otf"},
    {0x180734D4, "FONTSET_SST_STD_EUROPEAN_JPUH_TH_AR", "SSTThai-Roman.otf"},
    {0x180734D7, "FONTSET_SST_STD_EUROPEAN_JPUH_TH_AR_BOLD", "SSTThai-Bold.otf"},
    {0x18078044, "FONTSET_SST_STD_EUROPEAN_GB", "DFHEI5-SONY.ttf"},
    {0x180780C4, "FONTSET_SST_STD_EUROPEAN_GB_AR", "SSTArabic-Roman.otf"},
    {0x18079054, "FONTSET_SST_STD_EUROPEAN_GB_TH", "SSTThai-Roman.otf"},
    {0x1807A044, "FONTSET_SST_STD_EUROPEAN_GBUH", "e046323ms.ttf"},
    {0x1807A0C4, "FONTSET_SST_STD_EUROPEAN_GBUH_AR", "SSTArabic-Bold.otf"},
    {0x1807A444, "FONTSET_SST_STD_EUROPEAN_JPCJK", "SSTJpPro-Regular.otf"},
    {0x1807A4C4, "FONTSET_SST_STD_EUROPEAN_JPCJK_AR", "SSTArabic-Roman.otf"},
    {0x1807AC44, "FONTSET_SST_STD_EUROPEAN_GBCJK", "n023055ms.ttf"},
    {0x1807ACC4, "FONTSET_SST_STD_EUROPEAN_GBCJK_AR", "SSTArabic-Bold.otf"},
    {0x1807B054, "FONTSET_SST_STD_EUROPEAN_GBUH_TH", "SSTThai-Roman.otf"},
    {0x1807B0D4, "FONTSET_SST_STD_EUROPEAN_GBUH_TH_AR", "SSTThai-Bold.otf"},
    {0x1807B454, "FONTSET_SST_STD_EUROPEAN_JPCJK_TH", "SSTThai-Roman.otf"},
    {0x1807B4D4, "FONTSET_SST_STD_EUROPEAN_JPCJK_TH_AR", "SSTThai-Bold.otf"},
    {0x1807BC54, "FONTSET_SST_STD_EUROPEAN_GBCJK_TH", "SSTThai-Roman.otf"},
    {0x1807BCD4, "FONTSET_SST_STD_EUROPEAN_GBCJK_TH_AR", "SSTThai-Bold.otf"},
    {0x18080444, "FONTSET_SST_STD_JAPANESE_JP", "SSTAribStdB24-Regular.ttf"},
    {0x18080447, "FONTSET_SST_STD_JAPANESE_JP_BOLD", "SSTAribStdB24-Regular.ttf"},
    {0x18080454, "FONTSET_SST_STD_VIETNAMESE_JP", "SSTVietnamese-Roman.otf"},
    {0x18080457, "FONTSET_SST_STD_VIETNAMESE_JP_BOLD", "SSTVietnamese-Bold.otf"},
    {0x180804C4, "FONTSET_SST_STD_JAPANESE_JP_AR", "SSTAribStdB24-Regular.ttf"},
    {0x180804C7, "FONTSET_SST_STD_JAPANESE_JP_AR_BOLD", "SSTAribStdB24-Regular.ttf"},
    {0x18081454, "FONTSET_SST_STD_ASIAN_JP_TH", "SSTThai-Roman.otf"},
    {0x18081457, "FONTSET_SST_STD_ASIAN_JP_TH_BOLD", "SSTThai-Bold.otf"},
    {0x18082444, "FONTSET_SST_STD_JAPANESE_JPUH", "SSTAribStdB24-Regular.ttf"},
    {0x18082447, "FONTSET_SST_STD_JAPANESE_JPUH_BOLD", "SSTJpPro-Bold.otf"},
    {0x180824C4, "FONTSET_SST_STD_JAPANESE_JPUH_AR", "SSTAribStdB24-Regular.ttf"},
    {0x180824C7, "FONTSET_SST_STD_JAPANESE_JPUH_AR_BOLD", "SSTJpPro-Bold.otf"},
    {0x18083454, "FONTSET_SST_STD_ASIAN_JPUH_TH", "SSTThai-Roman.otf"},
    {0x18083457, "FONTSET_SST_STD_ASIAN_JPUH_TH_BOLD", "SSTThai-Bold.otf"},
    {0x180834D4, "FONTSET_SST_STD_ASIAN_JPUH_TH_AR", "SSTThai-Roman.otf"},
    {0x180834D7, "FONTSET_SST_STD_ASIAN_JPUH_TH_AR_BOLD", "SSTThai-Bold.otf"},
    {0x1808A444, "FONTSET_SST_STD_JAPANESE_JPCJK", "SSTAribStdB24-Regular.ttf"},
    {0x1808A4C4, "FONTSET_SST_STD_JAPANESE_JPCJK_AR", "SSTJpPro-Bold.otf"},
    {0x1808B454, "FONTSET_SST_STD_ASIAN_JPCJK_TH", "SSTThai-Roman.otf"},
    {0x1808B4D4, "FONTSET_SST_STD_ASIAN_JPCJK_TH_AR", "SSTThai-Bold.otf"},
    {0x180C8044, "FONTSET_SST_STD_SCHINESE_GB", "DFHEI5-SONY.ttf"},
    {0x180C80C4, "FONTSET_SST_STD_SCHINESE_GB_AR", "DFHEI5-SONY.ttf"},
    {0x180C9054, "FONTSET_SST_STD_ASIAN_GB_TH", "SSTThai-Roman.otf"},
    {0x180CA044, "FONTSET_SST_STD_SCHINESE_GBUH", "e046323ms.ttf"},
    {0x180CA0C4, "FONTSET_SST_STD_SCHINESE_GBUH_AR", "e046323ms.ttf"},
    {0x180CAC44, "FONTSET_SST_STD_SCHINESE_GBCJK", "n023055ms.ttf"},
    {0x180CACC4, "FONTSET_SST_STD_SCHINESE_GBCJK_AR", "SSTAribStdB24-Regular.ttf"},
    {0x180CB054, "FONTSET_SST_STD_ASIAN_GBUH_TH", "SSTThai-Roman.otf"},
    {0x180CB0D4, "FONTSET_SST_STD_ASIAN_GBUH_TH_AR", "SSTThai-Bold.otf"},
    {0x180CBC54, "FONTSET_SST_STD_ASIAN_GBCJK_TH", "SSTThai-Roman.otf"},
    {0x180CBCD4, "FONTSET_SST_STD_ASIAN_GBCJK_TH_AR", "SSTThai-Bold.otf"},
    {0x18170043, "FONTSET_SST_STD_EUROPEAN_LIGHT_ITALIC", "SST-LightItalic.otf"},
    {0x18170044, "FONTSET_SST_STD_EUROPEAN_ITALIC", "SST-Italic.otf"},
    {0x18170045, "FONTSET_SST_STD_EUROPEAN_MEDIUM_ITALIC", "SST-MediumItalic.otf"},
    {0x18170047, "FONTSET_SST_STD_EUROPEAN_BOLD_ITALIC", "SST-BoldItalic.otf"},
    {0x18170444, "FONTSET_SST_STD_EUROPEAN_JP_ITALIC", "SSTJpPro-Regular.otf"},
    {0x18170447, "FONTSET_SST_STD_EUROPEAN_JP_BOLD_ITALIC", "SSTJpPro-Bold.otf"},
    {0x18370044, "FONTSET_SST_TYPEWRITER_EUROPEAN", "SSTTypewriter-Roman.otf"},
    {0x18370047, "FONTSET_SST_TYPEWRITER_EUROPEAN_BOLD", "SSTTypewriter-Bd.otf"},
    {0x18370444, "FONTSET_SST_TYPEWRITER_EUROPEAN_JP", "SSTTypewriter-Roman.otf"},
    {0x18370447, "FONTSET_SST_TYPEWRITER_EUROPEAN_JP_BOLD", "SSTTypewriter-Bd.otf"},
};

struct FontSetCache {
    bool loaded = false;
    bool available = false;
    std::vector<unsigned char> bytes;
    std::filesystem::path path;
};

static std::mutex g_fontset_cache_mutex;
static std::unordered_map<u32, FontSetCache> g_fontset_cache;

static const SystemFontDefinition* FindSystemFontDefinition(u32 font_set_type) {
    for (const auto& def : kSystemFontDefinitions) {
        if (def.font_set_type == font_set_type) {
            return &def;
        }
    }
    return nullptr;
}

static std::filesystem::path GetSysFontBaseDir() {
    // Only use the configured path. No variations.
    std::filesystem::path base = Config::getSysFontPath();
    std::error_code ec;
    if (base.empty()) {
        LOG_ERROR(Lib_Font, "SystemFonts: SysFontPath not set");
        return {};
    }
    if (std::filesystem::is_directory(base, ec)) {
        return base;
    }
    if (std::filesystem::is_regular_file(base, ec)) {
        return base.parent_path();
    }
    LOG_ERROR(Lib_Font, "SystemFonts: SysFontPath '{}' is not a valid directory or file",
              base.string());
    return {};
}

static std::string MacroToCamel(const char* macro_key) {
    if (!macro_key) {
        return {};
    }
    std::string s(macro_key);
    // Expect prefix "FONTSET_"
    const std::string prefix = "FONTSET_";
    if (s.rfind(prefix, 0) != 0) {
        return {};
    }
    std::string out = "FontSet";
    // Split on '_'
    size_t pos = prefix.size();
    while (pos < s.size()) {
        size_t next = s.find('_', pos);
        const size_t len = (next == std::string::npos) ? (s.size() - pos) : (next - pos);
        std::string token = s.substr(pos, len);
        // Lowercase then capitalize first char
        for (auto& c : token) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        if (!token.empty()) {
            token[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(token[0])));
        }
        out += token;
        if (next == std::string::npos) {
            break;
        }
        pos = next + 1;
    }
    return out;
}

static std::filesystem::path ResolveSystemFontPath(u32 font_set_type) {
    if (const auto* def = FindSystemFontDefinition(font_set_type); def) {
        const auto base_dir = GetSysFontBaseDir();
        if (base_dir.empty()) {
            return {};
        }
        if (auto override_path = Config::getSystemFontOverride(def->config_key)) {
            if (!override_path->empty() && !override_path->is_absolute() &&
                !override_path->has_parent_path()) {
                return base_dir / *override_path;
            }
            LOG_ERROR(Lib_Font,
                      "SystemFonts: override for '{}' must be a filename only (no path): '{}'",
                      def->config_key, override_path->string());
            // ignore invalid override; fall back to default filename
        }
        // Also support new-style keys:
        //  - UpperCamelCase: 'FontSetSstStdEuropeanLight'
        //  - lowerCamelCase: 'fontSetSstStdEuropeanLight'
        const auto camel_key = MacroToCamel(def->config_key);
        if (!camel_key.empty()) {
            if (auto override_path2 = Config::getSystemFontOverride(camel_key)) {
                if (!override_path2->empty() && !override_path2->is_absolute() &&
                    !override_path2->has_parent_path()) {
                    return base_dir / *override_path2;
                }
                LOG_ERROR(Lib_Font,
                          "SystemFonts: override for '{}' must be a filename only (no path): '{}'",
                          camel_key, override_path2->string());
            }
            std::string lower_camel = camel_key;
            lower_camel[0] =
                static_cast<char>(std::tolower(static_cast<unsigned char>(lower_camel[0])));
            if (auto override_path3 = Config::getSystemFontOverride(lower_camel)) {
                if (!override_path3->empty() && !override_path3->is_absolute() &&
                    !override_path3->has_parent_path()) {
                    return base_dir / *override_path3;
                }
                LOG_ERROR(Lib_Font,
                          "SystemFonts: override for '{}' must be a filename only (no path): '{}'",
                          lower_camel, override_path3->string());
            }
        }
        if (def->default_file && *def->default_file) {
            return base_dir / def->default_file;
        }
    }
    LOG_ERROR(Lib_Font, "SystemFonts: unknown font set type=0x{:08X}", font_set_type);
    return {};
}

static const FontSetCache* EnsureFontSetCache(u32 font_set_type) {
    if (font_set_type == 0) {
        return nullptr;
    }
    std::scoped_lock lock(g_fontset_cache_mutex);
    auto& cache = g_fontset_cache[font_set_type];
    if (!cache.loaded) {
        cache.loaded = true;
        const auto path = ResolveSystemFontPath(font_set_type);
        if (!path.empty() && LoadFontFile(path, cache.bytes)) {
            if (!HasTrueTypeGlyf(cache.bytes)) {
                LOG_WARNING(Lib_Font,
                            "SystemFonts: '{}' lacks 'glyf' table (likely CFF) -> falling back",
                            path.string());
                cache.bytes.clear();
                cache.available = false;
            } else {
                cache.available = true;
                cache.path = path;
            }
        } else {
            cache.available = false;
            LOG_ERROR(Lib_Font, "SystemFonts: failed to load font for set type=0x{:08X} path='{}'",
                      font_set_type, path.string());
        }
        if (!cache.available) {
            if (EnsureSystemFontBlob()) {
                cache.bytes = g_system_font_bytes;
                cache.path = g_system_font_path;
                cache.available = true;
                LOG_WARNING(Lib_Font, "SystemFonts: using fallback font '{}' for type 0x{:08X}",
                            cache.path.string(), font_set_type);
            }
        }
    }
    return cache.available ? &cache : nullptr;
}

static bool HasTrueTypeGlyf(const std::vector<unsigned char>& bytes) {
    if (bytes.size() < 12) {
        return false;
    }
    const u8* ptr = bytes.data();
    const u32 num_tables = (ptr[4] << 8) | ptr[5];
    constexpr u32 kDirEntrySize = 16;
    if (bytes.size() < 12 + static_cast<size_t>(num_tables) * kDirEntrySize) {
        return false;
    }
    const u8* dir = ptr + 12;
    for (u32 i = 0; i < num_tables; ++i) {
        const u8* entry = dir + i * kDirEntrySize;
        const u32 tag = (entry[0] << 24) | (entry[1] << 16) | (entry[2] << 8) | entry[3];
        if (tag == (('g' << 24) | ('l' << 16) | ('y' << 8) | 'f')) {
            return true;
        }
    }
    return false;
}

static bool LoadFontFile(const std::filesystem::path& path, std::vector<unsigned char>& out_bytes) {
    if (path.empty()) {
        return false;
    }
    if (!LoadGuestFileBytes(path, out_bytes) || out_bytes.empty()) {
        return false;
    }
    return true;
}

static bool LoadFontFromPath(const std::filesystem::path& path) {
    if (path.empty()) {
        return false;
    }
    if (!LoadFontFile(path, g_system_font_bytes)) {
        return false;
    }
    g_system_font_available = true;
    g_system_font_path = path;
    LOG_INFO(Lib_Font, "system font fallback loaded");
    LOG_DEBUG(Lib_Font,
              "system font fallback params:\n"
              " path={}\n"
              " size_bytes={}\n",
              g_system_font_path.string(), g_system_font_bytes.size());
    return true;
}

// Try to load a reasonable fallback font from a directory by checking a list
// of common system font files used by PS4 firmware dumps.
static bool TryLoadFallbackFromDir(const std::filesystem::path& base_dir) {
    if (base_dir.empty()) {
        return false;
    }
    const auto fallback_name = Config::getSystemFontFallbackName();
    if (!fallback_name.empty()) {
        std::filesystem::path p = fallback_name;
        p = base_dir / p;
        if (LoadFontFromPath(p)) {
            return true;
        }
        LOG_ERROR(Lib_Font, "SystemFonts: fallback font '{}' not found under '{}'", fallback_name,
                  base_dir.string());
    }
    LOG_ERROR(Lib_Font, "SystemFonts: fallback not specified. Set [SystemFonts].fallback");
    return false;
}

static void LoadSystemFontBlob() {
    const auto base_dir = GetSysFontBaseDir();
    if (!base_dir.empty() && TryLoadFallbackFromDir(base_dir)) {
        return;
    }
    g_system_font_bytes.clear();
    g_system_font_available = false;
    LOG_ERROR(Lib_Font, "SystemFace: configured font '{}' missing; no fallback available",
              Config::getSysFontPath().string());
}

static bool EnsureSystemFontBlob() {
    std::call_once(g_system_font_once, LoadSystemFontBlob);
    return g_system_font_available;
}

static bool AttachSystemFont(FontState& st, Libraries::Font::OrbisFontHandle handle) {
    if (st.ext_face_ready) {
        return true;
    }
    const FontSetCache* cache = EnsureFontSetCache(st.font_set_type);
    const std::vector<unsigned char>* source_bytes = nullptr;
    std::filesystem::path source_path;
    if (cache) {
        source_bytes = &cache->bytes;
        source_path = cache->path;
    } else {
        if (!EnsureSystemFontBlob()) {
            return false;
        }
        source_bytes = &g_system_font_bytes;
        source_path = g_system_font_path;
    }
    if (!source_bytes || source_bytes->empty()) {
        return false;
    }
    st.ext_face_data = *source_bytes;
    st.ext_cache.clear();
    st.ext_scale_for_height = 0.0f;
    st.layout_cached = false;
    st.logged_ext_use = true; // skip "external (game font)" log for built-in fallback
    const int offset = stbtt_GetFontOffsetForIndex(st.ext_face_data.data(), 0);
    if (offset < 0) {
        LOG_ERROR(Lib_Font, "SystemFace: invalid font offset in '{}'", source_path.string());
        st.ext_face_data.clear();
        st.ext_face_ready = false;
        return false;
    }
    if (stbtt_InitFont(&st.ext_face, st.ext_face_data.data(), offset) == 0) {
        LOG_ERROR(Lib_Font, "SystemFace: stbtt_InitFont failed for '{}'", source_path.string());
        st.ext_face_data.clear();
        st.ext_face_ready = false;
        return false;
    }
    st.ext_face_ready = true;
    st.system_font_path = source_path;
    LOG_INFO(Lib_Font, "system font attached");
    LOG_DEBUG(Lib_Font,
              "system font attach params:\n"
              " handle={}\n"
              " path={}\n",
              static_cast<const void*>(handle), source_path.string());
    return true;
}

static std::string ReportSystemFaceRequest(FontState& st, Libraries::Font::OrbisFontHandle handle,
                                           bool& attached_out) {
    attached_out = AttachSystemFont(st, handle);
    if (attached_out) {
        st.system_requested = true;
        return {};
    }
    if (!st.system_requested) {
        st.system_requested = true;
        const auto configured = Config::getSysFontPath();
        return fmt::format("SystemFace: handle={} requested internal font but sysFontPath ('{}') "
                           "could not be loaded",
                           static_cast<const void*>(handle), configured.string());
    }
    return {};
}
} // namespace

namespace Libraries::Font {

struct FontLibOpaque {
    u16 magic;
    u16 reserved0;
    u32 lock_word;
    u32 flags;
    u8 reserved1[0x14];
    void* alloc_ctx;
    void** alloc_vtbl;
    u8 reserved2[0x50];
    void* sys_driver;
    void* fontset_registry;
    u8 reserved3[0x10];
    void* sysfonts_ctx;
    void* external_fonts_ctx;
    u32* device_cache_buf;
};
static_assert(sizeof(FontLibOpaque) == 0xB8, "FontLibOpaque size");
static_assert(offsetof(FontLibOpaque, alloc_ctx) == 0x20, "FontLibOpaque alloc_ctx offset");
static_assert(offsetof(FontLibOpaque, alloc_vtbl) == 0x28, "FontLibOpaque alloc_vtbl offset");
static_assert(offsetof(FontLibOpaque, sys_driver) == 0x80, "FontLibOpaque sys_driver offset");
static_assert(offsetof(FontLibOpaque, fontset_registry) == 0x88,
              "FontLibOpaque fontset_registry offset");
static_assert(offsetof(FontLibOpaque, sysfonts_ctx) == 0xA0, "FontLibOpaque sysfonts_ctx offset");
static_assert(offsetof(FontLibOpaque, external_fonts_ctx) == 0xA8,
              "FontLibOpaque external_fonts_ctx offset");
static_assert(offsetof(FontLibOpaque, device_cache_buf) == 0xB0,
              "FontLibOpaque device_cache_buf offset");
struct OrbisFontRenderer_ {};

static void* g_allocator_vtbl_stub[4] = {};
static std::uint8_t g_sys_driver_stub{};
static std::uint8_t g_fontset_registry_stub{};
static std::uint8_t g_sysfonts_ctx_stub{};
static std::uint8_t g_external_fonts_ctx_stub{};
static u32 g_device_cache_stub{};

s32 PS4_SYSV_ABI sceFontAttachDeviceCacheBuffer(OrbisFontLib library, void* buffer, u32 size) {
    if (!library) {
        return ORBIS_FONT_ERROR_INVALID_LIBRARY;
    }
    auto* lib = static_cast<FontLibOpaque*>(library);
    if (lib->magic != 0x0F01) {
        return ORBIS_FONT_ERROR_INVALID_LIBRARY;
    }
    constexpr u32 kMinCacheSize = 0x1020u;
    if (size < kMinCacheSize) {
        LOG_WARNING(Lib_Font, "AttachDeviceCacheBuffer: size {} too small (min {})", size,
                    kMinCacheSize);
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    if (lib->device_cache_buf && lib->device_cache_buf != &g_device_cache_stub) {
        LOG_WARNING(Lib_Font, "AttachDeviceCacheBuffer: library={} already attached",
                    static_cast<const void*>(library));
        return ORBIS_FONT_ERROR_ALREADY_ATTACHED;
    }

    std::uint8_t* owned_buffer = nullptr;
    if (!buffer) {
        owned_buffer = new (std::nothrow) std::uint8_t[size];
        if (!owned_buffer) {
            return ORBIS_FONT_ERROR_ALLOCATION_FAILED;
        }
        buffer = owned_buffer;
    }

    auto* header = static_cast<std::uint32_t*>(buffer);
    header[0] = size;
    const u32 usable_bytes = size > 0x1000 ? size - 0x1000 : 0;
    const u32 page_count = usable_bytes >> 12;
    if (page_count == 0) {
        if (owned_buffer) {
            delete[] owned_buffer;
        }
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    header[1] = page_count;
    header[2] = 0;
    header[3] = page_count;
    if (size >= 0x20) {
        auto* meta = reinterpret_cast<std::uint64_t*>(header + 4);
        *meta = 0x000FF800001000ULL;
    }

    lib->device_cache_buf = header;
    auto& state = GetLibState(library);
    if (owned_buffer) {
        state.owned_device_cache = owned_buffer;
        state.owned_device_cache_size = size;
        lib->flags |= 1u;
    } else {
        state.owned_device_cache = nullptr;
        state.owned_device_cache_size = 0;
    }

    LOG_INFO(Lib_Font, "device cache attach requested");
    LOG_DEBUG(Lib_Font,
              "device cache attach params:\n"
              " library={}\n"
              " buffer={}\n"
              " size={}\n"
              " pages={}\n"
              " owned_buffer={}\n",
              static_cast<const void*>(library), buffer, size, page_count,
              owned_buffer ? "true" : "false");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontBindRenderer(OrbisFontHandle fontHandle, OrbisFontRenderer renderer) {
    if (!fontHandle || !renderer) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto& st = GetState(fontHandle);
    st.bound_renderer = renderer;
    LOG_INFO(Lib_Font, "renderer bind requested");
    LOG_DEBUG(Lib_Font,
              "renderer bind params:\n"
              " handle={}\n"
              " renderer={}\n",
              static_cast<const void*>(fontHandle), static_cast<const void*>(renderer));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontCharacterGetBidiLevel(OrbisFontTextCharacter* textCharacter,
                                              int* bidiLevel) {
    if (!textCharacter || !bidiLevel) {
        LOG_DEBUG(Lib_Font, "Invalid parameter");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    *bidiLevel = textCharacter->bidiLevel;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontCharacterGetSyllableStringState() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontCharacterGetTextFontCode() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontCharacterGetTextOrder(OrbisFontTextCharacter* textCharacter,
                                              void** pTextOrder) {
    if (!pTextOrder) {
        LOG_DEBUG(Lib_Font, "Invalid parameter");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    if (!textCharacter) {
        LOG_DEBUG(Lib_Font, "Invalid parameter");
        *pTextOrder = NULL;
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    *pTextOrder = textCharacter->textOrder;
    return ORBIS_OK;
}

u32 PS4_SYSV_ABI sceFontCharacterLooksFormatCharacters(OrbisFontTextCharacter* textCharacter) {
    if (!textCharacter) {
        return 0;
    }

    return (textCharacter->formatFlags & 0x04) ? textCharacter->characterCode : 0;
}

u32 PS4_SYSV_ABI sceFontCharacterLooksWhiteSpace(OrbisFontTextCharacter* textCharacter) {
    if (!textCharacter) {
        return 0;
    }

    return (textCharacter->charType == 0x0E) ? textCharacter->characterCode : 0;
}

OrbisFontTextCharacter* PS4_SYSV_ABI
sceFontCharacterRefersTextBack(OrbisFontTextCharacter* textCharacter) {
    if (!textCharacter)
        return NULL;

    OrbisFontTextCharacter* current = textCharacter->prev;
    while (current) {
        if (current->unkn_0x31 == 0 && current->unkn_0x33 == 0) {
            return current;
        }
        current = current->prev;
    }

    return NULL;
}

OrbisFontTextCharacter* PS4_SYSV_ABI
sceFontCharacterRefersTextNext(OrbisFontTextCharacter* textCharacter) {
    if (!textCharacter)
        return NULL;

    OrbisFontTextCharacter* current = textCharacter->next;
    while (current) {
        if (current->unkn_0x31 == 0 && current->unkn_0x33 == 0) {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

s32 PS4_SYSV_ABI sceFontCharactersRefersTextCodes() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontClearDeviceCache() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontCloseFont() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontControl() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontCreateGraphicsDevice() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontCreateGraphicsService() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontCreateGraphicsServiceWithEdition() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontCreateLibrary(const OrbisFontMem* memory,
                                      OrbisFontLibCreateParams create_params,
                                      OrbisFontLib* pLibrary) {
    return sceFontCreateLibraryWithEdition(memory, create_params, 0, pLibrary);
}

s32 PS4_SYSV_ABI sceFontCreateLibraryWithEdition(const OrbisFontMem* memory,
                                                 OrbisFontLibCreateParams create_params,
                                                 u64 edition, OrbisFontLib* pLibrary) {
    LOG_INFO(Lib_Font, "library create requested");
    LOG_DEBUG(Lib_Font,
              "library create params:\n"
              " memory={}\n"
              " create_params={}\n"
              " edition={}\n"
              " out_library_ptr={}\n",
              static_cast<const void*>(memory), static_cast<const void*>(create_params), edition,
              static_cast<const void*>(pLibrary));

    if (!pLibrary) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    if (!memory) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    if (memory->mem_kind != 0x0F00 || !memory->iface || !memory->iface->alloc ||
        !memory->iface->dealloc) {
        return ORBIS_FONT_ERROR_INVALID_MEMORY;
    }
    auto* lib = new (std::nothrow) FontLibOpaque{};
    if (!lib) {
        return ORBIS_FONT_ERROR_ALLOCATION_FAILED;
    }
    lib->magic = 0x0F01;
    lib->lock_word = 0;
    lib->flags = 0;
    lib->alloc_ctx = const_cast<OrbisFontMem*>(memory);
    lib->alloc_vtbl = g_allocator_vtbl_stub;
    lib->sys_driver = &g_sys_driver_stub;
    lib->fontset_registry = &g_fontset_registry_stub;
    lib->sysfonts_ctx = nullptr;
    lib->external_fonts_ctx = nullptr;
    lib->device_cache_buf = nullptr;
    *pLibrary = lib;
    auto& state = GetLibState(lib);
    state = LibraryState{};
    state.backing_memory = memory;
    state.create_params = create_params;
    state.edition = edition;
    state.native = lib;
    state.owned_device_cache = nullptr;
    state.owned_device_cache_size = 0;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontDestroyLibrary(OrbisFontLib* pLibrary) {
    if (!pLibrary || !*pLibrary) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto lib = *pLibrary;
    LOG_INFO(Lib_Font, "library destroy requested");
    LOG_DEBUG(Lib_Font,
              "library destroy params:\n"
              " library_handle_ptr={}\n"
              " library_handle={}\n",
              static_cast<const void*>(pLibrary), static_cast<const void*>(lib));
    RemoveLibState(lib);
    delete static_cast<FontLibOpaque*>(lib);
    *pLibrary = nullptr;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontCreateRenderer(const OrbisFontMem* memory,
                                       OrbisFontRendererCreateParams create_params,
                                       OrbisFontRenderer* pRenderer) {
    return sceFontCreateRendererWithEdition(memory, create_params, 0, pRenderer);
}

s32 PS4_SYSV_ABI sceFontCreateRendererWithEdition(const OrbisFontMem* memory,
                                                  OrbisFontRendererCreateParams create_params,
                                                  u64 edition, OrbisFontRenderer* pRenderer) {
    LOG_INFO(Lib_Font, "renderer create requested");
    LOG_DEBUG(Lib_Font,
              "renderer create params:\n"
              " memory={}\n"
              " create_params={}\n"
              " edition={}\n"
              " out_renderer_ptr={}\n",
              static_cast<const void*>(memory), static_cast<const void*>(create_params), edition,
              static_cast<const void*>(pRenderer));

    if (!pRenderer) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    if (!memory) {
        return ORBIS_FONT_ERROR_INVALID_MEMORY;
    }
    auto* renderer = new (std::nothrow) OrbisFontRenderer_{};
    if (!renderer) {
        return ORBIS_FONT_ERROR_ALLOCATION_FAILED;
    }
    *pRenderer = renderer;
    LOG_INFO(Lib_Font, "CreateRenderer: memory={} selection={} edition={} renderer={}",
             static_cast<const void*>(memory), static_cast<const void*>(create_params), edition,
             static_cast<const void*>(renderer));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontCreateString() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontCreateWords() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontCreateWritingLine() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontDefineAttribute() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontDeleteGlyph(const OrbisFontMem* memory, OrbisFontGlyph* glyph) {
    LOG_INFO(Lib_Font, "glyph delete requested");
    LOG_DEBUG(Lib_Font,
              "glyph delete params:\n"
              " memory={}\n"
              " glyph_ptr={}\n",
              static_cast<const void*>(memory),
              glyph ? static_cast<const void*>(*glyph) : static_cast<const void*>(nullptr));

    if (!glyph) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    OrbisFontGlyph handle = *glyph;
    if (!handle) {
        return ORBIS_FONT_ERROR_INVALID_GLYPH;
    }

    if (handle->magic != 0x0F03) {
        return ORBIS_FONT_ERROR_INVALID_GLYPH;
    }

    // We currently own the allocation; the memory pointer is only recorded for ABI compatibility.
    (void)memory;
    if (!ForgetGeneratedGlyph(handle)) {
        return ORBIS_FONT_ERROR_INVALID_GLYPH;
    }

    auto* boxed = reinterpret_cast<GeneratedGlyph*>(handle);
    delete boxed;
    *glyph = nullptr;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontDestroyGraphicsDevice() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontDestroyGraphicsService() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontDestroyRenderer(OrbisFontRenderer* pRenderer) {
    if (!pRenderer || !*pRenderer) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto renderer = *pRenderer;
    LOG_INFO(Lib_Font, "renderer destroy requested");
    LOG_DEBUG(Lib_Font,
              "renderer destroy params:\n"
              " renderer_ptr={}\n"
              " renderer={}\n",
              static_cast<const void*>(pRenderer), static_cast<const void*>(renderer));
    delete static_cast<OrbisFontRenderer_*>(renderer);
    *pRenderer = nullptr;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontDestroyString() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontDestroyWords() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontDestroyWritingLine() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontDettachDeviceCacheBuffer() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGenerateCharGlyph(OrbisFontHandle glyph_handle, u32 codepoint,
                                          const OrbisFontGenerateGlyphParams* gen_params,
                                          OrbisFontGlyph* glyph_out) {
    LOG_INFO(Lib_Font, "glyph generation requested");
    LOG_DEBUG(Lib_Font,
              "glyph generation params:\n"
              " handle={}\n"
              " codepoint={}\n"
              " id=0x{:04X}\n"
              " glyph_form={}\n"
              " metrics_form={}\n"
              " form_options=0x{:X}\n"
              " mem={}\n",
              static_cast<const void*>(glyph_handle), codepoint,
              gen_params ? static_cast<u32>(gen_params->id) : 0u,
              gen_params ? static_cast<u32>(gen_params->glyph_form) : 0u,
              gen_params ? static_cast<u32>(gen_params->metrics_form) : 0u,
              gen_params ? static_cast<u32>(gen_params->form_options) : 0u,
              gen_params ? static_cast<const void*>(gen_params->mem)
                         : static_cast<const void*>(nullptr));

    if (!glyph_out) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    *glyph_out = nullptr;

    if (!glyph_handle) {
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }
    if (codepoint == 0) {
        return ORBIS_FONT_ERROR_NO_SUPPORT_CODE;
    }

    auto* st = TryGetState(glyph_handle);
    if (!st) {
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    const u8 glyph_form = gen_params ? gen_params->glyph_form : 0;
    const u8 metrics_form = gen_params ? gen_params->metrics_form : 0;
    const u16 form_options = gen_params ? gen_params->form_options : 0;
    const OrbisFontMem* glyph_memory = gen_params ? gen_params->mem : nullptr;

    if (gen_params && gen_params->id != 0x0FD3) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    if ((form_options & static_cast<u16>(~0x11)) != 0) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    if (glyph_form == 0 && metrics_form != 0) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    if ((glyph_form != 0 && metrics_form == 0) || glyph_form > 1 || metrics_form > 4) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    if (glyph_memory && glyph_memory->mem_kind != 0xF00) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    OrbisFontGlyphMetrics metrics{};
    const s32 metrics_res = sceFontGetCharGlyphMetrics(glyph_handle, codepoint, &metrics);
    if (metrics_res != ORBIS_OK) {
        return metrics_res;
    }

    auto* boxed = new (std::nothrow) GeneratedGlyph();
    if (!boxed) {
        return ORBIS_FONT_ERROR_ALLOCATION_FAILED;
    }

    boxed->codepoint = codepoint;
    boxed->metrics = metrics;
    boxed->glyph.magic = 0x0F03;
    boxed->glyph.flags = form_options;
    boxed->glyph.glyph_form = glyph_form;
    boxed->glyph.metrics_form = metrics_form;
    boxed->glyph.em_size = ClampToU16(st->scale_h);
    boxed->glyph.baseline = ClampToU16(metrics.h_bearing_y);
    boxed->glyph.height_px = ClampToU16(metrics.h);
    boxed->glyph.origin_x = 0;
    boxed->glyph.origin_y = boxed->glyph.baseline;
    boxed->glyph.scale_x = st->scale_w;
    boxed->glyph.base_scale = st->scale_h;
    boxed->glyph.memory = glyph_memory;
    boxed->owner_handle = glyph_handle;
    boxed->outline = {};
    boxed->outline.outline_flags = boxed->glyph.flags;
    boxed->outline.points_ptr = nullptr;
    boxed->outline.tags_ptr = nullptr;
    boxed->outline.contour_end_idx = nullptr;
    boxed->outline.contours_cnt = 0;
    boxed->outline.points_cnt = 0;
    boxed->outline_initialized = false;

    TrackGeneratedGlyph(&boxed->glyph);
    *glyph_out = &boxed->glyph;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetAttribute() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetCharGlyphCode() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetCharGlyphMetrics(OrbisFontHandle fontHandle, u32 code,
                                            OrbisFontGlyphMetrics* metrics) {
    if (!metrics) {
        LOG_DEBUG(Lib_Font, "sceFontGetCharGlyphMetrics: invalid params");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto& st = GetState(fontHandle);
    const stbtt_fontinfo* face = nullptr;
    float scale = 0.0f;

    if (st.ext_face_ready) {
        if (st.ext_scale_for_height == 0.0f)
            st.ext_scale_for_height = stbtt_ScaleForPixelHeight(&st.ext_face, st.scale_h);
        const int glyph = stbtt_FindGlyphIndex(&st.ext_face, static_cast<int>(code));
        if (glyph > 0) {
            face = &st.ext_face;
            scale = st.ext_scale_for_height;
            if (!st.logged_ext_use) {
                LOG_INFO(Lib_Font, "RenderFace: handle={} source=external (game font)",
                         static_cast<const void*>(fontHandle));
                st.logged_ext_use = true;
            }
        }
    }

    if (!face) {
        bool system_attached = false;
        const std::string sys_log = ReportSystemFaceRequest(st, fontHandle, system_attached);
        if (!sys_log.empty()) {
            LOG_ERROR(Lib_Font, "{}", sys_log);
        }
        if (system_attached) {
            face = &st.ext_face;
            if (st.ext_scale_for_height == 0.0f)
                st.ext_scale_for_height = ComputeScaleExt(&st.ext_face, st.scale_h);
            scale = st.ext_scale_for_height;
        }
    }

    if (face) {
        const int pixel_h = std::max(1, (int)std::lround(st.scale_h));
        const std::uint64_t key = MakeGlyphKey(code, pixel_h);
        GlyphEntry* ge = nullptr;
        if (auto it = st.ext_cache.find(key); it != st.ext_cache.end()) {
            ge = &it->second;
        }
        if (!ge) {
            GlyphEntry entry{};
            int aw = 0, lsb = 0;
            stbtt_GetCodepointHMetrics(face, static_cast<int>(code), &aw, &lsb);
            stbtt_GetCodepointBitmapBox(face, static_cast<int>(code), scale, scale, &entry.x0,
                                        &entry.y0, &entry.x1, &entry.y1);
            entry.w = std::max(0, entry.x1 - entry.x0);
            entry.h = std::max(0, entry.y1 - entry.y0);
            entry.advance = static_cast<float>(aw) * scale;
            entry.bearingX = static_cast<float>(lsb) * scale;
            ge = &st.ext_cache.emplace(key, std::move(entry)).first->second;
        }
        metrics->w = ge->w > 0 ? static_cast<float>(ge->w) : st.scale_w;
        metrics->h = ge->h > 0 ? static_cast<float>(ge->h) : st.scale_h;
        metrics->h_bearing_x = ge->bearingX;
        metrics->h_bearing_y = static_cast<float>(-ge->y0);
        metrics->h_adv = ge->advance > 0.0f ? ge->advance : st.scale_w;
        metrics->v_bearing_x = 0.0f;
        metrics->v_bearing_y = 0.0f;
        metrics->v_adv = 0.0f;
        LOG_TRACE(Lib_Font,
                  "GetCharGlyphMetrics: code=U+{:04X} src=external size=({}, {}) adv={} "
                  "bearing=({}, {}) box={}x{}",
                  code, st.scale_w, st.scale_h, metrics->h_adv, metrics->h_bearing_x,
                  metrics->h_bearing_y, metrics->w, metrics->h);
        return ORBIS_OK;
    }
    metrics->w = st.scale_w;
    metrics->h = st.scale_h;
    metrics->h_bearing_x = 0.0f;
    metrics->h_bearing_y = st.scale_h;
    metrics->h_adv = st.scale_w;
    metrics->v_bearing_x = 0.0f;
    metrics->v_bearing_y = 0.0f;
    metrics->v_adv = 0.0f;
    LOG_TRACE(Lib_Font, "GetCharGlyphMetrics(fallback): code=U+{:04X} size=({}, {}) box={}x{}",
              code, st.scale_w, st.scale_h, metrics->w, metrics->h);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetEffectSlant() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetEffectWeight() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetFontGlyphsCount() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetFontGlyphsOutlineProfile() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetFontMetrics() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetFontResolution() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetFontStyleInformation() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetGlyphExpandBufferState() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetHorizontalLayout(OrbisFontHandle fontHandle,
                                            OrbisFontHorizontalLayout* layout) {
    if (!fontHandle || !layout) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto* st_ptr = TryGetState(fontHandle);
    if (!st_ptr) {
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }
    auto& st = *st_ptr;
    if (!st.ext_face_ready) {
        bool system_attached = false;
        const std::string sys_log = ReportSystemFaceRequest(st, fontHandle, system_attached);
        if (!sys_log.empty()) {
            LOG_ERROR(Lib_Font, "{}", sys_log);
        }
    }
    if (st.ext_face_ready) {
        if (st.ext_scale_for_height == 0.0f)
            st.ext_scale_for_height = stbtt_ScaleForPixelHeight(&st.ext_face, st.scale_h);
        int asc = 0, desc = 0, gap = 0;
        stbtt_GetFontVMetrics(&st.ext_face, &asc, &desc, &gap);
        const float scale = st.ext_scale_for_height;
        layout->baselineOffset = static_cast<float>(asc) * scale;
        layout->lineAdvance = static_cast<float>(asc - desc + gap) * scale;
        layout->decorationExtent = 0.0f;
        LOG_INFO(
            Lib_Font,
            "GetHorizontalLayout: handle={} ext_ready={} baselineOffset={} lineAdvance={} scale={}",
            static_cast<const void*>(fontHandle), st.ext_face_ready, layout->baselineOffset,
            layout->lineAdvance, scale);
        return ORBIS_OK;
    }
    layout->baselineOffset = st.scale_h * 0.8f;
    layout->lineAdvance = st.scale_h;
    layout->decorationExtent = 0.0f;
    LOG_INFO(
        Lib_Font,
        "GetHorizontalLayout: fallback handle={} ext_ready={} baselineOffset={} lineAdvance={}",
        static_cast<const void*>(fontHandle), st.ext_face_ready, layout->baselineOffset,
        layout->lineAdvance);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetKerning(OrbisFontHandle fontHandle, u32 preCode, u32 code,
                                   OrbisFontKerning* kerning) {
    if (!kerning) {
        LOG_DEBUG(Lib_Font, "sceFontGetKerning: invalid params");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto& st = GetState(fontHandle);
    if (!st.ext_face_ready) {
        bool system_attached = false;
        const std::string sys_log = ReportSystemFaceRequest(st, fontHandle, system_attached);
        if (!sys_log.empty()) {
            LOG_ERROR(Lib_Font, "{}", sys_log);
        }
    }
    if (st.ext_face_ready) {
        if (st.ext_scale_for_height == 0.0f)
            st.ext_scale_for_height = stbtt_ScaleForPixelHeight(&st.ext_face, st.scale_h);
        int g1 = stbtt_FindGlyphIndex(&st.ext_face, static_cast<int>(preCode));
        int g2 = stbtt_FindGlyphIndex(&st.ext_face, static_cast<int>(code));
        if (g1 > 0 && g2 > 0) {
            const int kern = stbtt_GetCodepointKernAdvance(&st.ext_face, static_cast<int>(preCode),
                                                           static_cast<int>(code));
            const float kx = static_cast<float>(kern) * st.ext_scale_for_height;
            kerning->dx = kx;
            kerning->dy = 0.0f;
            kerning->px = 0.0f;
            kerning->py = 0.0f;
            LOG_TRACE(Lib_Font, "GetKerning: pre=U+{:04X} code=U+{:04X} dx={}", preCode, code, kx);
            return ORBIS_OK;
        }
    }
    kerning->dx = 0.0f;
    kerning->dy = 0.0f;
    kerning->px = 0.0f;
    kerning->py = 0.0f;
    LOG_TRACE(Lib_Font, "GetKerning: pre=U+{:04X} code=U+{:04X} dx=0 (no face)", preCode, code);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetLibrary(OrbisFontHandle fontHandle, OrbisFontLib* pLibrary) {
    if (!pLibrary) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    const auto& st = GetState(fontHandle);
    *pLibrary = st.library;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetPixelResolution() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetRenderCharGlyphMetrics(OrbisFontHandle fontHandle, u32 codepoint,
                                                  OrbisFontGlyphMetrics* out_metrics) {
    if (!fontHandle || !out_metrics) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto& st = GetState(fontHandle);
    if (!st.bound_renderer) {
        LOG_DEBUG(Lib_Font, "GetRenderCharGlyphMetrics: renderer not bound for handle={}",
                  static_cast<const void*>(fontHandle));
        return ORBIS_FONT_ERROR_NOT_BOUND_RENDERER;
    }
    return sceFontGetCharGlyphMetrics(fontHandle, codepoint, out_metrics);
}

s32 PS4_SYSV_ABI sceFontGetRenderEffectSlant() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetRenderEffectWeight() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetRenderScaledKerning() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetRenderScalePixel(OrbisFontHandle fontHandle, float* out_w,
                                            float* out_h) {
    if (!fontHandle || (!out_w && !out_h)) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto* st = TryGetState(fontHandle);
    if (!st) {
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }
    if (out_w)
        *out_w = st->scale_w;
    if (out_h)
        *out_h = st->scale_h;
    if (!st->bound_renderer) {
        LOG_DEBUG(Lib_Font,
                  "GetRenderScalePixel: no renderer bound; returning configured scale w={} h={}",
                  out_w ? *out_w : -1.0f, out_h ? *out_h : -1.0f);
    } else {
        LOG_DEBUG(Lib_Font, "GetRenderScalePixel: handle={} -> w={}, h={}",
                  static_cast<const void*>(fontHandle), out_w ? *out_w : -1.0f,
                  out_h ? *out_h : -1.0f);
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetRenderScalePoint(OrbisFontHandle fontHandle, float* out_w,
                                            float* out_h) {
    return sceFontGetScalePoint(fontHandle, out_w, out_h);
}

s32 PS4_SYSV_ABI sceFontGetResolutionDpi() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetScalePixel(OrbisFontHandle fontHandle, float* out_w, float* out_h) {
    if (!fontHandle || (!out_w && !out_h)) {
        LOG_DEBUG(Lib_Font, "sceFontGetScalePixel: invalid params");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto* st = TryGetState(fontHandle);
    if (!st) {
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }
    if (out_w)
        *out_w = st->scale_w;
    if (out_h)
        *out_h = st->scale_h;
    LOG_DEBUG(Lib_Font, "GetScalePixel: handle={} -> w={}, h={}",
              static_cast<const void*>(fontHandle), out_w ? *out_w : -1.0f, out_h ? *out_h : -1.0f);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetScalePoint(OrbisFontHandle fontHandle, float* out_w, float* out_h) {
    if (!fontHandle || (!out_w && !out_h)) {
        LOG_DEBUG(Lib_Font, "sceFontGetScalePoint: invalid params");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto* st = TryGetState(fontHandle);
    if (!st) {
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }
    const float default_w = PixelsToPoints(st->scale_w, st->dpi_x);
    const float default_h = PixelsToPoints(st->scale_h, st->dpi_y);
    const float point_w = st->scale_point_active ? st->scale_point_w : default_w;
    const float point_h = st->scale_point_active ? st->scale_point_h : default_h;
    if (out_w)
        *out_w = point_w;
    if (out_h)
        *out_h = point_h;
    LOG_DEBUG(Lib_Font, "GetScalePoint: handle={} -> w={}pt h={}pt (active={})",
              static_cast<const void*>(fontHandle), point_w, point_h, st->scale_point_active);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetScriptLanguage() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetTypographicDesign() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetVerticalLayout(OrbisFontHandle fontHandle,
                                          OrbisFontVerticalLayout* layout) {
    if (!layout) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto& st = GetState(fontHandle);
    if (!st.ext_face_ready) {
        bool system_attached = false;
        const std::string sys_log = ReportSystemFaceRequest(st, fontHandle, system_attached);
        if (!sys_log.empty()) {
            LOG_ERROR(Lib_Font, "{}", sys_log);
        }
    }
    if (st.ext_face_ready) {
        if (st.ext_scale_for_height == 0.0f)
            st.ext_scale_for_height = ComputeScaleExt(&st.ext_face, st.scale_h);
        int asc = 0, desc = 0, gap = 0;
        stbtt_GetFontVMetrics(&st.ext_face, &asc, &desc, &gap);
        layout->baselineOffsetX = 0.0f;
        layout->columnAdvance = static_cast<float>(asc - desc + gap) * st.ext_scale_for_height;
        layout->decorationSpan = 0.0f;
        return ORBIS_OK;
    }
    layout->baselineOffsetX = 0.0f;
    layout->columnAdvance = st.scale_h;
    layout->decorationSpan = 0.0f;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGlyphDefineAttribute() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGlyphGetAttribute() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGlyphGetGlyphForm() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGlyphGetMetricsForm() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGlyphGetScalePixel() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGlyphRefersMetrics() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGlyphRefersMetricsHorizontal() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGlyphRefersMetricsHorizontalAdvance() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGlyphRefersMetricsHorizontalX() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

OrbisFontGlyphOutline* PS4_SYSV_ABI sceFontGlyphRefersOutline(OrbisFontGlyph glyph) {
    if (!glyph || glyph->magic != 0x0F03 || glyph->glyph_form != 1) {
        return nullptr;
    }
    {
        std::scoped_lock lock(g_generated_glyph_mutex);
        if (g_generated_glyphs.find(glyph) == g_generated_glyphs.end()) {
            return nullptr;
        }
    }
    auto* boxed = reinterpret_cast<GeneratedGlyph*>(glyph);
    if (!boxed->outline_initialized) {
        if (!BuildTrueOutline(*boxed)) {
            BuildBoundingOutline(*boxed);
        }
    }
    return &boxed->outline;
}

s32 PS4_SYSV_ABI sceFontGlyphRenderImage() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGlyphRenderImageHorizontal() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGlyphRenderImageVertical() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsBeginFrame() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsDrawingCancel() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsDrawingFinish() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsEndFrame() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsExchangeResource() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsFillMethodInit() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsFillPlotInit() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsFillPlotSetLayout() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsFillPlotSetMapping() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsFillRatesInit() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsFillRatesSetFillEffect() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsFillRatesSetLayout() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsFillRatesSetMapping() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsGetDeviceUsage() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsRegionInit() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsRegionInitCircular() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsRegionInitRoundish() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsRelease() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsRenderResource() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsSetFramePolicy() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsSetupClipping() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsSetupColorRates() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsSetupFillMethod() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsSetupFillRates() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsSetupGlyphFill() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsSetupGlyphFillPlot() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsSetupHandleDefault() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsSetupLocation() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsSetupPositioning() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsSetupRotation() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsSetupScaling() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsSetupShapeFill() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsSetupShapeFillPlot() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsStructureCanvas() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsStructureCanvasSequence() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsStructureDesign() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsStructureDesignResource() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsStructureSurfaceTexture() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsUpdateClipping() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsUpdateColorRates() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsUpdateFillMethod() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsUpdateFillRates() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsUpdateGlyphFill() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsUpdateGlyphFillPlot() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsUpdateLocation() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsUpdatePositioning() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsUpdateRotation() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsUpdateScaling() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsUpdateShapeFill() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsUpdateShapeFillPlot() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontMemoryInit(OrbisFontMem* mem_desc, void* region_addr, u32 region_size,
                                   const OrbisFontMemInterface* iface, void* mspace_obj,
                                   OrbisFontMemDestroyCb destroy_cb, void* destroy_ctx) {
    if (!mem_desc) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    if (!iface && (!region_addr || region_size == 0)) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    mem_desc->mem_kind = 0x0F00;
    mem_desc->attr_bits = 0;
    mem_desc->region_base = region_addr;
    mem_desc->region_size = region_size;
    mem_desc->iface = iface;
    mem_desc->mspace_handle = mspace_obj;
    mem_desc->on_destroy = destroy_cb;
    mem_desc->destroy_ctx = destroy_ctx;
    mem_desc->some_ctx1 = nullptr;
    mem_desc->some_ctx2 = nullptr;

    LOG_INFO(Lib_Font, "font memory init requested");
    LOG_DEBUG(Lib_Font,
              "font memory init params:\n"
              " mem_desc={}\n"
              " region_addr={}\n"
              " region_size={}\n"
              " mspace_obj={}\n"
              " has_iface={}\n"
              " destroy_cb={}\n",
              static_cast<const void*>(mem_desc), static_cast<const void*>(region_addr),
              region_size, static_cast<const void*>(mspace_obj), iface != nullptr,
              reinterpret_cast<const void*>(destroy_cb));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontMemoryTerm(OrbisFontMem* mem_desc) {
    if (!mem_desc) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    if (mem_desc->mem_kind != 0x0F00) {
        return ORBIS_FONT_ERROR_INVALID_MEMORY;
    }
    if (mem_desc->on_destroy) {
        mem_desc->on_destroy(mem_desc, mem_desc->destroy_ctx, mem_desc->some_ctx1);
    }
    std::memset(mem_desc, 0, sizeof(*mem_desc));
    LOG_INFO(Lib_Font, "font memory term requested");
    LOG_DEBUG(Lib_Font,
              "font memory term params:\n"
              " mem_desc={}\n",
              static_cast<const void*>(mem_desc));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontOpenFontFile(OrbisFontLib library, const char* guest_path, u32 open_mode,
                                     const OrbisFontOpenParams* open_detail,
                                     OrbisFontHandle* out_handle) {
    if (!library || !guest_path || !out_handle) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    const auto host_path = ResolveGuestPath(guest_path);
    const std::filesystem::path path_to_open =
        host_path.empty() ? std::filesystem::path(guest_path) : host_path;

    std::vector<unsigned char> file_bytes;
    if (!LoadGuestFileBytes(path_to_open, file_bytes)) {
        LOG_WARNING(Lib_Font, "OpenFontFile: failed to open '{}'", path_to_open.string());
        return ORBIS_FONT_ERROR_FS_OPEN_FAILED;
    }
    if (file_bytes.size() > std::numeric_limits<u32>::max()) {
        LOG_WARNING(Lib_Font, "OpenFontFile: '{}' exceeds libSceFont size limit ({} bytes)",
                    path_to_open.string(), file_bytes.size());
        return ORBIS_FONT_ERROR_FS_OPEN_FAILED;
    }
    LOG_INFO(Lib_Font, "OpenFontFile: path='{}' size={} openMode={}", path_to_open.string(),
             static_cast<u32>(file_bytes.size()), open_mode);
    return sceFontOpenFontMemory(library, file_bytes.data(), static_cast<u32>(file_bytes.size()),
                                 open_detail, out_handle);
}

s32 PS4_SYSV_ABI sceFontOpenFontInstance(OrbisFontHandle fontHandle, OrbisFontHandle templateFont,
                                         OrbisFontHandle* pFontHandle) {
    if ((!templateFont && !fontHandle) || !pFontHandle) {
        LOG_ERROR(Lib_Font, "OpenFontInstance: invalid params base={} template={} out_ptr={}",
                  static_cast<const void*>(fontHandle), static_cast<const void*>(templateFont),
                  static_cast<const void*>(pFontHandle));
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    LOG_INFO(Lib_Font, "OpenFontInstance(begin): base={} template={} out_ptr={}",
             static_cast<const void*>(fontHandle), static_cast<const void*>(templateFont),
             static_cast<const void*>(pFontHandle));
    auto* src_state = TryGetState(templateFont);
    if (!src_state) {
        templateFont = nullptr;
        src_state = TryGetState(fontHandle);
    }
    if (!src_state) {
        LOG_ERROR(Lib_Font, "OpenFontInstance: template handle={} missing",
                  templateFont ? static_cast<const void*>(templateFont)
                               : static_cast<const void*>(fontHandle));
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }
    FontState* base_state = TryGetState(fontHandle);
    if (!base_state && fontHandle) {
        LOG_WARNING(Lib_Font,
                    "OpenFontInstance: base handle={} unknown, falling back to template state",
                    static_cast<const void*>(fontHandle));
    }
    auto* new_handle = new (std::nothrow) OrbisFontHandleOpaque{};
    if (!new_handle) {
        LOG_ERROR(Lib_Font, "OpenFontInstance: allocation failed");
        return ORBIS_FONT_ERROR_ALLOCATION_FAILED;
    }
    const std::size_t src_ext_bytes = src_state->ext_face_data.size();
    LOG_DEBUG(Lib_Font,
              "font instance clone template state:\n"
              " ext_ready={}\n"
              " bytes={}\n"
              " cache_size={}\n"
              " dpi_x={}\n"
              " dpi_y={}\n"
              " scale_w={}\n"
              " scale_h={}\n"
              " scale_point_active={}\n",
              src_state->ext_face_ready, src_ext_bytes, src_state->ext_cache.size(),
              src_state->dpi_x, src_state->dpi_y, src_state->scale_w, src_state->scale_h,
              src_state->scale_point_active);
    auto& dst = GetState(new_handle);
    dst = *src_state;
    dst.ext_face = {};
    dst.ext_cache.clear();
    dst.scratch.clear();
    dst.library = base_state ? base_state->library : src_state->library;
    dst.bound_renderer = base_state ? base_state->bound_renderer : src_state->bound_renderer;
    dst.logged_ext_use = false;
    bool stbtt_ok = false;
    if (dst.ext_face_ready && !dst.ext_face_data.empty()) {
        const int font_offset = src_state->ext_face.fontstart;
        LOG_INFO(Lib_Font, "reinitializing stbtt font for clone");
        LOG_DEBUG(Lib_Font,
                  "stbtt reinit params:\n"
                  " template_handle={}\n"
                  " new_handle={}\n"
                  " offset={}\n"
                  " data_size={}\n",
                  static_cast<const void*>(templateFont), static_cast<const void*>(new_handle),
                  font_offset, dst.ext_face_data.size());
        if (stbtt_InitFont(&dst.ext_face, dst.ext_face_data.data(), font_offset) == 0) {
            LOG_WARNING(Lib_Font, "stbtt_InitFont failed during clone");
            dst.ext_face_ready = false;
            dst.ext_cache.clear();
        } else {
            dst.ext_scale_for_height = ComputeScaleExt(&dst.ext_face, dst.scale_h);
            stbtt_ok = true;
        }
    } else {
        LOG_DEBUG(Lib_Font,
                  "font instance clone missing external face:\n"
                  " template_handle={}\n"
                  " ready={}\n"
                  " data_size={}\n",
                  static_cast<const void*>(templateFont), dst.ext_face_ready,
                  dst.ext_face_data.size());
        dst.ext_face_ready = false;
    }
    dst.layout_cached = false;
    *pFontHandle = new_handle;
    LOG_INFO(Lib_Font, "font instance clone requested");
    LOG_DEBUG(Lib_Font,
              "font instance clone result:\n"
              " base_handle={}\n"
              " template_handle={}\n"
              " new_handle={}\n"
              " ext_ready={}\n"
              " renderer_inherited={}\n"
              " stbtt_reinit={}\n"
              " library={}\n"
              " renderer={}\n"
              " dpi_x={}\n"
              " dpi_y={}\n"
              " scale_w={}\n"
              " scale_h={}\n",
              static_cast<const void*>(fontHandle), static_cast<const void*>(templateFont),
              static_cast<const void*>(new_handle), dst.ext_face_ready,
              dst.bound_renderer != nullptr, stbtt_ok, static_cast<const void*>(dst.library),
              static_cast<const void*>(dst.bound_renderer), dst.dpi_x, dst.dpi_y, dst.scale_w,
              dst.scale_h);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontOpenFontMemory(OrbisFontLib library, const void* fontAddress, u32 fontSize,
                                       const OrbisFontOpenParams* open_params,
                                       OrbisFontHandle* pFontHandle) {
    if (!library || !fontAddress || fontSize == 0 || !pFontHandle) {
        LOG_DEBUG(Lib_Font, "sceFontOpenFontMemory: invalid params");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    (void)open_params;
    auto* f = new OrbisFontHandleOpaque{};
    *pFontHandle = f;
    auto& st = GetState(f);
    st.library = library;
    st.font_set_type = 0;
    st.system_font_path.clear();
    const unsigned char* p = reinterpret_cast<const unsigned char*>(fontAddress);
    auto& ls = GetLibState(library);
    LOG_INFO(Lib_Font, "font memory open requested");
    LOG_DEBUG(Lib_Font,
              "font memory open params:\n"
              " library={}\n"
              " font_address={}\n"
              " font_size={}\n"
              " open_params={}\n"
              " out_handle={}\n"
              " sig='{}{}{}{}'\n"
              " external_supported={}\n"
              " external_formats=0x{:X}",
              static_cast<const void*>(library), fontAddress, fontSize,
              static_cast<const void*>(open_params), static_cast<const void*>(*pFontHandle),
              (fontSize >= 1 ? (char)p[0] : '?'), (fontSize >= 2 ? (char)p[1] : '?'),
              (fontSize >= 3 ? (char)p[2] : '?'), (fontSize >= 4 ? (char)p[3] : '?'),
              ls.support_external, ls.external_formats);
    st.ext_face_data.assign(reinterpret_cast<const unsigned char*>(fontAddress),
                            reinterpret_cast<const unsigned char*>(fontAddress) + fontSize);
    int font_count = stbtt_GetNumberOfFonts(st.ext_face_data.data());
    int chosen_index = 0;
    if (font_count > 1) {
        chosen_index = 0;
        if (open_params) {
            chosen_index =
                static_cast<int>(open_params->subfont_index % static_cast<u32>(font_count));
        }
    }
    int offset = stbtt_GetFontOffsetForIndex(st.ext_face_data.data(), chosen_index);
    const unsigned char* d = st.ext_face_data.data();
    const u32 sig32 = (fontSize >= 4)
                          ? (static_cast<u32>(d[0]) << 24) | (static_cast<u32>(d[1]) << 16) |
                                (static_cast<u32>(d[2]) << 8) | static_cast<u32>(d[3])
                          : 0u;
    const bool is_ttc = (font_count > 1);
    const bool is_otf_cff = (sig32 == 0x4F54544Fu);
    const bool is_ttf_sfnt = (sig32 == 0x00010000u) || (sig32 == 0x74727565u);
    const bool is_sfnt_typ1 = (sig32 == 0x74797031u);
    if (is_otf_cff) {
        LOG_WARNING(Lib_Font,
                    "ExternalFace: OTF/CFF detected (OTTO). CFF outlines are not supported;"
                    " handle={} fonts={} requested_index={} -> fallback may occur",
                    static_cast<const void*>(*pFontHandle), font_count, chosen_index);
    }
    if (stbtt_InitFont(&st.ext_face, st.ext_face_data.data(), offset)) {
        st.ext_face_ready = true;
        stbtt_GetFontVMetrics(&st.ext_face, &st.ext_ascent, &st.ext_descent, &st.ext_lineGap);
        st.ext_scale_for_height = stbtt_ScaleForPixelHeight(&st.ext_face, st.scale_h);
        LOG_INFO(Lib_Font, "external face ready");
        LOG_DEBUG(Lib_Font,
                  "external face params:\n"
                  " handle={}\n"
                  " ascent={}\n"
                  " descent={}\n"
                  " line_gap={}\n"
                  " scale={}\n"
                  " data_bytes={}\n"
                  " font_count={}\n"
                  " chosen_index={}\n"
                  " ttc={}\n"
                  " sig=0x{:08X}\n"
                  " kind={}\n",
                  static_cast<const void*>(*pFontHandle), st.ext_ascent, st.ext_descent,
                  st.ext_lineGap, st.ext_scale_for_height, (int)st.ext_face_data.size(), font_count,
                  chosen_index, is_ttc, sig32,
                  is_otf_cff ? "OTF/CFF (unsupported)"
                             : (is_ttf_sfnt ? "TTF (ready)"
                                            : (is_sfnt_typ1 ? "Type1(sfnt) (stub)" : "unknown")));
        if (is_ttf_sfnt) {
            LOG_INFO(Lib_Font, "external format TTF ready");
        } else if (is_otf_cff) {
            LOG_WARNING(Lib_Font, "external format OpenType-CFF unsupported");
        } else if (is_sfnt_typ1) {
            LOG_WARNING(Lib_Font, "external format Type1(sfnt) stub");
        }
    } else {
        LOG_WARNING(Lib_Font,
                    "ExternalFace: stbtt_InitFont failed for handle={} size={} fonts={}"
                    " chosen_index={} sig='{}{}{}{}' (OTF/CFF unsupported={})",
                    static_cast<const void*>(*pFontHandle), fontSize, font_count, chosen_index,
                    (fontSize >= 1 ? (char)p[0] : '?'), (fontSize >= 2 ? (char)p[1] : '?'),
                    (fontSize >= 3 ? (char)p[2] : '?'), (fontSize >= 4 ? (char)p[3] : '?'),
                    is_otf_cff);
        if (is_otf_cff) {
            LOG_WARNING(Lib_Font, "Stubbed: CFF outlines not implemented; system font unavailable");
        }
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontOpenFontSet(OrbisFontLib library, u32 fontSetType, u32 openMode,
                                    const OrbisFontOpenParams* open_params,
                                    OrbisFontHandle* pFontHandle) {
    if (!library) {
        return ORBIS_FONT_ERROR_INVALID_LIBRARY;
    }
    auto* lib = static_cast<FontLibOpaque*>(library);
    if (lib->magic != 0x0F01) {
        return ORBIS_FONT_ERROR_INVALID_LIBRARY;
    }
    if (!pFontHandle) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    if (!lib->sysfonts_ctx) {
        return ORBIS_FONT_ERROR_NO_SUPPORT_FUNCTION;
    }
    auto* f = new OrbisFontHandleOpaque{};
    *pFontHandle = f;
    auto& st = GetState(f);
    st.library = library;
    st.font_set_type = fontSetType;
    st.system_font_path.clear();

    LOG_INFO(Lib_Font, "font set open requested");
    LOG_DEBUG(Lib_Font,
              "font set open params:\n"
              " library={}\n"
              " font_set_type=0x{:08X}\n"
              " open_mode={}\n"
              " open_params={}\n"
              " out_handle={}\n",
              static_cast<const void*>(library), fontSetType, openMode,
              static_cast<const void*>(open_params), static_cast<const void*>(*pFontHandle));

    if (const auto* def = FindSystemFontDefinition(fontSetType)) {
        const auto pretty = MacroToCamel(def->config_key);
        LOG_DEBUG(Lib_Font,
                  "font set mapping:\n"
                  " type=0x{:08X}\n"
                  " key={}\n"
                  " default_file='{}'\n",
                  fontSetType, !pretty.empty() ? pretty.c_str() : def->config_key,
                  def->default_file ? def->default_file : "<none>");
    } else {
        LOG_DEBUG(Lib_Font,
                  "font set mapping:\n"
                  " type=0x{:08X}\n"
                  " key=(unknown)\n"
                  " default_file='<none>'\n",
                  fontSetType);
    }
    LogFontOpenParams(open_params);
    bool system_ok = false;
    const std::string sys_log = ReportSystemFaceRequest(st, f, system_ok);
    if (!sys_log.empty()) {
        LOG_ERROR(Lib_Font, "{}", sys_log);
    }

    LOG_DEBUG(Lib_Font,
              "font set open result:\n"
              " library={}\n"
              " font_set_type=0x{:08X}\n"
              " open_mode={}\n"
              " open_params={}\n"
              " handle={}\n"
              " system_available={}\n",
              static_cast<const void*>(library), fontSetType, openMode,
              static_cast<const void*>(open_params), static_cast<const void*>(*pFontHandle),
              system_ok);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontRebindRenderer(OrbisFontHandle fontHandle) {
    if (!fontHandle) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto& st = GetState(fontHandle);
    if (!st.bound_renderer) {
        return ORBIS_FONT_ERROR_NOT_BOUND_RENDERER;
    }
    LOG_INFO(Lib_Font, "RebindRenderer: handle={} renderer={}",
             static_cast<const void*>(fontHandle), static_cast<const void*>(st.bound_renderer));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontRenderCharGlyphImage(OrbisFontHandle fontHandle, u32 code,
                                             OrbisFontRenderSurface* surf, float x, float y,
                                             OrbisFontGlyphMetrics* metrics,
                                             OrbisFontRenderOutput* result) {
    return sceFontRenderCharGlyphImageHorizontal(fontHandle, code, surf, x, y, metrics, result);
}

s32 PS4_SYSV_ABI sceFontRenderCharGlyphImageHorizontal(OrbisFontHandle fontHandle, u32 code,
                                                       OrbisFontRenderSurface* surf, float x,
                                                       float y, OrbisFontGlyphMetrics* metrics,
                                                       OrbisFontRenderOutput* result) {
    LOG_INFO(Lib_Font,
             "RenderGlyph(H): handle={} code=U+{:04X} x={} y={} metrics={} result={} surf={}"
             " buf={} widthByte={} pixelSizeByte={} size={}x{} sc=[{},{}-{}:{}] styleFlag={}",
             static_cast<const void*>(fontHandle), code, x, y, static_cast<const void*>(metrics),
             static_cast<const void*>(result), static_cast<const void*>(surf),
             surf ? static_cast<const void*>(surf->buffer) : nullptr, surf ? surf->widthByte : -1,
             surf ? (int)surf->pixelSizeByte : -1, surf ? surf->width : -1,
             surf ? surf->height : -1, surf ? surf->sc_x0 : 0u, surf ? surf->sc_y0 : 0u,
             surf ? surf->sc_x1 : 0u, surf ? surf->sc_y1 : 0u, surf ? (int)surf->styleFlag : -1);
    if (!surf || !surf->buffer) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    LogStrideOnce(surf);
    LOG_DEBUG(Lib_Font,
              "RenderGlyph(H): handle={} code=U+{:04X} x={} y={} surf={} size={}x{} bpp={} "
              "sc=[{},{}-{}:{}]",
              static_cast<const void*>(fontHandle), code, x, y, static_cast<const void*>(surf),
              surf->width, surf->height, (int)surf->pixelSizeByte, surf->sc_x0, surf->sc_y0,
              surf->sc_x1, surf->sc_y1);
    if (result) {
        result->stage = nullptr;
        result->slot.maybe_addr = static_cast<u8*>(surf->buffer);
        result->slot.maybe_rowBytes = static_cast<u32>(std::max(0, surf->widthByte));
        result->slot.maybe_pixelSize = static_cast<u8>(std::max(1, (int)surf->pixelSizeByte));
        result->slot.maybe_pixelFmt = (result->slot.maybe_pixelSize == 4) ? 1 : 0;
    }
    auto& st = GetState(fontHandle);
    const OrbisFontStyleFrame* bound_style = nullptr;
    if (auto it = g_style_for_surface.find(surf); it != g_style_for_surface.end()) {
        bound_style = it->second;
    }
    const StyleFrameScaleState style_state = ResolveStyleFrameScale(bound_style, st);
    const bool style_scale_override = style_state.scale_overridden;
    float render_scale_w = style_state.scale_w;
    float render_scale_h = style_state.scale_h;
    float fw = render_scale_w;
    float fh = render_scale_h;
    int g_x0 = 0, g_y0 = 0, g_x1 = 0, g_y1 = 0;
    const stbtt_fontinfo* face = nullptr;
    float scale = 0.0f;

    const bool needs_custom_scale = std::fabs(render_scale_h - st.scale_h) > kScaleEpsilon;

    if (st.ext_face_ready) {
        if (st.ext_scale_for_height == 0.0f)
            st.ext_scale_for_height = ComputeScaleExt(&st.ext_face, st.scale_h);
        const int glyph_index = stbtt_FindGlyphIndex(&st.ext_face, static_cast<int>(code));
        if (glyph_index > 0) {
            face = &st.ext_face;
            scale = needs_custom_scale
                        ? ComputeScaleExt(&st.ext_face, std::max(0.01f, render_scale_h))
                        : st.ext_scale_for_height;
        }
    }

    if (!face) {
        bool system_attached = false;
        const std::string sys_log = ReportSystemFaceRequest(st, fontHandle, system_attached);
        if (!sys_log.empty()) {
            LOG_ERROR(Lib_Font, "{}", sys_log);
        }
        if (system_attached) {
            face = &st.ext_face;
            if (!needs_custom_scale) {
                if (st.ext_scale_for_height == 0.0f)
                    st.ext_scale_for_height = ComputeScaleExt(&st.ext_face, st.scale_h);
                scale = st.ext_scale_for_height;
            } else {
                scale = ComputeScaleExt(&st.ext_face, std::max(0.01f, render_scale_h));
            }
        }
    }

    const float frac_x = x - std::floor(x);
    const float frac_y = y - std::floor(y);
    const bool use_subpixel = (frac_x != 0.0f) || (frac_y != 0.0f);

    if (face) {
        LOG_DEBUG(Lib_Font, "RenderGlyphSrc(H): handle={} code=U+{:04X} src=external",
                  static_cast<const void*>(fontHandle), code);
        const int pixel_h = std::max(1, (int)std::lround(render_scale_h));
        const std::uint64_t key = MakeGlyphKey(code, pixel_h);
        GlyphEntry* ge = nullptr;
        if (!use_subpixel) {
            if (auto it = st.ext_cache.find(key); it != st.ext_cache.end()) {
                ge = &it->second;
            }
        }
        if (!ge) {
            GlyphEntry entry{};
            int aw = 0, lsb = 0;
            stbtt_GetCodepointHMetrics(face, static_cast<int>(code), &aw, &lsb);
            if (use_subpixel) {
                stbtt_GetCodepointBitmapBoxSubpixel(face, static_cast<int>(code), scale, scale,
                                                    frac_x, frac_y, &entry.x0, &entry.y0, &entry.x1,
                                                    &entry.y1);
            } else {
                stbtt_GetCodepointBitmapBox(face, static_cast<int>(code), scale, scale, &entry.x0,
                                            &entry.y0, &entry.x1, &entry.y1);
            }
            entry.w = std::max(0, entry.x1 - entry.x0);
            entry.h = std::max(0, entry.y1 - entry.y0);
            entry.advance = static_cast<float>(aw) * scale;
            entry.bearingX = static_cast<float>(lsb) * scale;
            if (!use_subpixel && entry.w > 0 && entry.h > 0) {
                entry.bitmap.resize(static_cast<size_t>(entry.w * entry.h));
                stbtt_MakeCodepointBitmap(face, entry.bitmap.data(), entry.w, entry.h, entry.w,
                                          scale, scale, static_cast<int>(code));
                ge = &st.ext_cache.emplace(key, std::move(entry)).first->second;
            } else if (use_subpixel) {
                fw = static_cast<float>(entry.w);
                fh = static_cast<float>(entry.h);
                g_x0 = entry.x0;
                g_y0 = entry.y0;
                g_x1 = entry.x1;
                g_y1 = entry.y1;
            }
        }
        if (ge) {
            fw = static_cast<float>(ge->w);
            fh = static_cast<float>(ge->h);
            g_x0 = ge->x0;
            g_y0 = ge->y0;
            g_x1 = ge->x1;
            g_y1 = ge->y1;
        }
    }
    if (metrics) {
        if (face) {
            int aw = 0, lsb = 0;
            stbtt_GetCodepointHMetrics(face, static_cast<int>(code), &aw, &lsb);
            metrics->w = fw;
            metrics->h = fh;
            metrics->h_bearing_x = static_cast<float>(lsb) * scale;
            metrics->h_bearing_y = static_cast<float>(-g_y0);
            metrics->h_adv = static_cast<float>(aw) * scale;
        } else {
            metrics->w = fw;
            metrics->h = fh;
            metrics->h_bearing_x = 0.0f;
            metrics->h_bearing_y = fh;
            metrics->h_adv = fw;
        }
        metrics->v_bearing_x = 0.0f;
        metrics->v_bearing_y = 0.0f;
        metrics->v_adv = 0.0f;
    }

    if (code == 0x20) {
        if (result) {
            result->new_x = 0;
            result->new_y = 0;
            result->new_w = 0;
            result->new_h = 0;
            result->ImageMetrics = {};
        }
        return ORBIS_OK;
    }

    if (!st.layout_cached) {
        UpdateCachedLayout(st);
    }
    float layout_baseline = st.layout_cached ? st.cached_baseline_y : 0.0f;
    if (style_scale_override && st.ext_face_ready) {
        const float override_scale = ComputeScaleExt(&st.ext_face, std::max(0.01f, render_scale_h));
        layout_baseline = static_cast<float>(st.ext_ascent) * override_scale;
    }

    float adjusted_y = y;
    if (face && layout_baseline > 0.0f) {
        const float top_expect = adjusted_y + static_cast<float>(g_y0);
        const float bottom_expect = top_expect + fh;
        if (bottom_expect <= 0.0f || top_expect < 0.0f) {
            adjusted_y += layout_baseline;
        }
    }

    int ix = static_cast<int>(x);
    int iy = static_cast<int>(adjusted_y);
    int left = ix;
    int top = iy;
    if (face) {
        left = ix + g_x0;
        top = iy + g_y0;
    }
    int iw = std::max(0, static_cast<int>(std::round(fw)));
    int ih = std::max(0, static_cast<int>(std::round(fh)));

    int sw = surf->width;
    int sh = surf->height;
    if (sw <= 0 || sh <= 0 || iw <= 0 || ih <= 0) {
        return ORBIS_OK;
    }

    int sx0 = 0, sy0 = 0, sx1 = sw, sy1 = sh;
    if (surf->sc_x1 > 0 || surf->sc_y1 > 0 || surf->sc_x0 > 0 || surf->sc_y0 > 0) {
        sx0 = static_cast<int>(surf->sc_x0);
        sy0 = static_cast<int>(surf->sc_y0);
        sx1 = static_cast<int>(surf->sc_x1);
        sy1 = static_cast<int>(surf->sc_y1);
        sx0 = std::clamp(sx0, 0, sw);
        sy0 = std::clamp(sy0, 0, sh);
        sx1 = std::clamp(sx1, 0, sw);
        sy1 = std::clamp(sy1, 0, sh);
    }

    int rx0 = left;
    int ry0 = top;
    int rx1 = left + iw;
    int ry1 = top + ih;

    rx0 = std::clamp(rx0, sx0, sx1);
    ry0 = std::clamp(ry0, sy0, sy1);
    rx1 = std::clamp(rx1, sx0, sx1);
    ry1 = std::clamp(ry1, sy0, sy1);

    int cw = std::max(0, rx1 - rx0);
    int ch = std::max(0, ry1 - ry0);
    if (ch == 0 && face) {
        LOG_DEBUG(Lib_Font,
                  "RenderGlyph(H): zero-height glyph code=U+{:04X} top={} ih={} g_y0={} g_y1={}"
                  " y={}, g_adv={} scale={} src=external",
                  code, top, ih, g_y0, g_y1, y, metrics ? metrics->h_adv : -1.0f, scale);
    }

    if (cw > 0 && ch > 0) {
        const int bpp = std::max(1, static_cast<int>(surf->pixelSizeByte));
        bool is_pua = (code >= 0xE000 && code <= 0xF8FF) || (code >= 0xF0000 && code <= 0xFFFFD) ||
                      (code >= 0x100000 && code <= 0x10FFFD);
        bool is_placeholder_ascii = (code == 'h' || code == 'p' || code == 'x' || code == 'o');
        if ((is_pua || is_placeholder_ascii) && g_logged_pua.insert(code).second) {
            LOG_DEBUG(
                Lib_Font,
                "GlyphTrace: code=U+{:04X} pua={} placeholder_ascii={} dst=[{},{} {}x{}] bpp={}",
                code, is_pua, is_placeholder_ascii, rx0, ry0, cw, ch, bpp);
        }
        if (face && iw > 0 && ih > 0) {
            const int pixel_h = std::max(1, (int)std::lround(render_scale_h));
            const std::uint64_t key = MakeGlyphKey(code, pixel_h);
            const GlyphEntry* ge = nullptr;
            if (auto it = st.ext_cache.find(key); it != st.ext_cache.end()) {
                ge = &it->second;
            }
            const float frac_x = x - std::floor(x);
            const float frac_y = y - std::floor(y);
            const bool use_subpixel = (frac_x != 0.0f) || (frac_y != 0.0f);
            const std::uint8_t* src_bitmap = nullptr;
            if (!use_subpixel && ge && ge->w == iw && ge->h == ih && !ge->bitmap.empty()) {
                src_bitmap = ge->bitmap.data();
            } else {
                st.scratch.assign(static_cast<size_t>(iw * ih), 0);
                if (use_subpixel) {
                    stbtt_MakeCodepointBitmapSubpixel(face, st.scratch.data(), iw, ih, iw, scale,
                                                      scale, frac_x, frac_y,
                                                      static_cast<int>(code));
                } else {
                    stbtt_MakeCodepointBitmap(face, st.scratch.data(), iw, ih, iw, scale, scale,
                                              static_cast<int>(code));
                }
                src_bitmap = st.scratch.data();
            }

            const std::uint8_t* eff_src = src_bitmap;

            const int src_x0 = std::max(0, sx0 - rx0);
            const int src_y0 = std::max(0, sy0 - ry0);
            const int dst_x0 = std::max(rx0, sx0);
            const int dst_y0 = std::max(ry0, sy0);
            const int copy_w = std::min(rx1, sx1) - dst_x0;
            const int copy_h = std::min(ry1, sy1) - dst_y0;
            if (copy_w > 0 && copy_h > 0) {
                LOG_TRACE(Lib_Font,
                          "RenderGlyph(H): code=U+{:04X} baseline=({}, {}) box=[{},{}-{}:{}] "
                          "dst=[{},{} {}x{}] bpp={}",
                          code, ix, iy, g_x0, g_y0, g_x1, g_y1, dst_x0, dst_y0, copy_w, copy_h,
                          bpp);
                for (int row = 0; row < copy_h; ++row) {
                    const int src_y = src_y0 + row;
                    const std::uint8_t* src_row = eff_src + src_y * iw + src_x0;
                    int row_shift = 0;
                    int row_dst_x = dst_x0 + row_shift;
                    int row_copy_w = copy_w;
                    if (row_dst_x < sx0) {
                        int delta = sx0 - row_dst_x;
                        row_dst_x = sx0;
                        if (delta < row_copy_w) {
                            src_row += delta;
                            row_copy_w -= delta;
                        } else {
                            row_copy_w = 0;
                        }
                    }
                    if (row_dst_x + row_copy_w > sx1) {
                        row_copy_w = std::max(0, sx1 - row_dst_x);
                    }
                    std::uint8_t* dst_row = static_cast<std::uint8_t*>(surf->buffer) +
                                            (dst_y0 + row) * surf->widthByte + row_dst_x * bpp;
                    if (bpp == 1) {
                        for (int col = 0; col < row_copy_w; ++col) {
                            std::uint8_t cov = src_row[col];
                            std::uint8_t& d = dst_row[col];
                            d = std::max(d, cov);
                        }
                    } else if (bpp == 4) {
                        std::uint32_t* px = reinterpret_cast<std::uint32_t*>(dst_row);
                        for (int col = 0; col < row_copy_w; ++col) {
                            std::uint8_t a = src_row[col];
                            px[col] = (static_cast<std::uint32_t>(a) << 24) | 0x00FFFFFFu;
                        }
                    } else {
                        for (int col = 0; col < row_copy_w; ++col) {
                            std::uint8_t a = src_row[col];
                            std::uint8_t* p = dst_row + col * bpp;
                            std::memset(p, 0xFF, static_cast<size_t>(bpp));
                            p[0] = a;
                        }
                    }
                }
            }
        } else {
            if (result) {
                result->new_x = 0;
                result->new_y = 0;
                result->new_w = 0;
                result->new_h = 0;
                result->ImageMetrics.width = 0;
                result->ImageMetrics.height = 0;
            }
            LOG_DEBUG(Lib_Font, "RenderGlyph(H): skip draw (no face/zero size) code=U+{:04X}",
                      code);
            return ORBIS_OK;
        }
    }

    if (result) {
        result->new_x = static_cast<u32>(rx0);
        result->new_y = static_cast<u32>(ry0);
        result->new_w = static_cast<u32>(cw);
        result->new_h = static_cast<u32>(ch);
        const float stride_bytes = static_cast<float>(std::max(0, surf->widthByte));
        result->ImageMetrics.bearing_x = metrics ? metrics->h_bearing_x : static_cast<float>(g_x0);
        result->ImageMetrics.bearing_y = metrics ? metrics->h_bearing_y : static_cast<float>(-g_y0);
        float adv_px = 0.0f;
        if (face) {
            int aw_tmp = 0, lsb_tmp = 0;
            stbtt_GetCodepointHMetrics(face, static_cast<int>(code), &aw_tmp, &lsb_tmp);
            adv_px = static_cast<float>(aw_tmp) * scale;
        }
        result->ImageMetrics.dv = adv_px;
        result->ImageMetrics.stride = stride_bytes;
        result->ImageMetrics.width = static_cast<u32>(cw);
        result->ImageMetrics.height = static_cast<u32>(ch);
        LOG_DEBUG(Lib_Font, "RenderGlyph(H): UpdateRect=[{},{} {}x{}] stride={} adv={}",
                  result->new_x, result->new_y, result->new_w, result->new_h,
                  result->ImageMetrics.stride, result->ImageMetrics.dv);
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontRenderCharGlyphImageVertical() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontRendererGetOutlineBufferSize() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontRendererResetOutlineBuffer() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontRendererSetOutlineBufferPolicy() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

void PS4_SYSV_ABI sceFontRenderSurfaceInit(OrbisFontRenderSurface* renderSurface, void* buffer,
                                           int bufWidthByte, int pixelSizeByte, int widthPixel,
                                           int heightPixel) {
    if (renderSurface) {
        renderSurface->buffer = buffer;
        renderSurface->widthByte = bufWidthByte;
        renderSurface->pixelSizeByte = static_cast<int8_t>(pixelSizeByte);
        renderSurface->pad0 = 0;
        renderSurface->styleFlag = 0;
        renderSurface->pad2 = 0;
        renderSurface->width = (widthPixel < 0) ? 0 : widthPixel;
        renderSurface->height = (heightPixel < 0) ? 0 : heightPixel;
        renderSurface->sc_x0 = 0;
        renderSurface->sc_y0 = 0;
        renderSurface->sc_x1 = static_cast<u32>(renderSurface->width);
        renderSurface->sc_y1 = static_cast<u32>(renderSurface->height);
        std::fill(std::begin(renderSurface->reserved_q), std::end(renderSurface->reserved_q), 0);
        LOG_INFO(Lib_Font,
                 "RenderSurfaceInit: buf={} widthByte={} pixelSizeByte={} size={}x{} "
                 "scissor=[0,0-{}:{}]",
                 static_cast<const void*>(buffer), bufWidthByte, pixelSizeByte,
                 renderSurface->width, renderSurface->height, renderSurface->sc_x1,
                 renderSurface->sc_y1);
    }
}

void PS4_SYSV_ABI sceFontRenderSurfaceSetScissor(OrbisFontRenderSurface* renderSurface, int x0,
                                                 int y0, int w, int h) {
    if (!renderSurface)
        return;

    int surfaceWidth = renderSurface->width;
    int clip_x0, clip_x1;

    if (surfaceWidth != 0) {
        if (x0 < 0) {
            clip_x0 = 0;
            clip_x1 = (w + x0 > surfaceWidth) ? surfaceWidth : w + x0;
            if (w <= -x0)
                clip_x1 = 0;
        } else {
            clip_x0 = (x0 > surfaceWidth) ? surfaceWidth : x0;
            clip_x1 = (w + x0 > surfaceWidth) ? surfaceWidth : w + x0;
        }
        renderSurface->sc_x0 = static_cast<u32>(clip_x0);
        renderSurface->sc_x1 = static_cast<u32>(clip_x1);
    }

    int surfaceHeight = renderSurface->height;
    int clip_y0, clip_y1;

    if (surfaceHeight != 0) {
        if (y0 < 0) {
            clip_y0 = 0;
            clip_y1 = (h + y0 > surfaceHeight) ? surfaceHeight : h + y0;
            if (h <= -y0)
                clip_y1 = 0;
        } else {
            clip_y0 = (y0 > surfaceHeight) ? surfaceHeight : y0;
            clip_y1 = (h + y0 > surfaceHeight) ? surfaceHeight : h + y0;
        }
        renderSurface->sc_y0 = static_cast<u32>(clip_y0);
        renderSurface->sc_y1 = static_cast<u32>(clip_y1);
    }
    LOG_INFO(Lib_Font, "RenderSurfaceSetScissor: [{},{}-{}:{}]", renderSurface->sc_x0,
             renderSurface->sc_y0, renderSurface->sc_x1, renderSurface->sc_y1);
}

s32 PS4_SYSV_ABI sceFontRenderSurfaceSetStyleFrame(OrbisFontRenderSurface* renderSurface,
                                                   OrbisFontStyleFrame* styleFrame) {
    if (!renderSurface)
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    if (!styleFrame) {
        g_style_for_surface.erase(renderSurface);
        renderSurface->styleFlag &= ~u8{0x1};
        renderSurface->reserved_q[0] = 0;
        LOG_INFO(Lib_Font, "RenderSurfaceSetStyleFrame: surf={} cleared",
                 static_cast<const void*>(renderSurface));
        return ORBIS_OK;
    }
    if (!EnsureStyleFrameInitialized(styleFrame))
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    g_style_for_surface[renderSurface] = styleFrame;
    renderSurface->styleFlag |= 0x1;
    renderSurface->reserved_q[0] = reinterpret_cast<u64>(styleFrame);
    LOG_INFO(Lib_Font,
             "RenderSurfaceSetStyleFrame: surf={} styleFrame={} flags=0x{:X} scale=({}, {}) "
             "dpi=({}, {}) mode={}",
             static_cast<const void*>(renderSurface), static_cast<const void*>(styleFrame),
             styleFrame->flags, styleFrame->scaleWidth, styleFrame->scaleHeight, styleFrame->dpiX,
             styleFrame->dpiY, styleFrame->scalingFlag);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetEffectSlant() {
    LOG_DEBUG(Lib_Font, "SetEffectSlant: no-op (effects not implemented)");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetEffectWeight() {
    LOG_DEBUG(Lib_Font, "SetEffectWeight: no-op (effects not implemented)");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetFontsOpenMode() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetResolutionDpi(OrbisFontHandle fontHandle, u32 h_dpi, u32 v_dpi) {
    if (!fontHandle) {
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }
    auto* st_ptr = TryGetState(fontHandle);
    if (!st_ptr) {
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }
    auto& st = *st_ptr;
    const u32 new_h = h_dpi == 0 ? 0x48u : h_dpi;
    const u32 new_v = v_dpi == 0 ? 0x48u : v_dpi;
    if (st.dpi_x == new_h && st.dpi_y == new_v) {
        LOG_TRACE(Lib_Font, "SetResolutionDpi: handle={} unchanged h_dpi={} v_dpi={}",
                  static_cast<const void*>(fontHandle), new_h, new_v);
        return ORBIS_OK;
    }
    st.dpi_x = new_h;
    st.dpi_y = new_v;
    st.layout_cached = false; // PS4 clears cached metrics when the resolution changes.
    LOG_INFO(Lib_Font, "resolution dpi set requested");
    LOG_DEBUG(Lib_Font,
              "resolution dpi params:\n"
              " handle={}\n"
              " h_dpi={}\n"
              " v_dpi={}\n",
              static_cast<const void*>(fontHandle), new_h, new_v);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetScalePixel(OrbisFontHandle fontHandle, float w, float h) {
    if (!fontHandle || w <= 0.0f || h <= 0.0f) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto* st_ptr = TryGetState(fontHandle);
    if (!st_ptr) {
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }
    auto& st = *st_ptr;
    st.scale_point_active = false;
    st.scale_w = w;
    st.scale_h = h;
    if (st.ext_face_ready)
        st.ext_scale_for_height = ComputeScaleExt(&st.ext_face, st.scale_h);
    bool system_attached = false;
    const std::string sys_log = ReportSystemFaceRequest(st, fontHandle, system_attached);
    if (!sys_log.empty()) {
        LOG_ERROR(Lib_Font, "{}", sys_log);
    }
    LOG_INFO(Lib_Font, "scale pixel set requested");
    LOG_DEBUG(Lib_Font,
              "scale pixel params:\n"
              " handle={}\n"
              " w={}\n"
              " h={}\n"
              " ext_scale={}\n"
              " ext_ready={}\n",
              static_cast<const void*>(fontHandle), w, h, st.ext_scale_for_height,
              st.ext_face_ready);
    st.layout_cached = false;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetScalePoint(OrbisFontHandle fontHandle, float w, float h) {
    if (!fontHandle || w <= 0.0f || h <= 0.0f) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto* st_ptr = TryGetState(fontHandle);
    if (!st_ptr) {
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }
    auto& st = *st_ptr;
    const float pixel_w = std::max(0.01f, PointsToPixels(w, st.dpi_x));
    const float pixel_h = std::max(0.01f, PointsToPixels(h, st.dpi_y));
    const bool unchanged_point = st.scale_point_active &&
                                 std::abs(st.scale_point_w - w) < kScaleEpsilon &&
                                 std::abs(st.scale_point_h - h) < kScaleEpsilon;
    const bool unchanged_pixels = std::abs(st.scale_w - pixel_w) < kScaleEpsilon &&
                                  std::abs(st.scale_h - pixel_h) < kScaleEpsilon;
    if (unchanged_point && unchanged_pixels) {
        LOG_TRACE(Lib_Font, "SetScalePoint: handle={} unchanged point=({}, {})",
                  static_cast<const void*>(fontHandle), w, h);
        return ORBIS_OK;
    }
    st.scale_point_active = true;
    st.scale_point_w = w;
    st.scale_point_h = h;
    st.scale_w = pixel_w;
    st.scale_h = pixel_h;
    if (st.ext_face_ready)
        st.ext_scale_for_height = ComputeScaleExt(&st.ext_face, st.scale_h);
    bool system_attached = false;
    const std::string sys_log = ReportSystemFaceRequest(st, fontHandle, system_attached);
    if (!sys_log.empty()) {
        LOG_ERROR(Lib_Font, "{}", sys_log);
    }
    st.layout_cached = false;
    LOG_INFO(Lib_Font, "scale point set requested");
    LOG_DEBUG(Lib_Font,
              "scale point params:\n"
              " handle={}\n"
              " point_w={}\n"
              " point_h={}\n"
              " pixel_w={}\n"
              " pixel_h={}\n"
              " dpi_x={}\n"
              " dpi_y={}\n"
              " system_attached={}\n",
              static_cast<const void*>(fontHandle), w, h, st.scale_w, st.scale_h, st.dpi_x,
              st.dpi_y, system_attached);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetScriptLanguage() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetTypographicDesign() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetupRenderEffectSlant() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetupRenderEffectWeight() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetupRenderScalePixel(OrbisFontHandle fontHandle, float w, float h) {
    auto rc = sceFontSetScalePixel(fontHandle, w, h);
    LOG_INFO(Lib_Font, "render scale pixel setup requested");
    LOG_DEBUG(Lib_Font,
              "render scale pixel setup params:\n"
              " handle={}\n"
              " w={}\n"
              " h={}\n",
              static_cast<const void*>(fontHandle), w, h);
    return rc;
}

s32 PS4_SYSV_ABI sceFontSetupRenderScalePoint(OrbisFontHandle fontHandle, float w, float h) {
    auto rc = sceFontSetScalePoint(fontHandle, w, h);
    LOG_INFO(Lib_Font, "render scale point setup requested");
    LOG_DEBUG(Lib_Font,
              "render scale point setup params:\n"
              " handle={}\n"
              " w={}\n"
              " h={}\n",
              static_cast<const void*>(fontHandle), w, h);
    return rc;
}

s32 PS4_SYSV_ABI sceFontStringGetTerminateCode() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStringGetTerminateOrder() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStringGetWritingForm() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStringRefersRenderCharacters() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStringRefersTextCharacters() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameGetEffectSlant(OrbisFontStyleFrame* styleFrame,
                                                 float* slantRatio) {
    if (!styleFrame || !slantRatio)
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    if (styleFrame->magic != kStyleFrameMagic)
        InitializeStyleFrame(*styleFrame);
    *slantRatio = (styleFrame->flags & kStyleFrameFlagSlant) ? styleFrame->slantRatio : 0.0f;
    LOG_DEBUG(Lib_Font, "StyleFrameGetEffectSlant: frame={} slant={}",
              static_cast<const void*>(styleFrame), *slantRatio);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameGetEffectWeight(OrbisFontStyleFrame* fontStyleFrame,
                                                  float* weightXScale, float* weightYScale,
                                                  uint32_t* mode) {
    if (!fontStyleFrame)
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    if (fontStyleFrame->magic != kStyleFrameMagic)
        InitializeStyleFrame(*fontStyleFrame);
    const bool has_weight = (fontStyleFrame->flags & kStyleFrameFlagWeight) != 0;
    if (weightXScale)
        *weightXScale = has_weight ? fontStyleFrame->weightXScale : 1.0f;
    if (weightYScale)
        *weightYScale = has_weight ? fontStyleFrame->weightYScale : 1.0f;
    if (mode)
        *mode = 0;
    LOG_DEBUG(Lib_Font, "StyleFrameGetEffectWeight: frame={} weight=({}, {}) mode={}",
              static_cast<const void*>(fontStyleFrame), weightXScale ? *weightXScale : -1.0f,
              weightYScale ? *weightYScale : -1.0f, mode ? *mode : 0u);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameGetResolutionDpi(const OrbisFontStyleFrame* styleFrame,
                                                   u32* h_dpi, u32* v_dpi) {
    if (!ValidateStyleFramePtr(styleFrame) || (!h_dpi && !v_dpi))
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    if (h_dpi)
        *h_dpi = styleFrame->dpiX > 0 ? static_cast<u32>(styleFrame->dpiX) : 0u;
    if (v_dpi)
        *v_dpi = styleFrame->dpiY > 0 ? static_cast<u32>(styleFrame->dpiY) : 0u;
    LOG_DEBUG(Lib_Font, "StyleFrameGetResolutionDpi: frame={} -> ({}, {})",
              static_cast<const void*>(styleFrame), h_dpi ? *h_dpi : 0u, v_dpi ? *v_dpi : 0u);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameGetScalePixel(const OrbisFontStyleFrame* styleFrame, float* w,
                                                float* h) {
    if (!ValidateStyleFramePtr(styleFrame) || (!w && !h))
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    const bool active = (styleFrame->flags & kStyleFrameFlagScale) != 0 &&
                        styleFrame->scalingFlag == static_cast<s32>(StyleFrameScalingMode::Pixel);
    if (w)
        *w = active ? styleFrame->scaleWidth : 0.0f;
    if (h)
        *h = active ? styleFrame->scaleHeight : 0.0f;
    LOG_DEBUG(Lib_Font, "StyleFrameGetScalePixel: frame={} -> w={}, h={}",
              static_cast<const void*>(styleFrame), w ? *w : 0.0f, h ? *h : 0.0f);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameGetScalePoint(const OrbisFontStyleFrame* styleFrame, float* w,
                                                float* h) {
    if (!ValidateStyleFramePtr(styleFrame) || (!w && !h))
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    const bool active = (styleFrame->flags & kStyleFrameFlagScale) != 0 &&
                        styleFrame->scalingFlag == static_cast<s32>(StyleFrameScalingMode::Point);
    if (w)
        *w = active ? styleFrame->scaleWidth : 0.0f;
    if (h)
        *h = active ? styleFrame->scaleHeight : 0.0f;
    LOG_DEBUG(Lib_Font, "StyleFrameGetScalePoint: frame={} -> w={}pt, h={}pt",
              static_cast<const void*>(styleFrame), w ? *w : 0.0f, h ? *h : 0.0f);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameInit(OrbisFontStyleFrame* styleFrame) {
    if (!styleFrame)
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    InitializeStyleFrame(*styleFrame);
    LOG_DEBUG(Lib_Font, "StyleFrameInit: frame={}", static_cast<const void*>(styleFrame));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameSetEffectSlant(OrbisFontStyleFrame* styleFrame,
                                                 float slantRatio) {
    if (!EnsureStyleFrameInitialized(styleFrame))
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    styleFrame->slantRatio = slantRatio;
    styleFrame->flags |= kStyleFrameFlagSlant;
    LOG_DEBUG(Lib_Font, "StyleFrameSetEffectSlant: frame={} slant={}",
              static_cast<const void*>(styleFrame), slantRatio);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameSetEffectWeight(OrbisFontStyleFrame* styleFrame,
                                                  float weightXScale, float weightYScale,
                                                  u32 mode) {
    (void)mode;
    if (!EnsureStyleFrameInitialized(styleFrame))
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    styleFrame->weightXScale = weightXScale;
    styleFrame->weightYScale = weightYScale;
    styleFrame->flags |= kStyleFrameFlagWeight;
    LOG_DEBUG(Lib_Font, "StyleFrameSetEffectWeight: frame={} weight=({}, {}) mode={}",
              static_cast<const void*>(styleFrame), weightXScale, weightYScale, mode);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameSetResolutionDpi(OrbisFontStyleFrame* styleFrame, u32 h_dpi,
                                                   u32 v_dpi) {
    if (!EnsureStyleFrameInitialized(styleFrame))
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    styleFrame->dpiX = static_cast<s32>(h_dpi);
    styleFrame->dpiY = static_cast<s32>(v_dpi);
    styleFrame->flags |= kStyleFrameFlagDpi;
    LOG_DEBUG(Lib_Font, "StyleFrameSetResolutionDpi: frame={} -> ({}, {})",
              static_cast<const void*>(styleFrame), h_dpi, v_dpi);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameSetScalePixel(OrbisFontStyleFrame* styleFrame, float w, float h) {
    if (!EnsureStyleFrameInitialized(styleFrame) || w <= 0.0f || h <= 0.0f)
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    styleFrame->scaleWidth = w;
    styleFrame->scaleHeight = h;
    styleFrame->scalingFlag = static_cast<s32>(StyleFrameScalingMode::Pixel);
    styleFrame->flags |= kStyleFrameFlagScale;
    LOG_DEBUG(Lib_Font, "StyleFrameSetScalePixel: frame={} -> ({}, {})",
              static_cast<const void*>(styleFrame), w, h);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameSetScalePoint(OrbisFontStyleFrame* styleFrame, float w, float h) {
    if (!EnsureStyleFrameInitialized(styleFrame) || w <= 0.0f || h <= 0.0f)
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    styleFrame->scaleWidth = w;
    styleFrame->scaleHeight = h;
    styleFrame->scalingFlag = static_cast<s32>(StyleFrameScalingMode::Point);
    styleFrame->flags |= kStyleFrameFlagScale;
    LOG_DEBUG(Lib_Font, "StyleFrameSetScalePoint: frame={} -> ({}, {})pt",
              static_cast<const void*>(styleFrame), w, h);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameUnsetEffectSlant(OrbisFontStyleFrame* styleFrame) {
    if (!EnsureStyleFrameInitialized(styleFrame))
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    styleFrame->flags &= ~kStyleFrameFlagSlant;
    styleFrame->slantRatio = 0.0f;
    LOG_DEBUG(Lib_Font, "StyleFrameUnsetEffectSlant: frame={}",
              static_cast<const void*>(styleFrame));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameUnsetEffectWeight(OrbisFontStyleFrame* styleFrame) {
    if (!EnsureStyleFrameInitialized(styleFrame))
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    styleFrame->flags &= ~kStyleFrameFlagWeight;
    styleFrame->weightXScale = 1.0f;
    styleFrame->weightYScale = 1.0f;
    LOG_DEBUG(Lib_Font, "StyleFrameUnsetEffectWeight: frame={}",
              static_cast<const void*>(styleFrame));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameUnsetScale(OrbisFontStyleFrame* styleFrame) {
    if (!EnsureStyleFrameInitialized(styleFrame))
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    styleFrame->flags &= ~kStyleFrameFlagScale;
    styleFrame->scalingFlag = static_cast<s32>(StyleFrameScalingMode::None);
    styleFrame->scaleWidth = 0.0f;
    styleFrame->scaleHeight = 0.0f;
    LOG_DEBUG(Lib_Font, "StyleFrameUnsetScale: frame={}", static_cast<const void*>(styleFrame));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSupportExternalFonts(OrbisFontLib library, u32 fontMax, u32 formats) {
    LOG_INFO(Lib_Font, "external font support requested");
    LOG_DEBUG(Lib_Font,
              "external font support params:\n"
              " library={}\n"
              " font_max={}\n"
              " formats_mask=0x{:X}\n",
              static_cast<const void*>(library), fontMax, formats);
    auto& ls = GetLibState(library);
    ls.support_external = true;
    ls.external_fontMax = fontMax;
    ls.external_formats = formats;
    LogExternalFormatSupport(formats);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSupportGlyphs() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSupportSystemFonts(OrbisFontLib library) {
    LOG_INFO(Lib_Font, "system font support requested");
    LOG_DEBUG(Lib_Font,
              "system font support params:\n"
              " library={}\n",
              static_cast<const void*>(library));

    if (!library) {
        return ORBIS_FONT_ERROR_INVALID_LIBRARY;
    }
    auto* lib = static_cast<FontLibOpaque*>(library);
    if (lib->magic != 0x0F01) {
        return ORBIS_FONT_ERROR_INVALID_LIBRARY;
    }

    auto& ls = GetLibState(library);
    if (ls.support_system) {
        return ORBIS_FONT_ERROR_ALREADY_SPECIFIED;
    }

    // Real implementation allocates a small system-font context; we just mark it available.
    lib->sysfonts_ctx = lib->sysfonts_ctx ? lib->sysfonts_ctx : &g_sysfonts_ctx_stub;
    ls.support_system = true;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontTextCodesStepBack() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontTextCodesStepNext() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontTextSourceInit() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontTextSourceRewind() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontTextSourceSetDefaultFont() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontTextSourceSetWritingForm() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontUnbindRenderer(OrbisFontHandle fontHandle) {
    LOG_DEBUG(Lib_Font, "sceFontUnbindRenderer fontHandle={}",
              static_cast<const void*>(fontHandle));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontWordsFindWordCharacters() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontWritingGetRenderMetrics() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontWritingInit() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontWritingLineClear() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontWritingLineGetOrderingSpace() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontWritingLineGetRenderMetrics() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontWritingLineRefersRenderStep() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontWritingLineWritesOrder() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontWritingRefersRenderStep() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontWritingRefersRenderStepCharacter() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontWritingSetMaskInvisible() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_00F4D778F1C88CB3() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_03C650025FBB0DE7() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_07EAB8A163B27E1A() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_09408E88E4F97CE3() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_09F92905ED82A814() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_0D142CEE1AB21ABE() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_14BD2E9E119C16F2() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_1AC53C9EDEAE8D75() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_1D401185D5E24C3D() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_1E83CD20C2CC996F() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_314B1F765B9FE78A() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_350E6725FEDE29E1() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_3DB773F0A604BF39() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_4FF49DD21E311B1C() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_526287664A493981() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_55CA718DBC84A6E9() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_563FC5F0706A8B4D() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_569E2ECD34290F45() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_5A04775B6BE47685() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_5FD93BCAB6F79750() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_62B5398F864BD3B4() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_6F9010294D822367() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_7757E947423A7A67() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_7E06BA52077F54FA() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_93B36DEA021311D6() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_94B0891E7111598A() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_9785C9128C2FE7CD() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_97DFBC9B65FBC0E1() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_ACD9717405D7D3CA() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_B19A8AEC3FD4F16F() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_C10F488AD7CF103D() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_D0C8B5FF4A6826C7() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_E48D3CD01C342A33() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_EAC96B2186B71E14() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_FE4788A96EF46256() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_FE7E5AE95D3058F5() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterlibSceFont(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("CUKn5pX-NVY", "libSceFont", 1, "libSceFont", sceFontAttachDeviceCacheBuffer);
    LIB_FUNCTION("3OdRkSjOcog", "libSceFont", 1, "libSceFont", sceFontBindRenderer);
    LIB_FUNCTION("6DFUkCwQLa8", "libSceFont", 1, "libSceFont", sceFontCharacterGetBidiLevel);
    LIB_FUNCTION("coCrV6IWplE", "libSceFont", 1, "libSceFont",
                 sceFontCharacterGetSyllableStringState);
    LIB_FUNCTION("zN3+nuA0SFQ", "libSceFont", 1, "libSceFont", sceFontCharacterGetTextFontCode);
    LIB_FUNCTION("mxgmMj-Mq-o", "libSceFont", 1, "libSceFont", sceFontCharacterGetTextOrder);
    LIB_FUNCTION("-P6X35Rq2-E", "libSceFont", 1, "libSceFont",
                 sceFontCharacterLooksFormatCharacters);
    LIB_FUNCTION("SaRlqtqaCew", "libSceFont", 1, "libSceFont", sceFontCharacterLooksWhiteSpace);
    LIB_FUNCTION("6Gqlv5KdTbU", "libSceFont", 1, "libSceFont", sceFontCharacterRefersTextBack);
    LIB_FUNCTION("BkjBP+YC19w", "libSceFont", 1, "libSceFont", sceFontCharacterRefersTextNext);
    LIB_FUNCTION("lVSR5ftvNag", "libSceFont", 1, "libSceFont", sceFontCharactersRefersTextCodes);
    LIB_FUNCTION("I9R5VC6eZWo", "libSceFont", 1, "libSceFont", sceFontClearDeviceCache);
    LIB_FUNCTION("vzHs3C8lWJk", "libSceFont", 1, "libSceFont", sceFontCloseFont);
    LIB_FUNCTION("MpKSBaYKluo", "libSceFont", 1, "libSceFont", sceFontControl);
    LIB_FUNCTION("WBNBaj9XiJU", "libSceFont", 1, "libSceFont", sceFontCreateGraphicsDevice);
    LIB_FUNCTION("4So0MC3oBIM", "libSceFont", 1, "libSceFont", sceFontCreateGraphicsService);
    LIB_FUNCTION("NlO5Qlhjkng", "libSceFont", 1, "libSceFont",
                 sceFontCreateGraphicsServiceWithEdition);
    LIB_FUNCTION("nWrfPI4Okmg", "libSceFont", 1, "libSceFont", sceFontCreateLibrary);
    LIB_FUNCTION("n590hj5Oe-k", "libSceFont", 1, "libSceFont", sceFontCreateLibraryWithEdition);
    LIB_FUNCTION("u5fZd3KZcs0", "libSceFont", 1, "libSceFont", sceFontCreateRenderer);
    LIB_FUNCTION("WaSFJoRWXaI", "libSceFont", 1, "libSceFont", sceFontCreateRendererWithEdition);
    LIB_FUNCTION("MO24vDhmS4E", "libSceFont", 1, "libSceFont", sceFontCreateString);
    LIB_FUNCTION("cYrMGk1wrMA", "libSceFont", 1, "libSceFont", sceFontCreateWords);
    LIB_FUNCTION("7rogx92EEyc", "libSceFont", 1, "libSceFont", sceFontCreateWritingLine);
    LIB_FUNCTION("8h-SOB-asgk", "libSceFont", 1, "libSceFont", sceFontDefineAttribute);
    LIB_FUNCTION("LHDoRWVFGqk", "libSceFont", 1, "libSceFont", sceFontDeleteGlyph);
    LIB_FUNCTION("5QG71IjgOpQ", "libSceFont", 1, "libSceFont", sceFontDestroyGraphicsDevice);
    LIB_FUNCTION("zZQD3EwJo3c", "libSceFont", 1, "libSceFont", sceFontDestroyGraphicsService);
    LIB_FUNCTION("FXP359ygujs", "libSceFont", 1, "libSceFont", sceFontDestroyLibrary);
    LIB_FUNCTION("exAxkyVLt0s", "libSceFont", 1, "libSceFont", sceFontDestroyRenderer);
    LIB_FUNCTION("SSCaczu2aMQ", "libSceFont", 1, "libSceFont", sceFontDestroyString);
    LIB_FUNCTION("hWE4AwNixqY", "libSceFont", 1, "libSceFont", sceFontDestroyWords);
    LIB_FUNCTION("PEjv7CVDRYs", "libSceFont", 1, "libSceFont", sceFontDestroyWritingLine);
    LIB_FUNCTION("UuY-OJF+f0k", "libSceFont", 1, "libSceFont", sceFontDettachDeviceCacheBuffer);
    LIB_FUNCTION("C-4Qw5Srlyw", "libSceFont", 1, "libSceFont", sceFontGenerateCharGlyph);
    LIB_FUNCTION("5kx49CAlO-M", "libSceFont", 1, "libSceFont", sceFontGetAttribute);
    LIB_FUNCTION("OINC0X9HGBY", "libSceFont", 1, "libSceFont", sceFontGetCharGlyphCode);
    LIB_FUNCTION("L97d+3OgMlE", "libSceFont", 1, "libSceFont", sceFontGetCharGlyphMetrics);
    LIB_FUNCTION("ynSqYL8VpoA", "libSceFont", 1, "libSceFont", sceFontGetEffectSlant);
    LIB_FUNCTION("d7dDgRY+Bzw", "libSceFont", 1, "libSceFont", sceFontGetEffectWeight);
    LIB_FUNCTION("ZB8xRemRRG8", "libSceFont", 1, "libSceFont", sceFontGetFontGlyphsCount);
    LIB_FUNCTION("4X14YSK4Ldk", "libSceFont", 1, "libSceFont", sceFontGetFontGlyphsOutlineProfile);
    LIB_FUNCTION("eb9S3zNlV5o", "libSceFont", 1, "libSceFont", sceFontGetFontMetrics);
    LIB_FUNCTION("tiIlroGki+g", "libSceFont", 1, "libSceFont", sceFontGetFontResolution);
    LIB_FUNCTION("3hVv3SNoL6E", "libSceFont", 1, "libSceFont", sceFontGetFontStyleInformation);
    LIB_FUNCTION("gVQpMBuB7fE", "libSceFont", 1, "libSceFont", sceFontGetGlyphExpandBufferState);
    LIB_FUNCTION("imxVx8lm+KM", "libSceFont", 1, "libSceFont", sceFontGetHorizontalLayout);
    LIB_FUNCTION("sDuhHGNhHvE", "libSceFont", 1, "libSceFont", sceFontGetKerning);
    LIB_FUNCTION("LzmHDnlcwfQ", "libSceFont", 1, "libSceFont", sceFontGetLibrary);
    LIB_FUNCTION("BozJej5T6fs", "libSceFont", 1, "libSceFont", sceFontGetPixelResolution);
    LIB_FUNCTION("IQtleGLL5pQ", "libSceFont", 1, "libSceFont", sceFontGetRenderCharGlyphMetrics);
    LIB_FUNCTION("Gqa5Pp7y4MU", "libSceFont", 1, "libSceFont", sceFontGetRenderEffectSlant);
    LIB_FUNCTION("woOjHrkjIYg", "libSceFont", 1, "libSceFont", sceFontGetRenderEffectWeight);
    LIB_FUNCTION("ryPlnDDI3rU", "libSceFont", 1, "libSceFont", sceFontGetRenderScaledKerning);
    LIB_FUNCTION("EY38A01lq2k", "libSceFont", 1, "libSceFont", sceFontGetRenderScalePixel);
    LIB_FUNCTION("FEafYUcxEGo", "libSceFont", 1, "libSceFont", sceFontGetRenderScalePoint);
    LIB_FUNCTION("8REoLjNGCpM", "libSceFont", 1, "libSceFont", sceFontGetResolutionDpi);
    LIB_FUNCTION("CkVmLoCNN-8", "libSceFont", 1, "libSceFont", sceFontGetScalePixel);
    LIB_FUNCTION("GoF2bhB7LYk", "libSceFont", 1, "libSceFont", sceFontGetScalePoint);
    LIB_FUNCTION("IrXeG0Lc6nA", "libSceFont", 1, "libSceFont", sceFontGetScriptLanguage);
    LIB_FUNCTION("7-miUT6pNQw", "libSceFont", 1, "libSceFont", sceFontGetTypographicDesign);
    LIB_FUNCTION("3BrWWFU+4ts", "libSceFont", 1, "libSceFont", sceFontGetVerticalLayout);
    LIB_FUNCTION("8-zmgsxkBek", "libSceFont", 1, "libSceFont", sceFontGlyphDefineAttribute);
    LIB_FUNCTION("oO33Uex4Ui0", "libSceFont", 1, "libSceFont", sceFontGlyphGetAttribute);
    LIB_FUNCTION("PXlA0M8ax40", "libSceFont", 1, "libSceFont", sceFontGlyphGetGlyphForm);
    LIB_FUNCTION("XUfSWpLhrUw", "libSceFont", 1, "libSceFont", sceFontGlyphGetMetricsForm);
    LIB_FUNCTION("lNnUqa1zA-M", "libSceFont", 1, "libSceFont", sceFontGlyphGetScalePixel);
    LIB_FUNCTION("ntrc3bEWlvQ", "libSceFont", 1, "libSceFont", sceFontGlyphRefersMetrics);
    LIB_FUNCTION("9kTbF59TjLs", "libSceFont", 1, "libSceFont", sceFontGlyphRefersMetricsHorizontal);
    LIB_FUNCTION("nJavPEdMDvM", "libSceFont", 1, "libSceFont",
                 sceFontGlyphRefersMetricsHorizontalAdvance);
    LIB_FUNCTION("JCnVgZgcucs", "libSceFont", 1, "libSceFont",
                 sceFontGlyphRefersMetricsHorizontalX);
    LIB_FUNCTION("R1T4i+DOhNY", "libSceFont", 1, "libSceFont", sceFontGlyphRefersOutline);
    LIB_FUNCTION("RmkXfBcZnrM", "libSceFont", 1, "libSceFont", sceFontGlyphRenderImage);
    LIB_FUNCTION("r4KEihtwxGs", "libSceFont", 1, "libSceFont", sceFontGlyphRenderImageHorizontal);
    LIB_FUNCTION("n22d-HIdmMg", "libSceFont", 1, "libSceFont", sceFontGlyphRenderImageVertical);
    LIB_FUNCTION("RL2cAQgyXR8", "libSceFont", 1, "libSceFont", sceFontGraphicsBeginFrame);
    LIB_FUNCTION("dUmIK6QjT7E", "libSceFont", 1, "libSceFont", sceFontGraphicsDrawingCancel);
    LIB_FUNCTION("X2Vl3yU19Zw", "libSceFont", 1, "libSceFont", sceFontGraphicsDrawingFinish);
    LIB_FUNCTION("DOmdOwV3Aqw", "libSceFont", 1, "libSceFont", sceFontGraphicsEndFrame);
    LIB_FUNCTION("zdYdKRQC3rw", "libSceFont", 1, "libSceFont", sceFontGraphicsExchangeResource);
    LIB_FUNCTION("UkMUIoj-e9s", "libSceFont", 1, "libSceFont", sceFontGraphicsFillMethodInit);
    LIB_FUNCTION("DJURdcnVUqo", "libSceFont", 1, "libSceFont", sceFontGraphicsFillPlotInit);
    LIB_FUNCTION("eQac6ftmBQQ", "libSceFont", 1, "libSceFont", sceFontGraphicsFillPlotSetLayout);
    LIB_FUNCTION("PEYQJa+MWnk", "libSceFont", 1, "libSceFont", sceFontGraphicsFillPlotSetMapping);
    LIB_FUNCTION("21g4m4kYF6g", "libSceFont", 1, "libSceFont", sceFontGraphicsFillRatesInit);
    LIB_FUNCTION("pJzji5FvdxU", "libSceFont", 1, "libSceFont",
                 sceFontGraphicsFillRatesSetFillEffect);
    LIB_FUNCTION("scaro-xEuUM", "libSceFont", 1, "libSceFont", sceFontGraphicsFillRatesSetLayout);
    LIB_FUNCTION("W66Kqtt0xU0", "libSceFont", 1, "libSceFont", sceFontGraphicsFillRatesSetMapping);
    LIB_FUNCTION("FzpLsBQEegQ", "libSceFont", 1, "libSceFont", sceFontGraphicsGetDeviceUsage);
    LIB_FUNCTION("W80hs0g5d+E", "libSceFont", 1, "libSceFont", sceFontGraphicsRegionInit);
    LIB_FUNCTION("S48+njg9p-o", "libSceFont", 1, "libSceFont", sceFontGraphicsRegionInitCircular);
    LIB_FUNCTION("wcOQ8Fz73+M", "libSceFont", 1, "libSceFont", sceFontGraphicsRegionInitRoundish);
    LIB_FUNCTION("YBaw2Yyfd5E", "libSceFont", 1, "libSceFont", sceFontGraphicsRelease);
    LIB_FUNCTION("qkySrQ4FGe0", "libSceFont", 1, "libSceFont", sceFontGraphicsRenderResource);
    LIB_FUNCTION("qzNjJYKVli0", "libSceFont", 1, "libSceFont", sceFontGraphicsSetFramePolicy);
    LIB_FUNCTION("9iRbHCtcx-o", "libSceFont", 1, "libSceFont", sceFontGraphicsSetupClipping);
    LIB_FUNCTION("KZ3qPyz5Opc", "libSceFont", 1, "libSceFont", sceFontGraphicsSetupColorRates);
    LIB_FUNCTION("LqclbpVzRvM", "libSceFont", 1, "libSceFont", sceFontGraphicsSetupFillMethod);
    LIB_FUNCTION("Wl4FiI4qKY0", "libSceFont", 1, "libSceFont", sceFontGraphicsSetupFillRates);
    LIB_FUNCTION("WC7s95TccVo", "libSceFont", 1, "libSceFont", sceFontGraphicsSetupGlyphFill);
    LIB_FUNCTION("zC6I4ty37NA", "libSceFont", 1, "libSceFont", sceFontGraphicsSetupGlyphFillPlot);
    LIB_FUNCTION("drZUF0XKTEI", "libSceFont", 1, "libSceFont", sceFontGraphicsSetupHandleDefault);
    LIB_FUNCTION("MEAmHMynQXE", "libSceFont", 1, "libSceFont", sceFontGraphicsSetupLocation);
    LIB_FUNCTION("XRUOmQhnYO4", "libSceFont", 1, "libSceFont", sceFontGraphicsSetupPositioning);
    LIB_FUNCTION("98XGr2Bkklg", "libSceFont", 1, "libSceFont", sceFontGraphicsSetupRotation);
    LIB_FUNCTION("Nj-ZUVOVAvc", "libSceFont", 1, "libSceFont", sceFontGraphicsSetupScaling);
    LIB_FUNCTION("p0avT2ggev0", "libSceFont", 1, "libSceFont", sceFontGraphicsSetupShapeFill);
    LIB_FUNCTION("0C5aKg9KghY", "libSceFont", 1, "libSceFont", sceFontGraphicsSetupShapeFillPlot);
    LIB_FUNCTION("4pA3qqAcYco", "libSceFont", 1, "libSceFont", sceFontGraphicsStructureCanvas);
    LIB_FUNCTION("cpjgdlMYdOM", "libSceFont", 1, "libSceFont",
                 sceFontGraphicsStructureCanvasSequence);
    LIB_FUNCTION("774Mee21wKk", "libSceFont", 1, "libSceFont", sceFontGraphicsStructureDesign);
    LIB_FUNCTION("Hp3NIFhUXvQ", "libSceFont", 1, "libSceFont",
                 sceFontGraphicsStructureDesignResource);
    LIB_FUNCTION("bhmZlml6NBs", "libSceFont", 1, "libSceFont",
                 sceFontGraphicsStructureSurfaceTexture);
    LIB_FUNCTION("5sAWgysOBfE", "libSceFont", 1, "libSceFont", sceFontGraphicsUpdateClipping);
    LIB_FUNCTION("W4e8obm+w6o", "libSceFont", 1, "libSceFont", sceFontGraphicsUpdateColorRates);
    LIB_FUNCTION("EgIn3QBajPs", "libSceFont", 1, "libSceFont", sceFontGraphicsUpdateFillMethod);
    LIB_FUNCTION("MnUYAs2jVuU", "libSceFont", 1, "libSceFont", sceFontGraphicsUpdateFillRates);
    LIB_FUNCTION("R-oVDMusYbc", "libSceFont", 1, "libSceFont", sceFontGraphicsUpdateGlyphFill);
    LIB_FUNCTION("b9R+HQuHSMI", "libSceFont", 1, "libSceFont", sceFontGraphicsUpdateGlyphFillPlot);
    LIB_FUNCTION("IN4P5pJADQY", "libSceFont", 1, "libSceFont", sceFontGraphicsUpdateLocation);
    LIB_FUNCTION("U+LLXdr2DxM", "libSceFont", 1, "libSceFont", sceFontGraphicsUpdatePositioning);
    LIB_FUNCTION("yStTYSeb4NM", "libSceFont", 1, "libSceFont", sceFontGraphicsUpdateRotation);
    LIB_FUNCTION("eDxmMoxE5xU", "libSceFont", 1, "libSceFont", sceFontGraphicsUpdateScaling);
    LIB_FUNCTION("Ax6LQJJq6HQ", "libSceFont", 1, "libSceFont", sceFontGraphicsUpdateShapeFill);
    LIB_FUNCTION("I5Rf2rXvBKQ", "libSceFont", 1, "libSceFont", sceFontGraphicsUpdateShapeFillPlot);
    LIB_FUNCTION("whrS4oksXc4", "libSceFont", 1, "libSceFont", sceFontMemoryInit);
    LIB_FUNCTION("h6hIgxXEiEc", "libSceFont", 1, "libSceFont", sceFontMemoryTerm);
    LIB_FUNCTION("RvXyHMUiLhE", "libSceFont", 1, "libSceFont", sceFontOpenFontFile);
    LIB_FUNCTION("JzCH3SCFnAU", "libSceFont", 1, "libSceFont", sceFontOpenFontInstance);
    LIB_FUNCTION("KXUpebrFk1U", "libSceFont", 1, "libSceFont", sceFontOpenFontMemory);
    LIB_FUNCTION("cKYtVmeSTcw", "libSceFont", 1, "libSceFont", sceFontOpenFontSet);
    LIB_FUNCTION("Z2cdsqJH+5k", "libSceFont", 1, "libSceFont", sceFontRebindRenderer);
    LIB_FUNCTION("3G4zhgKuxE8", "libSceFont", 1, "libSceFont", sceFontRenderCharGlyphImage);
    LIB_FUNCTION("kAenWy1Zw5o", "libSceFont", 1, "libSceFont",
                 sceFontRenderCharGlyphImageHorizontal);
    LIB_FUNCTION("i6UNdSig1uE", "libSceFont", 1, "libSceFont", sceFontRenderCharGlyphImageVertical);
    LIB_FUNCTION("amcmrY62BD4", "libSceFont", 1, "libSceFont", sceFontRendererGetOutlineBufferSize);
    LIB_FUNCTION("ai6AfGrBs4o", "libSceFont", 1, "libSceFont", sceFontRendererResetOutlineBuffer);
    LIB_FUNCTION("ydF+WuH0fAk", "libSceFont", 1, "libSceFont",
                 sceFontRendererSetOutlineBufferPolicy);
    LIB_FUNCTION("gdUCnU0gHdI", "libSceFont", 1, "libSceFont", sceFontRenderSurfaceInit);
    LIB_FUNCTION("vRxf4d0ulPs", "libSceFont", 1, "libSceFont", sceFontRenderSurfaceSetScissor);
    LIB_FUNCTION("0hr-w30SjiI", "libSceFont", 1, "libSceFont", sceFontRenderSurfaceSetStyleFrame);
    LIB_FUNCTION("TMtqoFQjjbA", "libSceFont", 1, "libSceFont", sceFontSetEffectSlant);
    LIB_FUNCTION("v0phZwa4R5o", "libSceFont", 1, "libSceFont", sceFontSetEffectWeight);
    LIB_FUNCTION("kihFGYJee7o", "libSceFont", 1, "libSceFont", sceFontSetFontsOpenMode);
    LIB_FUNCTION("I1acwR7Qp8E", "libSceFont", 1, "libSceFont", sceFontSetResolutionDpi);
    LIB_FUNCTION("N1EBMeGhf7E", "libSceFont", 1, "libSceFont", sceFontSetScalePixel);
    LIB_FUNCTION("sw65+7wXCKE", "libSceFont", 1, "libSceFont", sceFontSetScalePoint);
    LIB_FUNCTION("PxSR9UfJ+SQ", "libSceFont", 1, "libSceFont", sceFontSetScriptLanguage);
    LIB_FUNCTION("SnsZua35ngs", "libSceFont", 1, "libSceFont", sceFontSetTypographicDesign);
    LIB_FUNCTION("lz9y9UFO2UU", "libSceFont", 1, "libSceFont", sceFontSetupRenderEffectSlant);
    LIB_FUNCTION("XIGorvLusDQ", "libSceFont", 1, "libSceFont", sceFontSetupRenderEffectWeight);
    LIB_FUNCTION("6vGCkkQJOcI", "libSceFont", 1, "libSceFont", sceFontSetupRenderScalePixel);
    LIB_FUNCTION("nMZid4oDfi4", "libSceFont", 1, "libSceFont", sceFontSetupRenderScalePoint);
    LIB_FUNCTION("ObkDGDBsVtw", "libSceFont", 1, "libSceFont", sceFontStringGetTerminateCode);
    LIB_FUNCTION("+B-xlbiWDJ4", "libSceFont", 1, "libSceFont", sceFontStringGetTerminateOrder);
    LIB_FUNCTION("o1vIEHeb6tw", "libSceFont", 1, "libSceFont", sceFontStringGetWritingForm);
    LIB_FUNCTION("hq5LffQjz-s", "libSceFont", 1, "libSceFont", sceFontStringRefersRenderCharacters);
    LIB_FUNCTION("Avv7OApgCJk", "libSceFont", 1, "libSceFont", sceFontStringRefersTextCharacters);
    LIB_FUNCTION("lOfduYnjgbo", "libSceFont", 1, "libSceFont", sceFontStyleFrameGetEffectSlant);
    LIB_FUNCTION("HIUdjR-+Wl8", "libSceFont", 1, "libSceFont", sceFontStyleFrameGetEffectWeight);
    LIB_FUNCTION("VSw18Aqzl0U", "libSceFont", 1, "libSceFont", sceFontStyleFrameGetResolutionDpi);
    LIB_FUNCTION("2QfqfeLblbg", "libSceFont", 1, "libSceFont", sceFontStyleFrameGetScalePixel);
    LIB_FUNCTION("7x2xKiiB7MA", "libSceFont", 1, "libSceFont", sceFontStyleFrameGetScalePoint);
    LIB_FUNCTION("la2AOWnHEAc", "libSceFont", 1, "libSceFont", sceFontStyleFrameInit);
    LIB_FUNCTION("394sckksiCU", "libSceFont", 1, "libSceFont", sceFontStyleFrameSetEffectSlant);
    LIB_FUNCTION("faw77-pEBmU", "libSceFont", 1, "libSceFont", sceFontStyleFrameSetEffectWeight);
    LIB_FUNCTION("dB4-3Wdwls8", "libSceFont", 1, "libSceFont", sceFontStyleFrameSetResolutionDpi);
    LIB_FUNCTION("da4rQ4-+p-4", "libSceFont", 1, "libSceFont", sceFontStyleFrameSetScalePixel);
    LIB_FUNCTION("O997laxY-Ys", "libSceFont", 1, "libSceFont", sceFontStyleFrameSetScalePoint);
    LIB_FUNCTION("dUmABkAnVgk", "libSceFont", 1, "libSceFont", sceFontStyleFrameUnsetEffectSlant);
    LIB_FUNCTION("hwsuXgmKdaw", "libSceFont", 1, "libSceFont", sceFontStyleFrameUnsetEffectWeight);
    LIB_FUNCTION("bePC0L0vQWY", "libSceFont", 1, "libSceFont", sceFontStyleFrameUnsetScale);
    LIB_FUNCTION("mz2iTY0MK4A", "libSceFont", 1, "libSceFont", sceFontSupportExternalFonts);
    LIB_FUNCTION("71w5DzObuZI", "libSceFont", 1, "libSceFont", sceFontSupportGlyphs);
    LIB_FUNCTION("SsRbbCiWoGw", "libSceFont", 1, "libSceFont", sceFontSupportSystemFonts);
    LIB_FUNCTION("IPoYwwlMx-g", "libSceFont", 1, "libSceFont", sceFontTextCodesStepBack);
    LIB_FUNCTION("olSmXY+XP1E", "libSceFont", 1, "libSceFont", sceFontTextCodesStepNext);
    LIB_FUNCTION("oaJ1BpN2FQk", "libSceFont", 1, "libSceFont", sceFontTextSourceInit);
    LIB_FUNCTION("VRFd3diReec", "libSceFont", 1, "libSceFont", sceFontTextSourceRewind);
    LIB_FUNCTION("eCRMCSk96NU", "libSceFont", 1, "libSceFont", sceFontTextSourceSetDefaultFont);
    LIB_FUNCTION("OqQKX0h5COw", "libSceFont", 1, "libSceFont", sceFontTextSourceSetWritingForm);
    LIB_FUNCTION("1QjhKxrsOB8", "libSceFont", 1, "libSceFont", sceFontUnbindRenderer);
    LIB_FUNCTION("H-FNq8isKE0", "libSceFont", 1, "libSceFont", sceFontWordsFindWordCharacters);
    LIB_FUNCTION("fljdejMcG1c", "libSceFont", 1, "libSceFont", sceFontWritingGetRenderMetrics);
    LIB_FUNCTION("fD5rqhEXKYQ", "libSceFont", 1, "libSceFont", sceFontWritingInit);
    LIB_FUNCTION("1+DgKL0haWQ", "libSceFont", 1, "libSceFont", sceFontWritingLineClear);
    LIB_FUNCTION("JQKWIsS9joE", "libSceFont", 1, "libSceFont", sceFontWritingLineGetOrderingSpace);
    LIB_FUNCTION("nlU2VnfpqTM", "libSceFont", 1, "libSceFont", sceFontWritingLineGetRenderMetrics);
    LIB_FUNCTION("+FYcYefsVX0", "libSceFont", 1, "libSceFont", sceFontWritingLineRefersRenderStep);
    LIB_FUNCTION("wyKFUOWdu3Q", "libSceFont", 1, "libSceFont", sceFontWritingLineWritesOrder);
    LIB_FUNCTION("W-2WOXEHGck", "libSceFont", 1, "libSceFont", sceFontWritingRefersRenderStep);
    LIB_FUNCTION("f4Onl7efPEY", "libSceFont", 1, "libSceFont",
                 sceFontWritingRefersRenderStepCharacter);
    LIB_FUNCTION("BbCZjJizU4A", "libSceFont", 1, "libSceFont", sceFontWritingSetMaskInvisible);
    LIB_FUNCTION("APTXePHIjLM", "libSceFont", 1, "libSceFont", Func_00F4D778F1C88CB3);
    LIB_FUNCTION("A8ZQAl+7Dec", "libSceFont", 1, "libSceFont", Func_03C650025FBB0DE7);
    LIB_FUNCTION("B+q4oWOyfho", "libSceFont", 1, "libSceFont", Func_07EAB8A163B27E1A);
    LIB_FUNCTION("CUCOiOT5fOM", "libSceFont", 1, "libSceFont", Func_09408E88E4F97CE3);
    LIB_FUNCTION("CfkpBe2CqBQ", "libSceFont", 1, "libSceFont", Func_09F92905ED82A814);
    LIB_FUNCTION("DRQs7hqyGr4", "libSceFont", 1, "libSceFont", Func_0D142CEE1AB21ABE);
    LIB_FUNCTION("FL0unhGcFvI", "libSceFont", 1, "libSceFont", Func_14BD2E9E119C16F2);
    LIB_FUNCTION("GsU8nt6ujXU", "libSceFont", 1, "libSceFont", Func_1AC53C9EDEAE8D75);
    LIB_FUNCTION("HUARhdXiTD0", "libSceFont", 1, "libSceFont", Func_1D401185D5E24C3D);
    LIB_FUNCTION("HoPNIMLMmW8", "libSceFont", 1, "libSceFont", Func_1E83CD20C2CC996F);
    LIB_FUNCTION("MUsfdluf54o", "libSceFont", 1, "libSceFont", Func_314B1F765B9FE78A);
    LIB_FUNCTION("NQ5nJf7eKeE", "libSceFont", 1, "libSceFont", Func_350E6725FEDE29E1);
    LIB_FUNCTION("Pbdz8KYEvzk", "libSceFont", 1, "libSceFont", Func_3DB773F0A604BF39);
    LIB_FUNCTION("T-Sd0h4xGxw", "libSceFont", 1, "libSceFont", Func_4FF49DD21E311B1C);
    LIB_FUNCTION("UmKHZkpJOYE", "libSceFont", 1, "libSceFont", Func_526287664A493981);
    LIB_FUNCTION("VcpxjbyEpuk", "libSceFont", 1, "libSceFont", Func_55CA718DBC84A6E9);
    LIB_FUNCTION("Vj-F8HBqi00", "libSceFont", 1, "libSceFont", Func_563FC5F0706A8B4D);
    LIB_FUNCTION("Vp4uzTQpD0U", "libSceFont", 1, "libSceFont", Func_569E2ECD34290F45);
    LIB_FUNCTION("WgR3W2vkdoU", "libSceFont", 1, "libSceFont", Func_5A04775B6BE47685);
    LIB_FUNCTION("X9k7yrb3l1A", "libSceFont", 1, "libSceFont", Func_5FD93BCAB6F79750);
    LIB_FUNCTION("YrU5j4ZL07Q", "libSceFont", 1, "libSceFont", Func_62B5398F864BD3B4);
    LIB_FUNCTION("b5AQKU2CI2c", "libSceFont", 1, "libSceFont", Func_6F9010294D822367);
    LIB_FUNCTION("d1fpR0I6emc", "libSceFont", 1, "libSceFont", Func_7757E947423A7A67);
    LIB_FUNCTION("fga6Ugd-VPo", "libSceFont", 1, "libSceFont", Func_7E06BA52077F54FA);
    LIB_FUNCTION("k7Nt6gITEdY", "libSceFont", 1, "libSceFont", Func_93B36DEA021311D6);
    LIB_FUNCTION("lLCJHnERWYo", "libSceFont", 1, "libSceFont", Func_94B0891E7111598A);
    LIB_FUNCTION("l4XJEowv580", "libSceFont", 1, "libSceFont", Func_9785C9128C2FE7CD);
    LIB_FUNCTION("l9+8m2X7wOE", "libSceFont", 1, "libSceFont", Func_97DFBC9B65FBC0E1);
    LIB_FUNCTION("rNlxdAXX08o", "libSceFont", 1, "libSceFont", Func_ACD9717405D7D3CA);
    LIB_FUNCTION("sZqK7D-U8W8", "libSceFont", 1, "libSceFont", Func_B19A8AEC3FD4F16F);
    LIB_FUNCTION("wQ9IitfPED0", "libSceFont", 1, "libSceFont", Func_C10F488AD7CF103D);
    LIB_FUNCTION("0Mi1-0poJsc", "libSceFont", 1, "libSceFont", Func_D0C8B5FF4A6826C7);
    LIB_FUNCTION("5I080Bw0KjM", "libSceFont", 1, "libSceFont", Func_E48D3CD01C342A33);
    LIB_FUNCTION("6slrIYa3HhQ", "libSceFont", 1, "libSceFont", Func_EAC96B2186B71E14);
    LIB_FUNCTION("-keIqW70YlY", "libSceFont", 1, "libSceFont", Func_FE4788A96EF46256);
    LIB_FUNCTION("-n5a6V0wWPU", "libSceFont", 1, "libSceFont", Func_FE7E5AE95D3058F5);
};

} // namespace Libraries::Font
