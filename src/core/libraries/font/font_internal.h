// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

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
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "common/config.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/file_sys/fs.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/font/font.h"
#include "core/libraries/libs.h"
#include "font_error.h"

namespace Libraries::Font::Internal {

struct FontLibOpaque;
struct OrbisFontRenderer_;
struct FontSetCache;

template <typename T>
std::string DescribeValue(const T& value) {
    using Clean = std::remove_cvref_t<T>;
    if constexpr (std::is_same_v<Clean, bool>) {
        return value ? "true" : "false";
    } else if constexpr (std::is_pointer_v<Clean>) {
        if constexpr (std::is_same_v<Clean, const char*> || std::is_same_v<Clean, char*>) {
            return value ? fmt::format("\"{}\"", value) : "(null)";
        }
        return fmt::format("{}", reinterpret_cast<const void*>(value));
    } else if constexpr (std::is_floating_point_v<Clean>) {
        return fmt::format("{:.6g}", value);
    } else if constexpr (std::is_enum_v<Clean>) {
        using Underlying = std::underlying_type_t<Clean>;
        return DescribeValue(static_cast<Underlying>(value));
    } else if constexpr (std::is_same_v<Clean, std::string> ||
                         std::is_same_v<Clean, std::string_view>) {
        return fmt::format("\"{}\"", value);
    } else {
        return fmt::format("{}", value);
    }
}

struct ParamRecord {
    std::string name;
    std::string initial;
    std::string current;
};

template <typename T>
struct ChangeTracked {
    T before;
    T after;
};

template <typename T>
struct is_change_tracked : std::false_type {};

template <typename T>
struct is_change_tracked<ChangeTracked<T>> : std::true_type {
    using value_type = T;
};

template <typename T>
ChangeTracked<T> trackChange(T before, T after) {
    return ChangeTracked<T>{before, after};
}

inline std::string TrimName(std::string_view name) {
    const auto start = name.find_first_not_of(" \t\n\r");
    if (start == std::string_view::npos) {
        return {};
    }
    const auto end = name.find_last_not_of(" \t\n\r");
    return std::string{name.substr(start, end - start + 1)};
}

inline std::vector<std::string> SplitArgumentNames(std::string_view names) {
    std::vector<std::string> result;
    std::string current;
    int depth = 0;
    for (char c : names) {
        if (c == ',' && depth == 0) {
            result.push_back(TrimName(current));
            current.clear();
            continue;
        }
        if (c == '(' || c == '{' || c == '[') {
            ++depth;
        } else if (c == ')' || c == '}' || c == ']') {
            depth = std::max(0, depth - 1);
        }
        current.push_back(c);
    }
    if (!current.empty() ||
        (!names.empty() && names.find_first_not_of(" \t\n\r") != std::string_view::npos)) {
        result.push_back(TrimName(current));
    }
    return result;
}

template <typename T>
ParamRecord MakeParamRecord(std::string name, T&& value) {
    using Clean = std::remove_cvref_t<T>;
    if constexpr (is_change_tracked<Clean>::value) {
        using ValueType = typename Clean::value_type;
        return ParamRecord{std::move(name), DescribeValue(value.before),
                           DescribeValue(value.after)};
    } else {
        const std::string rendered = DescribeValue(value);
        return ParamRecord{std::move(name), rendered, rendered};
    }
}

inline std::string FormatParamRecords(const std::vector<ParamRecord>& records) {
    fmt::memory_buffer buffer;
    fmt::format_to(std::back_inserter(buffer), "params:\n");
    for (const auto& entry : records) {
        const bool changed = entry.initial != entry.current;
        if (changed) {
            fmt::format_to(std::back_inserter(buffer), "{}: {} -> {}\n", entry.name, entry.initial,
                           entry.current);
        } else {
            fmt::format_to(std::back_inserter(buffer), "{}: {}\n", entry.name, entry.initial);
        }
    }
    return fmt::to_string(buffer);
}

template <typename... Args>
std::string formatParamsImpl(const char* arg_names, Args&&... args) {
    std::vector<std::string> names = SplitArgumentNames(arg_names ? arg_names : "");
    std::vector<ParamRecord> records;
    records.reserve(sizeof...(Args));
    std::size_t index = 0;
    auto append_record = [&](auto&& value) {
        std::string name = (index < names.size() && !names[index].empty())
                               ? std::move(names[index])
                               : fmt::format("arg{}", index);
        ++index;
        records.push_back(MakeParamRecord(std::move(name), std::forward<decltype(value)>(value)));
    };
    (append_record(std::forward<Args>(args)), ...);
    return FormatParamRecords(records);
}

inline void log_info(std::string_view message) {
    LOG_INFO(Lib_Font, "{}", message);
}

inline void log_debug(std::string_view message) {
    LOG_DEBUG(Lib_Font, "{}", message);
}

#define formatParams(...) formatParamsImpl(#__VA_ARGS__ __VA_OPT__(, ) __VA_ARGS__)

struct FontSetSelector;

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
    // `scale_*` fields are controlled by style-state setters and are interpreted according to the
    // style frame `scaleUnit` (pixel vs point). Public APIs accept codepoints as UTF-32 scalar
    // values; glyph lookup and rendering uses FreeType + deterministic fallback selection.
    //
    // Fallback behavior (high level):
    // - A font handle may have an external face (opened from file/memory) and, optionally, a
    //   sysfont selection (font_set_type + primary sysfont + additional fallback sysfonts).
    // - When a glyph is missing from the primary face, the implementation may consult the external
    //   face and then the configured system fallback faces to preserve observable behavior.
    float scale_w = 16.0f;
    float scale_h = 16.0f;
    float scale_point_w = 12.0f;
    float scale_point_h = 12.0f;
    bool scale_point_active = false;
    float render_scale_w = 16.0f;
    float render_scale_h = 16.0f;
    float render_scale_point_w = 12.0f;
    float render_scale_point_h = 12.0f;
    bool render_scale_point_active = false;
    u32 dpi_x = 72;
    u32 dpi_y = 72;
    u32 font_set_type = 0;
    std::filesystem::path system_font_path;
    u32 system_font_id = 0;
    float system_font_scale_factor = 1.0f;
    s32 system_font_shift_value = 0;
    struct SystemFallbackFace {
        u32 font_id = 0;
        float scale_factor = 1.0f;
        s32 shift_value = 0;
        std::filesystem::path path;
        std::shared_ptr<std::vector<unsigned char>> bytes;
        FT_Face ft_face = nullptr;
        bool ready = false;
    };
    std::vector<SystemFallbackFace> system_fallback_faces;
    Libraries::Font::OrbisFontLib library = nullptr;
    bool ext_face_ready = false;
    std::vector<unsigned char> ext_face_data;
    FT_Face ext_ft_face = nullptr;
    float ext_scale_for_height = 0.0f;
    int ext_ascent = 0, ext_descent = 0, ext_lineGap = 0;
    int ext_units_per_em = 0;
    int ext_typo_ascent = 0, ext_typo_descent = 0, ext_typo_lineGap = 0;
    bool ext_use_typo_metrics = false;
    std::unordered_map<std::uint64_t, GlyphEntry> ext_cache;
    std::vector<std::uint8_t> scratch;
    bool logged_ext_use = false;
    float cached_baseline_y = 0.0f;
    bool layout_cached = false;
    Libraries::Font::OrbisFontRenderer bound_renderer = nullptr;
    bool system_requested = false;
    std::shared_ptr<std::vector<u8>> fontset_record_storage;
    std::shared_ptr<FontSetSelector> fontset_selector;
    float effect_weight_x = 1.0f;
    float effect_weight_y = 1.0f;
    u32 effect_weight_mode = 0;
    float effect_slant = 0.0f;
    float render_effect_weight_x = 1.0f;
    float render_effect_weight_y = 1.0f;
    u32 render_effect_weight_mode = 0;
    float render_effect_slant = 0.0f;

