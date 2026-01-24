// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <atomic>
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
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <fmt/format.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H
#include "common/config.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/file_sys/fs.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/font/font.h"
#include "core/libraries/font/font_internal.h"
#include "core/libraries/font/fontft.h"
#include "core/libraries/font/fontft_internal.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/process.h"
#include "core/libraries/kernel/time.h"
#include "core/libraries/libs.h"
#include "core/memory.h"
#include "core/tls.h"
#include "font_error.h"

#ifdef formatParams
#undef formatParams
#endif

namespace {
using Libraries::Font::OrbisFontGenerateGlyphParams;
using Libraries::Font::OrbisFontGlyph;
using Libraries::Font::OrbisFontGlyphMetrics;
using Libraries::Font::OrbisFontGlyphOpaque;
using Libraries::Font::OrbisFontGlyphOutline;
using Libraries::Font::OrbisFontGlyphOutlinePoint;
using Libraries::Font::OrbisFontMem;
using Libraries::Font::OrbisFontStyleFrame;
namespace Internal = Libraries::Font::Internal;

struct NamedParam {
    std::string_view name;
    std::string value;
};

template <typename T>
static std::string DescribeValue(const T& value) {
    using Clean = std::remove_cvref_t<T>;
    if constexpr (std::is_same_v<Clean, bool>) {
        return value ? "true" : "false";
    } else if constexpr (std::is_pointer_v<Clean>) {
        return fmt::format("{}", reinterpret_cast<const void*>(value));
    } else if constexpr (std::is_floating_point_v<Clean>) {
        return fmt::format("{:.6g}", value);
    } else if constexpr (std::is_enum_v<Clean>) {
        using Underlying = std::underlying_type_t<Clean>;
        return DescribeValue(static_cast<Underlying>(value));
    } else {
        return fmt::format("{}", value);
    }
}

template <typename T>
static NamedParam Param(std::string_view name, const T& value) {
    return NamedParam{name, DescribeValue(value)};
}

static std::string formatParams(std::initializer_list<NamedParam> params) {
    fmt::memory_buffer buffer;
    fmt::format_to(std::back_inserter(buffer), "params:\n");
    for (const auto& p : params) {
        fmt::format_to(std::back_inserter(buffer), "{}: {}\n", p.name, p.value);
    }
    return fmt::to_string(buffer);
}

struct FtExternalFaceObj {
    u32 refcount;
    u32 reserved04;
    u32 reserved08;
    u32 sub_font_index;
    u64 reserved10;
    FtExternalFaceObj* next;
    u64 reserved20;
    u64 reserved28;
    FT_Face face;
    const u8* font_data;
    u32 font_size;
    u32 sfnt_base;
};
static_assert(offsetof(FtExternalFaceObj, sub_font_index) == 0x0C);
static_assert(offsetof(FtExternalFaceObj, next) == 0x18);
static_assert(offsetof(FtExternalFaceObj, face) == 0x30);

using FontAllocFn = void*(PS4_SYSV_ABI*)(void* object, u32 size);
using FontFreeFn = void(PS4_SYSV_ABI*)(void* object, void* p);

struct LibraryState {
    bool support_system = false;
    bool support_external = false;
    u32 external_formats = 0;
    u32 external_fontMax = 0;
    const Libraries::Font::OrbisFontMem* backing_memory = nullptr;
    Libraries::Font::OrbisFontLibCreateParams create_params = nullptr;
    u64 edition = 0;
    void* alloc_ctx = nullptr;
    FontAllocFn alloc_fn = nullptr;
    FontFreeFn free_fn = nullptr;
    void* owned_mspace = nullptr;
    u32 owned_mspace_size = 0;
    void* owned_sysfonts_ctx = nullptr;
    u32 owned_sysfonts_ctx_size = 0;
    void* owned_external_fonts_ctx = nullptr;
    u32 owned_external_fonts_ctx_size = 0;
    void* owned_device_cache = nullptr;
    u32 owned_device_cache_size = 0;
};
static std::unordered_map<Libraries::Font::OrbisFontLib, LibraryState> g_library_state;

static void LogFontOpenError(s32 rc) {
    switch (rc) {
    case ORBIS_FONT_ERROR_INVALID_LIBRARY:
        LOG_ERROR(Lib_Font, "INVALID_LIBRARY");
        break;
    case ORBIS_FONT_ERROR_INVALID_PARAMETER:
        LOG_ERROR(Lib_Font, "INVALID_PARAMETER");
        break;
    case ORBIS_FONT_ERROR_ALLOCATION_FAILED:
        LOG_ERROR(Lib_Font, "ALLOCATION_FAILED");
        break;
    case ORBIS_FONT_ERROR_FS_OPEN_FAILED:
        LOG_ERROR(Lib_Font, "FS_OPEN_FAILED");
        break;
    case ORBIS_FONT_ERROR_NO_SUPPORT_FUNCTION:
        LOG_ERROR(Lib_Font, "NO_SUPPORT_FUNCTION");
        break;
    case ORBIS_FONT_ERROR_NO_SUPPORT_FORMAT:
        LOG_ERROR(Lib_Font, "NO_SUPPORT_FORMAT");
        break;
    case ORBIS_FONT_ERROR_NO_SUPPORT_FONTSET:
        LOG_ERROR(Lib_Font, "NO_SUPPORT_FONTSET");
        break;
    case ORBIS_FONT_ERROR_FONT_OPEN_MAX:
        LOG_ERROR(Lib_Font, "FONT_OPEN_MAX");
        break;
    default:
        LOG_ERROR(Lib_Font, "FAILED");
        break;
    }
}

static u16 ClampToU16(float value) {
    if (value <= 0.0f) {
        return 0;
    }
    const float clamped = std::min(value, static_cast<float>(std::numeric_limits<u16>::max()));
    return static_cast<u16>(std::lround(clamped));
}

static LibraryState& GetLibState(Libraries::Font::OrbisFontLib lib) {
    return g_library_state[lib];
}

static void RemoveLibState(Libraries::Font::OrbisFontLib lib) {
    if (auto it = g_library_state.find(lib); it != g_library_state.end()) {
        const auto free_fn = it->second.free_fn;
        const auto alloc_ctx = it->second.alloc_ctx;
        auto free_owned = [&](void* p) {
            if (!p) {
                return;
            }
            if (free_fn) {
                using FreeFnGuest = void(PS4_SYSV_ABI*)(void* object, void* p);
                const auto free_fn_guest = reinterpret_cast<FreeFnGuest>(free_fn);
                Core::ExecuteGuest(free_fn_guest, alloc_ctx, p);
                return;
            }
            std::free(p);
        };
        free_owned(it->second.owned_device_cache);
        free_owned(it->second.owned_external_fonts_ctx);
        free_owned(it->second.owned_sysfonts_ctx);
        free_owned(it->second.owned_mspace);
        g_library_state.erase(it);
    }
}

static void LogExternalFormatSupport(u32 formats_mask) {
    LOG_INFO(Lib_Font, "ExternalFormatsMask=0x{:X}", formats_mask);
}

static bool LoadFontFile(const std::filesystem::path& path, std::vector<unsigned char>& out_bytes);

static std::optional<std::filesystem::path> ResolveKnownSysFontAlias(
    const std::filesystem::path& sysfonts_dir, std::string_view ps4_filename) {
    const auto resolve_existing =
        [&](std::string_view filename) -> std::optional<std::filesystem::path> {
        const std::filesystem::path file_path{std::string(filename)};
        std::error_code ec;
        {
            const auto candidate = sysfonts_dir / file_path;
            if (std::filesystem::exists(candidate, ec)) {
                return candidate;
            }
        }
        {
            const auto candidate = sysfonts_dir / "font" / file_path;
            if (std::filesystem::exists(candidate, ec)) {
                return candidate;
            }
        }
        {
            const auto candidate = sysfonts_dir / "font2" / file_path;
            if (std::filesystem::exists(candidate, ec)) {
                return candidate;
            }
        }
        return std::nullopt;
    };

    static constexpr std::array<std::pair<std::string_view, std::string_view>, 41> kAliases = {{
        {"SST-EU-ROMAN-L.OTF", "SST-Light.otf"},
        {"SST-EU-ROMAN.OTF", "SST-Roman.otf"},
        {"SST-EU-ROMAN-M.OTF", "SST-Medium.otf"},
        {"SST-EU-ROMAN-R.OTF", "SST-Roman.otf"},
        {"SST-EU-ROMAN-I.OTF", "SST-Italic.otf"},
        {"SST-EU-ROMAN-B.OTF", "SST-Bold.otf"},
        {"SST-EU-ROMAN-BI.OTF", "SST-BoldItalic.otf"},
        {"SST-ITALIC-L.OTF", "SST-LightItalic.otf"},
        {"SST-ITALIC-R.OTF", "SST-Italic.otf"},
        {"SST-ITALIC-M.OTF", "SST-MediumItalic.otf"},
        {"SST-ITALIC-B.OTF", "SST-BoldItalic.otf"},
        {"SST-TYPEWRITER-R.OTF", "SSTTypewriter-Roman.otf"},
        {"SST-TYPEWRITER-B.OTF", "SSTTypewriter-Bd.otf"},
        {"SST-JPPRO-R.OTF", "SSTJpPro-Regular.otf"},
        {"SST-JPPRO-B.OTF", "SSTJpPro-Bold.otf"},
        {"SST-CNGB-HEI-R.TTF", "DFHEI5-SONY.ttf"},
        {"SST-ARIB-STD-B24-R.TTF", "SSTAribStdB24-Regular.ttf"},
        {"SST-ARABIC-R.OTF", "SSTArabic-Roman.otf"},
        {"SST-ARABIC-L.OTF", "SSTArabic-Light.otf"},
        {"SST-ARABIC-M.OTF", "SSTArabic-Medium.otf"},
        {"SST-ARABIC-B.OTF", "SSTArabic-Bold.otf"},
        {"SCE-EXT-HANGUL-L.OTF", "SCEPS4Yoongd-Light.otf"},
        {"SCE-EXT-HANGUL-R.OTF", "SCEPS4Yoongd-Medium.otf"},
        {"SCE-EXT-HANGUL-B.OTF", "SCEPS4Yoongd-Bold.otf"},
        {"SSTCC-SERIF-MONO.OTF", "e046323ms.ttf"},
        {"SSTCC-SERIF.OTF", "e046323ts.ttf"},
        {"SSTCC-SANSSERIF-MONO.OTF", "n023055ms.ttf"},
        {"SSTCC-SANSSERIF.OTF", "n023055ts.ttf"},
        {"SSTCC-CUSUAL.OTF", "d013013ds.ttf"},
        {"SSTCC-CURSIVE.OTF", "k006004ds.ttf"},
        {"SSTCC-SMALLCAPITAL.OTF", "c041056ts.ttf"},
        {"SCE-JP-CATTLEYA-L.OTF", "SCE-RDC-R-JPN.otf"},
        {"SCE-JP-CATTLEYA-B.OTF", "SCE-RDC-B-JPN.otf"},
        {"SST-THAI-L.OTF", "SSTThai-Light.otf"},
        {"SST-THAI-R.OTF", "SSTThai-Roman.otf"},
        {"SST-THAI-M.OTF", "SSTThai-Medium.otf"},
        {"SST-THAI-B.OTF", "SSTThai-Bold.otf"},
        {"SST-VIETNAMESE-L.OTF", "SSTVietnamese-Light.otf"},
        {"SST-VIETNAMESE-R.OTF", "SSTVietnamese-Roman.otf"},
        {"SST-VIETNAMESE-M.OTF", "SSTVietnamese-Medium.otf"},
        {"SST-VIETNAMESE-B.OTF", "SSTVietnamese-Bold.otf"},
    }};

    for (const auto& [from, to] : kAliases) {
        if (ps4_filename == from) {
            return resolve_existing(to);
        }
        if (ps4_filename == to) {
            if (auto reverse = resolve_existing(from)) {
                return reverse;
            }
        }
    }
    return std::nullopt;
}

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
    return Internal::ResolveGuestPath(guest_path);
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

static constexpr float kPointsPerInch = 72.0f;
static constexpr float kScaleEpsilon = 1e-4f;
constexpr u16 kStyleFrameMagic = 0x0F09;

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

struct StyleFrameScaleState {
    float scale_w;
    float scale_h;
    u32 dpi_x;
    u32 dpi_y;
};

static StyleFrameScaleState ResolveStyleFrameScale(const OrbisFontStyleFrame* style,
                                                   float base_scale_w, float base_scale_h,
                                                   u32 base_dpi_x, u32 base_dpi_y) {
    StyleFrameScaleState resolved{
        .scale_w = base_scale_w,
        .scale_h = base_scale_h,
        .dpi_x = base_dpi_x,
        .dpi_y = base_dpi_y,
    };
    if (!style || style->magic != kStyleFrameMagic) {
        return resolved;
    }
    if (style->hDpi != 0) {
        resolved.dpi_x = style->hDpi;
    }
    if (style->vDpi != 0) {
        resolved.dpi_y = style->vDpi;
    }
    if ((style->flags1 & 1u) == 0) {
        return resolved;
    }

    const bool unit_is_pixel = (style->scaleUnit == 0);
    if (unit_is_pixel) {
        resolved.scale_w = style->scalePixelW;
        resolved.scale_h = style->scalePixelH;
    } else {
        const u32 dpi_x = style->hDpi;
        const u32 dpi_y = style->vDpi;
        resolved.scale_w = dpi_x ? PointsToPixels(style->scalePixelW, dpi_x) : style->scalePixelW;
        resolved.scale_h = dpi_y ? PointsToPixels(style->scalePixelH, dpi_y) : style->scalePixelH;
    }
    return resolved;
}

static bool ValidateStyleFramePtr(const OrbisFontStyleFrame* frame) {
    return frame && frame->magic == kStyleFrameMagic;
}

static inline u32 LoadU32(const void* base, std::size_t off) {
    u32 v = 0;
    std::memcpy(&v, static_cast<const u8*>(base) + off, sizeof(v));
    return v;
}
static inline float LoadF32(const void* base, std::size_t off) {
    float v = 0.0f;
    std::memcpy(&v, static_cast<const u8*>(base) + off, sizeof(v));
    return v;
}
static inline void StoreU32(void* base, std::size_t off, u32 v) {
    std::memcpy(static_cast<u8*>(base) + off, &v, sizeof(v));
}
static inline void StoreF32(void* base, std::size_t off, float v) {
    std::memcpy(static_cast<u8*>(base) + off, &v, sizeof(v));
}

static void ClearRenderOutputs(Libraries::Font::OrbisFontGlyphMetrics* metrics,
                               Libraries::Font::OrbisFontRenderOutput* result) {
    if (metrics) {
        *metrics = {};
    }
    if (result) {
        *result = {};
    }
}

struct SystemFontDefinition {
    u32 font_set_type;
    const char* config_key;
    const char* default_file;
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

static std::filesystem::path ResolveSystemFontPathCandidate(const std::filesystem::path& base_dir,
                                                            const std::filesystem::path& filename) {
    if (base_dir.empty() || filename.empty()) {
        return {};
    }
    std::error_code ec;
    const auto is_file = [&](const std::filesystem::path& p) -> bool {
        return std::filesystem::is_regular_file(p, ec) && !ec;
    };

    const auto direct = base_dir / filename;
    if (is_file(direct)) {
        return direct;
    }

    const auto base_name = base_dir.filename().string();
    if (base_name != "font" && base_name != "font2") {
        const auto in_font = base_dir / "font" / filename;
        if (is_file(in_font)) {
            return in_font;
        }
        const auto in_font2 = base_dir / "font2" / filename;
        if (is_file(in_font2)) {
            return in_font2;
        }
    }

    if (base_name == "font" || base_name == "font2") {
        const auto container = base_dir.parent_path();
        const auto sibling = container / ((base_name == "font") ? "font2" : "font") / filename;
        if (is_file(sibling)) {
            return sibling;
        }
    }

    return direct;
}

static std::string MacroToCamel(const char* macro_key) {
    if (!macro_key) {
        return {};
    }
    std::string s(macro_key);
    const std::string prefix = "FONTSET_";
    if (s.rfind(prefix, 0) != 0) {
        return {};
    }
    std::string out = "FontSet";
    size_t pos = prefix.size();
    while (pos < s.size()) {
        size_t next = s.find('_', pos);
        const size_t len = (next == std::string::npos) ? (s.size() - pos) : (next - pos);
        std::string token = s.substr(pos, len);
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
                return ResolveSystemFontPathCandidate(base_dir, *override_path);
            }
            LOG_ERROR(Lib_Font,
                      "SystemFonts: override for '{}' must be a filename only (no path): '{}'",
                      def->config_key, override_path->string());
        }
        const auto camel_key = MacroToCamel(def->config_key);
        if (!camel_key.empty()) {
            if (auto override_path2 = Config::getSystemFontOverride(camel_key)) {
                if (!override_path2->empty() && !override_path2->is_absolute() &&
                    !override_path2->has_parent_path()) {
                    return ResolveSystemFontPathCandidate(base_dir, *override_path2);
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
                    return ResolveSystemFontPathCandidate(base_dir, *override_path3);
                }
                LOG_ERROR(Lib_Font,
                          "SystemFonts: override for '{}' must be a filename only (no path): '{}'",
                          lower_camel, override_path3->string());
            }
        }
        if (def->default_file && *def->default_file) {
            return ResolveSystemFontPathCandidate(base_dir, def->default_file);
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
            cache.available = true;
            cache.path = path;
        } else {
            cache.available = false;
            LOG_ERROR(Lib_Font, "SystemFonts: failed to load font for set type=0x{:08X} path='{}'",
                      font_set_type, path.string());
        }
    }
    return cache.available ? &cache : nullptr;
}

