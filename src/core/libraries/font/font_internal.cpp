// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "font_internal.h"

#include <array>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_TRUETYPE_TABLES_H

#include "core/libraries/font/fontft_internal.h"

namespace Libraries::Font::Internal {

Core::FileSys::MntPoints* g_mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();

using Libraries::Font::OrbisFontGenerateGlyphParams;
using Libraries::Font::OrbisFontGlyph;
using Libraries::Font::OrbisFontGlyphMetrics;
using Libraries::Font::OrbisFontGlyphOpaque;
using Libraries::Font::OrbisFontGlyphOutline;
using Libraries::Font::OrbisFontGlyphOutlinePoint;
using Libraries::Font::OrbisFontMem;
using Libraries::Font::OrbisFontStyleFrame;

std::unordered_map<Libraries::Font::OrbisFontHandle, FontState> g_font_state;

std::unordered_map<Libraries::Font::OrbisFontLib, LibraryState> g_library_state;

std::unordered_map<Libraries::Font::OrbisFontRenderSurface*,
                   const Libraries::Font::OrbisFontStyleFrame*>
    g_style_for_surface;

void* g_allocator_vtbl_stub[4] = {};
std::uint8_t g_sys_driver_stub{};
std::uint8_t g_fontset_registry_stub{};
std::uint8_t g_sysfonts_ctx_stub{};
std::uint8_t g_external_fonts_ctx_stub{};
u32 g_device_cache_stub{};

bool HasSfntTables(const std::vector<unsigned char>& bytes);
FontState* TryGetState(Libraries::Font::OrbisFontHandle h);
std::string ReportSystemFaceRequest(FontState& st, Libraries::Font::OrbisFontHandle handle,
                                    bool& attached_out);

namespace {

static FT_Library GetFreeTypeLibrary() {
    static std::once_flag once;
    static FT_Library lib = nullptr;
    std::call_once(once, [] {
        FT_Library created = nullptr;
        if (FT_Init_FreeType(&created) != 0) {
            created = nullptr;
        }
        lib = created;
    });
    return lib;
}

static constexpr FT_Int32 kFtLoadFlagsBase =
    static_cast<FT_Int32>(FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP | FT_LOAD_VERTICAL_LAYOUT);

static constexpr FT_Int32 kFtLoadFlagsRender =
    static_cast<FT_Int32>(kFtLoadFlagsBase | FT_LOAD_RENDER);

} // namespace

FT_Face CreateFreeTypeFaceFromBytes(const unsigned char* data, std::size_t size,
                                    u32 subfont_index) {
    if (!data || size == 0) {
        return nullptr;
    }
    FT_Library lib = GetFreeTypeLibrary();
    if (!lib) {
        return nullptr;
    }
    FT_Face face = nullptr;
    if (FT_New_Memory_Face(lib, reinterpret_cast<const FT_Byte*>(data), static_cast<FT_Long>(size),
                           static_cast<FT_Long>(subfont_index), &face) != 0) {
        return nullptr;
    }
    (void)FT_Select_Charmap(face, FT_ENCODING_UNICODE);
    return face;
}

void DestroyFreeTypeFace(FT_Face& face) {
    if (!face) {
        return;
    }
    FT_Done_Face(face);
    face = nullptr;
}

namespace {

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

static std::mutex g_sysfont_file_cache_mutex;
static std::unordered_map<std::string, std::shared_ptr<std::vector<unsigned char>>>
    g_sysfont_file_cache;

static std::shared_ptr<std::vector<unsigned char>> GetCachedFontBytes(
    const std::filesystem::path& host_path) {
    const std::string key = host_path.string();
    {
        std::scoped_lock lock(g_sysfont_file_cache_mutex);
        if (auto it = g_sysfont_file_cache.find(key); it != g_sysfont_file_cache.end()) {
            return it->second;
        }
    }

    std::vector<unsigned char> bytes;
    if (!LoadGuestFileBytes(host_path, bytes) || bytes.empty()) {
        return {};
    }

    auto shared = std::make_shared<std::vector<unsigned char>>(std::move(bytes));
    {
        std::scoped_lock lock(g_sysfont_file_cache_mutex);
        g_sysfont_file_cache.emplace(key, shared);
    }
    return shared;
}

} // namespace

FontState::~FontState() {
    DestroyFreeTypeFace(ext_ft_face);
    for (auto& fb : system_fallback_faces) {
        DestroyFreeTypeFace(fb.ft_face);
    }
}

std::mutex g_generated_glyph_mutex;
std::unordered_set<OrbisFontGlyph> g_generated_glyphs;

void TrackGeneratedGlyph(OrbisFontGlyph glyph) {
    std::scoped_lock lock(g_generated_glyph_mutex);
    g_generated_glyphs.insert(glyph);
}

bool ForgetGeneratedGlyph(OrbisFontGlyph glyph) {
    std::scoped_lock lock(g_generated_glyph_mutex);
    return g_generated_glyphs.erase(glyph) > 0;
}

GeneratedGlyph* TryGetGeneratedGlyph(OrbisFontGlyph glyph) {
    if (!glyph || glyph->magic != 0x0F03) {
        return nullptr;
    }
    std::scoped_lock lock(g_generated_glyph_mutex);
    if (g_generated_glyphs.find(glyph) == g_generated_glyphs.end()) {
        return nullptr;
    }
    return reinterpret_cast<GeneratedGlyph*>(glyph);
}

void PopulateGlyphMetricVariants(GeneratedGlyph& gg) {
    if (gg.metrics_initialized) {
        return;
    }
    gg.metrics_horizontal.width = gg.metrics.width;
    gg.metrics_horizontal.height = gg.metrics.height;
    gg.metrics_horizontal.horizontal.bearing_x = gg.metrics.Horizontal.bearingX;
    gg.metrics_horizontal.horizontal.bearing_y = gg.metrics.Horizontal.bearingY;
    gg.metrics_horizontal.horizontal.advance = gg.metrics.Horizontal.advance;

    gg.metrics_horizontal_x.width = gg.metrics.width;
    gg.metrics_horizontal_x.horizontal.bearing_x = gg.metrics.Horizontal.bearingX;
    gg.metrics_horizontal_x.horizontal.advance = gg.metrics.Horizontal.advance;

    gg.metrics_horizontal_adv.horizontal.advance = gg.metrics.Horizontal.advance;
    gg.origin_x = static_cast<float>(gg.bbox_x0);
    gg.origin_y = static_cast<float>(gg.bbox_y0);
    gg.metrics_initialized = true;
}

void BuildBoundingOutline(GeneratedGlyph& gg) {
    const float left = gg.metrics.Horizontal.bearingX;
    const float top = gg.metrics.Horizontal.bearingY;
    const float right = gg.metrics.Horizontal.bearingX + gg.metrics.width;
    const float bottom = gg.metrics.Horizontal.bearingY - gg.metrics.height;

    gg.outline_points.clear();
    gg.outline_tags.clear();
    gg.outline_contours.clear();

    gg.outline_points.push_back({left, top});
    gg.outline_points.push_back({right, top});
    gg.outline_points.push_back({right, bottom});
    gg.outline_points.push_back({left, bottom});

    gg.outline_tags.resize(gg.outline_points.size(), 1);
    gg.outline_contours.push_back(static_cast<u16>(gg.outline_points.size() - 1));

    gg.outline.points_ptr = gg.outline_points.data();
    gg.outline.tags_ptr = gg.outline_tags.data();
    gg.outline.contour_end_idx = gg.outline_contours.data();
    gg.outline.points_cnt = static_cast<s16>(gg.outline_points.size());
    gg.outline.contours_cnt = static_cast<s16>(gg.outline_contours.size());
    gg.outline_initialized = true;
}

bool BuildTrueOutline(GeneratedGlyph& gg) {
    auto* st = TryGetState(gg.owner_handle);
    if (!st) {
        return false;
    }

    FT_Face primary_face = nullptr;
    if (!ResolveFace(*st, gg.owner_handle, primary_face)) {
        return false;
    }

    const auto* native =
        reinterpret_cast<const Libraries::Font::FontHandleOpaqueNative*>(gg.owner_handle);
    if (!native || native->magic != 0x0F02) {
        return false;
    }

    u32 resolved_code = gg.codepoint;
    u32 resolved_font_id = 0;
    FT_Face resolved_face = primary_face;
    float resolved_scale_factor = 1.0f;
    float range_scale = 1.0f;
    s32 shift_x_units = 0;
    s32 shift_y_units = 0;

    if (native->open_info.fontset_record) {
        const auto* fontset_record =
            static_cast<const FontSetRecordHeader*>(native->open_info.fontset_record);
        const bool is_internal_fontset_record =
            fontset_record &&
            fontset_record->magic == Libraries::Font::Internal::FontSetSelector::kMagic;
        u32 mapped_font_id = 0;
        resolved_code = ResolveSysFontCodepoint(native->open_info.fontset_record,
                                                static_cast<u64>(gg.codepoint),
                                                native->open_info.fontset_flags, &mapped_font_id);
        if (resolved_code == 0) {
            return false;
        }

        resolved_font_id = mapped_font_id;
        if (!is_internal_fontset_record) {
            resolved_scale_factor = st->system_font_scale_factor;
        }
        if (st->system_requested) {
            if (mapped_font_id == st->system_font_id) {
                resolved_face = st->ext_ft_face;
                shift_y_units = st->system_font_shift_value;
            } else {
                resolved_face = nullptr;
                for (const auto& fb : st->system_fallback_faces) {
                    if (fb.ready && fb.ft_face && fb.font_id == mapped_font_id) {
                        resolved_face = fb.ft_face;
                        resolved_scale_factor = fb.scale_factor;
                        shift_y_units = fb.shift_value;
                        break;
                    }
                }
            }
        }
        if (!resolved_face) {
            return false;
        }
        if (!is_internal_fontset_record) {
            if (shift_y_units == 0) {
                shift_y_units = GetSysFontDesc(mapped_font_id).shift_value;
            }
            if (const std::uint8_t* range_rec =
                    FindSysFontRangeRecord(mapped_font_id, resolved_code)) {
                struct SysFontRangeRecordLocal {
                    /*0x00*/ u32 start;
                    /*0x04*/ u32 end;
                    /*0x08*/ s16 shift_x;
                    /*0x0A*/ s16 shift_y;
                    /*0x0C*/ float scale_mul;
                    /*0x10*/ u32 reserved_0x10;
                    /*0x14*/ u32 reserved_0x14;
                    /*0x18*/ u32 reserved_0x18;
                };
                static_assert(sizeof(SysFontRangeRecordLocal) == 0x1C);
                SysFontRangeRecordLocal rec{};
                std::memcpy(&rec, range_rec, sizeof(rec));
                shift_x_units = static_cast<s32>(rec.shift_x);
                shift_y_units += static_cast<s32>(rec.shift_y);
                range_scale = rec.scale_mul;
                if (range_scale == 0.0f) {
                    range_scale = 1.0f;
                }
            }
        }
    }

    if (!resolved_face) {
        return false;
    }

    const FT_UInt glyph_index =
        FT_Get_Char_Index(resolved_face, static_cast<FT_ULong>(resolved_code));
    if (glyph_index == 0) {
        return false;
    }

    const float scaled_w = gg.glyph.scale_x * resolved_scale_factor * range_scale;
    const float scaled_h = gg.glyph.base_scale * resolved_scale_factor * range_scale;
    const auto char_w = static_cast<FT_F26Dot6>(static_cast<s32>(scaled_w * 64.0f));
    const auto char_h = static_cast<FT_F26Dot6>(static_cast<s32>(scaled_h * 64.0f));
    if (FT_Set_Char_Size(resolved_face, char_w, char_h, 72, 72) != 0) {
        return false;
    }

    if (shift_x_units != 0 || shift_y_units != 0) {
        if (!resolved_face->size) {
            return false;
        }
        const long x_scale = static_cast<long>(resolved_face->size->metrics.x_scale);
        const long y_scale = static_cast<long>(resolved_face->size->metrics.y_scale);

        const auto round_fixed_mul = [](long fixed_16_16, s32 value) -> s32 {
            const long long prod =
                static_cast<long long>(fixed_16_16) * static_cast<long long>(value);
            const long long sign_adj =
                (static_cast<long long>(~static_cast<long long>(value)) >> 63) * -0x10000LL;
            const long long base = sign_adj + prod;
            long long tmp = base - 0x8000LL;
            if (tmp < 0) {
                tmp = base + 0x7FFFLL;
            }
            return static_cast<s32>(static_cast<u64>(tmp) >> 16);
        };

        FT_Vector delta{};
        delta.x = static_cast<FT_Pos>(round_fixed_mul(x_scale, shift_x_units));
        delta.y = static_cast<FT_Pos>(round_fixed_mul(y_scale, shift_y_units));
        FT_Set_Transform(resolved_face, nullptr, &delta);
    } else {
        FT_Set_Transform(resolved_face, nullptr, nullptr);
    }

    constexpr FT_Int32 kFtLoadFlagsBase =
        static_cast<FT_Int32>(FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP);
    const bool loaded = (FT_Load_Glyph(resolved_face, glyph_index, kFtLoadFlagsBase) == 0);
    FT_Set_Transform(resolved_face, nullptr, nullptr);
    if (!loaded) {
        return false;
    }

    const FT_GlyphSlot slot = resolved_face->glyph;
    if (!slot || slot->format != FT_GLYPH_FORMAT_OUTLINE) {
        return false;
    }

    const FT_Outline& outline = slot->outline;
    if (outline.n_points <= 0 || outline.n_contours <= 0 || !outline.points || !outline.tags ||
        !outline.contours) {
        return false;
    }

    gg.outline_points.clear();
    gg.outline_tags.clear();
    gg.outline_contours.clear();
    gg.outline_points.reserve(static_cast<std::size_t>(outline.n_points));
    gg.outline_tags.reserve(static_cast<std::size_t>(outline.n_points));
    gg.outline_contours.reserve(static_cast<std::size_t>(outline.n_contours));

    for (int i = 0; i < outline.n_points; ++i) {
        const FT_Vector& p = outline.points[i];
        gg.outline_points.push_back(
            {static_cast<float>(p.x) / 64.0f, static_cast<float>(p.y) / 64.0f});
        const std::uint8_t tag = outline.tags[i];
        gg.outline_tags.push_back(FT_CURVE_TAG(tag) == FT_CURVE_TAG_ON ? 1 : 0);
    }
    for (int c = 0; c < outline.n_contours; ++c) {
        gg.outline_contours.push_back(static_cast<u16>(outline.contours[c]));
    }

    const bool ok = !gg.outline_points.empty() && !gg.outline_contours.empty();
    if (!ok) {
        return false;
    }

    gg.outline.points_ptr = gg.outline_points.data();
    gg.outline.tags_ptr = gg.outline_tags.data();
    gg.outline.contour_end_idx = gg.outline_contours.data();
    gg.outline.points_cnt = static_cast<s16>(gg.outline_points.size());
    gg.outline.contours_cnt = static_cast<s16>(gg.outline_contours.size());
    gg.outline_initialized = true;
    return true;
}

u16 ClampToU16(float value) {
    if (value <= 0.0f) {
        return 0;
    }
    const float clamped = std::min(value, static_cast<float>(std::numeric_limits<u16>::max()));
    return static_cast<u16>(std::lround(clamped));
}

FontState& GetState(Libraries::Font::OrbisFontHandle h) {
    return g_font_state[h];
}

FontState* TryGetState(Libraries::Font::OrbisFontHandle h) {
    if (!h)
        return nullptr;
    auto it = g_font_state.find(h);
    if (it == g_font_state.end())
        return nullptr;
    return &it->second;
}

LibraryState& GetLibState(Libraries::Font::OrbisFontLib lib) {
    return g_library_state[lib];
}

void RemoveLibState(Libraries::Font::OrbisFontLib lib) {
    if (auto it = g_library_state.find(lib); it != g_library_state.end()) {
        if (it->second.owned_device_cache) {
            delete[] static_cast<std::uint8_t*>(it->second.owned_device_cache);
        }
        g_library_state.erase(it);
    }
}

void LogExternalFormatSupport(u32 formats_mask) {
    LOG_INFO(Lib_Font, "ExternalFormatsMask=0x{:X}", formats_mask);
}

bool LoadFontFile(const std::filesystem::path& path, std::vector<unsigned char>& out_bytes);

void LogFontOpenParams(const Libraries::Font::OrbisFontOpenParams* params) {
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

std::filesystem::path ResolveGuestPath(const char* guest_path) {
    if (!guest_path) {
        return {};
    }
    if (guest_path[0] != '/') {
        return std::filesystem::path(guest_path);
    }
    if (!g_mnt) {
        g_mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    }
    if (!g_mnt) {
        return {};
    }
    return g_mnt->GetHostPath(guest_path);
}

bool LoadGuestFileBytes(const std::filesystem::path& host_path,
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

FaceMetrics LoadFaceMetrics(FT_Face face) {
    FaceMetrics m{};
    if (!face) {
        return m;
    }
    m.units_per_em = static_cast<int>(face->units_per_EM);

    if (const auto* hhea =
            static_cast<const TT_HoriHeader*>(FT_Get_Sfnt_Table(face, ft_sfnt_hhea))) {
        m.hhea_ascent = static_cast<int>(hhea->Ascender);
        m.hhea_descent = static_cast<int>(hhea->Descender);
        m.hhea_lineGap = static_cast<int>(hhea->Line_Gap);
    } else {
        m.hhea_ascent = static_cast<int>(face->ascender);
        m.hhea_descent = static_cast<int>(face->descender);
        const int height = static_cast<int>(face->height);
        m.hhea_lineGap = height - (m.hhea_ascent - m.hhea_descent);
    }

    if (const auto* os2 = static_cast<const TT_OS2*>(FT_Get_Sfnt_Table(face, ft_sfnt_os2))) {
        m.has_typo = true;
        m.typo_ascent = static_cast<int>(os2->sTypoAscender);
        m.typo_descent = static_cast<int>(os2->sTypoDescender);
        m.typo_lineGap = static_cast<int>(os2->sTypoLineGap);
        m.use_typo = (os2->fsSelection & (1u << 7)) != 0;
    }
    return m;
}

void PopulateStateMetrics(FontState& st, const FaceMetrics& m) {
    st.ext_units_per_em = m.units_per_em;
    st.ext_ascent = m.use_typo && m.has_typo ? m.typo_ascent : m.hhea_ascent;
    st.ext_descent = m.use_typo && m.has_typo ? m.typo_descent : m.hhea_descent;
    st.ext_lineGap = m.use_typo && m.has_typo ? m.typo_lineGap : m.hhea_lineGap;
    st.ext_typo_ascent = m.typo_ascent;
    st.ext_typo_descent = m.typo_descent;
    st.ext_typo_lineGap = m.typo_lineGap;
    st.ext_use_typo_metrics = m.use_typo && m.has_typo;
}

float ComputeScaleExtForState(const FontState& st, FT_Face face, float pixel_h) {
    if (st.ext_units_per_em > 0) {
        return pixel_h / static_cast<float>(st.ext_units_per_em);
    }
    if (!face || face->units_per_EM == 0) {
        return 0.0f;
    }
    return pixel_h / static_cast<float>(face->units_per_EM);
}

float ComputeScaleForPixelHeight(const FontState& st, float pixel_h) {
    if (st.ext_units_per_em <= 0) {
        return 0.0f;
    }
    return pixel_h / static_cast<float>(st.ext_units_per_em);
}

const float kPointsPerInch = 72.0f;
const float kScaleEpsilon = 1e-4f;

float SafeDpiToFloat(u32 dpi) {
    return dpi == 0 ? kPointsPerInch : static_cast<float>(dpi);
}

float PointsToPixels(float pt, u32 dpi) {
    return pt * SafeDpiToFloat(dpi) / kPointsPerInch;
}

float PixelsToPoints(float px, u32 dpi) {
    const float dpi_f = SafeDpiToFloat(dpi);
    return px * kPointsPerInch / dpi_f;
}

const u16 kStyleFrameMagic = 0x0F09;
const u8 kStyleFrameFlagScale = 0x01;
const u8 kStyleFrameFlagSlant = 0x02;
const u8 kStyleFrameFlagWeight = 0x04;

StyleFrameScaleState ResolveStyleFrameScale(const OrbisFontStyleFrame* style, const FontState& st) {
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
    if ((style->flags1 & kStyleFrameFlagScale) != 0) {
        const bool unit_is_pixel = (style->scaleUnit == 0);
        const u32 dpi_x = style->hDpi;
        const u32 dpi_y = style->vDpi;
        resolved.scale_w = unit_is_pixel ? style->scalePixelW
                                         : (dpi_x ? PointsToPixels(style->scalePixelW, dpi_x)
                                                  : style->scalePixelW);
        resolved.scale_h = unit_is_pixel ? style->scalePixelH
                                         : (dpi_y ? PointsToPixels(style->scalePixelH, dpi_y)
                                                  : style->scalePixelH);
        resolved.scale_overridden = true;
    }
    if (!resolved.scale_overridden && st.scale_point_active) {
        resolved.scale_w = PointsToPixels(st.scale_point_w, resolved.dpi_x);
        resolved.scale_h = PointsToPixels(st.scale_point_h, resolved.dpi_y);
    }
    return resolved;
}

void InitializeStyleFrame(OrbisFontStyleFrame& frame) {
    std::memset(&frame, 0, sizeof(frame));
    frame.magic = kStyleFrameMagic;
    frame.hDpi = 72;
    frame.vDpi = 72;
}

bool EnsureStyleFrameInitialized(OrbisFontStyleFrame* frame) {
    return ValidateStyleFramePtr(frame);
}

bool ValidateStyleFramePtr(const OrbisFontStyleFrame* frame) {
    return frame && frame->magic == kStyleFrameMagic;
}

void UpdateCachedLayout(FontState& st) {
    if (!st.ext_face_ready || !st.ext_ft_face) {
        return;
    }
    if (st.ext_scale_for_height == 0.0f) {
        st.ext_scale_for_height =
            ComputeScaleExtForState(st, st.ext_ft_face, st.scale_h * st.system_font_scale_factor);
    }
    const float scale = st.ext_scale_for_height;
    st.cached_baseline_y = static_cast<float>(st.ext_ascent) * scale;
    st.layout_cached = true;
}

std::uint64_t MakeGlyphKey(u32 code, int pixel_h) {
    return (static_cast<std::uint64_t>(code) << 32) | static_cast<std::uint32_t>(pixel_h);
}

void ClearRenderOutputs(Libraries::Font::OrbisFontGlyphMetrics* metrics,
                        Libraries::Font::OrbisFontRenderOutput* result) {
    if (metrics) {
        *metrics = {};
    }
    if (result) {
        *result = {};
    }
}

bool ResolveFaceAndScale(FontState& st, Libraries::Font::OrbisFontHandle handle, float pixel_h,
                         FT_Face& face_out, float& scale_y_out) {
    face_out = nullptr;
    scale_y_out = 0.0f;
    if (st.ext_face_ready) {
        face_out = st.ext_ft_face;
    }
    if (!face_out) {
        bool system_attached = false;
        const std::string sys_log = ReportSystemFaceRequest(st, handle, system_attached);
        if (!sys_log.empty()) {
            LOG_ERROR(Lib_Font, "{}", sys_log);
        }
        if (system_attached) {
            face_out = st.ext_ft_face;
        }
    }
    if (pixel_h <= kScaleEpsilon) {
        return false;
    }
    if (!face_out) {
        return false;
    }
    scale_y_out = ComputeScaleExtForState(st, face_out, pixel_h * st.system_font_scale_factor);
    return scale_y_out > kScaleEpsilon;
}

bool ResolveFace(FontState& st, Libraries::Font::OrbisFontHandle handle, FT_Face& face_out) {
    face_out = nullptr;
    if (st.ext_face_ready) {
        face_out = st.ext_ft_face;
    }
    if (!face_out) {
        bool system_attached = false;
        const std::string sys_log = ReportSystemFaceRequest(st, handle, system_attached);
        if (!sys_log.empty()) {
            LOG_ERROR(Lib_Font, "{}", sys_log);
        }
        if (system_attached) {
            face_out = st.ext_ft_face;
        }
    }
    return face_out != nullptr;
}

// LLE: FUN_0100a690
s32 RenderCodepointToSurface(FontState& st, Libraries::Font::OrbisFontHandle handle, FT_Face face,
                             float pixel_w, float pixel_h,
                             Libraries::Font::OrbisFontRenderSurface* surf, u32 code, float x,
                             float y, Libraries::Font::OrbisFontGlyphMetrics* metrics,
                             Libraries::Font::OrbisFontRenderOutput* result, s32 shift_x_units,
                             s32 shift_y_units) {
    ClearRenderOutputs(metrics, result);

    if (!surf || !metrics || !result) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    if (!surf->buffer || surf->width <= 0 || surf->height <= 0 || surf->widthByte <= 0 ||
        surf->pixelSizeByte <= 0) {
        return ORBIS_FONT_ERROR_NO_SUPPORT_SURFACE;
    }

    const int bpp = static_cast<int>(surf->pixelSizeByte);
    if (bpp != 1 && bpp != 4) {
        return ORBIS_FONT_ERROR_NO_SUPPORT_SURFACE;
    }

    FT_Face primary_face = face;
    if (!primary_face) {
        float primary_scale_y = 0.0f;
        if (!ResolveFaceAndScale(st, handle, pixel_h, primary_face, primary_scale_y)) {
            return ORBIS_FONT_ERROR_NO_SUPPORT_GLYPH;
        }
    }

    auto resolve_scale_factor = [&](FT_Face candidate) -> float {
        if (!candidate) {
            return st.system_font_scale_factor;
        }
        if (candidate == st.ext_ft_face) {
            return st.system_font_scale_factor;
        }
        for (const auto& fb : st.system_fallback_faces) {
            if (fb.ready && fb.ft_face == candidate) {
                return fb.scale_factor;
            }
        }
        return st.system_font_scale_factor;
    };

    FT_Face resolved_face = primary_face;
    FT_UInt resolved_glyph_index =
        resolved_face ? FT_Get_Char_Index(resolved_face, static_cast<FT_ULong>(code)) : 0;
    float resolved_scale_factor = resolve_scale_factor(resolved_face);
    if (resolved_glyph_index == 0) {
        if (st.ext_face_ready && st.ext_ft_face && resolved_face != st.ext_ft_face) {
            const FT_UInt gi = FT_Get_Char_Index(st.ext_ft_face, static_cast<FT_ULong>(code));
            if (gi != 0) {
                resolved_face = st.ext_ft_face;
                resolved_glyph_index = gi;
                resolved_scale_factor = st.system_font_scale_factor;
            }
        }
        if (resolved_glyph_index == 0) {
            for (const auto& fb : st.system_fallback_faces) {
                if (!fb.ready || !fb.ft_face) {
                    continue;
                }
                const FT_UInt gi = FT_Get_Char_Index(fb.ft_face, static_cast<FT_ULong>(code));
                if (gi == 0) {
                    continue;
                }
                resolved_face = fb.ft_face;
                resolved_glyph_index = gi;
                resolved_scale_factor = fb.scale_factor;
                break;
            }
        }
    }
    if (!resolved_face || resolved_glyph_index == 0) {
        return ORBIS_FONT_ERROR_NO_SUPPORT_GLYPH;
    }

    const auto set_size = [&](float w, float h) -> bool {
        const auto char_w = static_cast<FT_F26Dot6>(static_cast<s32>(w * 64.0f));
        const auto char_h = static_cast<FT_F26Dot6>(static_cast<s32>(h * 64.0f));
        return FT_Set_Char_Size(resolved_face, char_w, char_h, 72, 72) == 0;
    };
    if (!set_size(pixel_w * resolved_scale_factor, pixel_h * resolved_scale_factor)) {
        return ORBIS_FONT_ERROR_NO_SUPPORT_GLYPH;
    }

    const float frac_x = x - std::floor(x);
    const float frac_y = y - std::floor(y);
    FT_Vector delta{};
    delta.x = static_cast<FT_Pos>(static_cast<s32>(frac_x * 64.0f));
    delta.y = static_cast<FT_Pos>(-static_cast<s32>(frac_y * 64.0f));

    if ((shift_x_units != 0 || shift_y_units != 0) && resolved_face->size) {
        const long x_scale = static_cast<long>(resolved_face->size->metrics.x_scale);
        const long y_scale = static_cast<long>(resolved_face->size->metrics.y_scale);

        const auto round_fixed_mul = [](long fixed_16_16, s32 value) -> s32 {
            const long long prod =
                static_cast<long long>(fixed_16_16) * static_cast<long long>(value);
            const long long sign_adj =
                (static_cast<long long>(~static_cast<long long>(value)) >> 63) * -0x10000LL;
            const long long base = sign_adj + prod;
            long long tmp = base - 0x8000LL;
            if (tmp < 0) {
                tmp = base + 0x7FFFLL;
            }
            return static_cast<s32>(static_cast<u64>(tmp) >> 16);
        };

        delta.x += static_cast<FT_Pos>(round_fixed_mul(x_scale, shift_x_units));
        delta.y += static_cast<FT_Pos>(round_fixed_mul(y_scale, shift_y_units));
    }
    FT_Set_Transform(resolved_face, nullptr, &delta);

    if (FT_Load_Glyph(resolved_face, resolved_glyph_index, kFtLoadFlagsRender) != 0) {
        FT_Set_Transform(resolved_face, nullptr, nullptr);
        return ORBIS_FONT_ERROR_NO_SUPPORT_GLYPH;
    }
    std::vector<unsigned char> glyph_bitmap;
    int x0 = 0, y0 = 0;
    int glyph_w = 0;
    int glyph_h = 0;

    const FT_GlyphSlot slot = resolved_face->glyph;
    glyph_w = static_cast<int>(slot->bitmap.width);
    glyph_h = static_cast<int>(slot->bitmap.rows);
    x0 = static_cast<int>(slot->bitmap_left);
    y0 = -static_cast<int>(slot->bitmap_top);
    FT_Set_Transform(resolved_face, nullptr, nullptr);

    const float bearing_x = static_cast<float>(slot->metrics.horiBearingX) / 64.0f;
    const float bearing_y = static_cast<float>(slot->metrics.horiBearingY) / 64.0f;
    const float advance = static_cast<float>(slot->metrics.horiAdvance) / 64.0f;
    const float width_px = static_cast<float>(slot->metrics.width) / 64.0f;
    const float height_px = static_cast<float>(slot->metrics.height) / 64.0f;

    metrics->width = width_px;
    metrics->height = height_px;
    metrics->Horizontal.bearingX = bearing_x;
    metrics->Horizontal.bearingY = bearing_y;
    metrics->Horizontal.advance = advance;
    metrics->Vertical.bearingX = 0.0f;
    metrics->Vertical.bearingY = 0.0f;
    metrics->Vertical.advance = 0.0f;

    glyph_bitmap.resize(static_cast<std::size_t>(glyph_w) * static_cast<std::size_t>(glyph_h));
    if (glyph_w > 0 && glyph_h > 0) {
        const int pitch = static_cast<int>(slot->bitmap.pitch);
        const unsigned char* src = reinterpret_cast<const unsigned char*>(slot->bitmap.buffer);
        if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {
            for (int row = 0; row < glyph_h; ++row) {
                const unsigned char* src_row = src + static_cast<std::size_t>(row) * pitch;
                unsigned char* dst_row =
                    glyph_bitmap.data() + static_cast<std::size_t>(row) * glyph_w;
                for (int col = 0; col < glyph_w; ++col) {
                    const unsigned char byte = src_row[col >> 3];
                    const unsigned char bit = static_cast<unsigned char>(0x80u >> (col & 7));
                    dst_row[col] = (byte & bit) ? 0xFFu : 0x00u;
                }
            }
        } else {
            for (int row = 0; row < glyph_h; ++row) {
                const unsigned char* src_row = src + static_cast<std::size_t>(row) * pitch;
                unsigned char* dst_row =
                    glyph_bitmap.data() + static_cast<std::size_t>(row) * glyph_w;
                std::memcpy(dst_row, src_row, static_cast<std::size_t>(glyph_w));
            }
        }
    }

    const int dest_x = static_cast<int>(std::floor(x)) + x0;
    const int dest_y = static_cast<int>(std::floor(y)) + y0;

    const int surface_w = std::max(surf->width, 0);
    const int surface_h = std::max(surf->height, 0);
    const int clip_x0 = std::clamp(static_cast<int>(surf->sc_x0), 0, surface_w);
    const int clip_y0 = std::clamp(static_cast<int>(surf->sc_y0), 0, surface_h);
    const int clip_x1 = std::clamp(static_cast<int>(surf->sc_x1), 0, surface_w);
    const int clip_y1 = std::clamp(static_cast<int>(surf->sc_y1), 0, surface_h);

    int update_x0 = dest_x;
    int update_y0 = dest_y;
    int update_w = 0;
    int update_h = 0;

    if (glyph_w > 0 && glyph_h > 0 && clip_x1 > clip_x0 && clip_y1 > clip_y0) {
        auto* dst_base = static_cast<std::uint8_t*>(surf->buffer);
        const int start_row = std::max(dest_y, clip_y0);
        const int end_row = std::min(dest_y + glyph_h, clip_y1);
        const int start_col = std::max(dest_x, clip_x0);
        const int end_col = std::min(dest_x + glyph_w, clip_x1);

        update_x0 = start_col;
        update_y0 = start_row;
        update_w = std::max(0, end_col - start_col);
        update_h = std::max(0, end_row - start_row);

        for (int row = start_row; row < end_row; ++row) {
            const int src_y = row - dest_y;
            if (src_y < 0 || src_y >= glyph_h)
                continue;
            const std::uint8_t* src_row =
                glyph_bitmap.data() + static_cast<std::size_t>(src_y) * glyph_w;
            std::uint8_t* dst_row = dst_base + static_cast<std::size_t>(row) *
                                                   static_cast<std::size_t>(surf->widthByte);
            for (int col = start_col; col < end_col; ++col) {
                const int src_x = col - dest_x;
                if (src_x < 0 || src_x >= glyph_w)
                    continue;
                const std::uint8_t cov = src_row[src_x];
                std::uint8_t* dst = dst_row + static_cast<std::size_t>(col) * bpp;
                if (bpp == 1) {
                    dst[0] = cov;
                } else {
                    dst[0] = cov;
                    dst[1] = cov;
                    dst[2] = cov;
                    dst[3] = cov;
                }
            }
        }
    }

    result->stage = nullptr;
    result->SurfaceImage.address = static_cast<u8*>(surf->buffer);
    result->SurfaceImage.widthByte = static_cast<u32>(surf->widthByte);
    result->SurfaceImage.pixelSizeByte = static_cast<u8>(surf->pixelSizeByte);
    result->SurfaceImage.pixelFormat = 0;
    result->SurfaceImage.pad16 = 0;
    result->UpdateRect.x = static_cast<u32>(std::max(update_x0, 0));
    result->UpdateRect.y = static_cast<u32>(std::max(update_y0, 0));
    result->UpdateRect.w = static_cast<u32>(std::max(update_w, 0));
    result->UpdateRect.h = static_cast<u32>(std::max(update_h, 0));
    result->SurfaceImage.address =
        static_cast<u8*>(surf->buffer) +
        static_cast<std::size_t>(result->UpdateRect.y) * static_cast<std::size_t>(surf->widthByte) +
        static_cast<std::size_t>(result->UpdateRect.x) * static_cast<std::size_t>(bpp);
    const auto floor_int = [](float v) -> int {
        int i = static_cast<int>(std::trunc(v));
        if (static_cast<float>(i) > v) {
            --i;
        }
        return i;
    };
    const auto ceil_int = [](float v) -> int {
        int i = static_cast<int>(std::trunc(v));
        if (static_cast<float>(i) < v) {
            ++i;
        }
        return i;
    };

    const float left_f = x + metrics->Horizontal.bearingX;
    const float top_f = y + metrics->Horizontal.bearingY;
    const float right_f = left_f + metrics->width;
    const float bottom_f = top_f - metrics->height;

    const int left_i = floor_int(left_f);
    const int top_i = floor_int(top_f);
    const int right_i = ceil_int(right_f);
    const int bottom_i = ceil_int(bottom_f);

    const float adv_f = x + metrics->Horizontal.advance;
    const float adv_snapped = static_cast<float>(floor_int(adv_f)) - x;

    result->ImageMetrics.bearingX = static_cast<float>(left_i) - x;
    result->ImageMetrics.bearingY = static_cast<float>(top_i) - y;
    result->ImageMetrics.advance = adv_snapped;
    int stride_i = right_i + 1;
    const float adjust = static_cast<float>(right_i) - right_f;
    const int tmp_i = floor_int(adv_f + adjust);
    const int adv_trunc_i = static_cast<int>(std::trunc(adv_f));
    if (adv_trunc_i == 0) {
        stride_i = tmp_i;
    }
    if (stride_i < tmp_i) {
        stride_i = tmp_i;
    }
    result->ImageMetrics.stride = static_cast<float>(stride_i) - x;
    result->ImageMetrics.width = static_cast<u32>(std::max(0, right_i - left_i));
    result->ImageMetrics.height = static_cast<u32>(std::max(0, top_i - bottom_i));
    return ORBIS_OK;
}

// LLE: FUN_0100e050
s32 RenderCodepointToSurfaceWithScale(FontState& st, Libraries::Font::OrbisFontHandle handle,
                                      FT_Face face, float scale_x, float scale_y,
                                      Libraries::Font::OrbisFontRenderSurface* surf, u32 code,
                                      float x, float y, Libraries::Font::OrbisFontGlyphMetrics* m,
                                      Libraries::Font::OrbisFontRenderOutput* result) {
    ClearRenderOutputs(m, result);

    if (!surf || !m || !result) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    if (scale_x <= kScaleEpsilon || scale_y <= kScaleEpsilon) {
        return ORBIS_FONT_ERROR_NO_SUPPORT_GLYPH;
    }
    if (!surf->buffer || surf->width <= 0 || surf->height <= 0 || surf->widthByte <= 0 ||
        surf->pixelSizeByte <= 0) {
        return ORBIS_FONT_ERROR_NO_SUPPORT_SURFACE;
    }
    const int bpp = static_cast<int>(surf->pixelSizeByte);
    if (bpp != 1 && bpp != 4) {
        return ORBIS_FONT_ERROR_NO_SUPPORT_SURFACE;
    }

    FT_Face primary_face = face;
    if (!ResolveFace(st, handle, primary_face)) {
        return ORBIS_FONT_ERROR_NO_SUPPORT_GLYPH;
    }

    FT_Face resolved_face = primary_face;
    FT_UInt resolved_glyph_index =
        resolved_face ? FT_Get_Char_Index(resolved_face, static_cast<FT_ULong>(code)) : 0;
    float resolved_scale_factor = st.system_font_scale_factor;
    if (resolved_glyph_index == 0) {
        for (const auto& fb : st.system_fallback_faces) {
            if (!fb.ready || !fb.ft_face) {
                continue;
            }
            const FT_UInt gi = FT_Get_Char_Index(fb.ft_face, static_cast<FT_ULong>(code));
            if (gi == 0) {
                continue;
            }
            resolved_face = fb.ft_face;
            resolved_glyph_index = gi;
            resolved_scale_factor = fb.scale_factor;
            break;
        }
    }
    if (!resolved_face || resolved_glyph_index == 0) {
        return ORBIS_FONT_ERROR_NO_SUPPORT_GLYPH;
    }

    const int base_units = (st.ext_units_per_em > 0)
                               ? st.ext_units_per_em
                               : static_cast<int>(resolved_face->units_per_EM);
    if (base_units <= 0) {
        return ORBIS_FONT_ERROR_NO_SUPPORT_GLYPH;
    }

    const float pixel_w = scale_x * static_cast<float>(base_units) * resolved_scale_factor;
    const float pixel_h = scale_y * static_cast<float>(base_units) * resolved_scale_factor;

    const auto set_size = [&](float w, float h) -> bool {
        const auto char_w = static_cast<FT_F26Dot6>(static_cast<s32>(w * 64.0f));
        const auto char_h = static_cast<FT_F26Dot6>(static_cast<s32>(h * 64.0f));
        return FT_Set_Char_Size(resolved_face, char_w, char_h, 72, 72) == 0;
    };
    if (!set_size(pixel_w, pixel_h)) {
        return ORBIS_FONT_ERROR_NO_SUPPORT_GLYPH;
    }

    const float frac_x = x - std::floor(x);
    const float frac_y = y - std::floor(y);
    FT_Vector delta{};
    delta.x = static_cast<FT_Pos>(static_cast<s32>(frac_x * 64.0f));
    delta.y = static_cast<FT_Pos>(-static_cast<s32>(frac_y * 64.0f));
    FT_Set_Transform(resolved_face, nullptr, &delta);

    if (FT_Load_Glyph(resolved_face, resolved_glyph_index, kFtLoadFlagsRender) != 0) {
        FT_Set_Transform(resolved_face, nullptr, nullptr);
        return ORBIS_FONT_ERROR_NO_SUPPORT_GLYPH;
    }
    std::vector<unsigned char> glyph_bitmap;
    int x0 = 0, y0 = 0;
    int glyph_w = 0;
    int glyph_h = 0;

    const FT_GlyphSlot slot = resolved_face->glyph;
    glyph_w = static_cast<int>(slot->bitmap.width);
    glyph_h = static_cast<int>(slot->bitmap.rows);
    x0 = static_cast<int>(slot->bitmap_left);
    y0 = -static_cast<int>(slot->bitmap_top);
    FT_Set_Transform(resolved_face, nullptr, nullptr);

    const float bearing_x = static_cast<float>(slot->metrics.horiBearingX) / 64.0f;
    const float bearing_y = static_cast<float>(slot->metrics.horiBearingY) / 64.0f;
    const float advance = static_cast<float>(slot->metrics.horiAdvance) / 64.0f;
    const float width_px = static_cast<float>(slot->metrics.width) / 64.0f;
    const float height_px = static_cast<float>(slot->metrics.height) / 64.0f;

    m->width = width_px;
    m->height = height_px;
    m->Horizontal.bearingX = bearing_x;
    m->Horizontal.bearingY = bearing_y;
    m->Horizontal.advance = advance;
    m->Vertical.bearingX = 0.0f;
    m->Vertical.bearingY = 0.0f;
    m->Vertical.advance = 0.0f;

    glyph_bitmap.resize(static_cast<std::size_t>(glyph_w) * static_cast<std::size_t>(glyph_h));
    if (glyph_w > 0 && glyph_h > 0) {
        const int pitch = static_cast<int>(slot->bitmap.pitch);
        const unsigned char* src = reinterpret_cast<const unsigned char*>(slot->bitmap.buffer);
        if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {
            for (int row = 0; row < glyph_h; ++row) {
                const unsigned char* src_row = src + static_cast<std::size_t>(row) * pitch;
                unsigned char* dst_row =
                    glyph_bitmap.data() + static_cast<std::size_t>(row) * glyph_w;
                for (int col = 0; col < glyph_w; ++col) {
                    const unsigned char byte = src_row[col >> 3];
                    const unsigned char bit = static_cast<unsigned char>(0x80u >> (col & 7));
                    dst_row[col] = (byte & bit) ? 0xFFu : 0x00u;
                }
            }
        } else {
            for (int row = 0; row < glyph_h; ++row) {
                const unsigned char* src_row = src + static_cast<std::size_t>(row) * pitch;
                unsigned char* dst_row =
                    glyph_bitmap.data() + static_cast<std::size_t>(row) * glyph_w;
                std::memcpy(dst_row, src_row, static_cast<std::size_t>(glyph_w));
            }
        }
    }

    const int dest_x = static_cast<int>(std::floor(x)) + x0;
    const int dest_y = static_cast<int>(std::floor(y)) + y0;

    const int surface_w = std::max(surf->width, 0);
    const int surface_h = std::max(surf->height, 0);
    const int clip_x0 = std::clamp(static_cast<int>(surf->sc_x0), 0, surface_w);
    const int clip_y0 = std::clamp(static_cast<int>(surf->sc_y0), 0, surface_h);
    const int clip_x1 = std::clamp(static_cast<int>(surf->sc_x1), 0, surface_w);
    const int clip_y1 = std::clamp(static_cast<int>(surf->sc_y1), 0, surface_h);

    int update_x0 = dest_x;
    int update_y0 = dest_y;
    int update_w = 0;
    int update_h = 0;

    if (glyph_w > 0 && glyph_h > 0 && clip_x1 > clip_x0 && clip_y1 > clip_y0) {
        auto* dst_base = static_cast<std::uint8_t*>(surf->buffer);
        const int start_row = std::max(dest_y, clip_y0);
        const int end_row = std::min(dest_y + glyph_h, clip_y1);
        const int start_col = std::max(dest_x, clip_x0);
        const int end_col = std::min(dest_x + glyph_w, clip_x1);

        update_x0 = start_col;
        update_y0 = start_row;
        update_w = std::max(0, end_col - start_col);
        update_h = std::max(0, end_row - start_row);

        for (int row = start_row; row < end_row; ++row) {
            const int src_y = row - dest_y;
            if (src_y < 0 || src_y >= glyph_h)
                continue;
            const std::uint8_t* src_row =
                glyph_bitmap.data() + static_cast<std::size_t>(src_y) * glyph_w;
            std::uint8_t* dst_row = dst_base + static_cast<std::size_t>(row) *
                                                   static_cast<std::size_t>(surf->widthByte);
            for (int col = start_col; col < end_col; ++col) {
                const int src_x = col - dest_x;
                if (src_x < 0 || src_x >= glyph_w)
                    continue;
                const std::uint8_t cov = src_row[src_x];
                std::uint8_t* dst = dst_row + static_cast<std::size_t>(col) * bpp;
                if (bpp == 1) {
                    dst[0] = cov;
                } else {
                    dst[0] = cov;
                    dst[1] = cov;
                    dst[2] = cov;
                    dst[3] = cov;
                }
            }
        }
    }

    result->stage = nullptr;
    result->SurfaceImage.address = static_cast<u8*>(surf->buffer);
    result->SurfaceImage.widthByte = static_cast<u32>(surf->widthByte);
    result->SurfaceImage.pixelSizeByte = static_cast<u8>(surf->pixelSizeByte);
    result->SurfaceImage.pixelFormat = 0;
    result->SurfaceImage.pad16 = 0;
    result->UpdateRect.x = static_cast<u32>(std::max(update_x0, 0));
    result->UpdateRect.y = static_cast<u32>(std::max(update_y0, 0));
    result->UpdateRect.w = static_cast<u32>(std::max(update_w, 0));
    result->UpdateRect.h = static_cast<u32>(std::max(update_h, 0));
    result->SurfaceImage.address =
        static_cast<u8*>(surf->buffer) +
        static_cast<std::size_t>(result->UpdateRect.y) * static_cast<std::size_t>(surf->widthByte) +
        static_cast<std::size_t>(result->UpdateRect.x) * static_cast<std::size_t>(bpp);
    const auto floor_int = [](float v) -> int {
        int i = static_cast<int>(std::trunc(v));
        if (static_cast<float>(i) > v) {
            --i;
        }
        return i;
    };
    const auto ceil_int = [](float v) -> int {
        int i = static_cast<int>(std::trunc(v));
        if (static_cast<float>(i) < v) {
            ++i;
        }
        return i;
    };

    const float left_f = x + m->Horizontal.bearingX;
    const float top_f = y + m->Horizontal.bearingY;
    const float right_f = left_f + m->width;
    const float bottom_f = top_f - m->height;

    const int left_i = floor_int(left_f);
    const int top_i = floor_int(top_f);
    const int right_i = ceil_int(right_f);
    const int bottom_i = ceil_int(bottom_f);

    const float adv_f = x + m->Horizontal.advance;
    const float adv_snapped = static_cast<float>(floor_int(adv_f)) - x;

    result->ImageMetrics.bearingX = static_cast<float>(left_i) - x;
    result->ImageMetrics.bearingY = static_cast<float>(top_i) - y;
    result->ImageMetrics.advance = adv_snapped;
    int stride_i = right_i + 1;
    const float adjust = static_cast<float>(right_i) - right_f;
    const int tmp_i = floor_int(adv_f + adjust);
    const int adv_trunc_i = static_cast<int>(std::trunc(adv_f));
    if (adv_trunc_i == 0) {
        stride_i = tmp_i;
    }
    if (stride_i < tmp_i) {
        stride_i = tmp_i;
    }
    result->ImageMetrics.stride = static_cast<float>(stride_i) - x;
    result->ImageMetrics.width = static_cast<u32>(std::max(0, right_i - left_i));
    result->ImageMetrics.height = static_cast<u32>(std::max(0, top_i - bottom_i));
    return ORBIS_OK;
}

const GlyphEntry* GetGlyphEntry(FontState& st, Libraries::Font::OrbisFontHandle handle, u32 code,
                                FT_Face& face_out, float& scale_out) {
    face_out = nullptr;
    scale_out = 0.0f;

    if (st.ext_face_ready) {
        face_out = st.ext_ft_face;
    }
    if (!face_out) {
        bool system_attached = false;
        const std::string sys_log = ReportSystemFaceRequest(st, handle, system_attached);
        if (!sys_log.empty()) {
            LOG_ERROR(Lib_Font, "SYSTEM_FONT_FAILED");
            LOG_DEBUG(Lib_Font, "{}", sys_log);
        }
        if (system_attached) {
            face_out = st.ext_ft_face;
        }
    }
    if (!face_out) {
        return nullptr;
    }

    float resolved_scale_factor = st.system_font_scale_factor;
    FT_Face resolved_face = face_out;
    FT_UInt resolved_glyph_index = FT_Get_Char_Index(resolved_face, static_cast<FT_ULong>(code));
    if (resolved_glyph_index == 0) {
        for (const auto& fb : st.system_fallback_faces) {
            if (!fb.ready || !fb.ft_face) {
                continue;
            }
            const FT_UInt gi = FT_Get_Char_Index(fb.ft_face, static_cast<FT_ULong>(code));
            if (gi == 0) {
                continue;
            }
            resolved_face = fb.ft_face;
            resolved_glyph_index = gi;
            resolved_scale_factor = fb.scale_factor;
            break;
        }
    }
    if (!resolved_face || resolved_glyph_index == 0) {
        return nullptr;
    }

    const float pixel_h_f = st.scale_h * resolved_scale_factor;
    const int pixel_h = std::max(1, static_cast<int>(std::lround(pixel_h_f)));
    const std::uint64_t key = MakeGlyphKey(code, pixel_h);

    GlyphEntry* ge = nullptr;
    if (auto it = st.ext_cache.find(key); it != st.ext_cache.end()) {
        ge = &it->second;
    }
    if (!ge) {
        const auto set_size = [&](float h) -> bool {
            const auto char_h = static_cast<FT_F26Dot6>(static_cast<s32>(h * 64.0f));
            return FT_Set_Char_Size(resolved_face, 0, char_h, 72, 72) == 0;
        };
        if (!set_size(pixel_h_f)) {
            return nullptr;
        }

        if (FT_Load_Glyph(resolved_face, resolved_glyph_index, kFtLoadFlagsRender) != 0) {
            return nullptr;
        }

        const FT_GlyphSlot slot = resolved_face->glyph;
        GlyphEntry entry{};
        entry.w = static_cast<int>(slot->bitmap.width);
        entry.h = static_cast<int>(slot->bitmap.rows);
        entry.x0 = static_cast<int>(slot->bitmap_left);
        entry.y0 = -static_cast<int>(slot->bitmap_top);
        entry.x1 = entry.x0 + entry.w;
        entry.y1 = entry.y0 + entry.h;
        entry.advance = static_cast<float>(slot->metrics.horiAdvance) / 64.0f;
        entry.bearingX = static_cast<float>(slot->bitmap_left);
        ge = &st.ext_cache.emplace(key, std::move(entry)).first->second;
    }

    face_out = resolved_face;
    scale_out = ComputeScaleExtForState(st, resolved_face, pixel_h_f);
    return ge;
}

constexpr SystemFontDefinition kSystemFontDefinitions[] = {
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

std::mutex g_fontset_cache_mutex;
std::unordered_map<u32, FontSetCache> g_fontset_cache;

const SystemFontDefinition* FindSystemFontDefinition(u32 font_set_type) {
    for (const auto& def : kSystemFontDefinitions) {
        if (def.font_set_type == font_set_type) {
            return &def;
        }
    }
    return nullptr;
}

static bool DirectoryContainsAnyFontFiles(const std::filesystem::path& dir) {
    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec)) {
        return false;
    }
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (ec) {
            return false;
        }
        if (!entry.is_regular_file(ec) || ec) {
            continue;
        }
        const auto ext = entry.path().extension().string();
        std::string lower;
        lower.reserve(ext.size());
        for (const char c : ext) {
            lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
        if (lower == ".otf" || lower == ".ttf" || lower == ".ttc" || lower == ".otc") {
            return true;
        }
    }
    return false;
}

static std::optional<std::filesystem::path> FindChildDirContainingFile(
    const std::filesystem::path& base_dir, const std::string& filename) {
    if (filename.empty()) {
        return std::nullopt;
    }

    std::error_code ec;
    if (!std::filesystem::is_directory(base_dir, ec)) {
        return std::nullopt;
    }

    std::optional<std::filesystem::path> match;
    for (const auto& entry : std::filesystem::directory_iterator(base_dir, ec)) {
        if (ec) {
            return std::nullopt;
        }
        if (!entry.is_directory(ec) || ec) {
            continue;
        }
        const auto candidate = entry.path() / filename;
        if (std::filesystem::is_regular_file(candidate, ec) && !ec) {
            if (match) {
                return std::nullopt;
            }
            match = entry.path();
        }
    }
    return match;
}

std::filesystem::path GetSysFontBaseDir() {
    std::filesystem::path base = Config::getSysFontPath();
    std::error_code ec;
    if (base.empty()) {
        LOG_ERROR(Lib_Font, "SystemFonts: SysFontPath not set");
        return {};
    }
    if (std::filesystem::is_directory(base, ec)) {
        if (DirectoryContainsAnyFontFiles(base)) {
            return base;
        }

        {
            const auto preferred = base / "font";
            if (DirectoryContainsAnyFontFiles(preferred)) {
                return preferred;
            }
        }

        const std::string fallback = Config::getSystemFontFallbackName();
        if (auto child = FindChildDirContainingFile(base, fallback)) {
            return *child;
        }

        std::optional<std::filesystem::path> sole_font_dir;
        for (const auto& entry : std::filesystem::directory_iterator(base, ec)) {
            if (ec) {
                break;
            }
            if (!entry.is_directory(ec) || ec) {
                continue;
            }
            if (DirectoryContainsAnyFontFiles(entry.path())) {
                if (sole_font_dir) {
                    sole_font_dir.reset();
                    break;
                }
                sole_font_dir = entry.path();
            }
        }
        if (sole_font_dir) {
            return *sole_font_dir;
        }

        LOG_ERROR(
            Lib_Font,
            "SystemFonts: SysFontPath '{}' contains no font files; set it to the directory that "
            "contains the .otf/.ttf files (or ensure [SystemFonts].fallback is present in exactly "
            "one child directory)",
            base.string());
        return {};
    }
    if (std::filesystem::is_regular_file(base, ec)) {
        return base.parent_path();
    }
    LOG_ERROR(Lib_Font, "SystemFonts: SysFontPath '{}' is not a valid directory or file",
              base.string());
    return {};
}

std::string MacroToCamel(const char* macro_key) {
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

std::filesystem::path ResolveSystemFontPath(u32 font_set_type) {
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
        }
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

std::filesystem::path ResolveSystemFontPathFromConfigOnly(u32 font_set_type) {
    const auto base_dir = GetSysFontBaseDir();
    if (base_dir.empty()) {
        return {};
    }

    const std::string key_a = fmt::format("fontset_0x{:08X}", font_set_type);
    const std::string key_b = fmt::format("fontSet_0x{:08X}", font_set_type);

    auto try_key = [&](const std::string& key) -> std::optional<std::filesystem::path> {
        if (auto override_path = Config::getSystemFontOverride(key)) {
            if (!override_path->empty() && override_path->is_absolute()) {
                return *override_path;
            }
            if (!override_path->empty() && !override_path->has_parent_path()) {
                return base_dir / *override_path;
            }
            LOG_ERROR(
                Lib_Font,
                "SystemFonts: override for '{}' must be a filename only or absolute path: '{}'",
                key, override_path->string());
        }
        return std::nullopt;
    };

    if (auto p = try_key(key_a)) {
        return *p;
    }
    if (auto p = try_key(key_b)) {
        return *p;
    }

    const std::string fallback = Config::getSystemFontFallbackName();
    if (!fallback.empty()) {
        return base_dir / fallback;
    }
    return {};
}

const FontSetCache* EnsureFontSetCache(u32 font_set_type) {
    if (font_set_type == 0) {
        return nullptr;
    }
    std::scoped_lock lock(g_fontset_cache_mutex);
    auto& cache = g_fontset_cache[font_set_type];
    if (!cache.loaded) {
        cache.loaded = true;
        const auto path = ResolveSystemFontPath(font_set_type);
        if (!path.empty() && LoadFontFile(path, cache.bytes)) {
            cache.available = HasSfntTables(cache.bytes);
            cache.path = cache.available ? path : std::filesystem::path{};
        } else {
            cache.available = false;
            LOG_ERROR(Lib_Font, "SystemFonts: failed to load font for set type=0x{:08X} path='{}'",
                      font_set_type, path.string());
        }
    }
    return cache.available ? &cache : nullptr;
}

bool HasSfntTables(const std::vector<unsigned char>& bytes) {
    if (bytes.size() < 12) {
        return false;
    }
    const u8* ptr = bytes.data();
    const u32 num_tables = (ptr[4] << 8) | ptr[5];
    constexpr u32 kDirEntrySize = 16;
    if (bytes.size() < 12 + static_cast<size_t>(num_tables) * kDirEntrySize) {
        return false;
    }
    return true;
}

bool LoadFontFile(const std::filesystem::path& path, std::vector<unsigned char>& out_bytes) {
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
    if (!file_name.empty() && (parent_name == "font" || parent_name == "font2")) {
        const auto container = parent.parent_path();
        const auto sibling = container / ((parent_name == "font") ? "font2" : "font") / file_name;
        if (std::filesystem::is_regular_file(sibling, ec) && !ec) {
            return try_load(sibling);
        }
    }

    return false;
}

bool AttachSystemFont(FontState& st, Libraries::Font::OrbisFontHandle handle) {
    if (st.ext_face_ready) {
        return true;
    }
    st.system_fallback_faces.clear();
    st.system_font_id = 0;
    st.system_font_scale_factor = 1.0f;
    st.system_font_shift_value = 0;

    const auto* native = reinterpret_cast<const Libraries::Font::FontHandleOpaqueNative*>(handle);
    const u32 subfont_index = native ? native->open_info.sub_font_index : 0u;
    auto* lib = static_cast<FontLibOpaque*>(st.library);
    if (!lib || lib->magic != 0x0F01 || st.font_set_type == 0) {
        return false;
    }

    std::filesystem::path primary_path = ResolveSystemFontPathFromConfigOnly(st.font_set_type);
    if (primary_path.empty()) {
        primary_path = ResolveSystemFontPath(st.font_set_type);
    }
    std::vector<unsigned char> primary_bytes;
    if (primary_path.empty() || !LoadFontFile(primary_path, primary_bytes)) {
        return false;
    }

    DestroyFreeTypeFace(st.ext_ft_face);
    st.ext_face_data = std::move(primary_bytes);
    st.ext_ft_face = CreateFreeTypeFaceFromBytes(st.ext_face_data.data(), st.ext_face_data.size(),
                                                 subfont_index);
    if (!st.ext_ft_face) {
        st.ext_face_data.clear();
        return false;
    }

    st.ext_cache.clear();
    st.scratch.clear();
    st.logged_ext_use = false;
    st.ext_scale_for_height = 0.0f;
    st.layout_cached = false;
    const FaceMetrics m = LoadFaceMetrics(st.ext_ft_face);
    PopulateStateMetrics(st, m);
    st.ext_face_ready = true;
    st.system_font_path = primary_path;
    st.system_requested = true;

    const auto base_dir = GetSysFontBaseDir();
    if (!base_dir.empty()) {
        auto resolve_override =
            [&](const std::string& key) -> std::optional<std::filesystem::path> {
            if (auto override_path = Config::getSystemFontOverride(key)) {
                if (!override_path->empty() && override_path->is_absolute()) {
                    return *override_path;
                }
                if (!override_path->empty() && !override_path->has_parent_path()) {
                    return base_dir / *override_path;
                }
            }
            return std::nullopt;
        };

        for (int i = 0; i < 8; ++i) {
            const std::string key_a =
                fmt::format("fontset_0x{:08X}_fallback{}", st.font_set_type, i);
            const std::string key_b =
                fmt::format("fontSet_0x{:08X}_fallback{}", st.font_set_type, i);
            std::optional<std::filesystem::path> p = resolve_override(key_a);
            if (!p) {
                p = resolve_override(key_b);
            }
            if (!p || p->empty()) {
                continue;
            }

            std::vector<unsigned char> fb_bytes;
            if (!LoadFontFile(*p, fb_bytes)) {
                continue;
            }
            FontState::SystemFallbackFace fb{};
            fb.font_id = static_cast<u32>(i + 1);
            fb.scale_factor = 1.0f;
            fb.shift_value = 0;
            fb.path = *p;
            fb.bytes = std::make_shared<std::vector<unsigned char>>(std::move(fb_bytes));
            fb.ft_face =
                CreateFreeTypeFaceFromBytes(fb.bytes->data(), fb.bytes->size(), subfont_index);
            fb.ready = (fb.ft_face != nullptr);
            if (fb.ready) {
                st.system_fallback_faces.push_back(std::move(fb));
            } else {
                DestroyFreeTypeFace(fb.ft_face);
            }
        }

        const std::string global_fallback_name = Config::getSystemFontFallbackName();
        if (!global_fallback_name.empty()) {
            const auto existing_name = primary_path.filename().string();
            if (existing_name != global_fallback_name) {
                const std::filesystem::path fb_path = base_dir / global_fallback_name;
                std::vector<unsigned char> fb_bytes;
                if (LoadFontFile(fb_path, fb_bytes)) {
                    FontState::SystemFallbackFace fb{};
                    fb.font_id = 0xFFFFFFFFu;
                    fb.scale_factor = 1.0f;
                    fb.shift_value = 0;
                    fb.path = fb_path;
                    fb.bytes = std::make_shared<std::vector<unsigned char>>(std::move(fb_bytes));
                    fb.ft_face = CreateFreeTypeFaceFromBytes(fb.bytes->data(), fb.bytes->size(),
                                                             subfont_index);
                    fb.ready = (fb.ft_face != nullptr);
                    if (fb.ready) {
                        st.system_fallback_faces.push_back(std::move(fb));
                    } else {
                        DestroyFreeTypeFace(fb.ft_face);
                    }
                }
            }
        }
    }

    LOG_INFO(Lib_Font, "system font attached");
    LOG_DEBUG(Lib_Font,
              "system font attach params:\n"
              " handle={}\n"
              " font_set_type=0x{:08X}\n"
              " path={}\n",
              static_cast<const void*>(handle), st.font_set_type, st.system_font_path.string());
    return true;
}

std::string ReportSystemFaceRequest(FontState& st, Libraries::Font::OrbisFontHandle handle,
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

} // namespace Libraries::Font::Internal