    ~FontState();
};

struct LibraryState {
    bool support_system = false;
    bool support_external = false;
    u32 external_formats = 0;
    u32 external_fontMax = 0;
    const Libraries::Font::OrbisFontMem* backing_memory = nullptr;
    Libraries::Font::OrbisFontLibCreateParams create_params = nullptr;
    u64 edition = 0;
    FontLibOpaque* native = nullptr;
    void* owned_device_cache = nullptr;
    u32 owned_device_cache_size = 0;
};

struct FaceMetrics {
    int units_per_em = 0;
    int hhea_ascent = 0;
    int hhea_descent = 0;
    int hhea_lineGap = 0;
    int typo_ascent = 0;
    int typo_descent = 0;
    int typo_lineGap = 0;
    bool has_typo = false;
    bool use_typo = false;
};

struct GeneratedGlyph {
    OrbisFontGlyphOpaque glyph{};
    OrbisFontGlyphMetrics metrics{};
    OrbisFontGlyphMetricsHorizontal metrics_horizontal{};
    OrbisFontGlyphMetricsHorizontalX metrics_horizontal_x{};
    OrbisFontGlyphMetricsHorizontalAdvance metrics_horizontal_adv{};
    float origin_x = 0.0f;
    float origin_y = 0.0f;
    float scale_x_used = 0.0f;
    float scale_y_used = 0.0f;
    int bbox_x0 = 0;
    int bbox_y0 = 0;
    int bbox_w = 0;
    int bbox_h = 0;
    u32 codepoint = 0;
    Libraries::Font::OrbisFontHandle owner_handle{};
    OrbisFontGlyphOutline outline{};
    std::vector<OrbisFontGlyphOutlinePoint> outline_points;
    std::vector<u8> outline_tags;
    std::vector<u16> outline_contours;
    bool metrics_initialized = false;
    bool outline_initialized = false;
};