static bool LoadFontFile(const std::filesystem::path& path, std::vector<unsigned char>& out_bytes) {
    if (path.empty()) {
        return false;
    }
    auto try_load = [&](const std::filesystem::path& p) -> bool {
        out_bytes.clear();
        if (!LoadGuestFileBytes(p, out_bytes) || out_bytes.empty()) {
            out_bytes.clear();
            return false;
        }
        return true;
    };

    if (try_load(path)) {
        return true;
    }

    std::error_code ec;
    const auto parent = path.parent_path();
    const auto parent_name = parent.filename().string();
    const auto file_name = path.filename();
    if (!file_name.empty() && parent_name != "font" && parent_name != "font2") {
        const auto cand_font = parent / "font" / file_name;
        if (std::filesystem::is_regular_file(cand_font, ec) && !ec) {
            if (try_load(cand_font)) {
                return true;
            }
        }
        const auto cand_font2 = parent / "font2" / file_name;
        if (std::filesystem::is_regular_file(cand_font2, ec) && !ec) {
            if (try_load(cand_font2)) {
                return true;
            }
        }
    }

    if (!file_name.empty() && (parent_name == "font" || parent_name == "font2")) {
        const auto container = parent.parent_path();
        const auto sibling = container / ((parent_name == "font") ? "font2" : "font") / file_name;
        if (std::filesystem::is_regular_file(sibling, ec) && !ec) {
            return try_load(sibling);
        }
    }

    return false;
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
    union {
        u8 reserved3[0x10];
        std::array<std::uint64_t, 2> sysfonts_ctx_name_overrides;
    };
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

#pragma pack(push, 1)
struct FontLibReserved1Main {
    /*0x00*/ u32 reserved_0x00;
    /*0x04*/ u32 mem_kind;
    /*0x08*/ u32 region_size;
    /*0x0C*/ void* region_base;
};
#pragma pack(pop)
static_assert(sizeof(FontLibReserved1Main) == sizeof(((FontLibOpaque*)nullptr)->reserved1),
              "FontLibReserved1Main size");
static_assert(offsetof(FontLibReserved1Main, mem_kind) == 0x04,
              "FontLibReserved1Main mem_kind offset");
static_assert(offsetof(FontLibReserved1Main, region_size) == 0x08,
              "FontLibReserved1Main region_size offset");
static_assert(offsetof(FontLibReserved1Main, region_base) == 0x0C,
              "FontLibReserved1Main region_base offset");

struct FontLibReserved2Iface {
    /*0x00*/ u8 reserved_00[0x20];
    /*0x20*/ std::uintptr_t alloc_fn;
    /*0x28*/ std::uintptr_t dealloc_fn;
    /*0x30*/ std::uintptr_t realloc_fn;
    /*0x38*/ std::uintptr_t calloc_fn;
    /*0x40*/ u8 reserved_40[0x10];
};
static_assert(sizeof(FontLibReserved2Iface) == sizeof(((FontLibOpaque*)nullptr)->reserved2),
              "FontLibReserved2Iface size");
static_assert(offsetof(FontLibReserved2Iface, alloc_fn) == 0x20,
              "FontLibReserved2Iface alloc_fn offset");
static_assert(offsetof(FontLibReserved2Iface, dealloc_fn) == 0x28,
              "FontLibReserved2Iface dealloc_fn offset");
static_assert(offsetof(FontLibReserved2Iface, realloc_fn) == 0x30,
              "FontLibReserved2Iface realloc_fn offset");
static_assert(offsetof(FontLibReserved2Iface, calloc_fn) == 0x38,
              "FontLibReserved2Iface calloc_fn offset");

struct FontLibTail {
    /*0x00*/ u8 reserved_00[0x04];
    /*0x04*/ u32 workspace_size;
    /*0x08*/ void* workspace;
    /*0x10*/ u8 reserved_10[0x10];
    /*0x20*/ void* list_head_ptr;
    /*0x28*/ OrbisFontHandle list_head;
    /*0x30*/ u8 reserved_30[0x18];
};
static_assert(offsetof(FontLibTail, workspace_size) == 0x04, "FontLibTail workspace_size offset");
static_assert(offsetof(FontLibTail, workspace) == 0x08, "FontLibTail workspace offset");
static_assert(offsetof(FontLibTail, list_head_ptr) == 0x20, "FontLibTail list_head_ptr offset");
static_assert(offsetof(FontLibTail, list_head) == 0x28, "FontLibTail list_head offset");
static_assert(sizeof(FontLibTail) == 0x48, "FontLibTail size");

#pragma pack(push, 1)
struct FontLibReserved1SysfontTail {
    /*0x00*/ u32 reserved_0x00;
    /*0x04*/ void* sysfont_desc_ptr;
    /*0x0C*/ u64 sysfont_flags;
};
#pragma pack(pop)
static_assert(sizeof(FontLibReserved1SysfontTail) == sizeof(((FontLibOpaque*)nullptr)->reserved1),
              "FontLibReserved1SysfontTail size");
static_assert(offsetof(FontLibReserved1SysfontTail, sysfont_desc_ptr) == 0x04,
              "FontLibReserved1SysfontTail sysfont_desc_ptr offset");
static_assert(offsetof(FontLibReserved1SysfontTail, sysfont_flags) == 0x0C,
              "FontLibReserved1SysfontTail sysfont_flags offset");

namespace {
static FontHandleOpaqueNative* GetNativeFont(OrbisFontHandle h);

static void LogCachedStyleOnce(OrbisFontHandle handle, const FontHandleOpaqueNative& font) {
    static std::mutex s_mutex;
    static std::unordered_set<OrbisFontHandle> s_logged;
    std::scoped_lock lock(s_mutex);
    if (s_logged.find(handle) != s_logged.end()) {
        return;
    }
    s_logged.insert(handle);

    LOG_DEBUG(Lib_Font,
              "BindRenderer: cached_style snapshot: hDpi={} vDpi={} scaleUnit={} baseScale={} "
              "scalePixelW={} scalePixelH={} effectWeightX={} effectWeightY={} slantRatio={}",
              font.cached_style.hDpi, font.cached_style.vDpi, font.cached_style.scaleUnit,
              font.cached_style.baseScale, font.cached_style.scalePixelW,
              font.cached_style.scalePixelH, font.cached_style.effectWeightX,
              font.cached_style.effectWeightY, font.cached_style.slantRatio);
}

static void LogRenderResultSample(OrbisFontHandle handle, u32 code,
                                  const OrbisFontGlyphMetrics& metrics,
                                  const OrbisFontRenderOutput& result) {
    static std::mutex s_mutex;
    static std::unordered_map<OrbisFontHandle, int> s_counts;
    std::scoped_lock lock(s_mutex);
    int& count = s_counts[handle];
    if (count >= 5) {
        return;
    }
    ++count;
    LOG_DEBUG(Lib_Font,
              "RenderSample: handle={} code=U+{:04X} update=[{},{} {}x{}] img=[bx={} by={} adv={} "
              "stride={} w={} h={}] metrics=[w={} h={} hbx={} hby={} hadv={}]",
              static_cast<const void*>(handle), code, result.UpdateRect.x, result.UpdateRect.y,
              result.UpdateRect.w, result.UpdateRect.h, result.ImageMetrics.bearingX,
              result.ImageMetrics.bearingY, result.ImageMetrics.advance, result.ImageMetrics.stride,
              result.ImageMetrics.width, result.ImageMetrics.height, metrics.width, metrics.height,
              metrics.Horizontal.bearingX, metrics.Horizontal.bearingY, metrics.Horizontal.advance);
}

static inline u8 CachedStyleCacheFlags(const OrbisFontStyleFrame& cached_style) {
    return static_cast<u8>(cached_style.cache_flags_and_direction & 0xFFu);
}

static inline void CachedStyleSetCacheFlags(OrbisFontStyleFrame& cached_style, u8 flags) {
    cached_style.cache_flags_and_direction =
        (cached_style.cache_flags_and_direction & 0xFFFFFF00u) | static_cast<u32>(flags);
}

static inline void CachedStyleSetDirectionWord(OrbisFontStyleFrame& cached_style, u16 word) {
    cached_style.cache_flags_and_direction =
        (cached_style.cache_flags_and_direction & 0x0000FFFFu) | (static_cast<u32>(word) << 16);
}

static inline float CachedStyleGetScalar(const OrbisFontStyleFrame& cached_style) {
    float value = 0.0f;
    std::memcpy(&value, &cached_style.cached_scalar_bits, sizeof(value));
    return value;
}

static inline void CachedStyleSetScalar(OrbisFontStyleFrame& cached_style, float value) {
    std::memcpy(&cached_style.cached_scalar_bits, &value, sizeof(value));
}

constexpr std::uintptr_t kFtRendererCreateFnAddr = 0x0100f6d0u;
constexpr std::uintptr_t kFtRendererDestroyFnAddr = 0x0100f790u;

static s32 PS4_SYSV_ABI FtRendererCreate(void* renderer) {
    if (!renderer) {
        return ORBIS_FONT_ERROR_INVALID_RENDERER;
    }

    auto* r = static_cast<Internal::RendererFtOpaque*>(renderer);
    r->ft_backend.renderer_header_0x10 = static_cast<void*>(&r->base.mem_kind);
    r->ft_backend.unknown_0x08 = 0;
    r->ft_backend.unknown_0x10 = 0;
    r->ft_backend.unknown_0x18 = 0;
    r->ft_backend.initialized_marker = r;
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI FtRendererDestroy(void* renderer) {
    if (!renderer) {
        return ORBIS_FONT_ERROR_INVALID_RENDERER;
    }
    auto* r = static_cast<Internal::RendererFtOpaque*>(renderer);
    r->ft_backend.initialized_marker = nullptr;
    return ORBIS_OK;
}

bool AcquireLibraryLock(FontLibOpaque* lib, u32& out_prev_lock_word) {
    if (!lib) {
        return false;
    }

    for (;;) {
        const u32 lock_word = lib->lock_word;
        if (lib->magic != 0x0F01) {
            return false;
        }
        if (static_cast<s32>(lock_word) < 0) {
            Libraries::Kernel::sceKernelUsleep(0x1e);
            continue;
        }

        std::atomic_ref<u32> ref(lib->lock_word);
        u32 expected = lock_word;
        if (ref.compare_exchange_weak(expected, lock_word | 0x80000000u,
                                      std::memory_order_acq_rel)) {
            out_prev_lock_word = lock_word;
            return true;
        }
        Libraries::Kernel::sceKernelUsleep(0x1e);
    }
}

void ReleaseLibraryLock(FontLibOpaque* lib, u32 prev_lock_word) {
    if (!lib) {
        return;
    }
    lib->lock_word = prev_lock_word & 0x7fffffff;
}
static FontHandleOpaqueNative* GetNativeFont(OrbisFontHandle h) {
    return h ? reinterpret_cast<FontHandleOpaqueNative*>(h) : nullptr;
}

static bool AcquireFontLock(FontHandleOpaqueNative* font, u32& out_prev_lock_word) {
    if (!font) {
        return false;
    }
    for (;;) {
        const u32 lock_word = font->lock_word;
        if (font->magic != 0x0F02) {
            return false;
        }
        if (static_cast<s32>(lock_word) < 0) {
            Libraries::Kernel::sceKernelUsleep(0x1e);
            continue;
        }
        std::atomic_ref<u32> ref(font->lock_word);
        u32 expected = lock_word;
        if (ref.compare_exchange_weak(expected, lock_word | 0x80000000u,
                                      std::memory_order_acq_rel)) {
            out_prev_lock_word = lock_word;
            return true;
        }
        Libraries::Kernel::sceKernelUsleep(0x1e);
    }
}

static void ReleaseFontLock(FontHandleOpaqueNative* font, u32 prev_lock_word) {
    if (!font) {
        return;
    }
    font->lock_word = prev_lock_word & 0x7fffffff;
}

static bool AcquireCachedStyleLock(FontHandleOpaqueNative* font, u32& out_prev_lock_word) {
    if (!font) {
        return false;
    }
    for (;;) {
        const u32 lock_word = font->cached_style.cache_lock_word;
        if (font->magic != 0x0F02) {
            return false;
        }
        if (static_cast<s32>(lock_word) < 0) {
            Libraries::Kernel::sceKernelUsleep(0x1e);
            continue;
        }
        std::atomic_ref<u32> ref(font->cached_style.cache_lock_word);
        u32 expected = lock_word;
        if (ref.compare_exchange_weak(expected, lock_word | 0x80000000u,
                                      std::memory_order_acq_rel)) {
            out_prev_lock_word = lock_word;
            return true;
        }
        Libraries::Kernel::sceKernelUsleep(0x1e);
    }
}

static void ReleaseCachedStyleLock(FontHandleOpaqueNative* font, u32 prev_lock_word) {
    if (!font) {
        return;
    }
    font->cached_style.cache_lock_word = prev_lock_word & 0x7fffffff;
}
} // namespace

namespace {}

struct OrbisFontRenderer_ {
    u16 magic = 0;
    u16 reserved0 = 0;
};
static_assert(offsetof(OrbisFontRenderer_, magic) == 0, "Renderer magic offset");

s32 PS4_SYSV_ABI sceFontAttachDeviceCacheBuffer(OrbisFontLib library, void* buffer, u32 size) {
    LOG_INFO(Lib_Font, "called");
    LOG_DEBUG(Lib_Font, "{}",
              formatParams({
                  Param("library", library),
                  Param("buffer", buffer),
                  Param("size", size),
              }));

    if (!library) {
        LOG_ERROR(Lib_Font, "INVALID_LIBRARY");
        return ORBIS_FONT_ERROR_INVALID_LIBRARY;
    }
    auto* lib = static_cast<FontLibOpaque*>(library);
    if (lib->magic != 0x0F01) {
        LOG_ERROR(Lib_Font, "INVALID_LIBRARY");
        return ORBIS_FONT_ERROR_INVALID_LIBRARY;
    }

    u32 prev_lock_word = 0;
    if (!AcquireLibraryLock(lib, prev_lock_word)) {
        LOG_ERROR(Lib_Font, "INVALID_LIBRARY");
        return ORBIS_FONT_ERROR_INVALID_LIBRARY;
    }

    const auto kBusy = reinterpret_cast<u32*>(
        static_cast<std::uintptr_t>(std::numeric_limits<std::uintptr_t>::max()));

    u32* current_cache = nullptr;
    for (;;) {
        current_cache = lib->device_cache_buf;
        if (current_cache != kBusy) {
            std::atomic_ref<u32*> ref(lib->device_cache_buf);
            u32* expected = current_cache;
            if (ref.compare_exchange_weak(expected, kBusy, std::memory_order_acq_rel)) {
                current_cache = expected;
                break;
            }
        }
        Libraries::Kernel::sceKernelUsleep(0x1e);
    }

    using AllocFn = void*(PS4_SYSV_ABI*)(void* object, u32 size);
    using FreeFn = void(PS4_SYSV_ABI*)(void* object, void* p);
    const auto alloc_fn = lib->alloc_vtbl ? reinterpret_cast<AllocFn>(lib->alloc_vtbl[0]) : nullptr;
    const auto free_fn = lib->alloc_vtbl ? reinterpret_cast<FreeFn>(lib->alloc_vtbl[1]) : nullptr;
    (void)alloc_fn;
    (void)free_fn;

    s32 rc = ORBIS_FONT_ERROR_INVALID_PARAMETER;
    u32* cache_to_store = current_cache;
    u32 owned = 0;

    if (current_cache != nullptr) {
        rc = ORBIS_FONT_ERROR_ALREADY_ATTACHED;
    } else if (size < 0x1020) {
        cache_to_store = nullptr;
        rc = ORBIS_FONT_ERROR_INVALID_PARAMETER;
    } else {
        u32* header = static_cast<u32*>(buffer);
        if (!header) {
            header = static_cast<u32*>(Core::ExecuteGuest(alloc_fn, lib->alloc_ctx, size));
            if (!header) {
                cache_to_store = nullptr;
                rc = ORBIS_FONT_ERROR_ALLOCATION_FAILED;
            } else {
                owned = 1;
            }
        }

        if (rc == ORBIS_FONT_ERROR_INVALID_PARAMETER) {
            header[0] = size;
            const u32 page_count = ((size - 0x1000) >> 12);
            header[1] = page_count;
            header[3] = page_count;
            header[2] = 0;
            *reinterpret_cast<std::uint64_t*>(header + 4) = 0x0FF800001000ULL;

            if (0x0FFF < (size - 0x1000)) {
                lib->flags |= owned;
                rc = ORBIS_OK;
                cache_to_store = header;
                if (owned) {
                    auto& state = GetLibState(library);
                    state.alloc_ctx = lib->alloc_ctx;
                    state.alloc_fn = alloc_fn;
                    state.free_fn = free_fn;
                    state.owned_device_cache = header;
                    state.owned_device_cache_size = size;
                }
            } else {
                if (!buffer) {
                    Core::ExecuteGuest(free_fn, lib->alloc_ctx, header);
                }
                cache_to_store = nullptr;
                rc = ORBIS_FONT_ERROR_INVALID_PARAMETER;
            }
        }
    }

    lib->device_cache_buf = cache_to_store;
    ReleaseLibraryLock(lib, prev_lock_word);
    if (rc != ORBIS_OK) {
        LOG_ERROR(Lib_Font, "FAILED");
    }
    return rc;
}

s32 PS4_SYSV_ABI sceFontBindRenderer(OrbisFontHandle fontHandle, OrbisFontRenderer renderer) {
    LOG_INFO(Lib_Font, "called");
    LOG_DEBUG(Lib_Font, "{}",
              formatParams({
                  Param("fontHandle", fontHandle),
                  Param("renderer", renderer),
              }));

    auto* font = GetNativeFont(fontHandle);
    if (!font) {
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    u32 prev_font_lock = 0;
    if (!AcquireFontLock(font, prev_font_lock)) {
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    u32 prev_cached_lock = 0;
    if (!AcquireCachedStyleLock(font, prev_cached_lock)) {
        ReleaseFontLock(font, prev_font_lock);
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    s32 rc = ORBIS_FONT_ERROR_ALREADY_BOUND_RENDERER;
    if (font->renderer_binding.renderer == nullptr) {
        rc = ORBIS_FONT_ERROR_INVALID_RENDERER;
        if (renderer && *reinterpret_cast<const u16*>(renderer) == 0x0F07) {
            rc = ORBIS_OK;

            std::memcpy(&font->cached_style.effectWeightY, &font->style_tail.slant_ratio,
                        sizeof(u64));

            std::memcpy(&font->cached_style, &font->style_frame[0], sizeof(u32));

            const u32 dpi_y = font->style_frame[1];
            const u32 scale_unit = font->style_tail.scale_unit;
            const u32 reserved_0x0c = font->style_tail.reserved_0x0c;
            const float scale_w = font->style_tail.scale_w;
            const float scale_h = font->style_tail.scale_h;
            const float effect_weight_x = font->style_tail.effect_weight_x;
            const float effect_weight_y = font->style_tail.effect_weight_y;

            font->cached_style.hDpi = dpi_y;
            font->cached_style.vDpi = scale_unit;
            font->cached_style.scaleUnit = reserved_0x0c;
            font->cached_style.baseScale = scale_w;
            font->cached_style.scalePixelW = scale_h;
            font->cached_style.scalePixelH = effect_weight_x;
            font->cached_style.effectWeightX = effect_weight_y;

            font->renderer_binding.renderer = renderer;
        }
    }

    ReleaseCachedStyleLock(font, prev_cached_lock);
    ReleaseFontLock(font, prev_font_lock);
    if (rc != ORBIS_OK) {
        if (rc == ORBIS_FONT_ERROR_ALREADY_BOUND_RENDERER) {
            LOG_ERROR(Lib_Font, "ALREADY_BOUND");
        } else if (rc == ORBIS_FONT_ERROR_INVALID_RENDERER) {
            LOG_ERROR(Lib_Font, "INVALID_RENDERER");
        } else {
            LOG_ERROR(Lib_Font, "FAILED");
        }
    }
    return rc;
}

s32 PS4_SYSV_ABI sceFontCharacterGetBidiLevel(OrbisFontTextCharacter* textCharacter,
                                              int* bidiLevel) {
    if (!textCharacter || !bidiLevel) {
        LOG_ERROR(Lib_Font, "invalid parameter");
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
        LOG_ERROR(Lib_Font, "invalid parameter (pTextOrder)");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    if (!textCharacter) {
        LOG_ERROR(Lib_Font, "invalid parameter (textCharacter)");
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
        if (current->unknown_0x31 == 0 && current->unknown_0x33 == 0) {
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
        if (current->unknown_0x31 == 0 && current->unknown_0x33 == 0) {
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

s32 PS4_SYSV_ABI sceFontClearDeviceCache(OrbisFontLib library) {
    if (!library) {
        return ORBIS_FONT_ERROR_INVALID_LIBRARY;
    }

    auto* lib = static_cast<FontLibOpaque*>(library);
    if (lib->magic != 0x0F01) {
        return ORBIS_FONT_ERROR_INVALID_LIBRARY;
    }

    u32 prev_lock_word = 0;
    if (!AcquireLibraryLock(lib, prev_lock_word)) {
        return ORBIS_FONT_ERROR_INVALID_LIBRARY;
    }

    const auto kBusy = reinterpret_cast<u32*>(
        static_cast<std::uintptr_t>(std::numeric_limits<std::uintptr_t>::max()));

    u32* current_cache = nullptr;
    for (;;) {
        current_cache = lib->device_cache_buf;
        if (current_cache != kBusy) {
            std::atomic_ref<u32*> ref(lib->device_cache_buf);
            u32* expected = current_cache;
            if (ref.compare_exchange_weak(expected, kBusy, std::memory_order_acq_rel)) {
                current_cache = expected;
                break;
            }
        }
        Libraries::Kernel::sceKernelUsleep(0x1e);
    }

    s32 rc = ORBIS_FONT_ERROR_NOT_ATTACHED_CACHE_BUFFER;
    if (current_cache != nullptr) {
        current_cache[3] = current_cache[1];
        current_cache[2] = 0;
        rc = ORBIS_OK;
    }

    lib->device_cache_buf = current_cache;
    ReleaseLibraryLock(lib, prev_lock_word);
    return rc;
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
    LOG_INFO(Lib_Font, "called");
    LOG_DEBUG(Lib_Font, "{}",
              formatParams({
                  Param("memory", memory),
                  Param("create_params", create_params),
                  Param("edition", edition),
                  Param("pLibrary", pLibrary),
              }));

    if (pLibrary) {
        *pLibrary = nullptr;
    }
    if (!memory) {
        LOG_ERROR(Lib_Font, "NULL_POINTER");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    if (memory->mem_kind != 0x0F00 || !memory->iface || !memory->iface->alloc ||
        !memory->iface->dealloc) {
        LOG_ERROR(Lib_Font, "INVALID_MEMORY");
        return ORBIS_FONT_ERROR_INVALID_MEMORY;
    }
    if (!create_params || !pLibrary) {
        LOG_ERROR(Lib_Font, "INVALID_PARAMETER");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    using MallocFn = void*(PS4_SYSV_ABI*)(void* object, u32 size);
    using FreeFn = void(PS4_SYSV_ABI*)(void* object, void* p);
    const auto malloc_fn = reinterpret_cast<MallocFn>(memory->iface->alloc);
    const auto free_fn = reinterpret_cast<FreeFn>(memory->iface->dealloc);
    if (!malloc_fn || !free_fn) {
        LOG_ERROR(Lib_Font, "INVALID_MEMORY");
        return ORBIS_FONT_ERROR_INVALID_MEMORY;
    }

    void* lib_mem = Core::ExecuteGuest(malloc_fn, memory->mspace_handle, 0x100);
    if (!lib_mem) {
        LOG_ERROR(Lib_Font, "ALLOCATION_FAILED");
        return ORBIS_FONT_ERROR_ALLOCATION_FAILED;
    }

    void* mspace = Core::ExecuteGuest(malloc_fn, memory->mspace_handle, 0x4000);
    if (!mspace) {
        Core::ExecuteGuest(free_fn, memory->mspace_handle, lib_mem);
        LOG_ERROR(Lib_Font, "ALLOCATION_FAILED");
        return ORBIS_FONT_ERROR_ALLOCATION_FAILED;
    }

    auto* lib_bytes = static_cast<u8*>(lib_mem);
    auto clear_20 = [&](std::size_t offset) { std::memset(lib_bytes + offset, 0, 0x20); };
    clear_20(0x00);
    clear_20(sizeof(FontLibOpaque) + offsetof(FontLibOpaque, alloc_vtbl));
    clear_20(sizeof(FontLibOpaque) + offsetof(FontLibOpaque, flags));
    clear_20(offsetof(FontLibOpaque, sysfonts_ctx));
    clear_20(offsetof(FontLibOpaque, sys_driver));
    clear_20(offsetof(FontLibOpaque, reserved2) + 0x30);
    clear_20(offsetof(FontLibOpaque, reserved2) + 0x10);
    clear_20(offsetof(FontLibOpaque, alloc_ctx));
    clear_20(offsetof(FontLibOpaque, reserved3) + 0x0C);
    clear_20(offsetof(FontLibOpaque, fontset_registry));

    auto* lib = static_cast<FontLibOpaque*>(lib_mem);

    auto* reserved1 = reinterpret_cast<FontLibReserved1Main*>(lib->reserved1);
    if (reserved1) {
        reserved1->mem_kind = 0x0F00;
        reserved1->region_size = memory->region_size;
        reserved1->region_base = memory->region_base;
    }

    lib->alloc_ctx = memory->mspace_handle;
    lib->alloc_vtbl = reinterpret_cast<void**>(const_cast<OrbisFontMemInterface*>(memory->iface));

    auto* reserved2 = reinterpret_cast<FontLibReserved2Iface*>(lib->reserved2);
    if (reserved2) {
        reserved2->alloc_fn = reinterpret_cast<std::uintptr_t>(memory->iface->alloc);
        reserved2->dealloc_fn = reinterpret_cast<std::uintptr_t>(memory->iface->dealloc);
        reserved2->realloc_fn = reinterpret_cast<std::uintptr_t>(memory->iface->realloc_fn);
        reserved2->calloc_fn = reinterpret_cast<std::uintptr_t>(memory->iface->calloc_fn);
    }

    lib->sys_driver = const_cast<void*>(create_params);

    auto* tail = reinterpret_cast<FontLibTail*>(lib + 1);
    if (tail) {
        tail->workspace_size = 0x4000;
        tail->workspace = mspace;
        tail->list_head_ptr = &tail->list_head;
        tail->list_head = nullptr;
    }

    const auto* sys_driver = reinterpret_cast<const Internal::SysDriver*>(create_params);
    const auto init_fn = sys_driver ? sys_driver->init : nullptr;
    if (!init_fn) {
        LOG_ERROR(Lib_Font, "INVALID_PARAMETER");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    const s32 init_rc = init_fn(memory, lib);
    if (init_rc != ORBIS_OK) {
        Core::ExecuteGuest(free_fn, memory->mspace_handle, mspace);
        Core::ExecuteGuest(free_fn, memory->mspace_handle, lib_mem);
        LOG_ERROR(Lib_Font, "INIT_FAILED");
        return init_rc;
    }

    s32 sdk_version = 0;
    u32 sdk_version_u32 = 0;
    if (Libraries::Kernel::sceKernelGetCompiledSdkVersion(&sdk_version) == ORBIS_OK) {
        sdk_version_u32 = static_cast<u32>(sdk_version);
    }
    const u32 edition_hi = static_cast<u32>(edition >> 32);
    u32 v = edition_hi;
    if (sdk_version_u32 != 0) {
        v = sdk_version_u32;
        if (edition_hi <= sdk_version_u32) {
            v = edition_hi;
        }
    }
    if (0x92ffffu < v) {
        auto* pad_00A_01E = lib->reserved1;
        auto* feature_word = reinterpret_cast<u32*>(pad_00A_01E);
        if (v < 0x1500000u) {
            u32 tmp = (*feature_word) | 1u;
            *feature_word = tmp;
            tmp |= 2u;
            *feature_word = tmp;
            *feature_word = tmp | 4u;
        } else if (v < 0x2000000u) {
            u32 tmp = *feature_word;
            tmp |= 2u;
            *feature_word = tmp;
            *feature_word = tmp | 4u;
        } else if (v <= 0x2000070u) {
            u32 tmp = *feature_word;
            *feature_word = tmp | 4u;
        }
    }

    lib->magic = 0x0F01;

    *pLibrary = lib;
    auto& state = GetLibState(lib);
    state = LibraryState{};
    state.backing_memory = memory;
    state.create_params = create_params;
    state.edition = edition;
    state.alloc_ctx = lib->alloc_ctx;
    state.alloc_fn = reinterpret_cast<FontAllocFn>(lib->alloc_vtbl[0]);
    state.free_fn = reinterpret_cast<FontFreeFn>(lib->alloc_vtbl[1]);
    state.owned_mspace = mspace;
    state.owned_mspace_size = 0x4000;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontDestroyLibrary(OrbisFontLib* pLibrary) {
    if (!pLibrary) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto lib = *pLibrary;
    if (!lib) {
        return ORBIS_FONT_ERROR_INVALID_LIBRARY;
    }
    LOG_INFO(Lib_Font, "library destroy requested");
    LOG_DEBUG(Lib_Font,
              "library destroy params:\n"
              " library_handle_ptr={}\n"
              " library_handle={}\n",
              static_cast<const void*>(pLibrary), static_cast<const void*>(lib));
    auto* native = static_cast<FontLibOpaque*>(lib);
    if (native->magic != 0x0F01) {
        return ORBIS_FONT_ERROR_INVALID_LIBRARY;
    }

    RemoveLibState(lib);

    using FreeFn = void(PS4_SYSV_ABI*)(void* object, void* p);
    const auto free_fn =
        native->alloc_vtbl ? reinterpret_cast<FreeFn>(native->alloc_vtbl[1]) : nullptr;
    if (free_fn) {
        Core::ExecuteGuest(free_fn, native->alloc_ctx, native);
    } else {
        std::free(native);
    }
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
    LOG_INFO(Lib_Font, "called");

    LOG_DEBUG(Lib_Font, "{}",
              formatParams({
                  Param("memory", memory),
                  Param("create_params", create_params),
                  Param("edition", edition),
                  Param("pRenderer", pRenderer),
              }));

    s32 rc = ORBIS_FONT_ERROR_INVALID_PARAMETER;
    if (!memory) {
        if (!pRenderer) {
            LOG_ERROR(Lib_Font, "INVALID_PARAMETER");
            return rc;
        }
        *pRenderer = nullptr;
        LOG_ERROR(Lib_Font, "INVALID_PARAMETER");
        return rc;
    }

    rc = ORBIS_FONT_ERROR_INVALID_MEMORY;
    if (memory->mem_kind == 0x0F00 && memory->iface && memory->iface->alloc &&
        memory->iface->dealloc) {
        if (!create_params) {
            rc = ORBIS_FONT_ERROR_INVALID_PARAMETER;
        } else {
            rc = ORBIS_FONT_ERROR_INVALID_PARAMETER;
            if (pRenderer) {
                using AllocFn = void*(PS4_SYSV_ABI*)(void* object, u32 size);
                using FreeFn = void(PS4_SYSV_ABI*)(void* object, void* p);
                const auto alloc_fn = reinterpret_cast<AllocFn>(memory->iface->alloc);
                const auto free_fn = reinterpret_cast<FreeFn>(memory->iface->dealloc);

                const auto* selection =
                    static_cast<const Libraries::FontFt::OrbisFontRendererSelection*>(
                        create_params);
                const u32 render_size = selection ? selection->size : 0u;
                void* renderer_mem =
                    Core::ExecuteGuest(alloc_fn, memory->mspace_handle, render_size);
                void* workspace = Core::ExecuteGuest(alloc_fn, memory->mspace_handle, 0x4000);

                rc = ORBIS_FONT_ERROR_ALLOCATION_FAILED;
                if (renderer_mem && workspace) {
                    auto* renderer = static_cast<Internal::RendererOpaque*>(renderer_mem);

                    std::memset(renderer, 0, offsetof(Internal::RendererOpaque, mem_kind));
                    renderer->magic = 0x0F07;
                    renderer->mem_kind = 0x0F00;
                    renderer->region_size = memory->region_size;
                    renderer->region_base = memory->region_base;
                    renderer->alloc_ctx = memory->mspace_handle;

                    renderer->mem_iface = memory->iface;
                    renderer->alloc_fn = reinterpret_cast<std::uintptr_t>(memory->iface->alloc);
                    renderer->free_fn = reinterpret_cast<std::uintptr_t>(memory->iface->dealloc);
                    renderer->realloc_fn =
                        reinterpret_cast<std::uintptr_t>(memory->iface->realloc_fn);
                    renderer->calloc_fn =
                        reinterpret_cast<std::uintptr_t>(memory->iface->calloc_fn);

                    renderer->selection =
                        const_cast<void*>(static_cast<const void*>(create_params));
                    renderer->outline_magic_0x90 = 0x400000000000ull;
                    renderer->workspace = workspace;
                    renderer->workspace_size = 0x4000;
                    renderer->reserved_a8 = 0;
                    renderer->outline_policy_flag = 0;

                    const uintptr_t create_fn_ptr =
                        selection ? selection->create_fn : static_cast<std::uintptr_t>(0);
                    using CreateFn = s32(PS4_SYSV_ABI*)(void* renderer);
                    CreateFn create_fn = reinterpret_cast<CreateFn>(create_fn_ptr);
                    if (create_fn_ptr == kFtRendererCreateFnAddr) {
                        create_fn = &FtRendererCreate;
                    }
                    if (!create_fn) {
                        rc = ORBIS_FONT_ERROR_FATAL;
                    } else {
                        rc = Core::ExecuteGuest(create_fn, renderer_mem);
                        if (rc == ORBIS_OK) {
                            s32 sdk_version = 0;
                            u32 sdk_version_u32 = 0;
                            if (Libraries::Kernel::sceKernelGetCompiledSdkVersion(&sdk_version) ==
                                ORBIS_OK) {
                                sdk_version_u32 = static_cast<u32>(sdk_version);
                            }
                            const u32 edition_hi = static_cast<u32>(edition >> 32);
                            u32 v = edition_hi;
                            if (sdk_version_u32 != 0) {
                                v = sdk_version_u32;
                                if (edition_hi <= sdk_version_u32) {
                                    v = edition_hi;
                                }
                            }
                            if ((v - 0x930000u) < 0x1bd0000u) {
                                renderer->feature_byte_0x0c |= 1u;
                                renderer->outline_policy_flag = 1;
                            }

                            *pRenderer = renderer_mem;
                            return ORBIS_OK;
                        }
                    }
                }

                if (workspace) {
                    Core::ExecuteGuest(free_fn, memory->mspace_handle, workspace);
                }
                if (renderer_mem) {
                    Core::ExecuteGuest(free_fn, memory->mspace_handle, renderer_mem);
                }
            }
        }
    }

    if (!pRenderer) {
        LOG_ERROR(Lib_Font, "FAILED");
        return rc;
    }
    *pRenderer = nullptr;
    if (rc == ORBIS_FONT_ERROR_INVALID_PARAMETER) {
        LOG_ERROR(Lib_Font, "INVALID_PARAMETER");
    } else if (rc == ORBIS_FONT_ERROR_INVALID_MEMORY) {
        LOG_ERROR(Lib_Font, "INVALID_MEMORY");
    } else if (rc == ORBIS_FONT_ERROR_ALLOCATION_FAILED) {
        LOG_ERROR(Lib_Font, "ALLOCATION_FAILED");
    } else if (rc == ORBIS_FONT_ERROR_FATAL) {
        LOG_ERROR(Lib_Font, "FATAL");
    } else {
        LOG_ERROR(Lib_Font, "FAILED");
    }
    return rc;
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

    (void)memory;
    if (!Internal::ForgetGeneratedGlyph(handle)) {
        return ORBIS_FONT_ERROR_INVALID_GLYPH;
    }

    auto* boxed = reinterpret_cast<Libraries::Font::Internal::GeneratedGlyph*>(handle);
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
    if (!pRenderer) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    auto* renderer = static_cast<Internal::RendererOpaque*>(*pRenderer);
    s32 rc = ORBIS_FONT_ERROR_INVALID_RENDERER;
    if (renderer && renderer->magic == 0x0F07) {
        const auto kBusy = static_cast<std::uintptr_t>(std::numeric_limits<std::uintptr_t>::max());

        std::uintptr_t selection_value = 0;
        for (;;) {
            selection_value = reinterpret_cast<std::uintptr_t>(renderer->selection);
            if (selection_value != kBusy) {
                std::atomic_ref<std::uintptr_t> ref(
                    *reinterpret_cast<std::uintptr_t*>(&renderer->selection));
                std::uintptr_t expected = selection_value;
                if (ref.compare_exchange_weak(expected, kBusy, std::memory_order_acq_rel)) {
                    selection_value = expected;
                    break;
                }
            }
            Libraries::Kernel::sceKernelUsleep(0x1e);
        }

        if (selection_value == 0) {
            rc = ORBIS_FONT_ERROR_FATAL;
        } else {
            const auto* selection =
                reinterpret_cast<const Libraries::FontFt::OrbisFontRendererSelection*>(
                    selection_value);
            const auto destroy_fn_ptr =
                selection ? selection->destroy_fn : static_cast<std::uintptr_t>(0);
            using DestroyFn = s32(PS4_SYSV_ABI*)(void* renderer);
            DestroyFn destroy_fn = reinterpret_cast<DestroyFn>(destroy_fn_ptr);
            if (destroy_fn_ptr == kFtRendererDestroyFnAddr) {
                destroy_fn = &FtRendererDestroy;
            }
            rc = destroy_fn ? Core::ExecuteGuest(destroy_fn, renderer) : ORBIS_FONT_ERROR_FATAL;
        }

        renderer->selection = nullptr;

        using FreeFn = void(PS4_SYSV_ABI*)(void* object, void* p);
        const auto free_fn = reinterpret_cast<FreeFn>(renderer->free_fn);
        void* alloc_ctx = renderer->alloc_ctx;
        void* workspace = renderer->workspace;
        if (workspace) {
            Core::ExecuteGuest(free_fn, alloc_ctx, workspace);
        }
        Core::ExecuteGuest(free_fn, alloc_ctx, renderer);

        *pRenderer = nullptr;
        return rc;
    }

    return rc;
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

    auto* st = Internal::TryGetState(glyph_handle);
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

    auto* boxed = new (std::nothrow) Libraries::Font::Internal::GeneratedGlyph();
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
    boxed->glyph.baseline = ClampToU16(metrics.Horizontal.bearingY);
    boxed->glyph.height_px = ClampToU16(metrics.height);
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

    Internal::TrackGeneratedGlyph(&boxed->glyph);
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
    LOG_INFO(Lib_Font, "called");

    LOG_DEBUG(Lib_Font, "{}",
              formatParams({
                  Param("fontHandle", fontHandle),
                  Param("code", code),
                  Param("metrics", metrics),
              }));

    const s32 rc = Internal::GetCharGlyphMetrics(fontHandle, code, metrics, false);
    if (rc != ORBIS_OK) {
        if (rc == ORBIS_FONT_ERROR_INVALID_FONT_HANDLE) {
            LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        } else if (rc == ORBIS_FONT_ERROR_INVALID_PARAMETER) {
            LOG_ERROR(Lib_Font, "INVALID_PARAMETER");
        } else if (rc == ORBIS_FONT_ERROR_NOT_BOUND_RENDERER) {
            LOG_ERROR(Lib_Font, "NOT_BOUND_RENDERER");
        }
    }
    return rc;
}

s32 PS4_SYSV_ABI sceFontGetEffectSlant(OrbisFontHandle fontHandle, float* slantRatio) {
    LOG_INFO(Lib_Font, "called");
    LOG_DEBUG(Lib_Font, "{}",
              formatParams({
                  Param("fontHandle", fontHandle),
                  Param("slantRatio", slantRatio),
              }));

    auto* font = GetNativeFont(fontHandle);
    if (!font || font->magic != 0x0F02) {
        if (slantRatio) {
            *slantRatio = 0.0f;
        }
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    u32 prev_lock = 0;
    if (!AcquireFontLock(font, prev_lock)) {
        if (slantRatio) {
            *slantRatio = 0.0f;
        }
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    const s32 rc = Internal::StyleStateGetSlantRatio(font->style_frame, slantRatio);
    ReleaseFontLock(font, prev_lock);

    if (rc != ORBIS_OK) {
        if (slantRatio) {
            *slantRatio = 0.0f;
        }
        LOG_ERROR(Lib_Font, "FAILED");
    }
    return rc;
}

s32 PS4_SYSV_ABI sceFontGetEffectWeight(OrbisFontHandle fontHandle, float* weightXScale,
                                        float* weightYScale, u32* mode) {
    LOG_INFO(Lib_Font, "called");
    LOG_DEBUG(Lib_Font, "{}",
              formatParams({
                  Param("fontHandle", fontHandle),
                  Param("weightXScale", weightXScale),
                  Param("weightYScale", weightYScale),
                  Param("mode", mode),
              }));

    auto* font = GetNativeFont(fontHandle);
    if (!font || font->magic != 0x0F02) {
        if (weightXScale) {
            *weightXScale = 1.0f;
        }
        if (weightYScale) {
            *weightYScale = 1.0f;
        }
        if (mode) {
            *mode = 0;
        }
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    u32 prev_lock = 0;
    if (!AcquireFontLock(font, prev_lock)) {
        if (weightXScale) {
            *weightXScale = 1.0f;
        }
        if (weightYScale) {
            *weightYScale = 1.0f;
        }
        if (mode) {
            *mode = 0;
        }
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    const s32 rc =
        Internal::StyleStateGetWeightScale(font->style_frame, weightXScale, weightYScale, mode);
    ReleaseFontLock(font, prev_lock);

    if (rc != ORBIS_OK) {
        if (weightXScale) {
            *weightXScale = 1.0f;
        }
        if (weightYScale) {
            *weightYScale = 1.0f;
        }
        if (mode) {
            *mode = 0;
        }
        LOG_ERROR(Lib_Font, "FAILED");
    }
    return rc;
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
    LOG_INFO(Lib_Font, "called");

    if (!fontHandle) {
        if (layout) {
            *layout = {};
        }
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    auto* font = GetNativeFont(fontHandle);
    if (!font || font->magic != 0x0F02) {
        if (layout) {
            *layout = {};
        }
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    u32 prev_lock = 0;
    if (!AcquireFontLock(font, prev_lock)) {
        if (layout) {
            *layout = {};
        }
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    if (!layout) {
        font->lock_word = prev_lock;
        LOG_ERROR(Lib_Font, "NULL_POINTER");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    LOG_DEBUG(Lib_Font, "{}",
              formatParams({
                  Param("fontHandle", fontHandle),
                  Param("layout", layout),
              }));

    Internal::HorizontalLayoutBlocks layout_blocks{};
    const s32 rc =
        Internal::ComputeHorizontalLayoutBlocksTyped(fontHandle, font->style_frame, &layout_blocks);
    font->lock_word = prev_lock;

    if (rc == ORBIS_OK) {
        layout->baselineOffset = layout_blocks.baseline();
        layout->lineAdvance = layout_blocks.line_advance();
        layout->decorationExtent = layout_blocks.effect_height();
        LOG_DEBUG(Lib_Font, "GetHorizontalLayout: out baseLineY={} lineHeight={} effectHeight={}",
                  layout->baselineOffset, layout->lineAdvance, layout->decorationExtent);
        return ORBIS_OK;
    }

    *layout = {};
    if (const auto* rec =
            static_cast<const Internal::FontSetRecordHeader*>(font->open_info.fontset_record)) {
        LOG_DEBUG(Lib_Font,
                  "GetHorizontalLayout: fail detail rc={} fontset_record={} magic=0x{:08X} "
                  "entry_count={} ctx_entry_index={} sub_font_index={}",
                  rc, static_cast<const void*>(font->open_info.fontset_record), rec->magic,
                  rec->entry_count, font->open_info.ctx_entry_index,
                  font->open_info.sub_font_index);
        const u32 entry_count = rec->entry_count;
        const auto* entries = reinterpret_cast<const u32*>(rec + 1);
        for (u32 i = 0; i < entry_count; ++i) {
            LOG_DEBUG(Lib_Font, "GetHorizontalLayout: fail detail fontset_entry[{}]={}", i,
                      entries[i]);
        }
    } else {
        LOG_DEBUG(Lib_Font,
                  "GetHorizontalLayout: fail detail rc={} fontset_record=null ctx_entry_index={} "
                  "sub_font_index={}",
                  rc, font->open_info.ctx_entry_index, font->open_info.sub_font_index);
    }
    LOG_ERROR(Lib_Font, "FAILED");
    return rc;
}

s32 PS4_SYSV_ABI sceFontGetKerning(OrbisFontHandle fontHandle, u32 preCode, u32 code,
                                   OrbisFontKerning* kerning) {
    if (!kerning) {
        LOG_DEBUG(Lib_Font, "invalid parameters");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto& st = Internal::GetState(fontHandle);
    if (!st.ext_face_ready || !st.ext_ft_face) {
        bool system_attached = false;
        const std::string sys_log =
            Internal::ReportSystemFaceRequest(st, fontHandle, system_attached);
        if (!sys_log.empty()) {
            LOG_ERROR(Lib_Font, "{}", sys_log);
        }
    }

    FT_Face resolved_face = st.ext_ft_face;
    float resolved_scale_factor = st.system_font_scale_factor;
    FT_UInt g1 =
        resolved_face ? FT_Get_Char_Index(resolved_face, static_cast<FT_ULong>(preCode)) : 0;
    FT_UInt g2 = resolved_face ? FT_Get_Char_Index(resolved_face, static_cast<FT_ULong>(code)) : 0;
    if (g1 == 0 || g2 == 0) {
        for (const auto& fb : st.system_fallback_faces) {
            if (!fb.ready || !fb.ft_face) {
                continue;
            }
            const FT_UInt fb_g1 = FT_Get_Char_Index(fb.ft_face, static_cast<FT_ULong>(preCode));
            const FT_UInt fb_g2 = FT_Get_Char_Index(fb.ft_face, static_cast<FT_ULong>(code));
            if (fb_g1 == 0 || fb_g2 == 0) {
                continue;
            }
            resolved_face = fb.ft_face;
            resolved_scale_factor = fb.scale_factor;
            g1 = fb_g1;
            g2 = fb_g2;
            break;
        }
    }

    if (resolved_face && g1 != 0 && g2 != 0) {
        const float scaled_w = st.scale_w * resolved_scale_factor;
        const float scaled_h = st.scale_h * resolved_scale_factor;
        const auto char_w = static_cast<FT_F26Dot6>(static_cast<s32>(scaled_w * 64.0f));
        const auto char_h = static_cast<FT_F26Dot6>(static_cast<s32>(scaled_h * 64.0f));
        (void)FT_Set_Char_Size(resolved_face, char_w, char_h, 72, 72);

        FT_Vector delta{};
        FT_Get_Kerning(resolved_face, g1, g2, FT_KERNING_DEFAULT, &delta);
        const float kx = static_cast<float>(delta.x) / 64.0f;
        kerning->offsetX = kx;
        kerning->offsetY = 0.0f;
        kerning->positionX = 0.0f;
        kerning->positionY = 0.0f;
        return ORBIS_OK;
    }
    kerning->offsetX = 0.0f;
    kerning->offsetY = 0.0f;
    kerning->positionX = 0.0f;
    kerning->positionY = 0.0f;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetLibrary(OrbisFontHandle fontHandle, OrbisFontLib* pLibrary) {
    if (!pLibrary) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    const auto& st = Internal::GetState(fontHandle);
    *pLibrary = st.library;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetPixelResolution() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetRenderCharGlyphMetrics(OrbisFontHandle fontHandle, u32 codepoint,
                                                  OrbisFontGlyphMetrics* out_metrics) {
    LOG_INFO(Lib_Font, "called");
    LOG_DEBUG(Lib_Font, "{}",
              formatParams({
                  Param("fontHandle", fontHandle),
                  Param("codepoint", codepoint),
                  Param("out_metrics", out_metrics),
              }));

    const s32 rc = Internal::GetCharGlyphMetrics(fontHandle, codepoint, out_metrics, true);
    if (rc != ORBIS_OK) {
        if (rc == ORBIS_FONT_ERROR_INVALID_FONT_HANDLE) {
            LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        } else if (rc == ORBIS_FONT_ERROR_INVALID_PARAMETER) {
            LOG_ERROR(Lib_Font, "INVALID_PARAMETER");
        } else if (rc == ORBIS_FONT_ERROR_NOT_BOUND_RENDERER) {
            LOG_ERROR(Lib_Font, "NOT_BOUND_RENDERER");
        }
    }
    return rc;
}

s32 PS4_SYSV_ABI sceFontGetRenderEffectSlant(OrbisFontHandle fontHandle, float* slantRatio) {
    s32 rc = ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    auto* font = GetNativeFont(fontHandle);
    if (font && font->magic == 0x0F02) {
        u32 prev_cached_lock = 0;
        if (AcquireCachedStyleLock(font, prev_cached_lock)) {
            if (font->renderer_binding.renderer == nullptr) {
                rc = ORBIS_FONT_ERROR_NOT_BOUND_RENDERER;
                font->cached_style.cache_lock_word = prev_cached_lock;
            } else {
                rc = Internal::StyleStateGetSlantRatio(&font->cached_style, slantRatio);
                font->cached_style.cache_lock_word = prev_cached_lock;
                if (rc == ORBIS_OK) {
                    return ORBIS_OK;
                }
            }
        }
    }
    if (slantRatio) {
        *slantRatio = 0.0f;
    }
    return rc;
}

s32 PS4_SYSV_ABI sceFontGetRenderEffectWeight(OrbisFontHandle fontHandle, float* weightXScale,
                                              float* weightYScale, u32* mode) {
    s32 rc = ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    auto* font = GetNativeFont(fontHandle);
    if (font && font->magic == 0x0F02) {
        u32 prev_cached_lock = 0;
        if (AcquireCachedStyleLock(font, prev_cached_lock)) {
            if (font->renderer_binding.renderer == nullptr) {
                rc = ORBIS_FONT_ERROR_NOT_BOUND_RENDERER;
                font->cached_style.cache_lock_word = prev_cached_lock;
            } else {
                rc = Internal::StyleStateGetWeightScale(&font->cached_style, weightXScale,
                                                        weightYScale, mode);
                font->cached_style.cache_lock_word = prev_cached_lock;
                if (rc == ORBIS_OK) {
                    return ORBIS_OK;
                }
            }
        }
    }
    if (weightXScale) {
        *weightXScale = 1.0f;
    }
    if (weightYScale) {
        *weightYScale = 1.0f;
    }
    if (mode) {
        *mode = 0;
    }
    return rc;
}

s32 PS4_SYSV_ABI sceFontGetRenderScaledKerning() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetRenderScalePixel(OrbisFontHandle fontHandle, float* out_w,
                                            float* out_h) {
    s32 rc = ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    auto* font = GetNativeFont(fontHandle);
    if (font) {
        u32 prev_cached_lock = 0;
        if (AcquireCachedStyleLock(font, prev_cached_lock)) {
            if (font->renderer_binding.renderer == nullptr) {
                rc = ORBIS_FONT_ERROR_NOT_BOUND_RENDERER;
            } else {
                rc = Internal::StyleStateGetScalePixel(&font->cached_style, out_w, out_h);
            }
            ReleaseCachedStyleLock(font, prev_cached_lock);
            if (rc == ORBIS_OK) {
                return rc;
            }
        }
    }
    if (out_w) {
        *out_w = 0.0f;
    }
    if (out_h) {
        *out_h = 0.0f;
    }
    return rc;
}

s32 PS4_SYSV_ABI sceFontGetRenderScalePoint(OrbisFontHandle fontHandle, float* out_w,
                                            float* out_h) {
    s32 rc = ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    auto* font = GetNativeFont(fontHandle);
    if (font) {
        u32 prev_cached_lock = 0;
        if (AcquireCachedStyleLock(font, prev_cached_lock)) {
            if (font->renderer_binding.renderer == nullptr) {
                rc = ORBIS_FONT_ERROR_NOT_BOUND_RENDERER;
            } else {
                rc = Internal::StyleStateGetScalePoint(&font->cached_style, out_w, out_h);
            }
            ReleaseCachedStyleLock(font, prev_cached_lock);
            if (rc == ORBIS_OK) {
                return rc;
            }
        }
    }
    if (out_w) {
        *out_w = 0.0f;
    }
    if (out_h) {
        *out_h = 0.0f;
    }
    return rc;
}

s32 PS4_SYSV_ABI sceFontGetResolutionDpi() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetScalePixel(OrbisFontHandle fontHandle, float* out_w, float* out_h) {
    LOG_INFO(Lib_Font, "called");

    if (!fontHandle) {
        if (out_w) {
            *out_w = 0.0f;
        }
        if (out_h) {
            *out_h = 0.0f;
        }
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    LOG_DEBUG(Lib_Font, "{}",
              formatParams({
                  Param("fontHandle", fontHandle),
                  Param("out_w", out_w),
                  Param("out_h", out_h),
              }));

    s32 rc = ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    auto* font = GetNativeFont(fontHandle);
    if (font && font->magic == 0x0F02) {
        u32 prev_font_lock = 0;
        if (AcquireFontLock(font, prev_font_lock)) {
            rc = Internal::StyleStateGetScalePixel(
                reinterpret_cast<const OrbisFontStyleFrame*>(font->style_frame), out_w, out_h);
            ReleaseFontLock(font, prev_font_lock);
            if (rc == ORBIS_OK) {
                return rc;
            }
        }
    }
    if (out_w) {
        *out_w = 0.0f;
    }
    if (out_h) {
        *out_h = 0.0f;
    }
    if (rc == ORBIS_FONT_ERROR_INVALID_PARAMETER) {
        LOG_ERROR(Lib_Font, "INVALID_PARAMETER");
    } else if (rc == ORBIS_FONT_ERROR_INVALID_FONT_HANDLE) {
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
    } else {
        LOG_ERROR(Lib_Font, "FAILED");
    }
    return rc;
}

s32 PS4_SYSV_ABI sceFontGetScalePoint(OrbisFontHandle fontHandle, float* out_w, float* out_h) {
    s32 rc = ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    auto* font = GetNativeFont(fontHandle);
    if (font) {
        u32 prev_font_lock = 0;
        if (AcquireFontLock(font, prev_font_lock)) {
            rc = Internal::StyleStateGetScalePoint(
                reinterpret_cast<const OrbisFontStyleFrame*>(font->style_frame), out_w, out_h);
            ReleaseFontLock(font, prev_font_lock);
            if (rc == ORBIS_OK) {
                return rc;
            }
        }
    }
    if (out_w) {
        *out_w = 0.0f;
    }
    if (out_h) {
        *out_h = 0.0f;
    }
    return rc;
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
    s32 rc = ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    auto* font = GetNativeFont(fontHandle);
    if (font && font->magic == 0x0F02) {
        u32 prev_font_lock = 0;
        if (AcquireFontLock(font, prev_font_lock)) {
            if (!layout) {
                font->lock_word = prev_font_lock;
                rc = ORBIS_FONT_ERROR_INVALID_PARAMETER;
            } else {
                Internal::VerticalLayoutBlocks layout_words{};
                rc = Internal::ComputeVerticalLayoutBlocksTyped(fontHandle, font->style_frame,
                                                                &layout_words);
                font->lock_word = prev_font_lock;
                if (rc == ORBIS_OK) {
                    layout->baselineOffsetX = layout_words.baseline_offset_x();
                    layout->columnAdvance = layout_words.column_advance();
                    layout->decorationSpan = layout_words.decoration_span();
                    return ORBIS_OK;
                }
            }
        }
    }

    if (layout) {
        layout->decorationSpan = 0.0f;
        *reinterpret_cast<std::uint64_t*>(layout) = 0;
    }
    return rc;
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
    auto* boxed = Internal::TryGetGeneratedGlyph(glyph);
    if (!boxed) {
        return nullptr;
    }
    if (!boxed->outline_initialized) {
        if (!Internal::BuildTrueOutline(*boxed)) {
            Internal::BuildBoundingOutline(*boxed);
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
    LOG_INFO(Lib_Font, "called");

    if (!mem_desc) {
        LOG_ERROR(Lib_Font, "NULL_POINTER");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    LOG_DEBUG(Lib_Font, "{}",
              formatParams({
                  Param("mem_desc", mem_desc),
                  Param("region_addr", region_addr),
                  Param("region_size", region_size),
                  Param("iface", iface),
                  Param("mspace_obj", mspace_obj),
                  Param("destroy_cb", reinterpret_cast<const void*>(destroy_cb)),
                  Param("destroy_ctx", destroy_ctx),
              }));

    if (!iface) {
        mem_desc->mem_kind = 0;
        if (!region_addr || region_size == 0) {
            LOG_ERROR(Lib_Font, "INVALID_PARAMETER");
            return ORBIS_FONT_ERROR_INVALID_PARAMETER;
        }
    }

    mem_desc->mem_kind = 0x0F00;
    mem_desc->attr_bits = 0;
    mem_desc->region_size = region_size;
    mem_desc->region_base = region_addr;
    mem_desc->mspace_handle = mspace_obj;
    mem_desc->iface = iface;
    mem_desc->on_destroy = destroy_cb;
    mem_desc->destroy_ctx = destroy_ctx;
    mem_desc->some_ctx1 = nullptr;
    mem_desc->some_ctx2 = mspace_obj;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontMemoryTerm(OrbisFontMem* mem_desc) {
    if (!mem_desc) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    if (mem_desc->mem_kind != 0x0F00) {
        return ORBIS_FONT_ERROR_INVALID_MEMORY;
    }

    if (static_cast<s8>(mem_desc->attr_bits & 0xFF) < 0) {
        if (mem_desc->iface && mem_desc->iface->mspace_destroy) {
            using DestroyFn = void(PS4_SYSV_ABI*)(void* parent, void* mspace);
            const auto destroy_fn = reinterpret_cast<DestroyFn>(mem_desc->iface->mspace_destroy);
            if (destroy_fn) {
                Core::ExecuteGuest(destroy_fn, mem_desc->some_ctx2, mem_desc->mspace_handle);
            }
        }
        std::memset(mem_desc, 0, sizeof(*mem_desc));
        return ORBIS_OK;
    }

    if (mem_desc->on_destroy) {
        mem_desc->mem_kind = 0;
        using DestroyFn =
            void(PS4_SYSV_ABI*)(OrbisFontMem * fontMemory, void* object, void* destroyArg);
        const auto destroy_fn = reinterpret_cast<DestroyFn>(mem_desc->on_destroy);
        Core::ExecuteGuest(destroy_fn, mem_desc, mem_desc->mspace_handle, mem_desc->destroy_ctx);
        return ORBIS_OK;
    }

    std::memset(mem_desc, 0, sizeof(*mem_desc));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontOpenFontFile(OrbisFontLib library, const char* guest_path, u32 open_mode,
                                     const OrbisFontOpenParams* open_detail,
                                     OrbisFontHandle* out_handle) {
    LOG_INFO(Lib_Font, "called");

    LOG_DEBUG(Lib_Font, "{}",
              formatParams({
                  Param("library", library),
                  Param("guest_path", guest_path),
                  Param("open_mode", open_mode),
                  Param("open_detail", open_detail),
                  Param("out_handle", out_handle),
              }));

    if (!library) {
        if (out_handle) {
            *out_handle = nullptr;
        }
        LOG_ERROR(Lib_Font, "INVALID_LIBRARY");
        return ORBIS_FONT_ERROR_INVALID_LIBRARY;
    }
    if (!out_handle) {
        LOG_ERROR(Lib_Font, "INVALID_PARAMETER");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    auto* lib = static_cast<FontLibOpaque*>(library);
    if (lib->magic != 0x0F01) {
        *out_handle = nullptr;
        LOG_ERROR(Lib_Font, "INVALID_LIBRARY");
        return ORBIS_FONT_ERROR_INVALID_LIBRARY;
    }

    u32 prev_lib_lock = 0;
    if (!AcquireLibraryLock(lib, prev_lib_lock)) {
        *out_handle = nullptr;
        LOG_ERROR(Lib_Font, "INVALID_LIBRARY");
        return ORBIS_FONT_ERROR_INVALID_LIBRARY;
    }

    const auto release_library_and_clear_out = [&] {
        ReleaseLibraryLock(lib, prev_lib_lock);
        *out_handle = nullptr;
    };

    if (!lib->fontset_registry || !lib->sys_driver) {
        release_library_and_clear_out();
        LOG_ERROR(Lib_Font, "INVALID_LIBRARY");
        return ORBIS_FONT_ERROR_INVALID_LIBRARY;
    }

    const u32 sub_font_index = open_detail ? open_detail->subfont_index : 0u;
    const s32 unique_id = open_detail ? open_detail->unique_id : -1;

    using AllocFn = void*(PS4_SYSV_ABI*)(void* object, u32 size);
    using FreeFn = void(PS4_SYSV_ABI*)(void* object, void* p);
    const auto alloc_fn = lib->alloc_vtbl ? reinterpret_cast<AllocFn>(lib->alloc_vtbl[0]) : nullptr;
    const auto free_fn = lib->alloc_vtbl ? reinterpret_cast<FreeFn>(lib->alloc_vtbl[1]) : nullptr;
    if (!alloc_fn || !free_fn) {
        release_library_and_clear_out();
        LOG_ERROR(Lib_Font, "INVALID_LIBRARY");
        return ORBIS_FONT_ERROR_INVALID_LIBRARY;
    }

    auto* ext_ctx = static_cast<u8*>(lib->external_fonts_ctx);
    if (!ext_ctx) {
        release_library_and_clear_out();
        LOG_ERROR(Lib_Font, "NO_SUPPORT_FUNCTION");
        return ORBIS_FONT_ERROR_NO_SUPPORT_FUNCTION;
    }

    if (!guest_path || unique_id < -1) {
        release_library_and_clear_out();
        LOG_ERROR(Lib_Font, "INVALID_PARAMETER");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    const auto host_path = ResolveGuestPath(guest_path);
    const std::filesystem::path path_to_open =
        host_path.empty() ? std::filesystem::path(guest_path) : host_path;
    const std::string host_path_str = path_to_open.string();
    if (host_path_str.empty()) {
        release_library_and_clear_out();
        LOG_ERROR(Lib_Font, "INVALID_PARAMETER");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    const std::string guest_name = std::filesystem::path(guest_path).filename().string();

    OrbisFontHandle handle = *out_handle;
    if (handle) {
        if (auto* h = GetNativeFont(handle)) {
            h->flags = 0;
        }
        Internal::RemoveState(handle);
    } else {
        handle = static_cast<OrbisFontHandle>(Core::ExecuteGuest(alloc_fn, lib->alloc_ctx, 0x100));
        if (!handle) {
            release_library_and_clear_out();
            LOG_ERROR(Lib_Font, "ALLOCATION_FAILED");
            return ORBIS_FONT_ERROR_ALLOCATION_FAILED;
        }
        std::memset(handle, 0, 0x100);
        if (auto* h = GetNativeFont(handle)) {
            h->flags = 0x10;
        }
    }

    auto cleanup_handle_for_error = [&](s32 err) {
        auto* h = GetNativeFont(handle);
        if (!h) {
            ReleaseLibraryLock(lib, prev_lib_lock);
            *out_handle = nullptr;
            return err;
        }
        h->magic = 0;
        h->open_info.unique_id_packed = 0;
        h->open_info.ctx_entry_index = 0;
        h->open_info.sub_font_index = 0;
        h->open_info.fontset_flags = 0;
        h->open_info.fontset_record = nullptr;
        h->open_info.reserved_0x18 = 0;
        h->library = library;
        const u16 prev_flags = h->flags;
        h->flags = 0;
        if ((prev_flags & 0x10) != 0) {
            Core::ExecuteGuest(free_fn, lib->alloc_ctx, handle);
        }
        Internal::RemoveState(handle);
        ReleaseLibraryLock(lib, prev_lib_lock);
        *out_handle = nullptr;
        return err;
    };

    auto* ext_header = reinterpret_cast<Internal::FontCtxHeader*>(ext_ctx);

    auto* ctx_lock_word = &ext_header->lock_word;
    for (;;) {
        const u32 lw = *ctx_lock_word;
        if (static_cast<s32>(lw) >= 0) {
            std::atomic_ref<u32> ref(*ctx_lock_word);
            u32 expected = lw;
            if (ref.compare_exchange_weak(expected, lw | 0x80000000u, std::memory_order_acq_rel)) {
                break;
            }
        }
        Libraries::Kernel::sceKernelUsleep(0x1e);
    }

    const u32 font_max = ext_header->max_entries;
    auto* entries_base = static_cast<Internal::FontCtxEntry*>(ext_header->base);
    if (font_max == 0 || !entries_base) {
        *ctx_lock_word = 0;
        const s32 err = cleanup_handle_for_error(ORBIS_FONT_ERROR_FONT_OPEN_MAX);
        LogFontOpenError(err);
        return err;
    }

    u32 entry_index = 0xffffffffu;
    u32 first_free = 0xffffffffu;
    for (u32 i = 0; i < font_max; i++) {
        auto* entry = &entries_base[i];
        const u32 active = entry->active;
        const u32 stored_id = entry->unique_id;
        if (active != 0 && stored_id == static_cast<u32>(unique_id)) {
            entry_index = i;
            break;
        }
        if (first_free == 0xffffffffu && active == 0) {
            first_free = i;
        }
    }

    if (entry_index == 0xffffffffu) {
        entry_index = first_free;
        if (entry_index == 0xffffffffu) {
            *ctx_lock_word = 0;
            const s32 err = cleanup_handle_for_error(ORBIS_FONT_ERROR_FONT_OPEN_MAX);
            LogFontOpenError(err);
            return err;
        }

        auto* entry = &entries_base[entry_index];
        const u32 stored_id =
            (unique_id != -1) ? static_cast<u32>(unique_id) : (entry_index ^ 0x80000000u);
        entry->reserved_0x00 = 0;
        entry->active = 1;
        entry->font_address = 0;
        entry->unique_id = stored_id;
        entry->lock_mode1 = 0;
        entry->lock_mode3 = 0;
        entry->lock_mode2 = 0;
        entry->obj_mode1 = nullptr;
        entry->obj_mode3 = nullptr;
        entry->obj_mode2 = nullptr;
        std::memset(entry->reserved_0x38, 0, sizeof(entry->reserved_0x38));
    }

    *ctx_lock_word = 0;

    const u32 packed_unique_id = (unique_id == -1) ? (entry_index + 0x40000000u)
                                                   : (static_cast<u32>(unique_id) | 0x80000000u);

    void* font_obj = nullptr;
    u32 entry_lock_word = 0;
    u8* entry_base =
        Internal::AcquireFontCtxEntry(ext_ctx, entry_index, open_mode, &font_obj, &entry_lock_word);
    auto* entry = entry_base ? reinterpret_cast<Internal::FontCtxEntry*>(entry_base) : nullptr;
    if (!entry_base) {
        const s32 err = cleanup_handle_for_error(ORBIS_FONT_ERROR_FONT_OPEN_MAX);
        LogFontOpenError(err);
        return err;
    }

    const u32 mode_low = open_mode & 0xF;

    const auto* driver =
        lib->sys_driver ? reinterpret_cast<const Internal::SysDriver*>(lib->sys_driver) : nullptr;
    const auto open_fn = driver ? driver->open : nullptr;
    const auto metric_fn = driver ? driver->metric : nullptr;
    const auto scale_fn = driver ? driver->scale : nullptr;
    if (!open_fn || !metric_fn || !scale_fn) {
        const s32 err = cleanup_handle_for_error(ORBIS_FONT_ERROR_INVALID_LIBRARY);
        LogFontOpenError(err);
        return err;
    }

    s32 rc = ORBIS_OK;
    u32 updated_lock = entry_lock_word;
    void* used_font_obj = font_obj;

    const auto open_driver_mode_for = [&](u32 m) -> u32 {
        if (m == 1) {
            return 5;
        }
        if (m == 2) {
            return 6;
        }
        return 7;
    };

    if ((entry_lock_word & 0x40000000u) == 0) {
        auto* tail = lib + 1;
        auto* tail_reserved1 =
            tail ? reinterpret_cast<FontLibReserved1SysfontTail*>(tail->reserved1) : nullptr;
        const u32 lib_flags = lib->flags;
        if (mode_low == 2) {
            if ((lib_flags & 0x100000u) != 0) {
                tail_reserved1->sysfont_flags |= 1;
            }
            if ((lib_flags & 0x10000u) != 0) {
                tail_reserved1->sysfont_flags |= 2;
            }
        } else {
            if ((lib_flags & 0x200000u) != 0) {
                tail_reserved1->sysfont_flags |= 1;
            }
            if ((lib_flags & 0x20000u) != 0) {
                tail_reserved1->sysfont_flags |= 2;
            }
        }

        if (mode_low != 1 && mode_low != 2 && mode_low != 3) {
            rc = ORBIS_FONT_ERROR_INVALID_PARAMETER;
        } else {
            used_font_obj = font_obj;
            rc = open_fn(library, open_driver_mode_for(mode_low), host_path_str.c_str(), 0,
                         sub_font_index, packed_unique_id, &used_font_obj);
        }

        if (tail_reserved1) {
            tail_reserved1->sysfont_flags = 0;
        }

        if (rc == ORBIS_OK) {
            if (mode_low == 3) {
                entry->obj_mode3 = used_font_obj;
            } else if (mode_low == 2) {
                entry->obj_mode2 = used_font_obj;
            } else {
                entry->obj_mode1 = used_font_obj;
            }
            updated_lock = entry_lock_word | 0x40000000u;
        }
    } else {
        rc = ORBIS_FONT_ERROR_FONT_OPEN_MAX;
        if ((entry_lock_word & 0x0FFFFFFFu) != 0x0FFFFFFFu) {
            for (auto* node = static_cast<Internal::FontObj*>(font_obj); node != nullptr;
                 node = node->next) {
                if (node->sub_font_index == sub_font_index) {
                    node->refcount++;
                    used_font_obj = node;
                    rc = ORBIS_OK;
                    break;
                }
            }
            if (rc != ORBIS_OK) {
                auto* tail = lib + 1;
                auto* tail_reserved1 =
                    tail ? reinterpret_cast<FontLibReserved1SysfontTail*>(tail->reserved1)
                         : nullptr;
                const u32 lib_flags = lib->flags;
                if (mode_low == 2) {
                    if ((lib_flags & 0x100000u) != 0) {
                        tail_reserved1->sysfont_flags |= 1;
                    }
                    if ((lib_flags & 0x10000u) != 0) {
                        tail_reserved1->sysfont_flags |= 2;
                    }
                } else {
                    if ((lib_flags & 0x200000u) != 0) {
                        tail_reserved1->sysfont_flags |= 1;
                    }
                    if ((lib_flags & 0x20000u) != 0) {
                        tail_reserved1->sysfont_flags |= 2;
                    }
                }

                used_font_obj = font_obj;
                rc = open_fn(library, open_driver_mode_for(mode_low), host_path_str.c_str(), 0,
                             sub_font_index, packed_unique_id, &used_font_obj);
                if (tail_reserved1) {
                    tail_reserved1->sysfont_flags = 0;
                }

                if (rc == ORBIS_OK) {
                    if (mode_low == 3) {
                        entry->obj_mode3 = used_font_obj;
                    } else if (mode_low == 2) {
                        entry->obj_mode2 = used_font_obj;
                    } else {
                        entry->obj_mode1 = used_font_obj;
                    }
                }
            }
        }
    }

    if (rc == ORBIS_OK) {
        LOG_WARNING(Lib_Font, "font file served: requested=\"{}\" served=\"{}\"", guest_name,
                    host_path_str);
        const u32 next_lock = updated_lock + 1;
        updated_lock = next_lock;

        auto* h = GetNativeFont(handle);
        if (!h) {
            const s32 err = cleanup_handle_for_error(ORBIS_FONT_ERROR_FATAL);
            LogFontOpenError(err);
            return err;
        }
        h->magic = 0x0F02;
        h->flags = static_cast<u16>(h->flags | static_cast<u16>(mode_low));
        h->open_info.unique_id_packed = packed_unique_id;
        h->open_info.ctx_entry_index = entry_index;
        h->open_info.fontset_flags = 0;
        h->open_info.fontset_record = nullptr;
        h->open_info.reserved_0x18 = 0;
        h->open_info.sub_font_index = sub_font_index;
        h->library = library;
        std::memset(&h->cached_style, 0, sizeof(h->cached_style));
        std::memset(h->reserved_0xcc, 0, sizeof(h->reserved_0xcc));
        std::memset(h->reserved_0x30, 0, sizeof(h->reserved_0x30));
        std::memset(h->reserved_0xe8, 0, sizeof(h->reserved_0xe8));
        h->prevFont = nullptr;
        h->nextFont = nullptr;

        u16 metric_buf[2] = {};
        (void)metric_fn(used_font_obj, 0x0e00, metric_buf);
        h->metricA = metric_buf[0];
        (void)metric_fn(used_font_obj, 0xea00, metric_buf);
        h->metricB = metric_buf[0];
        h->style_frame[0] = 0x48;
        h->style_frame[1] = 0x48;

        float scale = 0.0f;
        (void)scale_fn(used_font_obj, metric_buf, &scale);
        h->style_tail.scale_unit = 0;
        h->style_tail.reserved_0x0c = 0;
        h->style_tail.scale_w = scale;
        h->style_tail.scale_h = scale;
        h->style_tail.effect_weight_x = 0.0f;
        h->style_tail.effect_weight_y = 0.0f;
        h->style_tail.slant_ratio = 0.0f;
        h->style_tail.reserved_0x24 = 0.0f;

        *out_handle = handle;
    }

    {
        const u32 count = updated_lock & 0x0FFFFFFFu;
        if (mode_low == 3) {
            if (count == 0) {
                entry->obj_mode3 = nullptr;
            }
            entry->lock_mode3 = updated_lock & 0x7fffffffu;
        } else if (mode_low == 1) {
            if (count == 0) {
                entry->obj_mode1 = nullptr;
            }
            entry->lock_mode1 = updated_lock & 0x7fffffffu;
        } else {
            if (count == 0) {
                entry->obj_mode2 = nullptr;
            }
            entry->lock_mode2 = updated_lock & 0x7fffffffu;
        }

        if (count == 0 && entry->obj_mode1 == nullptr && entry->obj_mode3 == nullptr &&
            entry->obj_mode2 == nullptr) {
            entry->unique_id = 0;
            entry->active = 0;
        }
    }

    if (rc != ORBIS_OK) {
        const s32 err = cleanup_handle_for_error(rc);
        LogFontOpenError(err);
        return err;
    }

    const auto* handle_native = GetNativeFont(handle);
    if (handle_native && (handle_native->flags & 0x10) != 0) {
        auto* tail = reinterpret_cast<FontLibTail*>(lib + 1);
        auto* list_lock = &tail->list_head_ptr;
        void* list_ptr = nullptr;
        for (;;) {
            list_ptr = *list_lock;
            if (list_ptr != reinterpret_cast<void*>(std::numeric_limits<std::uintptr_t>::max())) {
                std::atomic_ref<void*> ref(*list_lock);
                void* expected = list_ptr;
                if (ref.compare_exchange_weak(
                        expected,
                        reinterpret_cast<void*>(std::numeric_limits<std::uintptr_t>::max()),
                        std::memory_order_acq_rel)) {
                    break;
                }
            }
            Libraries::Kernel::sceKernelUsleep(0x1e);
        }

        auto* head_ptr = reinterpret_cast<OrbisFontHandle*>(list_ptr);
        if (head_ptr) {
            const OrbisFontHandle old_head = *head_ptr;
            if (auto* h = GetNativeFont(handle)) {
                h->prevFont = nullptr;
                h->nextFont = old_head;
            }
            if (old_head) {
                if (auto* old_native = GetNativeFont(old_head)) {
                    old_native->prevFont = handle;
                }
            }
            *head_ptr = handle;
        }
        *list_lock = list_ptr;
    }

    ReleaseLibraryLock(lib, prev_lib_lock);

    std::vector<unsigned char> file_bytes;
    if (LoadGuestFileBytes(path_to_open, file_bytes) &&
        file_bytes.size() <= std::numeric_limits<u32>::max()) {
        auto& st = Internal::GetState(handle);
        Internal::DestroyFreeTypeFace(st.ext_ft_face);
        for (auto& fb : st.system_fallback_faces) {
            Internal::DestroyFreeTypeFace(fb.ft_face);
        }
        st = {};
        st.library = library;
        st.font_set_type = 0;
        st.system_font_path.clear();
        st.ext_face_data = std::move(file_bytes);

        const u32 chosen_index = open_detail ? open_detail->subfont_index : 0u;
        st.ext_ft_face = Internal::CreateFreeTypeFaceFromBytes(
            st.ext_face_data.data(), st.ext_face_data.size(), chosen_index);
        if (st.ext_ft_face) {
            st.ext_cache.clear();
            st.scratch.clear();
            st.logged_ext_use = false;
            st.ext_scale_for_height = 0.0f;
            st.layout_cached = false;
            const Internal::FaceMetrics m = Internal::LoadFaceMetrics(st.ext_ft_face);
            Internal::PopulateStateMetrics(st, m);
            st.ext_face_ready = true;
        } else {
            st.ext_face_ready = false;
        }
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontOpenFontInstance(OrbisFontHandle fontHandle, OrbisFontHandle setupFont,
                                         OrbisFontHandle* pFontHandle) {
    LOG_INFO(Lib_Font, "called");

    if (!fontHandle) {
        if (pFontHandle) {
            *pFontHandle = nullptr;
        }
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    if (!setupFont && !pFontHandle) {
        LOG_ERROR(Lib_Font, "INVALID_PARAMETER");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    LOG_DEBUG(Lib_Font, "{}",
              formatParams({
                  Param("fontHandle", fontHandle),
                  Param("setupFont", setupFont),
                  Param("pFontHandle", pFontHandle),
              }));

    auto* src = GetNativeFont(fontHandle);
    if (!src || src->magic != 0x0F02) {
        if (pFontHandle) {
            *pFontHandle = nullptr;
        }
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    u32 prev_font_lock = 0;
    if (!AcquireFontLock(src, prev_font_lock)) {
        if (pFontHandle) {
            *pFontHandle = nullptr;
        }
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    auto release_src_lock = [&] { ReleaseFontLock(src, prev_font_lock); };

    auto* lib = static_cast<FontLibOpaque*>(src->library);
    if (!lib || lib->magic != 0x0F01) {
        release_src_lock();
        if (pFontHandle) {
            *pFontHandle = nullptr;
        }
        LOG_ERROR(Lib_Font, "INVALID_LIBRARY");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    using AllocFn = void*(PS4_SYSV_ABI*)(void* object, u32 size);
    using FreeFn = void(PS4_SYSV_ABI*)(void* object, void* p);
    const auto alloc_fn = lib->alloc_vtbl ? reinterpret_cast<AllocFn>(lib->alloc_vtbl[0]) : nullptr;
    const auto free_fn = lib->alloc_vtbl ? reinterpret_cast<FreeFn>(lib->alloc_vtbl[1]) : nullptr;

    const u32 open_mode_low = static_cast<u32>(src->flags) & 0x0Fu;
    const u32 src_sub_font_index = src->open_info.sub_font_index;

    const bool publishable_sysfont_instance = false;

    const auto* fontset_record =
        static_cast<const Internal::FontSetRecordHeader*>(src->open_info.fontset_record);
    const u32* entry_indices = nullptr;
    u32 entry_count = 0;
    u8* ctx = nullptr;

    if (!fontset_record) {
        entry_count = 1;
        entry_indices = &src->open_info.ctx_entry_index;
        ctx = static_cast<u8*>(lib->external_fonts_ctx);
    } else {
        entry_count = fontset_record->entry_count;
        entry_indices = reinterpret_cast<const u32*>(fontset_record + 1);
        ctx = static_cast<u8*>(lib->sysfonts_ctx);
    }

    if (!publishable_sysfont_instance && (!ctx || !entry_indices || entry_count == 0)) {
        release_src_lock();
        if (pFontHandle) {
            *pFontHandle = nullptr;
        }
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    OrbisFontHandle out_handle = setupFont;
    bool owned = false;
    if (!out_handle) {
        if (!alloc_fn || !free_fn) {
            release_src_lock();
            if (pFontHandle) {
                *pFontHandle = nullptr;
            }
            LOG_ERROR(Lib_Font, "INVALID_LIBRARY");
            return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
        }
        out_handle =
            static_cast<OrbisFontHandle>(Core::ExecuteGuest(alloc_fn, lib->alloc_ctx, 0x100));
        if (!out_handle) {
            release_src_lock();
            if (pFontHandle) {
                *pFontHandle = nullptr;
            }
            LOG_ERROR(Lib_Font, "ALLOCATION_FAILED");
            return ORBIS_FONT_ERROR_ALLOCATION_FAILED;
        }
        std::memset(out_handle, 0, 0x100);
        if (auto* out_native = GetNativeFont(out_handle)) {
            out_native->flags = 0x10;
        }
        owned = true;
    } else {
        if (auto* out_native = GetNativeFont(out_handle)) {
            out_native->flags = 0;
        }
        Internal::RemoveState(out_handle);
    }

    std::memcpy(out_handle, fontHandle, 0x100);
    auto* dst = GetNativeFont(out_handle);
    dst->lock_word = 0;
    dst->cached_style.cache_lock_word = 0;
    dst->flags = static_cast<u16>((dst->flags & 0xFF0Fu) | (owned ? 0x10u : 0u));

    if (!publishable_sysfont_instance) {
        auto* ctx_header = reinterpret_cast<Internal::FontCtxHeader*>(ctx);
        auto* entries_base = ctx_header ? static_cast<u8*>(ctx_header->base) : nullptr;
        const u32 max_entries = ctx_header ? ctx_header->max_entries : 0u;
        if (!entries_base || entry_count > max_entries) {
            dst->magic = 0;
            if (owned && free_fn) {
                Core::ExecuteGuest(free_fn, lib->alloc_ctx, out_handle);
            }
            release_src_lock();
            if (pFontHandle) {
                *pFontHandle = nullptr;
            }
            LOG_ERROR(Lib_Font, "INVALID_PARAMETER");
            return ORBIS_FONT_ERROR_INVALID_PARAMETER;
        }

        struct IncRecord {
            u32 entry_index;
            Internal::FontObj* node;
        };
        std::vector<IncRecord> increments;
        increments.reserve(entry_count);

        auto lock_word_ptr_for_mode = [&](Internal::FontCtxEntry* entry, u32 m) -> u32* {
            if (!entry) {
                return nullptr;
            }
            if (m == 3) {
                return &entry->lock_mode3;
            }
            if (m == 1) {
                return &entry->lock_mode1;
            }
            if (m == 2) {
                return &entry->lock_mode2;
            }
            return nullptr;
        };
        auto obj_slot_for_mode = [&](Internal::FontCtxEntry* entry, u32 m) -> void** {
            if (!entry) {
                return nullptr;
            }
            if (m == 3) {
                return &entry->obj_mode3;
            }
            if (m == 1) {
                return &entry->obj_mode1;
            }
            if (m == 2) {
                return &entry->obj_mode2;
            }
            return nullptr;
        };

        s32 rc = ORBIS_OK;
        for (u32 i = 0; i < entry_count; i++) {
            const u32 entry_index = entry_indices[i];
            if (static_cast<s32>(entry_index) < 0) {
                continue;
            }
            if (entry_index >= max_entries) {
                rc = ORBIS_FONT_ERROR_INVALID_PARAMETER;
                break;
            }

            auto* entry = reinterpret_cast<Internal::FontCtxEntry*>(
                entries_base + entry_index * sizeof(Internal::FontCtxEntry));
            auto* lock_word_ptr = lock_word_ptr_for_mode(entry, open_mode_low);
            auto** obj_slot = obj_slot_for_mode(entry, open_mode_low);
            if (!lock_word_ptr || !obj_slot) {
                rc = ORBIS_FONT_ERROR_INVALID_PARAMETER;
                break;
            }

            u32 prev_lock = 0;
            for (;;) {
                const u32 lw = *lock_word_ptr;
                if (static_cast<s32>(lw) >= 0) {
                    std::atomic_ref<u32> ref(*lock_word_ptr);
                    u32 expected = lw;
                    if (ref.compare_exchange_weak(expected, lw | 0x80000000u,
                                                  std::memory_order_acq_rel)) {
                        prev_lock = lw;
                        break;
                    }
                }
                Libraries::Kernel::sceKernelUsleep(0x1e);
            }

            auto unlock_entry = [&](u32 updated) { *lock_word_ptr = updated & 0x7fffffffu; };

            if ((prev_lock & 0x40000000u) == 0) {
                unlock_entry(prev_lock);
                rc = ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
                break;
            }
            if ((prev_lock & 0x0FFFFFFFu) == 0x0FFFFFFFu) {
                unlock_entry(prev_lock);
                rc = ORBIS_FONT_ERROR_FONT_OPEN_MAX;
                break;
            }

            auto* node = static_cast<Internal::FontObj*>(*obj_slot);
            for (; node != nullptr; node = node->next) {
                if (node->sub_font_index == src_sub_font_index) {
                    break;
                }
            }
            if (!node) {
                unlock_entry(prev_lock);
                rc = ORBIS_FONT_ERROR_FATAL;
                break;
            }

            node->refcount++;
            unlock_entry(prev_lock + 1);
            increments.push_back({entry_index, node});
        }

        if (rc != ORBIS_OK) {
            for (const auto& inc : increments) {
                if (inc.entry_index >= max_entries || !inc.node) {
                    continue;
                }
                auto* entry = reinterpret_cast<Internal::FontCtxEntry*>(
                    entries_base + inc.entry_index * sizeof(Internal::FontCtxEntry));
                auto* lock_word_ptr = lock_word_ptr_for_mode(entry, open_mode_low);
                if (!lock_word_ptr) {
                    continue;
                }
                u32 prev_lock = 0;
                for (;;) {
                    const u32 lw = *lock_word_ptr;
                    if (static_cast<s32>(lw) >= 0) {
                        std::atomic_ref<u32> ref(*lock_word_ptr);
                        u32 expected = lw;
                        if (ref.compare_exchange_weak(expected, lw | 0x80000000u,
                                                      std::memory_order_acq_rel)) {
                            prev_lock = lw;
                            break;
                        }
                    }
                    Libraries::Kernel::sceKernelUsleep(0x1e);
                }
                if ((prev_lock & 0x0FFFFFFFu) != 0) {
                    inc.node->refcount--;
                    *lock_word_ptr = (prev_lock - 1) & 0x7fffffffu;
                } else {
                    *lock_word_ptr = prev_lock & 0x7fffffffu;
                }
            }

            dst->magic = 0;
            dst->open_info.unique_id_packed = 0;
            dst->open_info.ctx_entry_index = 0;
            dst->open_info.sub_font_index = 0;
            dst->open_info.fontset_flags = 0;
            dst->open_info.fontset_record = nullptr;
            dst->open_info.reserved_0x18 = 0;
            dst->library = lib;
            const u16 prev_flags = dst->flags;
            dst->flags = 0;
            if ((prev_flags & 0x10) != 0 && free_fn) {
                Core::ExecuteGuest(free_fn, lib->alloc_ctx, out_handle);
            }
            Internal::RemoveState(out_handle);

            release_src_lock();
            if (pFontHandle) {
                *pFontHandle = nullptr;
            }
            LOG_ERROR(Lib_Font, "FAILED");
            return rc;
        }
    }

    if (owned) {
        auto* tail = reinterpret_cast<FontLibTail*>(lib + 1);
        auto* list_lock = tail ? &tail->list_head_ptr : nullptr;
        void* list_ptr = nullptr;
        for (;;) {
            list_ptr = list_lock ? *list_lock : nullptr;
            if (list_ptr != reinterpret_cast<void*>(std::numeric_limits<std::uintptr_t>::max())) {
                std::atomic_ref<void*> ref(*list_lock);
                void* expected = list_ptr;
                if (ref.compare_exchange_weak(
                        expected,
                        reinterpret_cast<void*>(std::numeric_limits<std::uintptr_t>::max()),
                        std::memory_order_acq_rel)) {
                    break;
                }
            }
            Libraries::Kernel::sceKernelUsleep(0x1e);
        }

        auto* head_ptr = reinterpret_cast<OrbisFontHandle*>(list_ptr);
        if (head_ptr) {
            const OrbisFontHandle old_head = *head_ptr;
            if (auto* out_native = GetNativeFont(out_handle)) {
                out_native->prevFont = nullptr;
                out_native->nextFont = old_head;
            }
            if (old_head) {
                if (auto* old_native = GetNativeFont(old_head)) {
                    old_native->prevFont = out_handle;
                }
            }
            *head_ptr = out_handle;
        }
        *list_lock = list_ptr;
    }

    release_src_lock();

    if (auto* src_state = Internal::TryGetState(fontHandle)) {
        auto& dst_state = Internal::GetState(out_handle);
        dst_state = *src_state;
        dst_state.ext_cache.clear();
        dst_state.scratch.clear();
        dst_state.logged_ext_use = false;

        const u32 subfont_index = out_handle && GetNativeFont(out_handle)
                                      ? GetNativeFont(out_handle)->open_info.sub_font_index
                                      : 0u;

        dst_state.ext_scale_for_height = 0.0f;
        dst_state.layout_cached = false;
        dst_state.ext_face_ready = false;
        dst_state.ext_ft_face = nullptr;

        if (!dst_state.ext_face_data.empty()) {
            dst_state.ext_ft_face = Internal::CreateFreeTypeFaceFromBytes(
                dst_state.ext_face_data.data(), dst_state.ext_face_data.size(), subfont_index);
        }
        if (dst_state.ext_ft_face) {
            const Internal::FaceMetrics m = Internal::LoadFaceMetrics(dst_state.ext_ft_face);
            Internal::PopulateStateMetrics(dst_state, m);
            dst_state.ext_face_ready = true;
        }

        for (auto& fb : dst_state.system_fallback_faces) {
            fb.ft_face = nullptr;
            fb.ready = false;
            if (!fb.bytes || fb.bytes->empty()) {
                continue;
            }
            fb.ft_face = Internal::CreateFreeTypeFaceFromBytes(fb.bytes->data(), fb.bytes->size(),
                                                               subfont_index);
            fb.ready = (fb.ft_face != nullptr);
        }
    } else {
        auto& dst_state = Internal::GetState(out_handle);
        Internal::DestroyFreeTypeFace(dst_state.ext_ft_face);
        for (auto& fb : dst_state.system_fallback_faces) {
            Internal::DestroyFreeTypeFace(fb.ft_face);
        }
        dst_state = {};
        dst_state.library = static_cast<OrbisFontLib>(src->library);
        if (fontset_record) {
            dst_state.font_set_type = fontset_record->font_set_type;
            dst_state.system_requested = true;
        }
    }

    if (pFontHandle) {
        *pFontHandle = out_handle;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontOpenFontMemory(OrbisFontLib library, const void* fontAddress, u32 fontSize,
                                       const OrbisFontOpenParams* open_params,
                                       OrbisFontHandle* pFontHandle) {
    LOG_INFO(Lib_Font, "called");

    LOG_DEBUG(Lib_Font, "{}",
              formatParams({
                  Param("library", library),
                  Param("fontAddress", fontAddress),
                  Param("fontSize", fontSize),
                  Param("open_params", open_params),
                  Param("pFontHandle", pFontHandle),
              }));

    s32 rc = ORBIS_FONT_ERROR_INVALID_LIBRARY;
    if (!library) {
        if (pFontHandle) {
            *pFontHandle = nullptr;
        }
        LOG_ERROR(Lib_Font, "INVALID_LIBRARY");
        return rc;
    }

    auto* lib = static_cast<FontLibOpaque*>(library);
    if (lib->magic != 0x0F01) {
        if (pFontHandle) {
            *pFontHandle = nullptr;
        }
        LOG_ERROR(Lib_Font, "INVALID_LIBRARY");
        return rc;
    }

    u32 prev_lib_lock = 0;
    if (!AcquireLibraryLock(lib, prev_lib_lock)) {
        if (pFontHandle) {
            *pFontHandle = nullptr;
        }
        LOG_ERROR(Lib_Font, "INVALID_LIBRARY");
        return ORBIS_FONT_ERROR_INVALID_LIBRARY;
    }

    auto release_library_and_clear_out = [&] {
        ReleaseLibraryLock(lib, prev_lib_lock);
        if (pFontHandle) {
            *pFontHandle = nullptr;
        }
    };

    if (!lib->fontset_registry || !lib->sys_driver) {
        release_library_and_clear_out();
        LOG_ERROR(Lib_Font, "INVALID_LIBRARY");
        return ORBIS_FONT_ERROR_INVALID_LIBRARY;
    }

    using AllocFn = void*(PS4_SYSV_ABI*)(void* object, u32 size);
    using FreeFn = void(PS4_SYSV_ABI*)(void* object, void* p);
    const auto alloc_fn = lib->alloc_vtbl ? reinterpret_cast<AllocFn>(lib->alloc_vtbl[0]) : nullptr;
    const auto free_fn = lib->alloc_vtbl ? reinterpret_cast<FreeFn>(lib->alloc_vtbl[1]) : nullptr;
    if (!alloc_fn || !free_fn) {
        release_library_and_clear_out();
        LOG_ERROR(Lib_Font, "INVALID_LIBRARY");
        return ORBIS_FONT_ERROR_INVALID_LIBRARY;
    }

    auto* ext_ctx = static_cast<u8*>(lib->external_fonts_ctx);
    if (!ext_ctx) {
        release_library_and_clear_out();
        LOG_ERROR(Lib_Font, "NO_SUPPORT_FUNCTION");
        return ORBIS_FONT_ERROR_NO_SUPPORT_FUNCTION;
    }

    const u32 sub_font_index = open_params ? open_params->subfont_index : 0u;
    const s32 unique_id = open_params ? open_params->unique_id : -1;
    if (!fontAddress || fontSize == 0 || unique_id < -1 || !pFontHandle) {
        release_library_and_clear_out();
        LOG_ERROR(Lib_Font, "INVALID_PARAMETER");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    LOG_WARNING(Lib_Font, "font memory requested: addr={} size={} subfont_index={} unique_id={}",
                static_cast<const void*>(fontAddress), fontSize, sub_font_index, unique_id);

    OrbisFontHandle handle = *pFontHandle;
    if (handle) {
        if (auto* h = GetNativeFont(handle)) {
            h->flags = 0;
        }
        Internal::RemoveState(handle);
    } else {
        handle = static_cast<OrbisFontHandle>(Core::ExecuteGuest(alloc_fn, lib->alloc_ctx, 0x100));
        if (!handle) {
            release_library_and_clear_out();
            LOG_ERROR(Lib_Font, "ALLOCATION_FAILED");
            return ORBIS_FONT_ERROR_ALLOCATION_FAILED;
        }
        std::memset(handle, 0, 0x100);
        if (auto* h = GetNativeFont(handle)) {
            h->flags = 0x10;
        }
    }

    auto cleanup_handle_for_error = [&](s32 err) {
        auto* h = GetNativeFont(handle);
        if (!h) {
            ReleaseLibraryLock(lib, prev_lib_lock);
            *pFontHandle = nullptr;
            return err;
        }
        h->magic = 0;
        h->open_info.unique_id_packed = 0;
        h->open_info.ctx_entry_index = 0;
        h->open_info.sub_font_index = 0;
        h->open_info.fontset_flags = 0;
        h->open_info.fontset_record = nullptr;
        h->open_info.reserved_0x18 = 0;
        h->library = library;
        const u16 prev_flags = h->flags;
        h->flags = 0;
        if ((prev_flags & 0x10) != 0) {
            Core::ExecuteGuest(free_fn, lib->alloc_ctx, handle);
        }
        Internal::RemoveState(handle);
        ReleaseLibraryLock(lib, prev_lib_lock);
        *pFontHandle = nullptr;
        return err;
    };

    auto* ext_header = reinterpret_cast<Internal::FontCtxHeader*>(ext_ctx);

    auto* ctx_lock_word = &ext_header->lock_word;
    for (;;) {
        const u32 lw = *ctx_lock_word;
        if (static_cast<s32>(lw) >= 0) {
            std::atomic_ref<u32> ref(*ctx_lock_word);
            u32 expected = lw;
            if (ref.compare_exchange_weak(expected, lw | 0x80000000u, std::memory_order_acq_rel)) {
                break;
            }
        }
        Libraries::Kernel::sceKernelUsleep(0x1e);
    }

    const u32 font_max = ext_header->max_entries;
    auto* entries_base = static_cast<Internal::FontCtxEntry*>(ext_header->base);

    u32 entry_index = 0xffffffffu;
    if (font_max != 0 && entries_base) {
        const u32 unique_u32 = static_cast<u32>(unique_id);
        u32 first_free = 0xffffffffu;
        for (u32 i = 0; i < font_max; i++) {
            const auto* entry = &entries_base[i];
            const u32 active = entry->active;
            const u32 stored_id = entry->unique_id;
            const u64 stored_addr = entry->font_address;

            if (((active != 0) && (stored_id == unique_u32)) ||
                stored_addr == reinterpret_cast<u64>(fontAddress)) {
                entry_index = i;
                break;
            }
            if (first_free == 0xffffffffu && active == 0) {
                first_free = i;
            }
        }

        if (entry_index == 0xffffffffu) {
            entry_index = first_free;
            if (entry_index != 0xffffffffu) {
                auto* entry = &entries_base[entry_index];
                const u32 stored_id =
                    (unique_u32 != 0xffffffffu) ? unique_u32 : (entry_index ^ 0x80000000u);
                entry->reserved_0x00 = 0;
                entry->active = 1;
                entry->font_address = reinterpret_cast<u64>(fontAddress);
                entry->unique_id = stored_id;
                entry->lock_mode1 = 0;
                entry->lock_mode3 = 0;
                entry->lock_mode2 = 0;
                entry->obj_mode1 = nullptr;
                entry->obj_mode3 = nullptr;
                entry->obj_mode2 = nullptr;
                std::memset(entry->reserved_0x38, 0, sizeof(entry->reserved_0x38));
            }
        }
    }

    *ctx_lock_word = 0;

    if (entry_index == 0xffffffffu) {
        const s32 err = cleanup_handle_for_error(ORBIS_FONT_ERROR_FONT_OPEN_MAX);
        LogFontOpenError(err);
        return err;
    }

    const u32 uvar15 = (unique_id == -1) ? (entry_index + 0x40000000u)
                                         : (static_cast<u32>(unique_id) | 0x80000000u);

    auto* entry = &entries_base[entry_index];
    auto* entry_lock_word = &entry->lock_mode1;
    u32 entry_prev = 0;
    u32 entry_locked = 0;
    for (;;) {
        entry_prev = *entry_lock_word;
        if (static_cast<s32>(entry_prev) >= 0) {
            std::atomic_ref<u32> ref(*entry_lock_word);
            u32 expected = entry_prev;
            const u32 desired = entry_prev | 0x80000000u;
            if (ref.compare_exchange_weak(expected, desired, std::memory_order_acq_rel)) {
                entry_locked = desired;
                break;
            }
        }
        Libraries::Kernel::sceKernelUsleep(0x1e);
    }

    auto release_entry_lock = [&] {
        if ((entry_locked & 0x0FFFFFFFu) == 0) {
            entry->obj_mode1 = nullptr;
            if (entry->obj_mode3 == nullptr && entry->obj_mode2 == nullptr) {
                entry->unique_id = 0;
                entry->active = 0;
            }
        }
        *entry_lock_word = entry_locked & 0x7fffffffu;
    };

    void* font_obj = entry->obj_mode1;

    const auto* driver =
        lib->sys_driver ? reinterpret_cast<const Internal::SysDriver*>(lib->sys_driver) : nullptr;
    const auto open_fn = driver ? driver->open : nullptr;
    const auto metric_fn = driver ? driver->metric : nullptr;
    const auto scale_fn = driver ? driver->scale : nullptr;
    if (!open_fn || !metric_fn || !scale_fn) {
        release_entry_lock();
        const s32 err = cleanup_handle_for_error(ORBIS_FONT_ERROR_INVALID_LIBRARY);
        LogFontOpenError(err);
        return err;
    }

    rc = ORBIS_OK;
    bool need_open = (entry_prev & 0x40000000u) == 0;

    if (!need_open) {
        rc = ORBIS_FONT_ERROR_FONT_OPEN_MAX;
        if ((entry_prev & 0x0FFFFFFFu) != 0x0FFFFFFFu) {
            for (auto* node = static_cast<Internal::FontObj*>(font_obj); node != nullptr;
                 node = node->next) {
                if (node->sub_font_index == sub_font_index) {
                    node->refcount++;
                    font_obj = node;
                    rc = ORBIS_OK;
                    break;
                }
            }
            if (rc != ORBIS_OK) {
                need_open = true;
            }
        }
    }

    if (need_open) {
        auto* tail = lib + 1;
        auto* tail_reserved1 =
            tail ? reinterpret_cast<FontLibReserved1SysfontTail*>(tail->reserved1) : nullptr;
        const u32 lib_flags = lib->flags;
        if ((lib_flags & 0x200000u) != 0) {
            tail_reserved1->sysfont_flags |= 1;
        }
        if ((lib_flags & 0x20000u) != 0) {
            tail_reserved1->sysfont_flags |= 2;
        }

        rc = open_fn(library, 1, fontAddress, fontSize, sub_font_index, uvar15, &font_obj);
        if (tail_reserved1) {
            tail_reserved1->sysfont_flags = 0;
        }

        if (rc == ORBIS_OK) {
            entry->obj_mode1 = font_obj;
            entry_locked = entry_prev | 0xC0000000u;
        }
    }

    if (rc == ORBIS_OK) {
        auto* h = GetNativeFont(handle);
        if (!h) {
            release_entry_lock();
            const s32 err = cleanup_handle_for_error(ORBIS_FONT_ERROR_FATAL);
            LogFontOpenError(err);
            return err;
        }
        h->magic = 0x0F02;
        h->flags = static_cast<u16>(h->flags | 1);
        entry_locked = entry_locked + 1;
        h->open_info.unique_id_packed = uvar15;
        h->open_info.ctx_entry_index = entry_index;
        h->open_info.fontset_flags = 0;
        h->open_info.fontset_record = nullptr;
        h->open_info.reserved_0x18 = 0;
        h->open_info.sub_font_index = sub_font_index;
        h->library = library;
        std::memset(&h->cached_style, 0, sizeof(h->cached_style));
        std::memset(h->reserved_0xcc, 0, sizeof(h->reserved_0xcc));
        std::memset(h->reserved_0x30, 0, sizeof(h->reserved_0x30));
        std::memset(h->reserved_0xe8, 0, sizeof(h->reserved_0xe8));
        h->prevFont = nullptr;
        h->nextFont = nullptr;

        u16 metric_buf[2] = {};
        (void)metric_fn(font_obj, 0x0e00, metric_buf);
        h->metricA = metric_buf[0];
        (void)metric_fn(font_obj, 0xea00, metric_buf);
        h->metricB = metric_buf[0];
        h->style_frame[0] = 0x48;
        h->style_frame[1] = 0x48;

        float scale = 0.0f;
        (void)scale_fn(font_obj, metric_buf, &scale);
        h->style_tail.scale_unit = 0;
        h->style_tail.reserved_0x0c = 0;
        h->style_tail.scale_w = scale;
        h->style_tail.scale_h = scale;
        h->style_tail.effect_weight_x = 0.0f;
        h->style_tail.effect_weight_y = 0.0f;
        h->style_tail.slant_ratio = 0.0f;
        h->style_tail.reserved_0x24 = 0.0f;

        *pFontHandle = handle;
    }

    release_entry_lock();

    if (rc != ORBIS_OK) {
        const s32 err = cleanup_handle_for_error(rc);
        LogFontOpenError(err);
        return err;
    }

    const auto* handle_native = GetNativeFont(handle);
    if (handle_native && (handle_native->flags & 0x10) != 0) {
        auto* tail = reinterpret_cast<FontLibTail*>(lib + 1);
        auto* list_lock = &tail->list_head_ptr;
        void* list_ptr = nullptr;
        for (;;) {
            list_ptr = *list_lock;
            if (list_ptr != reinterpret_cast<void*>(std::numeric_limits<std::uintptr_t>::max())) {
                std::atomic_ref<void*> ref(*list_lock);
                void* expected = list_ptr;
                if (ref.compare_exchange_weak(
                        expected,
                        reinterpret_cast<void*>(std::numeric_limits<std::uintptr_t>::max()),
                        std::memory_order_acq_rel)) {
                    break;
                }
            }
            Libraries::Kernel::sceKernelUsleep(0x1e);
        }

        auto* head_ptr = reinterpret_cast<OrbisFontHandle*>(list_ptr);
        if (head_ptr) {
            const OrbisFontHandle old_head = *head_ptr;
            if (auto* h = GetNativeFont(handle)) {
                h->prevFont = nullptr;
                h->nextFont = old_head;
            }
            if (old_head) {
                if (auto* old_native = GetNativeFont(old_head)) {
                    old_native->prevFont = handle;
                }
            }
            *head_ptr = handle;
        }
        *list_lock = list_ptr;
    }

    ReleaseLibraryLock(lib, prev_lib_lock);

    auto& st = Internal::GetState(handle);
    Internal::DestroyFreeTypeFace(st.ext_ft_face);
    for (auto& fb : st.system_fallback_faces) {
        Internal::DestroyFreeTypeFace(fb.ft_face);
    }
    st = {};
    st.library = library;
    st.font_set_type = 0;
    st.system_font_path.clear();
    {
        const u8* src = static_cast<const u8*>(fontAddress);
        st.ext_face_data.assign(src, src + fontSize);
    }

    const u32 chosen_index = open_params ? open_params->subfont_index : 0u;
    st.ext_ft_face = Internal::CreateFreeTypeFaceFromBytes(st.ext_face_data.data(),
                                                           st.ext_face_data.size(), chosen_index);

    if (st.ext_ft_face) {
        st.ext_cache.clear();
        st.scratch.clear();
        st.logged_ext_use = false;
        st.ext_scale_for_height = 0.0f;
        st.layout_cached = false;
        const Internal::FaceMetrics m = Internal::LoadFaceMetrics(st.ext_ft_face);
        Internal::PopulateStateMetrics(st, m);
        st.ext_face_ready = true;
    } else {
        st.ext_face_ready = false;
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontOpenFontSet(OrbisFontLib library, u32 fontSetType, u32 openMode,
                                    const OrbisFontOpenParams* open_params,
                                    OrbisFontHandle* pFontHandle) {
    LOG_INFO(Lib_Font, "called");
    LOG_DEBUG(Lib_Font, "{}",
              formatParams({
                  Param("library", library),
                  Param("fontSetType", fontSetType),
                  Param("openMode", openMode),
                  Param("open_params", open_params),
                  Param("pFontHandle", pFontHandle),
              }));

    {
        auto* lib_local = static_cast<FontLibOpaque*>(library);
        if (!lib_local || lib_local->magic != 0x0F01) {
            if (pFontHandle) {
                *pFontHandle = nullptr;
            }
            LOG_ERROR(Lib_Font, "INVALID_LIBRARY");
            return ORBIS_FONT_ERROR_INVALID_LIBRARY;
        }
        if (!pFontHandle) {
            LOG_ERROR(Lib_Font, "INVALID_PARAMETER");
            return ORBIS_FONT_ERROR_INVALID_PARAMETER;
        }

        u32 prev_lib_lock = 0;
        if (!AcquireLibraryLock(lib_local, prev_lib_lock)) {
            *pFontHandle = nullptr;
            LOG_ERROR(Lib_Font, "INVALID_LIBRARY");
            return ORBIS_FONT_ERROR_INVALID_LIBRARY;
        }
        const auto release_library_and_clear_out = [&](s32 err) -> s32 {
            ReleaseLibraryLock(lib_local, prev_lib_lock);
            *pFontHandle = nullptr;
            return err;
        };

        if (openMode != 1 && openMode != 2 && openMode != 3) {
            LOG_ERROR(Lib_Font, "INVALID_PARAMETER");
            return release_library_and_clear_out(ORBIS_FONT_ERROR_INVALID_PARAMETER);
        }
        const u32 mode_low = openMode & 0x0Fu;
        if (!lib_local->fontset_registry || !lib_local->sys_driver) {
            LOG_ERROR(Lib_Font, "INVALID_LIBRARY");
            return release_library_and_clear_out(ORBIS_FONT_ERROR_INVALID_LIBRARY);
        }

        using AllocFn = void*(PS4_SYSV_ABI*)(void* object, u32 size);
        using FreeFn = void(PS4_SYSV_ABI*)(void* object, void* p);
        const auto alloc_fn =
            lib_local->alloc_vtbl ? reinterpret_cast<AllocFn>(lib_local->alloc_vtbl[0]) : nullptr;
        const auto free_fn =
            lib_local->alloc_vtbl ? reinterpret_cast<FreeFn>(lib_local->alloc_vtbl[1]) : nullptr;
        if (!alloc_fn || !free_fn) {
            LOG_ERROR(Lib_Font, "INVALID_MEMORY");
            return release_library_and_clear_out(ORBIS_FONT_ERROR_INVALID_MEMORY);
        }

        // System font sets always use sub-font-index 0.
        const u32 sub_font_index = 0u;

        if (!lib_local->sysfonts_ctx) {
            LOG_ERROR(Lib_Font, "NO_SUPPORT_FUNCTION");
            return release_library_and_clear_out(ORBIS_FONT_ERROR_NO_SUPPORT_FUNCTION);
        }

        std::filesystem::path primary_path =
            Internal::ResolveSystemFontPathFromConfigOnly(fontSetType);
        if (primary_path.empty()) {
            primary_path = Internal::ResolveSystemFontPath(fontSetType);
        }
        if (primary_path.empty()) {
            LOG_ERROR(Lib_Font, "NO_SUPPORT_FONTSET");
            return release_library_and_clear_out(ORBIS_FONT_ERROR_NO_SUPPORT_FONTSET);
        }
        LOG_INFO(Lib_Font, "OpenFontSet: stage=resolved_primary_path");

        std::vector<unsigned char> primary_bytes;
        if (!Internal::LoadFontFile(primary_path, primary_bytes)) {
            LOG_ERROR(Lib_Font, "FONT_OPEN_FAILED path='{}' sysFontPath='{}'",
                      primary_path.string(), Config::getSysFontPath().string());
            return release_library_and_clear_out(ORBIS_FONT_ERROR_FONT_OPEN_FAILED);
        }
        LOG_INFO(Lib_Font, "OpenFontSet: stage=loaded_primary_bytes");

        OrbisFontHandle handle = *pFontHandle;
        const bool had_existing_handle = (handle != nullptr);
        if (handle) {
            if (auto* h = GetNativeFont(handle)) {
                h->flags = 0;
            }
            Internal::RemoveState(handle);
        } else {
            handle = static_cast<OrbisFontHandle>(
                Core::ExecuteGuest(alloc_fn, lib_local->alloc_ctx, 0x100));
            if (!handle) {
                LOG_ERROR(Lib_Font, "ALLOCATION_FAILED");
                return release_library_and_clear_out(ORBIS_FONT_ERROR_ALLOCATION_FAILED);
            }
            std::memset(handle, 0, 0x100);
            if (auto* h = GetNativeFont(handle)) {
                h->flags = 0x10;
            }
        }
        LOG_INFO(Lib_Font, "OpenFontSet: stage=handle_ready");

        auto cleanup_handle_for_error = [&](s32 rc) -> s32 {
            auto* h = GetNativeFont(handle);
            if (!h) {
                return release_library_and_clear_out(rc);
            }
            const u16 prev_flags = h->flags;
            h->magic = 0;
            h->open_info.unique_id_packed = 0;
            h->open_info.ctx_entry_index = 0;
            h->open_info.sub_font_index = 0;
            h->open_info.fontset_flags = 0;
            h->open_info.fontset_record = nullptr;
            h->open_info.reserved_0x18 = 0;
            h->library = library;
            h->flags = 0;
            if ((prev_flags & 0x10) != 0) {
                Core::ExecuteGuest(free_fn, lib_local->alloc_ctx, handle);
            }
            Internal::RemoveState(handle);
            return release_library_and_clear_out(rc);
        };

        auto* h = GetNativeFont(handle);
        if (!h) {
            return cleanup_handle_for_error(ORBIS_FONT_ERROR_FATAL);
        }

        h->magic = 0;
        h->flags = static_cast<u16>(h->flags & 0x10);
        h->open_info.unique_id_packed = 0;
        h->open_info.ctx_entry_index = 0;
        h->open_info.fontset_flags = 0;
        h->open_info.fontset_record = nullptr;
        h->open_info.reserved_0x18 = 0;
        h->open_info.sub_font_index = sub_font_index;
        h->library = library;
        std::memset(&h->cached_style, 0, sizeof(h->cached_style));
        std::memset(h->reserved_0xcc, 0, sizeof(h->reserved_0xcc));
        std::memset(h->reserved_0x30, 0, sizeof(h->reserved_0x30));
        std::memset(h->reserved_0xe8, 0, sizeof(h->reserved_0xe8));
        h->prevFont = nullptr;
        h->nextFont = nullptr;
        h->style_frame[0] = 0x48;
        h->style_frame[1] = 0x48;
        h->style_tail.scale_unit = 0;
        h->style_tail.reserved_0x0c = 0;
        {
            const auto* driver =
                reinterpret_cast<const Internal::SysDriver*>(lib_local->sys_driver);
            const auto pixel_res_fn = driver ? driver->pixel_resolution : nullptr;
            const u32 pixel_res = pixel_res_fn ? pixel_res_fn() : 0;
            const float scale = (pixel_res != 0) ? (1000.0f / static_cast<float>(pixel_res)) : 0.0f;
            h->style_tail.scale_w = scale;
            h->style_tail.scale_h = scale;
        }
        h->style_tail.effect_weight_x = 0.0f;
        h->style_tail.effect_weight_y = 0.0f;
        h->style_tail.slant_ratio = 0.0f;
        h->style_tail.reserved_0x24 = 0.0f;

        auto& st = Internal::GetState(handle);
        Internal::DestroyFreeTypeFace(st.ext_ft_face);
        for (auto& fb : st.system_fallback_faces) {
            Internal::DestroyFreeTypeFace(fb.ft_face);
        }
        st = {};
        st.library = library;
        st.font_set_type = fontSetType;
        st.system_font_path = primary_path;
        st.system_requested = true;
        st.system_font_id = 0;
        st.system_font_scale_factor = 1.0f;
        st.system_font_shift_value = 0;
        st.ext_face_data = std::move(primary_bytes);
        st.ext_ft_face = Internal::CreateFreeTypeFaceFromBytes(
            st.ext_face_data.data(), st.ext_face_data.size(), sub_font_index);
        if (!st.ext_ft_face) {
            LOG_ERROR(Lib_Font, "FONT_OPEN_FAILED");
            return cleanup_handle_for_error(ORBIS_FONT_ERROR_FONT_OPEN_FAILED);
        }
        LOG_INFO(Lib_Font, "OpenFontSet: stage=created_primary_face");
        const Internal::FaceMetrics m = Internal::LoadFaceMetrics(st.ext_ft_face);
        Internal::PopulateStateMetrics(st, m);
        st.ext_face_ready = true;

        {
            const u16 units = static_cast<u16>(st.ext_ft_face->units_per_EM);
            h->metricA = units;
            u16 ascender = units;
            if (const TT_OS2* os2 =
                    static_cast<const TT_OS2*>(FT_Get_Sfnt_Table(st.ext_ft_face, ft_sfnt_os2))) {
                ascender = static_cast<u16>(os2->sTypoAscender);
            }
            h->metricB = ascender;

            const auto* driver =
                reinterpret_cast<const Internal::SysDriver*>(lib_local->sys_driver);
            const auto pixel_res_fn = driver ? driver->pixel_resolution : nullptr;
            const u32 pixel_res = pixel_res_fn ? pixel_res_fn() : 0;
            const float scale = (pixel_res != 0)
                                    ? (static_cast<float>(units) / static_cast<float>(pixel_res))
                                    : 0.0f;
            h->style_tail.scale_w = scale;
            h->style_tail.scale_h = scale;
        }

        auto compute_sysfont_scale_factor = [](FT_Face face, int units_per_em) -> float {
            (void)units_per_em;
            if (!face) {
                return 1.0f;
            }
            const TT_OS2* os2 = static_cast<const TT_OS2*>(FT_Get_Sfnt_Table(face, ft_sfnt_os2));
            if (os2) {
                if (os2->sTypoAscender == 770 && os2->sTypoDescender == -230) {
                    return 1024.0f / 952.0f;
                }
            }
            return 1.0f;
        };
        auto compute_sysfont_shift_value = [](FT_Face face) -> s32 {
            if (!face) {
                return 0;
            }
            const TT_OS2* os2 = static_cast<const TT_OS2*>(FT_Get_Sfnt_Table(face, ft_sfnt_os2));
            if (!os2) {
                return 0;
            }
            const bool is_jp_pro_metrics = (os2->sTypoAscender == 880) &&
                                           (os2->sTypoDescender == -120) &&
                                           (os2->sTypoLineGap == 1);
            if (is_jp_pro_metrics) {
                const auto* vhea =
                    static_cast<const TT_VertHeader*>(FT_Get_Sfnt_Table(face, ft_sfnt_vhea));
                if (vhea) {
                    const int units = static_cast<int>(face->units_per_EM);
                    const int gap = static_cast<int>(os2->sTypoLineGap);
                    const int shift = 1024 - units - gap;
                    if (shift > 0 && shift < 128) {
                        return static_cast<s32>(shift);
                    }
                }
            }
            return 0;
        };
        st.system_font_scale_factor =
            compute_sysfont_scale_factor(st.ext_ft_face, st.ext_units_per_em);
        st.system_font_shift_value = compute_sysfont_shift_value(st.ext_ft_face);
        LOG_DEBUG(Lib_Font, "SystemFonts: primary='{}' unitsPerEm={} scaleFactor={} shiftValue={}",
                  primary_path.filename().string(), st.ext_units_per_em,
                  st.system_font_scale_factor, st.system_font_shift_value);

        std::string preferred_latin_name_lower;
        const auto base_dir = Internal::GetSysFontBaseDir();
        if (!base_dir.empty()) {
            auto resolve_existing =
                [&](std::string_view filename) -> std::optional<std::filesystem::path> {
                const std::filesystem::path file_path{std::string(filename)};
                std::error_code ec;
                {
                    const auto candidate = base_dir / file_path;
                    if (std::filesystem::exists(candidate, ec)) {
                        return candidate;
                    }
                }
                {
                    const auto candidate = base_dir / "font" / file_path;
                    if (std::filesystem::exists(candidate, ec)) {
                        return candidate;
                    }
                }
                {
                    const auto candidate = base_dir / "font2" / file_path;
                    if (std::filesystem::exists(candidate, ec)) {
                        return candidate;
                    }
                }
                return std::nullopt;
            };

            auto resolve_sysfont_path = [&](const std::filesystem::path& candidate)
                -> std::optional<std::filesystem::path> {
                if (candidate.empty()) {
                    return std::nullopt;
                }
                std::error_code ec;
                if (std::filesystem::exists(candidate, ec)) {
                    return candidate;
                }

                const auto filename = candidate.filename().string();
                if (filename.empty()) {
                    return std::nullopt;
                }

                if (auto direct = resolve_existing(filename)) {
                    return direct;
                }
                if (auto alias = ResolveKnownSysFontAlias(base_dir, filename)) {
                    return alias;
                }
                return std::nullopt;
            };

            auto resolve_override =
                [&](const std::string& key) -> std::optional<std::filesystem::path> {
                if (auto override_path = Config::getSystemFontOverride(key)) {
                    if (!override_path->empty() && override_path->is_absolute()) {
                        return *override_path;
                    }
                    if (!override_path->empty() && !override_path->has_parent_path()) {
                        return base_dir / *override_path;
                    }
                    LOG_ERROR(Lib_Font,
                              "SystemFonts: override for '{}' must be a filename only or absolute "
                              "path: '{}'",
                              key, override_path->string());
                }
                return std::nullopt;
            };

            auto lower_ascii = [](std::string s) {
                for (auto& c : s) {
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                return s;
            };

            const std::string primary_name_lower = lower_ascii(primary_path.filename().string());

            auto has_fallback_name_lower = [&](const std::string& candidate_lower) -> bool {
                if (candidate_lower.empty() || candidate_lower == primary_name_lower) {
                    return true;
                }
                for (const auto& fb : st.system_fallback_faces) {
                    if (lower_ascii(fb.path.filename().string()) == candidate_lower) {
                        return true;
                    }
                }
                return false;
            };

            auto add_fallback_face = [&](const std::filesystem::path& p) {
                LOG_DEBUG(Lib_Font, "SystemFonts: add_fallback_face begin");
                std::vector<unsigned char> fb_bytes;
                if (!Internal::LoadFontFile(p, fb_bytes)) {
                    LOG_DEBUG(Lib_Font, "SystemFonts: add_fallback_face failed (LoadFontFile)");
                    return;
                }
                LOG_DEBUG(Lib_Font, "SystemFonts: add_fallback_face bytes_size={}",
                          fb_bytes.size());
                Internal::FontState::SystemFallbackFace fb{};
                fb.font_id = 0xffffffffu;
                fb.scale_factor = 1.0f;
                fb.shift_value = 0;
                fb.path = p;
                fb.bytes = std::make_shared<std::vector<unsigned char>>(std::move(fb_bytes));
                fb.ft_face = Internal::CreateFreeTypeFaceFromBytes(
                    fb.bytes->data(), fb.bytes->size(), sub_font_index);
                fb.ready = (fb.ft_face != nullptr);
                if (fb.ready) {
                    LOG_DEBUG(Lib_Font, "SystemFonts: add_fallback_face face={}",
                              fmt::ptr(fb.ft_face));
                    fb.scale_factor = compute_sysfont_scale_factor(
                        fb.ft_face, fb.ft_face ? static_cast<int>(fb.ft_face->units_per_EM) : 0);
                    fb.shift_value = compute_sysfont_shift_value(fb.ft_face);
                    LOG_DEBUG(
                        Lib_Font,
                        "SystemFonts: fallback='{}' unitsPerEm={} scaleFactor={} shiftValue={}",
                        fb.path.filename().string(),
                        fb.ft_face ? static_cast<int>(fb.ft_face->units_per_EM) : 0,
                        fb.scale_factor, fb.shift_value);
                    LOG_DEBUG(Lib_Font, "SystemFonts: add_fallback_face push_back begin");
                    st.system_fallback_faces.push_back(std::move(fb));
                    LOG_DEBUG(Lib_Font, "SystemFonts: add_fallback_face done");
                } else {
                    Internal::DestroyFreeTypeFace(fb.ft_face);
                }
            };

            {
                const u32 tag = (fontSetType >> 8) & 0xFFu;
                const u32 variant = (fontSetType >> 20) & 0x0Fu;
                u32 style_suffix = fontSetType & 0xFFu;

                switch (style_suffix) {
                case 0xC3:
                    style_suffix = 0x43;
                    break;
                case 0xC4:
                    style_suffix = 0x44;
                    break;
                case 0xC5:
                    style_suffix = 0x45;
                    break;
                case 0xC7:
                    style_suffix = 0x47;
                    break;
                default:
                    break;
                }

                const char* latin_file = nullptr;
                if (tag != 0x00u && tag != 0x10u) {
                    if (variant == 0x3u) {
                        switch (style_suffix) {
                        case 0x44:
                            latin_file = "SSTTypewriter-Roman.otf";
                            break;
                        case 0x47:
                            latin_file = "SSTTypewriter-Bd.otf";
                            break;
                        default:
                            break;
                        }
                    } else if (variant == 0x1u) {
                        switch (style_suffix) {
                        case 0x43:
                            latin_file = "SST-LightItalic.otf";
                            break;
                        case 0x44:
                            latin_file = "SST-Italic.otf";
                            break;
                        case 0x45:
                            latin_file = "SST-MediumItalic.otf";
                            break;
                        case 0x47:
                            latin_file = "SST-BoldItalic.otf";
                            break;
                        default:
                            break;
                        }
                    } else {
                        switch (style_suffix) {
                        case 0x43:
                            latin_file = "SST-Light.otf";
                            break;
                        case 0x44:
                            latin_file = "SST-Roman.otf";
                            break;
                        case 0x45:
                            latin_file = "SST-Medium.otf";
                            break;
                        case 0x47:
                            latin_file = "SST-Bold.otf";
                            break;
                        default:
                            break;
                        }
                    }
                }

                if (latin_file) {
                    if (auto latin_path = resolve_sysfont_path(base_dir / latin_file)) {
                        preferred_latin_name_lower = lower_ascii(latin_path->filename().string());
                        if (!has_fallback_name_lower(preferred_latin_name_lower)) {
                            add_fallback_face(*latin_path);
                        }
                    }
                }
            }

            {
                const u32 style_suffix = fontSetType & 0xFFu;
                const char* arabic_file = nullptr;
                switch (style_suffix) {
                case 0xC3:
                    arabic_file = "SSTArabic-Light.otf";
                    break;
                case 0xC4:
                    arabic_file = "SSTArabic-Roman.otf";
                    break;
                case 0xC5:
                    arabic_file = "SSTArabic-Medium.otf";
                    break;
                case 0xC7:
                    arabic_file = "SSTArabic-Bold.otf";
                    break;
                case 0xD3:
                    arabic_file = "SSTArabic-Light.otf";
                    break;
                case 0xD4:
                    arabic_file = "SSTArabic-Roman.otf";
                    break;
                case 0xD5:
                    arabic_file = "SSTArabic-Medium.otf";
                    break;
                case 0xD7:
                    arabic_file = "SSTArabic-Bold.otf";
                    break;
                default:
                    break;
                }
                if (arabic_file) {
                    if (auto arabic_path = resolve_sysfont_path(base_dir / arabic_file)) {
                        const std::string arabic_lower =
                            lower_ascii(arabic_path->filename().string());
                        if (!has_fallback_name_lower(arabic_lower)) {
                            add_fallback_face(*arabic_path);
                        }
                    }
                }
            }

            {
                const u32 tag = (fontSetType >> 8) & 0xFFu;
                const u32 style_suffix = fontSetType & 0xFFu;

                auto add_named_fallback = [&](std::string_view filename) {
                    if (filename.empty()) {
                        return;
                    }
                    if (auto p = resolve_sysfont_path(base_dir / std::filesystem::path{filename})) {
                        const std::string lower = lower_ascii(p->filename().string());
                        if (!has_fallback_name_lower(lower)) {
                            add_fallback_face(*p);
                        }
                    }
                };

                const bool is_bold_like = (style_suffix == 0x47u) || (style_suffix == 0x57u) ||
                                          (style_suffix == 0xC7u) || (style_suffix == 0xD7u);

                const bool needs_jppro = (tag == 0x04u) || (tag == 0x24u) || (tag == 0x34u) ||
                                         (tag == 0x84u) || (tag == 0xA4u) || (tag == 0xACu) ||
                                         (tag == 0xB4u) || (tag == 0xBCu);
                const bool needs_cngb = (tag == 0x80u) || (tag == 0x84u) || (tag == 0x90u) ||
                                        (tag == 0xA0u) || (tag == 0xA4u) || (tag == 0xACu) ||
                                        (tag == 0xB0u) || (tag == 0xB4u) || (tag == 0xBCu);
                const bool needs_hangul = (tag == 0x24u) || (tag == 0x34u) || (tag == 0xA0u) ||
                                          (tag == 0xA4u) || (tag == 0xACu) || (tag == 0xB0u) ||
                                          (tag == 0xB4u) || (tag == 0xBCu);

                u32 sea_weight_code = style_suffix;
                switch (sea_weight_code) {
                case 0xD3u:
                    sea_weight_code = 0x53u;
                    break;
                case 0xD4u:
                    sea_weight_code = 0x54u;
                    break;
                case 0xD5u:
                    sea_weight_code = 0x55u;
                    break;
                case 0xD7u:
                    sea_weight_code = 0x57u;
                    break;
                default:
                    break;
                }
                const bool is_sea_weight = (sea_weight_code == 0x53u) ||
                                           (sea_weight_code == 0x54u) ||
                                           (sea_weight_code == 0x55u) || (sea_weight_code == 0x57u);

                if (is_sea_weight && primary_name_lower.rfind("sstvietnamese-", 0) != 0) {
                    const char* vn_file = nullptr;
                    switch (sea_weight_code) {
                    case 0x53:
                        vn_file = "SSTVietnamese-Light.otf";
                        break;
                    case 0x54:
                        vn_file = "SSTVietnamese-Roman.otf";
                        break;
                    case 0x55:
                        vn_file = "SSTVietnamese-Medium.otf";
                        break;
                    case 0x57:
                        vn_file = "SSTVietnamese-Bold.otf";
                        break;
                    default:
                        break;
                    }
                    if (vn_file) {
                        add_named_fallback(vn_file);
                    }
                }

                if (is_sea_weight &&
                    ((tag == 0x10u) || (tag == 0x14u) || (tag == 0x34u) || (tag == 0x90u) ||
                     (tag == 0x94u) || (tag == 0xB0u) || (tag == 0xB4u) || (tag == 0xBCu))) {
                    const char* th_file = nullptr;
                    switch (sea_weight_code) {
                    case 0x53:
                        th_file = "SSTThai-Light.otf";
                        break;
                    case 0x54:
                        th_file = "SSTThai-Roman.otf";
                        break;
                    case 0x55:
                        th_file = "SSTThai-Medium.otf";
                        break;
                    case 0x57:
                        th_file = "SSTThai-Bold.otf";
                        break;
                    default:
                        break;
                    }
                    if (th_file) {
                        add_named_fallback(th_file);
                    }
                }

                if (needs_jppro && primary_name_lower.rfind("sstjppro-", 0) != 0) {
                    add_named_fallback(is_bold_like ? "SSTJpPro-Bold.otf" : "SSTJpPro-Regular.otf");
                }
                if (needs_cngb && primary_name_lower != "dfhei5-sony.ttf") {
                    add_named_fallback("DFHEI5-SONY.ttf");
                }
                if (needs_hangul && primary_name_lower.rfind("sceps4yoongd-", 0) != 0) {
                    add_named_fallback(is_bold_like ? "SCEPS4Yoongd-Bold.otf"
                                                    : "SCEPS4Yoongd-Medium.otf");
                }
            }

            for (int i = 0; i < 8; ++i) {
                const std::string key_a =
                    fmt::format("fontset_0x{:08X}_fallback{}", fontSetType, i);
                const std::string key_b =
                    fmt::format("fontSet_0x{:08X}_fallback{}", fontSetType, i);
                std::optional<std::filesystem::path> p = resolve_override(key_a);
                if (!p) {
                    p = resolve_override(key_b);
                }
                if (!p || p->empty()) {
                    continue;
                }

                std::optional<std::filesystem::path> p_resolved;
                if (p->is_absolute()) {
                    std::error_code ec;
                    if (std::filesystem::exists(*p, ec)) {
                        p_resolved = *p;
                    }
                } else {
                    p_resolved = resolve_sysfont_path(*p);
                }
                if (!p_resolved) {
                    continue;
                }

                std::vector<unsigned char> fb_bytes;
                if (!Internal::LoadFontFile(*p_resolved, fb_bytes)) {
                    continue;
                }

                Internal::FontState::SystemFallbackFace fb{};
                fb.font_id = 0xffffffffu;
                fb.scale_factor = 1.0f;
                fb.shift_value = 0;
                fb.path = *p_resolved;
                fb.bytes = std::make_shared<std::vector<unsigned char>>(std::move(fb_bytes));
                fb.ft_face = Internal::CreateFreeTypeFaceFromBytes(
                    fb.bytes->data(), fb.bytes->size(), sub_font_index);
                fb.ready = (fb.ft_face != nullptr);
                if (fb.ready) {
                    fb.scale_factor = compute_sysfont_scale_factor(
                        fb.ft_face, fb.ft_face ? static_cast<int>(fb.ft_face->units_per_EM) : 0);
                    fb.shift_value = compute_sysfont_shift_value(fb.ft_face);
                    LOG_DEBUG(
                        Lib_Font,
                        "SystemFonts: fallback='{}' unitsPerEm={} scaleFactor={} shiftValue={}",
                        fb.path.filename().string(),
                        fb.ft_face ? static_cast<int>(fb.ft_face->units_per_EM) : 0,
                        fb.scale_factor, fb.shift_value);
                    st.system_fallback_faces.push_back(std::move(fb));
                } else {
                    Internal::DestroyFreeTypeFace(fb.ft_face);
                }
            }

            {
                const std::string global_fallback_name = Config::getSystemFontFallbackName();
                if (!global_fallback_name.empty()) {
                    const auto existing_name = primary_path.filename().string();
                    if (existing_name != global_fallback_name) {
                        std::filesystem::path fb_path = global_fallback_name;
                        if (!fb_path.is_absolute()) {
                            fb_path = base_dir / global_fallback_name;
                        }
                        if (const auto resolved_path = resolve_sysfont_path(fb_path)) {
                            const std::string fb_lower =
                                lower_ascii(resolved_path->filename().string());
                            if (!has_fallback_name_lower(fb_lower)) {
                                add_fallback_face(*resolved_path);
                            }
                        }
                    }
                }
            }
        }

        {
            const auto* driver =
                lib_local->sys_driver
                    ? reinterpret_cast<const Internal::SysDriver*>(lib_local->sys_driver)
                    : nullptr;
            const auto open_fn = driver ? driver->open : nullptr;
            if (!open_fn) {
                return cleanup_handle_for_error(ORBIS_FONT_ERROR_INVALID_LIBRARY);
            }

            auto resolve_served_path = [](const std::filesystem::path& p) -> std::filesystem::path {
                if (p.empty()) {
                    return {};
                }
                std::error_code ec;
                if (std::filesystem::is_regular_file(p, ec) && !ec) {
                    return p;
                }
                const auto parent = p.parent_path();
                const auto parent_name = parent.filename().string();
                const auto file_name = p.filename();
                if (!file_name.empty() && (parent_name == "font" || parent_name == "font2")) {
                    const auto container = parent.parent_path();
                    const auto sibling =
                        container / ((parent_name == "font") ? "font2" : "font") / file_name;
                    if (std::filesystem::is_regular_file(sibling, ec) && !ec) {
                        return sibling;
                    }
                } else {
                    const auto in_font2_dir = parent / "font2" / file_name;
                    if (std::filesystem::is_regular_file(in_font2_dir, ec) && !ec) {
                        return in_font2_dir;
                    }
                    const auto in_font_dir = parent / "font" / file_name;
                    if (std::filesystem::is_regular_file(in_font_dir, ec) && !ec) {
                        return in_font_dir;
                    }
                }
                return p;
            };

            auto stable_unique_id_for = [](std::string_view s) -> s32 {
                std::uint32_t h = 2166136261u;
                for (unsigned char c : s) {
                    const unsigned char lower = static_cast<unsigned char>(std::tolower(c));
                    h ^= static_cast<std::uint32_t>(lower);
                    h *= 16777619u;
                }
                h &= 0x7fffffffu;
                if (h == 0u) {
                    h = 1u;
                }
                return static_cast<s32>(h);
            };

            auto open_driver_mode_for = [](u32 m) -> u32 {
                if (m == 1) {
                    return 5;
                }
                if (m == 2) {
                    return 6;
                }
                return 7;
            };

            LOG_INFO(Lib_Font, "OpenFontSet: stage=open_sysfonts_begin");
            auto open_sysfonts_entry =
                [&](const std::filesystem::path& requested_path) -> std::optional<u32> {
                const std::filesystem::path served_path = resolve_served_path(requested_path);
                if (served_path.empty()) {
                    return std::nullopt;
                }
                const std::string host_path_str = served_path.string();
                if (host_path_str.empty()) {
                    return std::nullopt;
                }
                u32 open_arg4 = 0;
                {
                    std::error_code ec;
                    const auto sz = std::filesystem::file_size(served_path, ec);
                    if (!ec && sz <= static_cast<std::uintmax_t>(std::numeric_limits<u32>::max())) {
                        open_arg4 = static_cast<u32>(sz);
                    }
                }
                LOG_INFO(Lib_Font, "OpenFontSet: stage=sysfonts_entry_begin");
                const s32 unique_id = stable_unique_id_for(host_path_str);

                auto* sys_ctx = static_cast<u8*>(lib_local->sysfonts_ctx);
                auto* sys_header =
                    sys_ctx ? reinterpret_cast<Internal::FontCtxHeader*>(sys_ctx) : nullptr;
                auto* entries_base =
                    sys_header ? static_cast<Internal::FontCtxEntry*>(sys_header->base) : nullptr;
                const u32 max_entries = sys_header ? sys_header->max_entries : 0u;
                if (!sys_header || !entries_base || max_entries == 0) {
                    return std::nullopt;
                }

                u32* ctx_lock_word = &sys_header->lock_word;
                for (;;) {
                    const u32 lw = *ctx_lock_word;
                    if (static_cast<s32>(lw) >= 0) {
                        std::atomic_ref<u32> ref(*ctx_lock_word);
                        u32 expected = lw;
                        if (ref.compare_exchange_weak(expected, lw | 0x80000000u,
                                                      std::memory_order_acq_rel)) {
                            break;
                        }
                    }
                    Libraries::Kernel::sceKernelUsleep(0x1e);
                }

                u32 entry_index = 0xffffffffu;
                u32 first_free = 0xffffffffu;
                for (u32 i = 0; i < max_entries; i++) {
                    auto* entry = &entries_base[i];
                    const u32 active = entry->active;
                    const u32 stored_id = entry->unique_id;
                    if (active != 0 && stored_id == static_cast<u32>(unique_id)) {
                        entry_index = i;
                        break;
                    }
                    if (first_free == 0xffffffffu && active == 0) {
                        first_free = i;
                    }
                }

                if (entry_index == 0xffffffffu) {
                    entry_index = first_free;
                    if (entry_index == 0xffffffffu) {
                        *ctx_lock_word = 0;
                        return std::nullopt;
                    }

                    auto* entry = &entries_base[entry_index];
                    entry->reserved_0x00 = 0;
                    entry->active = 1;
                    entry->font_address = 0;
                    entry->unique_id = static_cast<u32>(unique_id);
                    entry->lock_mode1 = 0;
                    entry->lock_mode3 = 0;
                    entry->lock_mode2 = 0;
                    entry->obj_mode1 = nullptr;
                    entry->obj_mode3 = nullptr;
                    entry->obj_mode2 = nullptr;
                    std::memset(entry->reserved_0x38, 0, sizeof(entry->reserved_0x38));
                }

                *ctx_lock_word = 0;

                LOG_INFO(Lib_Font, "OpenFontSet: stage=sysfonts_entry_lock_begin");
                void* font_obj = nullptr;
                u32 entry_lock_word = 0;
                u8* entry_u8 = Internal::AcquireFontCtxEntry(sys_ctx, entry_index, mode_low,
                                                             &font_obj, &entry_lock_word);
                auto* entry =
                    entry_u8 ? reinterpret_cast<Internal::FontCtxEntry*>(entry_u8) : nullptr;
                if (!entry) {
                    return std::nullopt;
                }
                LOG_INFO(Lib_Font, "OpenFontSet: stage=sysfonts_entry_lock_ok");

                void* used_font_obj = font_obj;
                u32 updated_lock = entry_lock_word;
                s32 rc = ORBIS_FONT_ERROR_FONT_OPEN_MAX;

                if ((entry_lock_word & 0x40000000u) == 0) {
                    used_font_obj = font_obj;
                    auto* tail = lib_local + 1;
                    auto* tail_reserved1 =
                        tail ? reinterpret_cast<FontLibReserved1SysfontTail*>(tail->reserved1)
                             : nullptr;
                    if (tail_reserved1) {
                        const u32 lib_flags = lib_local->flags;
                        if (mode_low == 2) {
                            if ((lib_flags & 0x100000u) != 0) {
                                tail_reserved1->sysfont_flags |= 1;
                            }
                            if ((lib_flags & 0x10000u) != 0) {
                                tail_reserved1->sysfont_flags |= 2;
                            }
                        } else {
                            if ((lib_flags & 0x200000u) != 0) {
                                tail_reserved1->sysfont_flags |= 1;
                            }
                            if ((lib_flags & 0x20000u) != 0) {
                                tail_reserved1->sysfont_flags |= 2;
                            }
                        }
                    }
                    LOG_INFO(Lib_Font, "OpenFontSet: stage=sysfonts_driver_open_call");
                    rc = open_fn(library, open_driver_mode_for(mode_low), host_path_str.c_str(),
                                 open_arg4, sub_font_index, entry_index, &used_font_obj);
                    if (tail_reserved1) {
                        tail_reserved1->sysfont_flags = 0;
                    }
                    if (rc == ORBIS_OK) {
                        LOG_INFO(Lib_Font, "OpenFontSet: stage=sysfonts_driver_open_ok");
                        if (mode_low == 3) {
                            entry->obj_mode3 = used_font_obj;
                        } else if (mode_low == 2) {
                            entry->obj_mode2 = used_font_obj;
                        } else {
                            entry->obj_mode1 = used_font_obj;
                        }
                        updated_lock = entry_lock_word | 0x40000000u;
                    }
                } else {
                    rc = ORBIS_FONT_ERROR_FONT_OPEN_MAX;
                    if ((entry_lock_word & 0x0FFFFFFFu) != 0x0FFFFFFFu) {
                        for (auto* node = static_cast<Internal::FontObj*>(font_obj);
                             node != nullptr; node = node->next) {
                            if (node->sub_font_index == sub_font_index) {
                                node->refcount++;
                                used_font_obj = node;
                                rc = ORBIS_OK;
                                break;
                            }
                        }
                        if (rc != ORBIS_OK) {
                            used_font_obj = font_obj;
                            auto* tail = lib_local + 1;
                            auto* tail_reserved1 =
                                tail ? reinterpret_cast<FontLibReserved1SysfontTail*>(
                                           tail->reserved1)
                                     : nullptr;
                            if (tail_reserved1) {
                                const u32 lib_flags = lib_local->flags;
                                if (mode_low == 2) {
                                    if ((lib_flags & 0x100000u) != 0) {
                                        tail_reserved1->sysfont_flags |= 1;
                                    }
                                    if ((lib_flags & 0x10000u) != 0) {
                                        tail_reserved1->sysfont_flags |= 2;
                                    }
                                } else {
                                    if ((lib_flags & 0x200000u) != 0) {
                                        tail_reserved1->sysfont_flags |= 1;
                                    }
                                    if ((lib_flags & 0x20000u) != 0) {
                                        tail_reserved1->sysfont_flags |= 2;
                                    }
                                }
                            }
                            rc = open_fn(library, open_driver_mode_for(mode_low),
                                         host_path_str.c_str(), open_arg4, sub_font_index,
                                         entry_index, &used_font_obj);
                            if (tail_reserved1) {
                                tail_reserved1->sysfont_flags = 0;
                            }
                            if (rc == ORBIS_OK) {
                                if (mode_low == 3) {
                                    entry->obj_mode3 = used_font_obj;
                                } else if (mode_low == 2) {
                                    entry->obj_mode2 = used_font_obj;
                                } else {
                                    entry->obj_mode1 = used_font_obj;
                                }
                            }
                        }
                    }
                }

                if (rc == ORBIS_OK) {
                    updated_lock = updated_lock + 1;
                }

                const u32 count = updated_lock & 0x0FFFFFFFu;
                if (mode_low == 3) {
                    if (count == 0) {
                        entry->obj_mode3 = nullptr;
                    }
                    entry->lock_mode3 = updated_lock & 0x7fffffffu;
                } else if (mode_low == 1) {
                    if (count == 0) {
                        entry->obj_mode1 = nullptr;
                    }
                    entry->lock_mode1 = updated_lock & 0x7fffffffu;
                } else {
                    if (count == 0) {
                        entry->obj_mode2 = nullptr;
                    }
                    entry->lock_mode2 = updated_lock & 0x7fffffffu;
                }

                if (count == 0 && entry->obj_mode1 == nullptr && entry->obj_mode3 == nullptr &&
                    entry->obj_mode2 == nullptr) {
                    entry->unique_id = 0;
                    entry->active = 0;
                }

                if (rc != ORBIS_OK) {
                    return std::nullopt;
                }
                LOG_INFO(Lib_Font, "OpenFontSet: stage=sysfonts_entry_done");
                return entry_index;
            };

            std::vector<u32> font_ids;
            font_ids.reserve(1 + st.system_fallback_faces.size());

            const auto primary_entry = open_sysfonts_entry(primary_path);
            if (!primary_entry) {
                return cleanup_handle_for_error(ORBIS_FONT_ERROR_FONT_OPEN_FAILED);
            }
            st.system_font_id = *primary_entry;
            font_ids.push_back(st.system_font_id);
            LOG_INFO(Lib_Font, "OpenFontSet: stage=sysfonts_primary_opened");
            u32 opened_fallbacks = 0;
            for (auto& fb : st.system_fallback_faces) {
                if (!fb.ready || !fb.ft_face) {
                    continue;
                }
                const auto fb_entry = open_sysfonts_entry(fb.path);
                if (!fb_entry) {
                    continue;
                }
                fb.font_id = *fb_entry;
                font_ids.push_back(fb.font_id);
                LOG_INFO(Lib_Font, "OpenFontSet: stage=sysfonts_fallback_opened");
                ++opened_fallbacks;
            }
            LOG_INFO(Lib_Font, "OpenFontSet: stage=sysfonts_fallbacks_opened");

            LOG_INFO(Lib_Font, "OpenFontSet: stage=before_make_shared_selector");
            st.fontset_selector = std::make_shared<Internal::FontSetSelector>();
            LOG_INFO(Lib_Font, "OpenFontSet: stage=after_make_shared_selector");
            st.fontset_selector->magic = Internal::FontSetSelector::kMagic;
            st.fontset_selector->font_set_type = fontSetType;
            st.fontset_selector->library = lib_local;
            st.fontset_selector->mode_low = mode_low;
            st.fontset_selector->sub_font_index = sub_font_index;
            st.fontset_selector->primary_font_id = st.system_font_id;
            auto lower_ascii_local = [](std::string s) {
                for (auto& c : s) {
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                return s;
            };
            const auto select_preferred_latin_id = [&](const std::string& name_lower) -> bool {
                if (name_lower.empty()) {
                    return false;
                }
                for (const auto& fb : st.system_fallback_faces) {
                    const auto candidate = lower_ascii_local(fb.path.filename().string());
                    if (candidate == name_lower && fb.font_id != 0xffffffffu) {
                        st.fontset_selector->roman_font_id = fb.font_id;
                        return true;
                    }
                }
                return false;
            };
            if (!select_preferred_latin_id(preferred_latin_name_lower)) {
                (void)select_preferred_latin_id("sst-roman.otf");
            }
            for (const auto& fb : st.system_fallback_faces) {
                const auto name = lower_ascii_local(fb.path.filename().string());
                if (name.rfind("sstarabic-", 0) == 0 && fb.font_id != 0xffffffffu) {
                    st.fontset_selector->arabic_font_id = fb.font_id;
                    break;
                }
            }
            st.fontset_selector->candidates.clear();
            st.fontset_selector->candidates.reserve(font_ids.size());
            for (u32 id : font_ids) {
                st.fontset_selector->candidates.push_back({id, nullptr});
            }

            const u32 entry_count = static_cast<u32>(font_ids.size());
            const std::size_t indices_bytes = static_cast<std::size_t>(entry_count) * sizeof(u32);
            const std::size_t ptr_off_unaligned =
                sizeof(Internal::FontSetRecordHeader) + indices_bytes;
            const std::size_t ptr_align = alignof(const Internal::FontSetSelector*);
            const std::size_t ptr_off = (ptr_off_unaligned + (ptr_align - 1u)) & ~(ptr_align - 1u);
            const std::size_t rec_size = ptr_off + sizeof(const Internal::FontSetSelector*);

            LOG_INFO(Lib_Font, "OpenFontSet: stage=before_make_shared_record");
            st.fontset_record_storage = std::make_shared<std::vector<u8>>(rec_size);
            LOG_INFO(Lib_Font, "OpenFontSet: stage=after_make_shared_record");
            std::memset(st.fontset_record_storage->data(), 0, st.fontset_record_storage->size());

            auto* header = std::construct_at(reinterpret_cast<Internal::FontSetRecordHeader*>(
                st.fontset_record_storage->data()));
            header->font_set_type = fontSetType;
            header->magic = Internal::FontSetSelector::kMagic;
            header->entry_count = entry_count;

            auto* entries = reinterpret_cast<u32*>(st.fontset_record_storage->data() +
                                                   sizeof(Internal::FontSetRecordHeader));
            for (u32 i = 0; i < entry_count; ++i) {
                std::construct_at(entries + i, font_ids[i]);
            }

            std::construct_at(reinterpret_cast<const Internal::FontSetSelector**>(
                                  st.fontset_record_storage->data() + ptr_off),
                              st.fontset_selector.get());

            h->open_info.fontset_record = header;
        }

        h->magic = 0x0F02;
        h->flags = static_cast<u16>(h->flags | static_cast<u16>(mode_low));
        if (!had_existing_handle) {
            *pFontHandle = handle;
        }
        LOG_INFO(Lib_Font, "OpenFontSet: stage=published_handle");
        if ((h->flags & 0x10) != 0) {
            auto* tail = reinterpret_cast<FontLibTail*>(lib_local + 1);
            auto* list_lock = &tail->list_head_ptr;
            void* list_ptr = nullptr;
            for (;;) {
                list_ptr = *list_lock;
                if (list_ptr !=
                    reinterpret_cast<void*>(std::numeric_limits<std::uintptr_t>::max())) {
                    std::atomic_ref<void*> ref(*list_lock);
                    void* expected = list_ptr;
                    if (ref.compare_exchange_weak(
                            expected,
                            reinterpret_cast<void*>(std::numeric_limits<std::uintptr_t>::max()),
                            std::memory_order_acq_rel)) {
                        break;
                    }
                }
                Libraries::Kernel::sceKernelUsleep(0x1e);
            }

            auto* head_ptr = reinterpret_cast<OrbisFontHandle*>(list_ptr);
            if (head_ptr) {
                const OrbisFontHandle old_head = *head_ptr;
                if (auto* hn = GetNativeFont(handle)) {
                    hn->prevFont = nullptr;
                    hn->nextFont = old_head;
                }
                if (old_head) {
                    if (auto* old_native = GetNativeFont(old_head)) {
                        old_native->prevFont = handle;
                    }
                }
                *head_ptr = handle;
            }
            *list_lock = list_ptr;
        }

        ReleaseLibraryLock(lib_local, prev_lib_lock);
        LOG_INFO(Lib_Font, "OpenFontSet: stage=done");
        return ORBIS_OK;
    }
}

s32 PS4_SYSV_ABI sceFontRebindRenderer(OrbisFontHandle fontHandle) {
    LOG_INFO(Lib_Font, "called");

    if (!fontHandle) {
        LOG_ERROR(Lib_Font, "NULL_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    LOG_DEBUG(Lib_Font, "{}", formatParams({Param("fontHandle", fontHandle)}));

    auto* font = GetNativeFont(fontHandle);
    if (!font || font->magic != 0x0F02) {
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    u32 prev_font_lock = 0;
    if (!AcquireFontLock(font, prev_font_lock)) {
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    u32 prev_cached_lock = 0;
    if (!AcquireCachedStyleLock(font, prev_cached_lock)) {
        ReleaseFontLock(font, prev_font_lock);
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    s32 rc = ORBIS_FONT_ERROR_FATAL;
    void* renderer = font->renderer_binding.renderer;
    if (!renderer) {
        rc = ORBIS_FONT_ERROR_NOT_BOUND_RENDERER;
    } else if (*reinterpret_cast<const u16*>(renderer) != 0x0F07) {
        rc = ORBIS_FONT_ERROR_FATAL;
    } else {
        std::memcpy(&font->cached_style.effectWeightY, &font->style_tail.slant_ratio, sizeof(u64));

        std::memcpy(&font->cached_style, &font->style_frame[0], sizeof(u32));

        const u32 dpi_y = font->style_frame[1];
        const u32 scale_unit = font->style_tail.scale_unit;
        const u32 reserved_0x0c = font->style_tail.reserved_0x0c;
        const float scale_w = font->style_tail.scale_w;
        const float scale_h = font->style_tail.scale_h;
        const float effect_weight_x = font->style_tail.effect_weight_x;
        const float effect_weight_y = font->style_tail.effect_weight_y;

        font->cached_style.hDpi = dpi_y;
        font->cached_style.vDpi = scale_unit;
        font->cached_style.scaleUnit = reserved_0x0c;
        font->cached_style.baseScale = scale_w;
        font->cached_style.scalePixelW = scale_h;
        font->cached_style.scalePixelH = effect_weight_x;
        font->cached_style.effectWeightX = effect_weight_y;
        rc = ORBIS_OK;
    }

    ReleaseCachedStyleLock(font, prev_cached_lock);
    ReleaseFontLock(font, prev_font_lock);

    if (rc != ORBIS_OK) {
        if (rc == ORBIS_FONT_ERROR_NOT_BOUND_RENDERER) {
            LOG_ERROR(Lib_Font, "NOT_BOUND_RENDERER");
        } else {
            LOG_ERROR(Lib_Font, "FAILED");
        }
    }
    return rc;
}

s32 PS4_SYSV_ABI sceFontRenderCharGlyphImage(OrbisFontHandle fontHandle, u32 code,
                                             OrbisFontRenderSurface* surf, float x, float y,
                                             OrbisFontGlyphMetrics* metrics,
                                             OrbisFontRenderOutput* result) {
    LOG_INFO(Lib_Font, "called");

    if (!fontHandle) {
        ClearRenderOutputs(metrics, result);
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    auto* font = GetNativeFont(fontHandle);
    if (!font || font->magic != 0x0F02) {
        ClearRenderOutputs(metrics, result);
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    LOG_DEBUG(Lib_Font, "{}",
              formatParams({
                  Param("fontHandle", fontHandle),
                  Param("code", code),
                  Param("surf", surf),
                  Param("x", x),
                  Param("y", y),
                  Param("metrics", metrics),
                  Param("result", result),
              }));

    u32 prev_lock_word = 0;
    if (!AcquireFontLock(font, prev_lock_word)) {
        ClearRenderOutputs(metrics, result);
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    s32 rc = ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    if (static_cast<s16>(font->flags) >= 0) {
        s32 pre_rc = ORBIS_OK;
        float baseline_add = 0.0f;

        auto cache_cachedstyle_baseline = [&]() -> s32 {
            const u8 cached_flags = CachedStyleCacheFlags(font->cached_style);
            if ((cached_flags & 0x1) == 0) {
                pre_rc = Internal::ComputeHorizontalLayoutBlocksToBytes(
                    fontHandle, &font->cached_style, font->cached_style.layout_cache_bytes);
                if (pre_rc == ORBIS_OK) {
                    CachedStyleSetCacheFlags(font->cached_style,
                                             static_cast<u8>(cached_flags | 0x1));
                    pre_rc = ORBIS_OK;
                }
            }
            if (pre_rc == ORBIS_OK) {
                const auto layout_words =
                    Internal::HorizontalLayoutBlocksIo{
                        Internal::LayoutWordsBytes(font->cached_style.layout_cache_bytes)}
                        .fields();
                baseline_add = layout_words.baseline.value();
            }
            return pre_rc;
        };

        if (!surf || (surf->styleFlag & 0x1) == 0) {
            (void)cache_cachedstyle_baseline();
        } else {
            struct RenderSurfaceSystemUse {
                OrbisFontStyleFrame* styleframe;
                float catchedScale;
                std::uint8_t padding[88 - sizeof(OrbisFontStyleFrame*) - sizeof(float)];
            };
            static_assert(sizeof(RenderSurfaceSystemUse) ==
                          sizeof(((OrbisFontRenderSurface*)nullptr)->reserved_q));

            auto* sys = reinterpret_cast<RenderSurfaceSystemUse*>(surf->reserved_q);
            auto* styleframe = sys ? sys->styleframe : nullptr;

            if (!styleframe || (styleframe->flags1 & 1) == 0) {
                (void)cache_cachedstyle_baseline();
            } else {
                struct StyleStateBlock {
                    /*0x00*/ u32 dpi_x;
                    /*0x04*/ u32 dpi_y;
                    /*0x08*/ u32 vDpi_flag;
                    /*0x0C*/ u32 reserved0c;
                    /*0x10*/ float scale_x;
                    /*0x14*/ float scale_y;
                    /*0x18*/ float effect_wx;
                    /*0x1C*/ float effect_wy;
                    /*0x20*/ float slant;
                };
                StyleStateBlock state{};
                state.dpi_x = styleframe->hDpi;
                state.dpi_y = styleframe->vDpi;
                state.vDpi_flag = 0;
                state.reserved0c = 0;
                state.effect_wx = 0.0f;
                state.effect_wy = 0.0f;
                state.slant = 0.0f;
                if ((styleframe->flags1 & 2) != 0) {
                    state.slant = styleframe->slantRatio;
                }
                if ((styleframe->flags1 & 4) != 0) {
                    state.effect_wx = styleframe->effectWeightX;
                    state.effect_wy = styleframe->effectWeightY;
                }

                pre_rc = Internal::StyleStateGetScalePixel(&styleframe->hDpi, &state.scale_x,
                                                           &state.scale_y);
                Internal::HorizontalLayoutBlocks layout_words{};
                if (pre_rc == ORBIS_OK) {
                    pre_rc = Internal::ComputeHorizontalLayoutBlocksTyped(fontHandle, &state,
                                                                          &layout_words);
                }
                if (pre_rc == ORBIS_OK) {
                    baseline_add = layout_words.baseline();
                    if (sys) {
                        sys->catchedScale = baseline_add;
                    }
                }
            }
        }

        const float y_used = y + baseline_add;
        {
            static std::mutex s_baseline_log_mutex;
            static std::unordered_map<OrbisFontHandle, int> s_baseline_log_counts;
            std::lock_guard<std::mutex> lock(s_baseline_log_mutex);
            int& count = s_baseline_log_counts[fontHandle];
            if (count < 5) {
                LOG_DEBUG(Lib_Font,
                          "RenderBaseline: handle={} code=U+{:04X} y_in={} baseline_add={} "
                          "y_used={} pre_rc={}",
                          static_cast<const void*>(fontHandle), code, y, baseline_add, y_used,
                          pre_rc);
                ++count;
            }
        }

        CachedStyleSetDirectionWord(font->cached_style, 1);

        if (code == 0) {
            rc = ORBIS_FONT_ERROR_NO_SUPPORT_CODE;
        } else if (!surf || !metrics || !result) {
            rc = ORBIS_FONT_ERROR_INVALID_PARAMETER;
        } else if (pre_rc != ORBIS_OK) {
            rc = pre_rc;
        } else {
            rc = Internal::RenderCharGlyphImageCore(fontHandle, code, surf, x, y_used, metrics,
                                                    result);
        }
    } else {
        s32 pre_rc = ORBIS_OK;
        float scalar_add = 0.0f;

        auto cache_cachedstyle_scalar = [&]() -> s32 {
            const u8 cached_flags = CachedStyleCacheFlags(font->cached_style);
            if ((cached_flags & 0x2) == 0) {
                Internal::VerticalLayoutBlocks layout_words{};
                pre_rc = Internal::ComputeVerticalLayoutBlocksTyped(fontHandle, &font->cached_style,
                                                                    &layout_words);
                if (pre_rc == ORBIS_OK) {
                    scalar_add = layout_words.baseline_offset_x();
                    CachedStyleSetScalar(font->cached_style, scalar_add);
                    CachedStyleSetCacheFlags(font->cached_style,
                                             static_cast<u8>(cached_flags | 0x2));
                }
            }
            if (pre_rc == ORBIS_OK && (CachedStyleCacheFlags(font->cached_style) & 0x2) != 0) {
                scalar_add = CachedStyleGetScalar(font->cached_style);
            }
            return pre_rc;
        };

        if (!surf || (surf->styleFlag & 0x1) == 0) {
            (void)cache_cachedstyle_scalar();
        } else {
            struct RenderSurfaceSystemUse {
                OrbisFontStyleFrame* styleframe;
                float catchedScale;
                std::uint8_t padding[88 - sizeof(OrbisFontStyleFrame*) - sizeof(float)];
            };
            static_assert(sizeof(RenderSurfaceSystemUse) ==
                          sizeof(((OrbisFontRenderSurface*)nullptr)->reserved_q));

            auto* sys = reinterpret_cast<RenderSurfaceSystemUse*>(surf->reserved_q);
            auto* styleframe = sys ? sys->styleframe : nullptr;
            if (!styleframe || (styleframe->flags1 & 1) == 0) {
                (void)cache_cachedstyle_scalar();
            } else {
                struct StyleStateBlock {
                    /*0x00*/ u32 dpi_x;
                    /*0x04*/ u32 dpi_y;
                    /*0x08*/ u32 vDpi_flag;
                    /*0x0C*/ u32 reserved0c;
                    /*0x10*/ float scale_x;
                    /*0x14*/ float scale_y;
                    /*0x18*/ float effect_wx;
                    /*0x1C*/ float effect_wy;
                    /*0x20*/ float slant;
                };
                StyleStateBlock state{};
                state.dpi_x = styleframe->hDpi;
                state.dpi_y = styleframe->vDpi;
                state.vDpi_flag = 0;
                state.reserved0c = 0;
                state.effect_wx = 0.0f;
                state.effect_wy = 0.0f;
                state.slant = 0.0f;
                if ((styleframe->flags1 & 2) != 0) {
                    state.slant = styleframe->slantRatio;
                }
                if ((styleframe->flags1 & 4) != 0) {
                    state.effect_wx = styleframe->effectWeightX;
                    state.effect_wy = styleframe->effectWeightY;
                }

                pre_rc = Internal::StyleStateGetScalePixel(&styleframe->hDpi, &state.scale_x,
                                                           &state.scale_y);
                Internal::VerticalLayoutBlocks layout_words{};
                if (pre_rc == ORBIS_OK) {
                    pre_rc = Internal::ComputeVerticalLayoutBlocksTyped(fontHandle, &state,
                                                                        &layout_words);
                }
                if (pre_rc == ORBIS_OK) {
                    scalar_add = layout_words.baseline_offset_x();
                    if (sys) {
                        sys->catchedScale = scalar_add;
                    }
                } else {
                    (void)cache_cachedstyle_scalar();
                }
            }
        }

        const float x_used = x + scalar_add;
        CachedStyleSetDirectionWord(font->cached_style, 2);

        if (code == 0) {
            rc = ORBIS_FONT_ERROR_NO_SUPPORT_CODE;
        } else if (!surf || !metrics || !result) {
            rc = ORBIS_FONT_ERROR_INVALID_PARAMETER;
        } else if (pre_rc != ORBIS_OK) {
            rc = pre_rc;
        } else {
            rc = Internal::RenderCharGlyphImageCore(fontHandle, code, surf, x_used, y, metrics,
                                                    result);
        }
    }

    font->lock_word = prev_lock_word;

    if (rc != ORBIS_OK) {
        ClearRenderOutputs(metrics, result);
        LOG_ERROR(Lib_Font, "FAILED");
        return rc;
    }
    LogRenderResultSample(fontHandle, code, *metrics, *result);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontRenderCharGlyphImageHorizontal(OrbisFontHandle fontHandle, u32 code,
                                                       OrbisFontRenderSurface* surf, float x,
                                                       float y, OrbisFontGlyphMetrics* metrics,
                                                       OrbisFontRenderOutput* result) {
    LOG_INFO(Lib_Font, "called");

    if (!fontHandle) {
        ClearRenderOutputs(metrics, result);
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    auto* font = GetNativeFont(fontHandle);
    if (!font || font->magic != 0x0F02) {
        ClearRenderOutputs(metrics, result);
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    if (code == 0) {
        ClearRenderOutputs(metrics, result);
        LOG_ERROR(Lib_Font, "NO_SUPPORT_CODE");
        return ORBIS_FONT_ERROR_NO_SUPPORT_CODE;
    }

    if (!surf || !metrics || !result) {
        ClearRenderOutputs(metrics, result);
        LOG_ERROR(Lib_Font, "INVALID_PARAMETER");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    LOG_DEBUG(Lib_Font, "{}",
              formatParams({
                  Param("fontHandle", fontHandle),
                  Param("code", code),
                  Param("surf", surf),
                  Param("x", x),
                  Param("y", y),
                  Param("metrics", metrics),
                  Param("result", result),
              }));

    u32 prev_lock_word = 0;
    if (!AcquireFontLock(font, prev_lock_word)) {
        ClearRenderOutputs(metrics, result);
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    CachedStyleSetDirectionWord(font->cached_style, 1);

    const s32 rc =
        Internal::RenderCharGlyphImageCore(fontHandle, code, surf, x, y, metrics, result);
    font->lock_word = prev_lock_word;

    if (rc != ORBIS_OK) {
        ClearRenderOutputs(metrics, result);
        LOG_ERROR(Lib_Font, "FAILED");
        return rc;
    }
    LogRenderResultSample(fontHandle, code, *metrics, *result);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontRenderCharGlyphImageVertical(OrbisFontHandle fontHandle, u32 code,
                                                     OrbisFontRenderSurface* surf, float x, float y,
                                                     OrbisFontGlyphMetrics* metrics,
                                                     OrbisFontRenderOutput* result) {
    LOG_INFO(Lib_Font, "called");

    if (!fontHandle) {
        ClearRenderOutputs(metrics, result);
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    auto* font = GetNativeFont(fontHandle);
    if (!font || font->magic != 0x0F02) {
        ClearRenderOutputs(metrics, result);
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    if (code == 0) {
        ClearRenderOutputs(metrics, result);
        LOG_ERROR(Lib_Font, "NO_SUPPORT_CODE");
        return ORBIS_FONT_ERROR_NO_SUPPORT_CODE;
    }

    if (!surf || !metrics || !result) {
        ClearRenderOutputs(metrics, result);
        LOG_ERROR(Lib_Font, "INVALID_PARAMETER");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    LOG_DEBUG(Lib_Font, "{}",
              formatParams({
                  Param("fontHandle", fontHandle),
                  Param("code", code),
                  Param("surf", surf),
                  Param("x", x),
                  Param("y", y),
                  Param("metrics", metrics),
                  Param("result", result),
              }));

    u32 prev_lock_word = 0;
    if (!AcquireFontLock(font, prev_lock_word)) {
        ClearRenderOutputs(metrics, result);
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    CachedStyleSetDirectionWord(font->cached_style, 2);

    const s32 rc =
        Internal::RenderCharGlyphImageCore(fontHandle, code, surf, x, y, metrics, result);
    font->lock_word = prev_lock_word;

    if (rc != ORBIS_OK) {
        ClearRenderOutputs(metrics, result);
        LOG_ERROR(Lib_Font, "FAILED");
        return rc;
    }
    LogRenderResultSample(fontHandle, code, *metrics, *result);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontRendererGetOutlineBufferSize(OrbisFontRenderer fontRenderer, u32* size) {
    LOG_INFO(Lib_Font, "called");

    if (!size) {
        LOG_ERROR(Lib_Font, "INVALID_PARAMETER");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    *size = 0;

    auto* renderer = static_cast<Internal::RendererOpaque*>(fontRenderer);
    if (!renderer || renderer->magic != 0x0F07) {
        LOG_ERROR(Lib_Font, "INVALID_RENDERER");
        return ORBIS_FONT_ERROR_INVALID_RENDERER;
    }

    LOG_DEBUG(Lib_Font, "{}",
              formatParams({
                  Param("fontRenderer", fontRenderer),
                  Param("size", size),
              }));

    *size = static_cast<u32>(renderer->workspace_size);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontRendererResetOutlineBuffer(OrbisFontRenderer fontRenderer) {
    LOG_INFO(Lib_Font, "called");

    auto* renderer = static_cast<Internal::RendererOpaque*>(fontRenderer);
    if (!renderer || renderer->magic != 0x0F07) {
        LOG_ERROR(Lib_Font, "INVALID_RENDERER");
        return ORBIS_FONT_ERROR_INVALID_RENDERER;
    }

    LOG_DEBUG(Lib_Font, "{}", formatParams({Param("fontRenderer", fontRenderer)}));

    if (renderer->workspace && renderer->workspace_size) {
        std::memset(renderer->workspace, 0, static_cast<std::size_t>(renderer->workspace_size));
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontRendererSetOutlineBufferPolicy(OrbisFontRenderer fontRenderer,
                                                       u64 bufferPolicy, u32 basalSize,
                                                       u32 limitSize) {
    LOG_INFO(Lib_Font, "called");

    auto* renderer = static_cast<Internal::RendererOpaque*>(fontRenderer);
    if (!renderer || renderer->magic != 0x0F07) {
        LOG_ERROR(Lib_Font, "INVALID_RENDERER");
        return ORBIS_FONT_ERROR_INVALID_RENDERER;
    }

    LOG_DEBUG(Lib_Font, "{}",
              formatParams({
                  Param("fontRenderer", fontRenderer),
                  Param("bufferPolicy", bufferPolicy),
                  Param("basalSize", basalSize),
                  Param("limitSize", limitSize),
              }));

    if (limitSize != 0 && basalSize > limitSize) {
        LOG_ERROR(Lib_Font, "INVALID_PARAMETER");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    using AllocFn = void*(PS4_SYSV_ABI*)(void* object, u32 size);
    using FreeFn = void(PS4_SYSV_ABI*)(void* object, void* p);

    const auto alloc_fn = reinterpret_cast<AllocFn>(renderer->alloc_fn);
    const auto free_fn = reinterpret_cast<FreeFn>(renderer->free_fn);
    if (!alloc_fn || !free_fn || !renderer->alloc_ctx) {
        LOG_ERROR(Lib_Font, "INVALID_MEMORY");
        return ORBIS_FONT_ERROR_INVALID_MEMORY;
    }

    std::size_t desired_size = static_cast<std::size_t>(renderer->workspace_size);
    desired_size = std::max(desired_size, static_cast<std::size_t>(basalSize));
    if (limitSize != 0) {
        desired_size = std::min(desired_size, static_cast<std::size_t>(limitSize));
    }

    if (desired_size == 0) {
        desired_size = 0x4000;
    }

    if (!renderer->workspace || renderer->workspace_size != desired_size) {
        void* new_workspace =
            Core::ExecuteGuest(alloc_fn, renderer->alloc_ctx, static_cast<u32>(desired_size));
        if (!new_workspace) {
            LOG_ERROR(Lib_Font, "ALLOCATION_FAILED");
            return ORBIS_FONT_ERROR_ALLOCATION_FAILED;
        }

        if (renderer->workspace) {
            Core::ExecuteGuest(free_fn, renderer->alloc_ctx, renderer->workspace);
        }

        renderer->workspace = new_workspace;
        renderer->workspace_size = desired_size;
    }

    (void)bufferPolicy;
    return ORBIS_OK;
}

void PS4_SYSV_ABI sceFontRenderSurfaceInit(OrbisFontRenderSurface* renderSurface, void* buffer,
                                           int bufWidthByte, int pixelSizeByte, int widthPixel,
                                           int heightPixel) {
    LOG_INFO(Lib_Font, "called");

    if (!renderSurface) {
        LOG_ERROR(Lib_Font, "NULL_POINTER");
        return;
    }

    LOG_DEBUG(Lib_Font, "{}",
              formatParams({
                  Param("renderSurface", renderSurface),
                  Param("buffer", buffer),
                  Param("bufWidthByte", bufWidthByte),
                  Param("pixelSizeByte", pixelSizeByte),
                  Param("widthPixel", widthPixel),
                  Param("heightPixel", heightPixel),
              }));

    LOG_DEBUG(Lib_Font, "RenderSurfaceInit: writing struct");
    const u32 w_nonneg = (widthPixel < 0) ? 0u : static_cast<u32>(widthPixel);
    const u32 h_nonneg = (heightPixel < 0) ? 0u : static_cast<u32>(heightPixel);
    renderSurface->buffer = buffer;
    renderSurface->widthByte = bufWidthByte;
    renderSurface->pixelSizeByte = static_cast<int8_t>(pixelSizeByte);
    renderSurface->pad0 = 0;
    renderSurface->styleFlag = 0;
    renderSurface->pad2 = 0;
    renderSurface->width = static_cast<s32>(w_nonneg);
    renderSurface->height = static_cast<s32>(h_nonneg);
    renderSurface->sc_x0 = 0;
    renderSurface->sc_y0 = 0;
    renderSurface->sc_x1 = w_nonneg;
    renderSurface->sc_y1 = h_nonneg;
    LOG_DEBUG(Lib_Font, "RenderSurfaceInit: done");
}

void PS4_SYSV_ABI sceFontRenderSurfaceSetScissor(OrbisFontRenderSurface* renderSurface, int x0,
                                                 int y0, int w, int h) {
    LOG_INFO(Lib_Font, "called");

    if (!renderSurface) {
        LOG_ERROR(Lib_Font, "NULL_POINTER");
        return;
    }

    LOG_DEBUG(Lib_Font, "{}",
              formatParams({
                  Param("renderSurface", renderSurface),
                  Param("x0", x0),
                  Param("y0", y0),
                  Param("w", w),
                  Param("h", h),
              }));

    const u32 surface_w = static_cast<u32>(renderSurface->width);
    if (surface_w != 0) {
        u32 x1;
        u32 x0_out;
        u32 w_u = static_cast<u32>(w);
        if (x0 < 0) {
            x1 = w_u + static_cast<u32>(x0);
            if (surface_w < x1) {
                x1 = surface_w;
            }
            if (w_u <= static_cast<u32>(-x0)) {
                x1 = 0;
            }
            x0_out = 0;
        } else {
            x1 = surface_w;
            x0_out = surface_w;
            if (static_cast<u32>(x0) <= surface_w) {
                if (surface_w < w_u) {
                    w_u = surface_w;
                }
                x1 = w_u + static_cast<u32>(x0);
                x0_out = static_cast<u32>(x0);
                if (surface_w < x1) {
                    x1 = surface_w;
                }
            }
        }
        renderSurface->sc_x0 = x0_out;
        renderSurface->sc_x1 = x1;
    }

    const u32 surface_h = static_cast<u32>(renderSurface->height);
    if (surface_h == 0) {
        return;
    }

    u32 y0_out;
    u32 y1 = surface_h;
    u32 h_u = static_cast<u32>(h);
    if (y0 < 0) {
        y0_out = 0;
        if (h_u <= static_cast<u32>(-y0)) {
            y1 = 0;
            renderSurface->sc_y0 = 0;
            renderSurface->sc_y1 = 0;
            return;
        }
    } else {
        y0_out = surface_h;
        if (surface_h < static_cast<u32>(y0)) {
            renderSurface->sc_y0 = y0_out;
            renderSurface->sc_y1 = y1;
            return;
        }
        y0_out = static_cast<u32>(y0);
        if (surface_h < h_u) {
            h_u = surface_h;
        }
    }

    const u32 y1_candidate = h_u + static_cast<u32>(y0);
    if (y1_candidate <= surface_h) {
        y1 = y1_candidate;
    }
    renderSurface->sc_y0 = y0_out;
    renderSurface->sc_y1 = y1;
}

s32 PS4_SYSV_ABI sceFontRenderSurfaceSetStyleFrame(OrbisFontRenderSurface* renderSurface,
                                                   OrbisFontStyleFrame* styleFrame) {
    LOG_INFO(Lib_Font, "called");

    if (!renderSurface) {
        LOG_ERROR(Lib_Font, "NULL_POINTER");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    {
        auto* memory = Core::Memory::Instance();
        if (memory && !memory->IsValidMapping(reinterpret_cast<VAddr>(renderSurface),
                                              sizeof(OrbisFontRenderSurface))) {
            LOG_ERROR(Lib_Font, "INVALID_ADDR renderSurface={}", fmt::ptr(renderSurface));
            return ORBIS_FONT_ERROR_INVALID_PARAMETER;
        }
        if (styleFrame && !memory->IsValidMapping(reinterpret_cast<VAddr>(styleFrame),
                                                  sizeof(OrbisFontStyleFrame))) {
            LOG_ERROR(Lib_Font, "INVALID_ADDR styleFrame={}", fmt::ptr(styleFrame));
            return ORBIS_FONT_ERROR_INVALID_PARAMETER;
        }
    }

    LOG_DEBUG(Lib_Font, "{}",
              formatParams({
                  Param("renderSurface", renderSurface),
                  Param("styleFrame", styleFrame),
              }));
    if (!styleFrame) {
        renderSurface->styleFlag &= ~u8{0x1};
        renderSurface->reserved_q[0] = 0;
        renderSurface->reserved_q[1] = 0;
        return ORBIS_OK;
    }
    if (styleFrame->magic != kStyleFrameMagic) {
        LOG_ERROR(Lib_Font, "INVALID_PARAMETER");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    renderSurface->styleFlag |= 0x1;
    renderSurface->reserved_q[0] = reinterpret_cast<u64>(styleFrame);
    renderSurface->reserved_q[1] = 0;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetEffectSlant(OrbisFontHandle fontHandle, float slantRatio) {
    LOG_INFO(Lib_Font, "called");
    LOG_DEBUG(Lib_Font, "{}",
              formatParams({
                  Param("fontHandle", fontHandle),
                  Param("slantRatio", slantRatio),
              }));

    auto* font = GetNativeFont(fontHandle);
    if (!font || font->magic != 0x0F02) {
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    u32 prev_lock = 0;
    if (!AcquireFontLock(font, prev_lock)) {
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    if (Internal::StyleStateSetSlantRatio(font->style_frame, slantRatio) != 0) {
        font->cached_style.layout_cache_state = 0;
    }
    ReleaseFontLock(font, prev_lock);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetEffectWeight(OrbisFontHandle fontHandle, float weightXScale,
                                        float weightYScale, u32 mode) {
    LOG_INFO(Lib_Font, "called");
    LOG_DEBUG(Lib_Font, "{}",
              formatParams({
                  Param("fontHandle", fontHandle),
                  Param("weightXScale", weightXScale),
                  Param("weightYScale", weightYScale),
                  Param("mode", mode),
              }));

    if (mode != 0) {
        LOG_ERROR(Lib_Font, "INVALID_PARAMETER");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    auto* font = GetNativeFont(fontHandle);
    if (!font || font->magic != 0x0F02) {
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    u32 prev_lock = 0;
    if (!AcquireFontLock(font, prev_lock)) {
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    if (Internal::StyleStateSetWeightScale(font->style_frame, weightXScale, weightYScale) != 0) {
        font->cached_style.layout_cache_state = 0;
    }
    ReleaseFontLock(font, prev_lock);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetFontsOpenMode() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetResolutionDpi(OrbisFontHandle fontHandle, u32 h_dpi, u32 v_dpi) {
    auto* font = GetNativeFont(fontHandle);
    if (!font) {
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    u32 prev_font_lock = 0;
    if (!AcquireFontLock(font, prev_font_lock)) {
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    if (Internal::StyleStateSetDpi(font->style_frame, h_dpi, v_dpi) != 0) {
        font->cached_style.layout_cache_state = 0;
    }

    if (auto* st = Internal::TryGetState(fontHandle)) {
        st->dpi_x = font->style_frame[0];
        st->dpi_y = font->style_frame[1];
        st->ext_scale_for_height = 0.0f;
        st->layout_cached = false;
    }

    ReleaseFontLock(font, prev_font_lock);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetScalePixel(OrbisFontHandle fontHandle, float w, float h) {
    LOG_INFO(Lib_Font, "called");

    if (!fontHandle) {
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    LOG_DEBUG(Lib_Font, "{}",
              formatParams({Param("fontHandle", fontHandle), Param("w", w), Param("h", h)}));

    auto* font = GetNativeFont(fontHandle);
    if (!font || font->magic != 0x0F02) {
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    u32 prev_font_lock = 0;
    if (!AcquireFontLock(font, prev_font_lock)) {
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    if (Internal::StyleStateSetScalePixel(font->style_frame, w, h) != 0) {
        font->cached_style.layout_cache_state = 0;
    }

    if (auto* st = Internal::TryGetState(fontHandle)) {
        st->scale_w = w;
        st->scale_h = h;
        st->scale_point_active = false;
        st->ext_scale_for_height = 0.0f;
        st->layout_cached = false;
    }
    ReleaseFontLock(font, prev_font_lock);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetScalePoint(OrbisFontHandle fontHandle, float w, float h) {
    LOG_INFO(Lib_Font, "called");

    if (!fontHandle) {
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    LOG_DEBUG(Lib_Font, "{}",
              formatParams({Param("fontHandle", fontHandle), Param("w", w), Param("h", h)}));

    auto* font = GetNativeFont(fontHandle);
    if (!font || font->magic != 0x0F02) {
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    u32 prev_font_lock = 0;
    if (!AcquireFontLock(font, prev_font_lock)) {
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    if (Internal::StyleStateSetScalePoint(font->style_frame, w, h) != 0) {
        font->cached_style.layout_cache_state = 0;
    }

    if (auto* st = Internal::TryGetState(fontHandle)) {
        st->scale_point_w = w;
        st->scale_point_h = h;
        st->scale_point_active = true;
        st->ext_scale_for_height = 0.0f;
        st->layout_cached = false;
    }
    ReleaseFontLock(font, prev_font_lock);
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

s32 PS4_SYSV_ABI sceFontSetupRenderEffectSlant(OrbisFontHandle fontHandle, float slantRatio) {
    LOG_INFO(Lib_Font, "called");

    if (!fontHandle) {
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    LOG_DEBUG(Lib_Font, "{}",
              formatParams({
                  Param("fontHandle", fontHandle),
                  Param("slantRatio", slantRatio),
              }));

    auto* font = GetNativeFont(fontHandle);
    if (!font || font->magic != 0x0F02) {
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    u32 prev_cached_lock = 0;
    if (!AcquireCachedStyleLock(font, prev_cached_lock)) {
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    s32 rc = ORBIS_FONT_ERROR_NOT_BOUND_RENDERER;
    if (font->renderer_binding.renderer != nullptr) {
        if (Internal::StyleStateSetSlantRatio(&font->cached_style, slantRatio) != 0) {
            font->cached_style.cache_flags_and_direction &= 0xFFFF0000u;
        }
        rc = ORBIS_OK;
    }

    ReleaseCachedStyleLock(font, prev_cached_lock);
    if (rc != ORBIS_OK) {
        if (rc == ORBIS_FONT_ERROR_NOT_BOUND_RENDERER) {
            LOG_ERROR(Lib_Font, "NOT_BOUND");
        } else {
            LOG_ERROR(Lib_Font, "FAILED");
        }
    }
    return rc;
}

s32 PS4_SYSV_ABI sceFontSetupRenderEffectWeight(OrbisFontHandle fontHandle, float weightXScale,
                                                float weightYScale, u32 mode) {
    LOG_INFO(Lib_Font, "called");

    if (!fontHandle) {
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    LOG_DEBUG(Lib_Font, "{}",
              formatParams({
                  Param("fontHandle", fontHandle),
                  Param("weightXScale", weightXScale),
                  Param("weightYScale", weightYScale),
                  Param("mode", mode),
              }));

    auto* font = GetNativeFont(fontHandle);
    if (!font || font->magic != 0x0F02) {
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    u32 prev_cached_lock = 0;
    if (!AcquireCachedStyleLock(font, prev_cached_lock)) {
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    s32 rc = ORBIS_FONT_ERROR_NOT_BOUND_RENDERER;
    if (font->renderer_binding.renderer != nullptr) {
        rc = ORBIS_FONT_ERROR_INVALID_PARAMETER;
        if (mode == 0) {
            if (Internal::StyleStateSetWeightScale(&font->cached_style, weightXScale,
                                                   weightYScale) != 0) {
                font->cached_style.cache_flags_and_direction &= 0xFFFF0000u;
            }
            rc = ORBIS_OK;
        }
    }

    ReleaseCachedStyleLock(font, prev_cached_lock);
    if (rc != ORBIS_OK) {
        if (rc == ORBIS_FONT_ERROR_NOT_BOUND_RENDERER) {
            LOG_ERROR(Lib_Font, "NOT_BOUND");
        } else if (rc == ORBIS_FONT_ERROR_INVALID_PARAMETER) {
            LOG_ERROR(Lib_Font, "INVALID_PARAMETER");
        } else {
            LOG_ERROR(Lib_Font, "FAILED");
        }
    }
    return rc;
}

s32 PS4_SYSV_ABI sceFontSetupRenderScalePixel(OrbisFontHandle fontHandle, float w, float h) {
    LOG_INFO(Lib_Font, "called");

    if (!fontHandle) {
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    LOG_DEBUG(Lib_Font, "{}",
              formatParams({Param("fontHandle", fontHandle), Param("w", w), Param("h", h)}));

    auto* font = GetNativeFont(fontHandle);
    if (!font || font->magic != 0x0F02) {
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    u32 prev_cached_lock = 0;
    if (!AcquireCachedStyleLock(font, prev_cached_lock)) {
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    s32 rc = ORBIS_FONT_ERROR_NOT_BOUND_RENDERER;
    if (font->renderer_binding.renderer != nullptr) {
        if (Internal::StyleStateSetScalePixel(&font->cached_style, w, h) != 0) {
            font->cached_style.cache_flags_and_direction &= 0xFFFF0000u;
        }
        rc = ORBIS_OK;
        if (auto* st = Internal::TryGetState(fontHandle)) {
            st->render_scale_w = w;
            st->render_scale_h = h;
            st->render_scale_point_active = false;
        }
    }

    ReleaseCachedStyleLock(font, prev_cached_lock);
    if (rc != ORBIS_OK) {
        if (rc == ORBIS_FONT_ERROR_NOT_BOUND_RENDERER) {
            LOG_ERROR(Lib_Font, "NOT_BOUND");
        } else {
            LOG_ERROR(Lib_Font, "FAILED");
        }
    }
    return rc;
}

s32 PS4_SYSV_ABI sceFontSetupRenderScalePoint(OrbisFontHandle fontHandle, float w, float h) {
    auto* font = GetNativeFont(fontHandle);
    if (!font) {
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    u32 prev_cached_lock = 0;
    if (!AcquireCachedStyleLock(font, prev_cached_lock)) {
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    s32 rc = ORBIS_FONT_ERROR_NOT_BOUND_RENDERER;
    if (font->renderer_binding.renderer != nullptr) {
        if (Internal::StyleStateSetScalePoint(&font->cached_style, w, h) != 0) {
            font->cached_style.cache_flags_and_direction &= 0xFFFF0000u;
        }
        rc = ORBIS_OK;
        if (auto* st = Internal::TryGetState(fontHandle)) {
            st->render_scale_point_w = w;
            st->render_scale_point_h = h;
            st->render_scale_point_active = true;
        }
    }

    ReleaseCachedStyleLock(font, prev_cached_lock);
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
    if (!styleFrame || styleFrame->magic != kStyleFrameMagic) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    if ((styleFrame->flags1 & 2u) == 0) {
        return ORBIS_FONT_ERROR_UNSET_PARAMETER;
    }
    if (!slantRatio) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    *slantRatio = styleFrame->slantRatio;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameGetEffectWeight(OrbisFontStyleFrame* fontStyleFrame,
                                                  float* weightXScale, float* weightYScale,
                                                  uint32_t* mode) {
    if (!fontStyleFrame || fontStyleFrame->magic != kStyleFrameMagic) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    if ((fontStyleFrame->flags1 & 4u) == 0) {
        return ORBIS_FONT_ERROR_UNSET_PARAMETER;
    }
    if (weightXScale) {
        *weightXScale = fontStyleFrame->effectWeightX + 1.0f;
    }
    if (weightYScale) {
        *weightYScale = fontStyleFrame->effectWeightY + 1.0f;
    }
    if (mode) {
        *mode = 0;
    }
    if (!weightXScale && !weightYScale && !mode) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameGetResolutionDpi(const OrbisFontStyleFrame* styleFrame,
                                                   u32* h_dpi, u32* v_dpi) {
    if (!styleFrame || styleFrame->magic != kStyleFrameMagic) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    if (h_dpi) {
        *h_dpi = styleFrame->hDpi;
    } else if (!v_dpi) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    if (v_dpi) {
        *v_dpi = styleFrame->vDpi;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameGetScalePixel(const OrbisFontStyleFrame* styleFrame, float* w,
                                                float* h) {
    if (!styleFrame || styleFrame->magic != kStyleFrameMagic) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    if ((styleFrame->flags1 & 1u) == 0) {
        return ORBIS_FONT_ERROR_UNSET_PARAMETER;
    }
    if (!w && !h) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    const u32 scale_unit = styleFrame->scaleUnit;
    if (w) {
        float out = styleFrame->scalePixelW;
        if (scale_unit != 0 && styleFrame->hDpi != 0) {
            out = out * (static_cast<float>(styleFrame->hDpi) / kPointsPerInch);
        }
        *w = out;
    }
    if (h) {
        float out = styleFrame->scalePixelH;
        if (scale_unit != 0 && styleFrame->vDpi != 0) {
            out = out * (static_cast<float>(styleFrame->vDpi) / kPointsPerInch);
        }
        *h = out;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameGetScalePoint(const OrbisFontStyleFrame* styleFrame, float* w,
                                                float* h) {
    if (!styleFrame || styleFrame->magic != kStyleFrameMagic) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    if ((styleFrame->flags1 & 1u) == 0) {
        return ORBIS_FONT_ERROR_UNSET_PARAMETER;
    }
    if (!w && !h) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    const u32 scale_unit = styleFrame->scaleUnit;
    if (w) {
        float out = styleFrame->scalePixelW;
        if (scale_unit == 0 && styleFrame->hDpi != 0) {
            out = out * (kPointsPerInch / static_cast<float>(styleFrame->hDpi));
        }
        *w = out;
    }
    if (h) {
        float out = styleFrame->scalePixelH;
        if (scale_unit == 0 && styleFrame->vDpi != 0) {
            out = out * (kPointsPerInch / static_cast<float>(styleFrame->vDpi));
        }
        *h = out;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameInit(OrbisFontStyleFrame* styleFrame) {
    if (!styleFrame) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    std::memset(reinterpret_cast<u8*>(&styleFrame->layout_cache_bytes[0x08]), 0, 0x20);
    std::memset(reinterpret_cast<u8*>(&styleFrame->layout_cache_state), 0, 0x20);
    std::memset(reinterpret_cast<u8*>(&styleFrame->scaleUnit), 0, 0x20);
    styleFrame->magic = kStyleFrameMagic;
    styleFrame->flags1 = 0;
    styleFrame->flags2 = 0;
    styleFrame->hDpi = 0x48;
    styleFrame->vDpi = 0x48;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameSetEffectSlant(OrbisFontStyleFrame* styleFrame,
                                                 float slantRatio) {
    if (!styleFrame || styleFrame->magic != kStyleFrameMagic) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    float clamped = slantRatio;
    if (clamped > 1.0f) {
        clamped = 1.0f;
    } else if (clamped < -1.0f) {
        clamped = -1.0f;
    }
    if (styleFrame->slantRatio != clamped) {
        styleFrame->slantRatio = clamped;
    }
    styleFrame->flags1 |= 2u;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameSetEffectWeight(OrbisFontStyleFrame* styleFrame,
                                                  float weightXScale, float weightYScale,
                                                  u32 mode) {
    if (!styleFrame || styleFrame->magic != kStyleFrameMagic) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    if (mode != 0) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    constexpr float kClamp = 0.04f;
    auto clamp_delta = [&](float delta) {
        if (delta > kClamp) {
            return kClamp;
        }
        if (delta < -kClamp) {
            return -kClamp;
        }
        return delta;
    };

    const float dx = clamp_delta(weightXScale - 1.0f);
    const float dy = clamp_delta(weightYScale - 1.0f);
    if (styleFrame->effectWeightX != dx) {
        styleFrame->effectWeightX = dx;
    }
    if (styleFrame->effectWeightY != dy) {
        styleFrame->effectWeightY = dy;
    }
    styleFrame->flags1 |= 4u;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameSetResolutionDpi(OrbisFontStyleFrame* styleFrame, u32 h_dpi,
                                                   u32 v_dpi) {
    if (!styleFrame || styleFrame->magic != kStyleFrameMagic) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    if (h_dpi == 0) {
        h_dpi = 0x48;
    }
    if (v_dpi == 0) {
        v_dpi = 0x48;
    }
    styleFrame->hDpi = h_dpi;
    styleFrame->vDpi = v_dpi;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameSetScalePixel(OrbisFontStyleFrame* styleFrame, float w, float h) {
    if (!styleFrame || styleFrame->magic != kStyleFrameMagic) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    styleFrame->scaleUnit = 0;
    styleFrame->scalePixelW = w;
    styleFrame->scalePixelH = h;
    styleFrame->flags1 |= 1u;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameSetScalePoint(OrbisFontStyleFrame* styleFrame, float w, float h) {
    if (!styleFrame || styleFrame->magic != kStyleFrameMagic) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    styleFrame->scaleUnit = 1;
    styleFrame->scalePixelW = w;
    styleFrame->scalePixelH = h;
    styleFrame->flags1 |= 1u;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameUnsetEffectSlant(OrbisFontStyleFrame* styleFrame) {
    if (!styleFrame || styleFrame->magic != kStyleFrameMagic) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    styleFrame->flags1 &= 0xfdu;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameUnsetEffectWeight(OrbisFontStyleFrame* styleFrame) {
    if (!styleFrame || styleFrame->magic != kStyleFrameMagic) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    styleFrame->flags1 &= 0xfbu;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameUnsetScale(OrbisFontStyleFrame* styleFrame) {
    if (!styleFrame || styleFrame->magic != kStyleFrameMagic) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    styleFrame->flags1 &= 0xfeu;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSupportExternalFonts(OrbisFontLib library, u32 fontMax, u32 formats) {
    LOG_INFO(Lib_Font, "called");
    LOG_DEBUG(Lib_Font, "{}",
              formatParams({
                  Param("library", library),
                  Param("fontMax", fontMax),
                  Param("formats", formats),
              }));

    if (!library) {
        LOG_ERROR(Lib_Font, "INVALID_LIBRARY");
        return ORBIS_FONT_ERROR_INVALID_LIBRARY;
    }
    auto* lib = static_cast<FontLibOpaque*>(library);
    if (lib->magic != 0x0F01) {
        LOG_ERROR(Lib_Font, "INVALID_LIBRARY");
        return ORBIS_FONT_ERROR_INVALID_LIBRARY;
    }
    u32 prev_lock_word = 0;
    if (!AcquireLibraryLock(lib, prev_lock_word)) {
        LOG_ERROR(Lib_Font, "INVALID_LIBRARY");
        return ORBIS_FONT_ERROR_INVALID_LIBRARY;
    }

    using AllocFn = void*(PS4_SYSV_ABI*)(void* object, u32 size);
    using FreeFn = void(PS4_SYSV_ABI*)(void* object, void* p);
    const auto alloc_fn = lib->alloc_vtbl ? reinterpret_cast<AllocFn>(lib->alloc_vtbl[0]) : nullptr;
    const auto free_fn = lib->alloc_vtbl ? reinterpret_cast<FreeFn>(lib->alloc_vtbl[1]) : nullptr;

    if (lib->external_fonts_ctx) {
        ReleaseLibraryLock(lib, prev_lock_word);
        LOG_ERROR(Lib_Font, "ALREADY_SPECIFIED");
        return ORBIS_FONT_ERROR_ALREADY_SPECIFIED;
    }

    const u32 ctx_size = (fontMax << 6) | 0x20u;
    void* ctx = Core::ExecuteGuest(alloc_fn, lib->alloc_ctx, ctx_size);
    if (!ctx) {
        ReleaseLibraryLock(lib, prev_lock_word);
        LOG_ERROR(Lib_Font, "ALLOCATION_FAILED");
        return ORBIS_FONT_ERROR_ALLOCATION_FAILED;
    }
    std::memset(ctx, 0, ctx_size);

    auto* header = static_cast<Internal::FontCtxHeader*>(ctx);
    void* list_head = nullptr;
    if (fontMax != 0) {
        list_head = static_cast<u8*>(ctx) + sizeof(Internal::FontCtxHeader);
    }
    if (fontMax != 0) {
        auto* entry = static_cast<Internal::FontCtxEntry*>(list_head);
        for (u32 i = 0; i < fontMax; i++) {
            std::memset(&entry[i], 0, sizeof(entry[i]));
        }
    }

    header->lock_word = 0;
    header->max_entries = fontMax;
    header->base = list_head;

    const auto* driver =
        lib->sys_driver ? reinterpret_cast<const Internal::SysDriver*>(lib->sys_driver) : nullptr;
    const auto support_fn = driver ? driver->support_formats : nullptr;
    if (!support_fn) {
        Core::ExecuteGuest(free_fn, lib->alloc_ctx, ctx);
        ReleaseLibraryLock(lib, prev_lock_word);
        LOG_ERROR(Lib_Font, "INVALID_LIBRARY");
        return ORBIS_FONT_ERROR_INVALID_LIBRARY;
    }
    const s32 support_rc = support_fn(library, formats);
    if (support_rc != ORBIS_OK) {
        Core::ExecuteGuest(free_fn, lib->alloc_ctx, ctx);
        ReleaseLibraryLock(lib, prev_lock_word);
        LOG_ERROR(Lib_Font, "SUPPORT_FAILED");
        return support_rc;
    }

    lib->external_fonts_ctx = ctx;

    auto& ls = GetLibState(library);
    ls.alloc_ctx = lib->alloc_ctx;
    ls.alloc_fn = alloc_fn;
    ls.free_fn = free_fn;
    ls.support_external = true;
    ls.external_fontMax = fontMax;
    ls.external_formats = formats;
    ls.owned_external_fonts_ctx = ctx;
    ls.owned_external_fonts_ctx_size = ctx_size;

    ReleaseLibraryLock(lib, prev_lock_word);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSupportGlyphs() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSupportSystemFonts(OrbisFontLib library) {
    LOG_INFO(Lib_Font, "called");
    LOG_DEBUG(Lib_Font, "{}", formatParams({Param("library", library)}));

    if (!library) {
        LOG_ERROR(Lib_Font, "INVALID_LIBRARY");
        return ORBIS_FONT_ERROR_INVALID_LIBRARY;
    }
    auto* lib = static_cast<FontLibOpaque*>(library);
    if (lib->magic != 0x0F01) {
        LOG_ERROR(Lib_Font, "INVALID_LIBRARY");
        return ORBIS_FONT_ERROR_INVALID_LIBRARY;
    }
    u32 prev_lock_word = 0;
    if (!AcquireLibraryLock(lib, prev_lock_word)) {
        LOG_ERROR(Lib_Font, "INVALID_LIBRARY");
        return ORBIS_FONT_ERROR_INVALID_LIBRARY;
    }

    using AllocFn = void*(PS4_SYSV_ABI*)(void* object, u32 size);
    using FreeFn = void(PS4_SYSV_ABI*)(void* object, void* p);
    const auto alloc_fn = lib->alloc_vtbl ? reinterpret_cast<AllocFn>(lib->alloc_vtbl[0]) : nullptr;
    const auto free_fn = lib->alloc_vtbl ? reinterpret_cast<FreeFn>(lib->alloc_vtbl[1]) : nullptr;

    if (lib->sysfonts_ctx) {
        ReleaseLibraryLock(lib, prev_lock_word);
        LOG_ERROR(Lib_Font, "ALREADY_SPECIFIED");
        return ORBIS_FONT_ERROR_ALREADY_SPECIFIED;
    }

    constexpr u32 kSysCtxSize = 0x1020;
    void* ctx = Core::ExecuteGuest(alloc_fn, lib->alloc_ctx, kSysCtxSize);
    if (!ctx) {
        ReleaseLibraryLock(lib, prev_lock_word);
        LOG_ERROR(Lib_Font, "ALLOCATION_FAILED");
        return ORBIS_FONT_ERROR_ALLOCATION_FAILED;
    }
    std::memset(ctx, 0, kSysCtxSize);

    auto* header = static_cast<Internal::FontCtxHeader*>(ctx);
    header->lock_word = 0;
    header->max_entries = 0x40;
    header->base = reinterpret_cast<u8*>(ctx) + sizeof(Internal::FontCtxHeader);

    auto* entries = static_cast<Internal::FontCtxEntry*>(header->base);
    for (u32 i = 0; i < header->max_entries; i++) {
        std::memset(&entries[i], 0, sizeof(entries[i]));
    }

    const auto* driver =
        lib->sys_driver ? reinterpret_cast<const Internal::SysDriver*>(lib->sys_driver) : nullptr;
    const auto support_fn = driver ? driver->support_formats : nullptr;
    if (!support_fn) {
        Core::ExecuteGuest(free_fn, lib->alloc_ctx, ctx);
        ReleaseLibraryLock(lib, prev_lock_word);
        LOG_ERROR(Lib_Font, "INVALID_LIBRARY");
        return ORBIS_FONT_ERROR_INVALID_LIBRARY;
    }
    const s32 support_rc = support_fn(library, 0x52);
    if (support_rc != ORBIS_OK) {
        Core::ExecuteGuest(free_fn, lib->alloc_ctx, ctx);
        ReleaseLibraryLock(lib, prev_lock_word);
        LOG_ERROR(Lib_Font, "SUPPORT_FAILED");
        return support_rc;
    }

    lib->sysfonts_ctx = ctx;

    auto& ls = GetLibState(library);
    ls.alloc_ctx = lib->alloc_ctx;
    ls.alloc_fn = alloc_fn;
    ls.free_fn = free_fn;
    ls.support_system = true;
    ls.owned_sysfonts_ctx = ctx;
    ls.owned_sysfonts_ctx_size = kSysCtxSize;

    if (!Internal::g_mnt) {
        Internal::g_mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    }
    if (Internal::g_mnt) {
        const auto sysfont_base = GetSysFontBaseDir();
        if (!sysfont_base.empty() && !Internal::g_mnt->GetMount("/:dev_font:")) {
            Internal::g_mnt->Mount(sysfont_base, "/:dev_font:", true);
        }
    }

    ReleaseLibraryLock(lib, prev_lock_word);
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
    LOG_INFO(Lib_Font, "called");
    LOG_DEBUG(Lib_Font, "{}", formatParams({Param("fontHandle", fontHandle)}));

    auto* font = GetNativeFont(fontHandle);
    if (!font) {
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    u32 prev_cached_lock = 0;
    if (!AcquireCachedStyleLock(font, prev_cached_lock)) {
        LOG_ERROR(Lib_Font, "INVALID_HANDLE");
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    s32 rc = ORBIS_FONT_ERROR_NOT_BOUND_RENDERER;
    if (font->renderer_binding.renderer != nullptr) {
        rc = ORBIS_OK;
        font->renderer_binding.renderer = nullptr;
        if (auto* st = Internal::TryGetState(fontHandle)) {
            st->bound_renderer = nullptr;
        }
    }

    ReleaseCachedStyleLock(font, prev_cached_lock);
    if (rc != ORBIS_OK) {
        if (rc == ORBIS_FONT_ERROR_NOT_BOUND_RENDERER) {
            LOG_ERROR(Lib_Font, "NOT_BOUND");
        } else {
            LOG_ERROR(Lib_Font, "FAILED");
        }
    }
    return rc;
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