struct SysDriver {
    using PixelResFn = u32(PS4_SYSV_ABI*)();
    using InitFn = s32(PS4_SYSV_ABI*)(const void* memory, void* library);
    using TermFn = s32(PS4_SYSV_ABI*)(void* library);
    using SupportFn = s32(PS4_SYSV_ABI*)(void* library, u32 formats);
    using OpenFn = s32(PS4_SYSV_ABI*)(void* library, u32 mode, const void* fontAddress,
                                      u32 fontSize, u32 subFontIndex, u32 uniqueWord,
                                      void** inoutFontObj);
    using CloseFn = s32(PS4_SYSV_ABI*)(void* fontObj, u32 flags);
    using ScaleFn = s32(PS4_SYSV_ABI*)(void* fontObj, u16* outUnitsPerEm, float* outScale);
    using MetricFn = s32(PS4_SYSV_ABI*)(void* fontObj, u32 metricId, u16* outMetric);
    using GlyphsCountFn = s32(PS4_SYSV_ABI*)(void* fontObj, u32* out_count);
    using GlyphIndexFn = s32(PS4_SYSV_ABI*)(void* fontObj, u32 codepoint_u16, u32* out_glyph_index);

    using SetCharWithDpiFn = s32(PS4_SYSV_ABI*)(void* fontObj, u32 dpi_x, u32 dpi_y, float scale_x,
                                                float scale_y, float* out_scale_x,
                                                float* out_scale_y);
    using SetCharDefaultDpiFn = s32(PS4_SYSV_ABI*)(void* fontObj, float scale_x, float scale_y,
                                                   float* out_scale_x, float* out_scale_y);
    using ComputeLayoutFn = s32(PS4_SYSV_ABI*)(void* fontObj, const void* style_state_block,
                                               u8 (*out_words)[16]);
    using ComputeLayoutAltFn = s32(PS4_SYSV_ABI*)(void* fontObj, const void* style_state_block,
                                                  u8 (*out_words)[16]);

    using LoadGlyphCachedFn = s32(PS4_SYSV_ABI*)(void* fontObj, u32 glyphIndex, s32 mode,
                                                 std::uint64_t* out_words);
    using GetGlyphMetricsFn = s32(PS4_SYSV_ABI*)(
        void* fontObj, std::uint32_t* opt_param2, std::uint8_t mode, std::uint8_t* out_params,
        Libraries::Font::OrbisFontGlyphMetrics* out_metrics);
    using ApplyGlyphAdjustFn = s32(PS4_SYSV_ABI*)(void* fontObj, u32 p2, u32 glyphIndex, s32 p4,
                                                  s32 p5, u32* inoutGlyphIndex);
    using ConfigureGlyphFn = s32(PS4_SYSV_ABI*)(void* fontObj, std::uint32_t* in_params, s32 mode,
                                                std::uint32_t* inout_state);

    /*0x00*/ u8 reserved_00[0x10];
    /*0x10*/ PixelResFn pixel_resolution;
    /*0x18*/ InitFn init;
    /*0x20*/ TermFn term;
    /*0x28*/ SupportFn support_formats;
    /*0x30*/ u8 reserved_30[0x08];
    /*0x38*/ OpenFn open;
    /*0x40*/ CloseFn close;
    /*0x48*/ u8 reserved_48[0x08];
    /*0x50*/ ScaleFn scale;
    /*0x58*/ u8 reserved_58[0x08];
    /*0x60*/ MetricFn metric;
    /*0x68*/ GlyphsCountFn glyphs_count;
    /*0x70*/ u8 reserved_70[0x08];
    /*0x78*/ GlyphIndexFn glyph_index;
    /*0x80*/ SetCharWithDpiFn set_char_with_dpi;
    /*0x88*/ SetCharDefaultDpiFn set_char_default_dpi;
    /*0x90*/ u8 reserved_90[0x10];
    /*0xA0*/ ComputeLayoutFn compute_layout;
    /*0xA8*/ LoadGlyphCachedFn load_glyph_cached;
    /*0xB0*/ u8 reserved_B0[0x08];
    /*0xB8*/ GetGlyphMetricsFn get_glyph_metrics;
    /*0xC0*/ u8 reserved_C0[0x20];
    /*0xE0*/ ApplyGlyphAdjustFn apply_glyph_adjust;
    /*0xE8*/ u8 reserved_E8[0x20];
    /*0x108*/ ConfigureGlyphFn configure_glyph;
    /*0x110*/ u8 reserved_0x110_pad[0x08];
    /*0x118*/ ComputeLayoutAltFn compute_layout_alt;
};
static_assert(sizeof(SysDriver) == 0x120, "SysDriver size");
static_assert(offsetof(SysDriver, pixel_resolution) == 0x10, "SysDriver pixel_resolution offset");
static_assert(offsetof(SysDriver, init) == 0x18, "SysDriver init offset");
static_assert(offsetof(SysDriver, term) == 0x20, "SysDriver term offset");
static_assert(offsetof(SysDriver, support_formats) == 0x28, "SysDriver support_formats offset");
static_assert(offsetof(SysDriver, open) == 0x38, "SysDriver open offset");
static_assert(offsetof(SysDriver, close) == 0x40, "SysDriver close offset");
static_assert(offsetof(SysDriver, scale) == 0x50, "SysDriver scale offset");
static_assert(offsetof(SysDriver, metric) == 0x60, "SysDriver metric offset");
static_assert(offsetof(SysDriver, glyphs_count) == 0x68, "SysDriver glyphs_count offset");
static_assert(offsetof(SysDriver, glyph_index) == 0x78, "SysDriver glyph_index offset");
static_assert(offsetof(SysDriver, set_char_with_dpi) == 0x80, "SysDriver set_char_with_dpi offset");
static_assert(offsetof(SysDriver, set_char_default_dpi) == 0x88,
              "SysDriver set_char_default_dpi offset");
static_assert(offsetof(SysDriver, compute_layout) == 0xA0, "SysDriver compute_layout offset");
static_assert(offsetof(SysDriver, load_glyph_cached) == 0xA8, "SysDriver load_glyph_cached offset");
static_assert(offsetof(SysDriver, get_glyph_metrics) == 0xB8, "SysDriver get_glyph_metrics offset");
static_assert(offsetof(SysDriver, apply_glyph_adjust) == 0xE0,
              "SysDriver apply_glyph_adjust offset");
static_assert(offsetof(SysDriver, configure_glyph) == 0x108, "SysDriver configure_glyph offset");
static_assert(offsetof(SysDriver, compute_layout_alt) == 0x118,
              "SysDriver compute_layout_alt offset");
struct StyleStateBlock {
    /*0x00*/ u32 dpi_x;
    /*0x04*/ u32 dpi_y;
    /*0x08*/ u32 scale_unit;
    /*0x0C*/ u32 reserved_0x0c;
    /*0x10*/ float scale_w;
    /*0x14*/ float scale_h;
    /*0x18*/ float effect_weight_x;
    /*0x1C*/ float effect_weight_y;
    /*0x20*/ float slant_ratio;
    /*0x24*/ float reserved_0x24;
};
static_assert(sizeof(StyleStateBlock) == 0x28, "StyleStateBlock size");
static_assert(offsetof(StyleStateBlock, dpi_x) == 0x00, "StyleStateBlock dpi_x offset");
static_assert(offsetof(StyleStateBlock, dpi_y) == 0x04, "StyleStateBlock dpi_y offset");
static_assert(offsetof(StyleStateBlock, scale_unit) == 0x08, "StyleStateBlock scale_unit offset");
static_assert(offsetof(StyleStateBlock, scale_w) == 0x10, "StyleStateBlock scale_w offset");
static_assert(offsetof(StyleStateBlock, scale_h) == 0x14, "StyleStateBlock scale_h offset");
static_assert(offsetof(StyleStateBlock, effect_weight_x) == 0x18,
              "StyleStateBlock effect_weight_x offset");
static_assert(offsetof(StyleStateBlock, effect_weight_y) == 0x1C,
              "StyleStateBlock effect_weight_y offset");
static_assert(offsetof(StyleStateBlock, slant_ratio) == 0x20, "StyleStateBlock slant_ratio offset");

struct FontSetSelector {
    static constexpr u32 kMagic = 0x53464C42u;

    struct Candidate {
        u32 font_id = 0xffffffffu;
        FT_Face face = nullptr;
    };

    u32 magic = kMagic;
    u32 font_set_type = 0;
    void* library = nullptr;
    u32 mode_low = 0;
    u32 sub_font_index = 0;
    u32 primary_font_id = 0xffffffffu;
    u32 roman_font_id = 0xffffffffu;
    u32 arabic_font_id = 0xffffffffu;
    std::vector<Candidate> candidates;
};

struct FontSetRecordHeader {
    /*0x00*/ u32 font_set_type;
    /*0x04*/ u8 reserved_0x04[0x18];
    /*0x1C*/ u32 magic;
    /*0x20*/ u32 entry_count;
};
static_assert(sizeof(FontSetRecordHeader) == 0x24, "FontSetRecordHeader size");
static_assert(offsetof(FontSetRecordHeader, font_set_type) == 0x00,
              "FontSetRecordHeader font_set_type offset");
static_assert(offsetof(FontSetRecordHeader, magic) == 0x1C, "FontSetRecordHeader magic offset");
static_assert(offsetof(FontSetRecordHeader, entry_count) == 0x20,
              "FontSetRecordHeader entry_count offset");

struct FontSetRecordView {
    const FontSetRecordHeader* header = nullptr;
    const u32* entry_indices = nullptr;
    const FontSetSelector* selector = nullptr;
};

inline std::size_t FontSetRecordSelectorOffset(u32 entry_count) {
    const std::size_t indices_bytes = static_cast<std::size_t>(entry_count) * sizeof(u32);
    const std::size_t ptr_off_unaligned = sizeof(FontSetRecordHeader) + indices_bytes;
    const std::size_t ptr_align = alignof(const FontSetSelector*);
    return (ptr_off_unaligned + (ptr_align - 1u)) & ~(ptr_align - 1u);
}

inline FontSetRecordView MakeFontSetRecordView(const FontSetRecordHeader* record) {
    if (!record) {
        return {};
    }
    FontSetRecordView view{};
    view.header = record;
    view.entry_indices = reinterpret_cast<const u32*>(static_cast<const void*>(record + 1));
    const std::size_t ptr_off = FontSetRecordSelectorOffset(record->entry_count);
    view.selector = *reinterpret_cast<const FontSetSelector* const*>(
        static_cast<const void*>(reinterpret_cast<const u8*>(record) + ptr_off));
    return view;
}

struct FontCtxHeader {
    /*0x00*/ u32 lock_word;
    /*0x04*/ u32 max_entries;
    /*0x08*/ void* base;
    /*0x10*/ u8 reserved_0x10[0x10];
};
static_assert(sizeof(FontCtxHeader) == 0x20, "FontCtxHeader size");
static_assert(offsetof(FontCtxHeader, lock_word) == 0x00, "FontCtxHeader lock_word offset");
static_assert(offsetof(FontCtxHeader, max_entries) == 0x04, "FontCtxHeader max_entries offset");
static_assert(offsetof(FontCtxHeader, base) == 0x08, "FontCtxHeader base offset");

struct FontCtxEntry {
    /*0x00*/ u32 reserved_0x00;
    /*0x04*/ u32 active;
    /*0x08*/ u64 font_address;
    /*0x10*/ u32 unique_id;
    /*0x14*/ u32 lock_mode1;
    /*0x18*/ u32 lock_mode3;
    /*0x1C*/ u32 lock_mode2;
    /*0x20*/ void* obj_mode1;
    /*0x28*/ void* obj_mode3;
    /*0x30*/ void* obj_mode2;
    /*0x38*/ u8 reserved_0x38[0x08];
};
static_assert(sizeof(FontCtxEntry) == 0x40, "FontCtxEntry size");
static_assert(offsetof(FontCtxEntry, lock_mode1) == 0x14, "FontCtxEntry lock_mode1 offset");
static_assert(offsetof(FontCtxEntry, lock_mode3) == 0x18, "FontCtxEntry lock_mode3 offset");
static_assert(offsetof(FontCtxEntry, lock_mode2) == 0x1C, "FontCtxEntry lock_mode2 offset");
static_assert(offsetof(FontCtxEntry, obj_mode1) == 0x20, "FontCtxEntry obj_mode1 offset");
static_assert(offsetof(FontCtxEntry, obj_mode3) == 0x28, "FontCtxEntry obj_mode3 offset");
static_assert(offsetof(FontCtxEntry, obj_mode2) == 0x30, "FontCtxEntry obj_mode2 offset");

struct FontObj {
    /*0x00*/ u32 refcount;
    /*0x04*/ u32 reserved_0x04;
    /*0x08*/ u32 reserved_0x08;
    /*0x0C*/ u32 sub_font_index;
    /*0x10*/ FontObj* prev;
    /*0x18*/ FontObj* next;
    /*0x20*/ u8 reserved_0x20[0x08];
    /*0x28*/ void* open_ctx_0x28;
    /*0x30*/ void* ft_face;
    /*0x38*/ Libraries::Font::OrbisFontHandle font_handle;
    /*0x40*/ s32 shift_units_x;
    /*0x44*/ s32 shift_units_y;
    /*0x48*/ std::uint64_t layout_seed_pair;
    /*0x50*/ float scale_x_0x50;
    /*0x54*/ float scale_y_0x54;
    /*0x58*/ void* ft_ctx_0x58;
    /*0x60*/ u8 reserved_0x60[0x04];
    /*0x64*/ s32 cached_glyph_index_0x64;
    /*0x68*/ std::uint64_t cached_units_x_0x68;
    /*0x70*/ std::uint64_t cached_units_y_0x70;
    /*0x78*/ s64 shift_cache_x;
    /*0x80*/ s64 shift_cache_y;
    /*0x88*/ std::array<std::uint64_t, 2> layout_seed_vec;
    /*0x98*/ std::array<std::uint64_t, 2> layout_scale_vec;
    /*0xA8*/ u8 reserved_0xA8[0x130 - 0xA8];
    /*0x130*/ u32 glyph_cfg_word_0x130;
    /*0x134*/ u8 glyph_cfg_mode_0x134;
    /*0x135*/ u8 glyph_cfg_byte_0x135;
    /*0x136*/ u8 glyph_cfg_byte_0x136;
    /*0x137*/ u8 reserved_0x137[0x200 - 0x137];
};
static_assert(offsetof(FontObj, sub_font_index) == 0x0C, "FontObj sub_font_index offset");
static_assert(offsetof(FontObj, next) == 0x18, "FontObj next offset");
static_assert(offsetof(FontObj, ft_face) == 0x30, "FontObj ft_face offset");
static_assert(offsetof(FontObj, font_handle) == 0x38, "FontObj font_handle offset");
static_assert(offsetof(FontObj, shift_units_x) == 0x40, "FontObj shift_units_x offset");
static_assert(offsetof(FontObj, shift_units_y) == 0x44, "FontObj shift_units_y offset");
static_assert(offsetof(FontObj, layout_seed_pair) == 0x48, "FontObj layout_seed_pair offset");
static_assert(offsetof(FontObj, shift_cache_x) == 0x78, "FontObj shift_cache_x offset");
static_assert(offsetof(FontObj, shift_cache_y) == 0x80, "FontObj shift_cache_y offset");
static_assert(offsetof(FontObj, layout_seed_vec) == 0x88, "FontObj layout_seed_vec offset");
static_assert(offsetof(FontObj, layout_scale_vec) == 0x98, "FontObj layout_scale_vec offset");
static_assert(offsetof(FontObj, glyph_cfg_word_0x130) == 0x130, "FontObj glyph_cfg_word offset");
static_assert(sizeof(FontObj) == 0x200, "FontObj size");

struct SystemFontDefinition {
    u32 font_set_type;
    const char* config_key;
    const char* default_file;
};

struct FontSetCache {
    bool loaded = false;
    bool available = false;
    std::vector<unsigned char> bytes;
    std::filesystem::path path;
};

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

struct RendererOpaque {
    /*0x00*/ u16 magic;
    /*0x02*/ u16 reserved02;
    /*0x04*/ u8 reserved_04[0x08];
    /*0x0C*/ u8 feature_byte_0x0c;
    /*0x0D*/ u8 reserved_0d[0x03];
    /*0x10*/ u32 mem_kind;
    /*0x14*/ u32 region_size;
    /*0x18*/ void* region_base;
    /*0x20*/ void* alloc_ctx;
    /*0x28*/ const OrbisFontMemInterface* mem_iface;
    /*0x30*/ u8 reserved_30[0x20];
    /*0x50*/ std::uintptr_t alloc_fn;
    /*0x58*/ std::uintptr_t free_fn;
    /*0x60*/ std::uintptr_t realloc_fn;
    /*0x68*/ std::uintptr_t calloc_fn;
    /*0x70*/ u8 reserved_70[0x10];
    /*0x80*/ void* selection;
    /*0x88*/ u8 reserved_88[0x08];
    /*0x90*/ u64 outline_magic_0x90;
    /*0x98*/ void* workspace;
    /*0xA0*/ u64 workspace_size;
    /*0xA8*/ u32 reserved_a8;
    /*0xAC*/ u32 outline_policy_flag;
};
static_assert(sizeof(RendererOpaque) == 0xB0, "RendererOpaque size");
static_assert(offsetof(RendererOpaque, magic) == 0x00, "RendererOpaque magic offset");
static_assert(offsetof(RendererOpaque, feature_byte_0x0c) == 0x0C,
              "RendererOpaque feature byte offset");
static_assert(offsetof(RendererOpaque, mem_kind) == 0x10, "RendererOpaque mem_kind offset");
static_assert(offsetof(RendererOpaque, region_size) == 0x14, "RendererOpaque region_size offset");
static_assert(offsetof(RendererOpaque, alloc_ctx) == 0x20, "RendererOpaque alloc_ctx offset");
static_assert(offsetof(RendererOpaque, mem_iface) == 0x28, "RendererOpaque mem_iface offset");
static_assert(offsetof(RendererOpaque, alloc_fn) == 0x50, "RendererOpaque alloc_fn offset");
static_assert(offsetof(RendererOpaque, free_fn) == 0x58, "RendererOpaque free_fn offset");
static_assert(offsetof(RendererOpaque, selection) == 0x80, "RendererOpaque selection offset");
static_assert(offsetof(RendererOpaque, workspace) == 0x98, "RendererOpaque workspace offset");
static_assert(offsetof(RendererOpaque, reserved_a8) == 0xA8, "RendererOpaque reserved_a8 offset");
static_assert(offsetof(RendererOpaque, outline_policy_flag) == 0xAC,
              "RendererOpaque outline_policy_flag offset");

struct RendererFtBackend {
    /*0x00*/ void* renderer_header_0x10;
    /*0x08*/ std::uintptr_t unknown_0x08;
    /*0x10*/ std::uintptr_t unknown_0x10;
    /*0x18*/ std::uintptr_t unknown_0x18;
    /*0x20*/ void* initialized_marker;
    /*0x28*/ u8 reserved_0x28[0x20];
};
static_assert(sizeof(RendererFtBackend) == 0x48, "RendererFtBackend size");
static_assert(offsetof(RendererFtBackend, renderer_header_0x10) == 0x00,
              "RendererFtBackend renderer_header_0x10 offset");
static_assert(offsetof(RendererFtBackend, initialized_marker) == 0x20,
              "RendererFtBackend initialized_marker offset");

struct RendererFtOpaque {
    /*0x000*/ RendererOpaque base;
    /*0x0B0*/ u8 reserved_0x0B0[0x70];
    /*0x120*/ RendererFtBackend ft_backend;
};
static_assert(sizeof(RendererFtOpaque) == 0x168, "RendererFtOpaque size");
static_assert(offsetof(RendererFtOpaque, ft_backend) == 0x120,
              "RendererFtOpaque ft_backend offset");

struct OrbisFontRenderer_ {};

struct StyleFrameScaleState {
    float scale_w;
    float scale_h;
    u32 dpi_x;
    u32 dpi_y;
    bool scale_overridden;
    bool dpi_overridden;
};

extern const float kPointsPerInch;
extern const float kScaleEpsilon;

extern const u16 kStyleFrameMagic;
extern const u8 kStyleFrameFlagScale;
extern const u8 kStyleFrameFlagWeight;
extern const u8 kStyleFrameFlagSlant;

extern Core::FileSys::MntPoints* g_mnt;
extern std::unordered_map<Libraries::Font::OrbisFontHandle, FontState> g_font_state;
extern std::unordered_map<Libraries::Font::OrbisFontLib, LibraryState> g_library_state;
extern std::unordered_map<Libraries::Font::OrbisFontRenderSurface*,
                          const Libraries::Font::OrbisFontStyleFrame*>
    g_style_for_surface;
extern std::mutex g_generated_glyph_mutex;
extern std::unordered_set<Libraries::Font::OrbisFontGlyph> g_generated_glyphs;

extern void* g_allocator_vtbl_stub[4];
extern std::uint8_t g_sys_driver_stub;
extern std::uint8_t g_fontset_registry_stub;
extern std::uint8_t g_sysfonts_ctx_stub;
extern std::uint8_t g_external_fonts_ctx_stub;
extern u32 g_device_cache_stub;

FontState& GetState(Libraries::Font::OrbisFontHandle h);
FontState* TryGetState(Libraries::Font::OrbisFontHandle h);
LibraryState& GetLibState(Libraries::Font::OrbisFontLib lib);
void RemoveLibState(Libraries::Font::OrbisFontLib lib);
FT_Face CreateFreeTypeFaceFromBytes(const unsigned char* data, std::size_t size, u32 subfont_index);
void DestroyFreeTypeFace(FT_Face& face);
void LogExternalFormatSupport(u32 formats_mask);
void LogFontOpenParams(const Libraries::Font::OrbisFontOpenParams* params);
std::filesystem::path ResolveGuestPath(const char* guest_path);
bool LoadGuestFileBytes(const std::filesystem::path& host_path,
                        std::vector<unsigned char>& out_bytes);
FaceMetrics LoadFaceMetrics(FT_Face face);
void PopulateStateMetrics(FontState& st, const FaceMetrics& m);
float ComputeScaleExtForState(const FontState& st, FT_Face face, float pixel_h);
float ComputeScaleForPixelHeight(const FontState& st, float pixel_h);
float SafeDpiToFloat(u32 dpi);
float PointsToPixels(float pt, u32 dpi);
float PixelsToPoints(float px, u32 dpi);
u16 ClampToU16(float value);
StyleFrameScaleState ResolveStyleFrameScale(const Libraries::Font::OrbisFontStyleFrame* style,
                                            const FontState& st);
void InitializeStyleFrame(Libraries::Font::OrbisFontStyleFrame& frame);
bool EnsureStyleFrameInitialized(Libraries::Font::OrbisFontStyleFrame* frame);
bool ValidateStyleFramePtr(const Libraries::Font::OrbisFontStyleFrame* frame);
void UpdateCachedLayout(FontState& st);
std::uint64_t MakeGlyphKey(u32 code, int pixel_h);
void LogStrideOnce(const Libraries::Font::OrbisFontRenderSurface* surf);
void ClearRenderOutputs(Libraries::Font::OrbisFontGlyphMetrics* metrics,
                        Libraries::Font::OrbisFontRenderOutput* result);
bool ResolveFaceAndScale(FontState& st, Libraries::Font::OrbisFontHandle handle, float pixel_h,
                         FT_Face& face_out, float& scale_y_out);
bool ResolveFace(FontState& st, Libraries::Font::OrbisFontHandle handle, FT_Face& face_out);
s32 RenderCodepointToSurface(FontState& st, Libraries::Font::OrbisFontHandle handle, FT_Face face,
                             float pixel_w, float pixel_h,
                             Libraries::Font::OrbisFontRenderSurface* surf, u32 code, float x,
                             float y, Libraries::Font::OrbisFontGlyphMetrics* metrics,
                             Libraries::Font::OrbisFontRenderOutput* result, s32 shift_x_units,
                             s32 shift_y_units);
s32 RenderCodepointToSurfaceWithScale(FontState& st, Libraries::Font::OrbisFontHandle handle,
                                      FT_Face face, float scale_x, float scale_y,
                                      Libraries::Font::OrbisFontRenderSurface* surf, u32 code,
                                      float x, float y, Libraries::Font::OrbisFontGlyphMetrics* m,
                                      Libraries::Font::OrbisFontRenderOutput* result);
const GlyphEntry* GetGlyphEntry(FontState& st, Libraries::Font::OrbisFontHandle handle, u32 code,
                                FT_Face& face_out, float& scale_out);
const SystemFontDefinition* FindSystemFontDefinition(u32 font_set_type);
std::filesystem::path GetSysFontBaseDir();
std::string MacroToCamel(const char* macro_key);
std::filesystem::path ResolveSystemFontPath(u32 font_set_type);
std::filesystem::path ResolveSystemFontPathFromConfigOnly(u32 font_set_type);
const struct FontSetCache* EnsureFontSetCache(u32 font_set_type);
bool HasSfntTables(const std::vector<unsigned char>& bytes);
bool LoadFontFile(const std::filesystem::path& path, std::vector<unsigned char>& out_bytes);
bool AttachSystemFont(FontState& st, Libraries::Font::OrbisFontHandle handle);
std::string ReportSystemFaceRequest(FontState& st, Libraries::Font::OrbisFontHandle handle,
                                    bool& attached_out);
void TrackGeneratedGlyph(Libraries::Font::OrbisFontGlyph glyph);
bool ForgetGeneratedGlyph(Libraries::Font::OrbisFontGlyph glyph);
GeneratedGlyph* TryGetGeneratedGlyph(Libraries::Font::OrbisFontGlyph glyph);
void PopulateGlyphMetricVariants(GeneratedGlyph& gg);
void BuildBoundingOutline(GeneratedGlyph& gg);
bool BuildTrueOutline(GeneratedGlyph& gg);

} // namespace Libraries::Font::Internal
