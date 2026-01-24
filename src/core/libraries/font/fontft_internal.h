// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "core/libraries/font/font.h"

namespace Libraries::Font {

struct FontHandleOpenInfo {
    /*0x00*/ u32 unique_id_packed;
    /*0x04*/ u32 ctx_entry_index;
    /*0x08*/ u32 sub_font_index;
    /*0x0C*/ u32 fontset_flags;
    /*0x10*/ const void* fontset_record;
    /*0x18*/ u64 reserved_0x18;
};
static_assert(sizeof(FontHandleOpenInfo) == 0x20, "FontHandleOpenInfo size");
static_assert(offsetof(FontHandleOpenInfo, sub_font_index) == 0x08,
              "FontHandleOpenInfo sub_font_index offset");
static_assert(offsetof(FontHandleOpenInfo, fontset_record) == 0x10,
              "FontHandleOpenInfo fontset_record offset");

struct FontHandleStyleStateTail {
    /*0x00*/ u32 scale_unit;
    /*0x04*/ u32 reserved_0x0c;
    /*0x08*/ float scale_w;
    /*0x0C*/ float scale_h;
    /*0x10*/ float effect_weight_x;
    /*0x14*/ float effect_weight_y;
    /*0x18*/ float slant_ratio;
    /*0x1C*/ float reserved_0x24;
};
static_assert(sizeof(FontHandleStyleStateTail) == 0x20, "FontHandleStyleStateTail size");

struct FontHandleOpaqueNative {
    u16 magic;
    u16 flags;
    u32 lock_word;
    FontHandleOpenInfo open_info;
    OrbisFontLib library;
    struct RendererBinding {
        void* renderer;
        u8 reserved_0x08[0x10 - sizeof(void*)];
    };
    static_assert(sizeof(RendererBinding) == 0x10, "RendererBinding size");
    union {
        u8 reserved_0x30[0x10];
        RendererBinding renderer_binding;
    };
    u32 style_frame[2];
    union {
        u8 reserved_0x48[0x20];
        FontHandleStyleStateTail style_tail;
    };
    OrbisFontStyleFrame cached_style;
    u16 metricA;
    u16 metricB;
    u8 reserved_0xcc[0x1C];
    u8 reserved_0xe8[0x08];
    OrbisFontHandle prevFont;
    OrbisFontHandle nextFont;
};
static_assert(sizeof(FontHandleOpaqueNative) == 0x100, "FontHandleOpaqueNative size");
static_assert(offsetof(FontHandleOpaqueNative, renderer_binding) == 0x30,
              "FontHandleOpaqueNative renderer_binding offset");
static_assert(offsetof(FontHandleOpaqueNative, magic) == 0x00,
              "FontHandleOpaqueNative magic offset");
static_assert(offsetof(FontHandleOpaqueNative, lock_word) == 0x04,
              "FontHandleOpaqueNative lock_word offset");
static_assert(offsetof(FontHandleOpaqueNative, open_info) == 0x08,
              "FontHandleOpaqueNative open_info offset");
static_assert(offsetof(FontHandleOpaqueNative, library) == 0x28,
              "FontHandleOpaqueNative library offset");
static_assert(offsetof(FontHandleOpaqueNative, reserved_0x30) == 0x30,
              "FontHandleOpaqueNative reserved_0x30 offset");
static_assert(offsetof(FontHandleOpaqueNative, style_frame) == 0x40,
              "FontHandleOpaqueNative style_frame offset");
static_assert(offsetof(FontHandleOpaqueNative, reserved_0x48) == 0x48,
              "FontHandleOpaqueNative reserved_0x48 offset");
static_assert(offsetof(FontHandleOpaqueNative, cached_style) == 0x68,
              "FontHandleOpaqueNative cached_style offset");
static_assert(offsetof(FontHandleOpaqueNative, metricA) == 0xC8,
              "FontHandleOpaqueNative metricA offset");
static_assert(offsetof(FontHandleOpaqueNative, metricB) == 0xCA,
              "FontHandleOpaqueNative metricB offset");
static_assert(offsetof(FontHandleOpaqueNative, prevFont) == 0xF0,
              "FontHandleOpaqueNative prev offset");
static_assert(offsetof(FontHandleOpaqueNative, nextFont) == 0xF8,
              "FontHandleOpaqueNative next offset");

} // namespace Libraries::Font

namespace Libraries::Font::Internal {

struct alignas(16) LayoutWord {
    std::array<u32, 4> words{};

    constexpr u8* data() {
        return reinterpret_cast<u8*>(words.data());
    }
    constexpr const u8* data() const {
        return reinterpret_cast<const u8*>(words.data());
    }
};
static_assert(sizeof(LayoutWord) == 16, "LayoutWord size");
static_assert(alignof(LayoutWord) == 16, "LayoutWord alignment");

struct alignas(16) HorizontalLayoutMetricsWord {
    float line_advance = 0.0f;
    float baseline = 0.0f;
    float x_bound_lo = 0.0f;
    float x_bound_hi = 0.0f;
};
static_assert(sizeof(HorizontalLayoutMetricsWord) == 16, "HorizontalLayoutMetricsWord size");
static_assert(alignof(HorizontalLayoutMetricsWord) == 16, "HorizontalLayoutMetricsWord alignment");

struct alignas(16) HorizontalLayoutEffectsWord {
    u32 unknown_0x00 = 0;
    u32 unknown_0x04 = 0;
    float effect_height = 0.0f;
    float packed_i8_adj_u13_lane1 = 0.0f;
};
static_assert(sizeof(HorizontalLayoutEffectsWord) == 16, "HorizontalLayoutEffectsWord size");
static_assert(alignof(HorizontalLayoutEffectsWord) == 16, "HorizontalLayoutEffectsWord alignment");

struct alignas(16) HorizontalLayoutAuxWord {
    float packed_i8_adj_u13_lane0 = 0.0f;
    u32 unknown_0x04 = 0;
    u32 unknown_0x08 = 0;
    u32 unknown_0x0C = 0;
};
static_assert(sizeof(HorizontalLayoutAuxWord) == 16, "HorizontalLayoutAuxWord size");
static_assert(alignof(HorizontalLayoutAuxWord) == 16, "HorizontalLayoutAuxWord alignment");

struct alignas(16) HorizontalLayoutBlocks {
    HorizontalLayoutMetricsWord metrics;
    HorizontalLayoutEffectsWord effects;
    HorizontalLayoutAuxWord aux;

    constexpr void* words() {
        return &metrics;
    }
    constexpr const void* words() const {
        return &metrics;
    }

    struct Values {
        float line_advance = 0.0f;
        float baseline = 0.0f;
        float effect_height = 0.0f;
        u64 x_bounds_bits = 0;
        u64 i8_adj_u13_bits = 0;
    };

    float line_advance() const {
        return metrics.line_advance;
    }

    float baseline() const {
        return metrics.baseline;
    }

    float effect_height() const {
        return effects.effect_height;
    }

    u64 x_bounds_bits() const {
        return static_cast<u64>(std::bit_cast<u32>(metrics.x_bound_lo)) |
               (static_cast<u64>(std::bit_cast<u32>(metrics.x_bound_hi)) << 32);
    }

    u64 i8_adj_u13_bits() const {
        return static_cast<u64>(std::bit_cast<u32>(effects.packed_i8_adj_u13_lane1)) |
               (static_cast<u64>(std::bit_cast<u32>(aux.packed_i8_adj_u13_lane0)) << 32);
    }

    Values values() const {
        return {
            .line_advance = line_advance(),
            .baseline = baseline(),
            .effect_height = effect_height(),
            .x_bounds_bits = x_bounds_bits(),
            .i8_adj_u13_bits = i8_adj_u13_bits(),
        };
    }

    void set_line_advance(float v) {
        metrics.line_advance = v;
    }

    void set_baseline(float v) {
        metrics.baseline = v;
    }

    void set_x_bounds(float lo, float hi) {
        metrics.x_bound_lo = lo;
        metrics.x_bound_hi = hi;
    }

    void set_effect_height(float v) {
        effects.effect_height = v;
    }

    void set_i8_adj_u13_lanes(float lane0, float lane1) {
        aux.packed_i8_adj_u13_lane0 = lane0;
        effects.packed_i8_adj_u13_lane1 = lane1;
    }
};
static_assert(sizeof(HorizontalLayoutBlocks) == 0x30, "HorizontalLayoutBlocks size");
static_assert(alignof(HorizontalLayoutBlocks) == 16, "HorizontalLayoutBlocks alignment");
static_assert(offsetof(HorizontalLayoutBlocks, metrics) == 0x00,
              "HorizontalLayoutBlocks metrics offset");
static_assert(offsetof(HorizontalLayoutBlocks, effects) == 0x10,
              "HorizontalLayoutBlocks effects offset");
static_assert(offsetof(HorizontalLayoutBlocks, aux) == 0x20, "HorizontalLayoutBlocks aux offset");

struct alignas(16) VerticalLayoutMetricsWord {
    float column_advance = 0.0f;
    float baseline_offset_x = 0.0f;
    float unknown_0x08 = 0.0f;
    float unknown_0x0C = 0.0f;
};
static_assert(sizeof(VerticalLayoutMetricsWord) == 16, "VerticalLayoutMetricsWord size");
static_assert(alignof(VerticalLayoutMetricsWord) == 16, "VerticalLayoutMetricsWord alignment");

struct alignas(16) VerticalLayoutDecorationWord {
    u32 unknown_0x00 = 0;
    float decoration_span = 0.0f;
    float unknown_0x08 = 0.0f;
    float unknown_0x0C = 0.0f;
};
static_assert(sizeof(VerticalLayoutDecorationWord) == 16, "VerticalLayoutDecorationWord size");
static_assert(alignof(VerticalLayoutDecorationWord) == 16,
              "VerticalLayoutDecorationWord alignment");

struct alignas(16) VerticalLayoutBlocks {
    VerticalLayoutMetricsWord metrics;
    VerticalLayoutDecorationWord decoration;

    constexpr void* words() {
        return &metrics;
    }
    constexpr const void* words() const {
        return &metrics;
    }

    float column_advance() const {
        return metrics.column_advance;
    }

    float baseline_offset_x() const {
        return metrics.baseline_offset_x;
    }

    float decoration_span() const {
        return decoration.decoration_span;
    }

    struct Values {
        float column_advance = 0.0f;
        float baseline_offset_x = 0.0f;
        float decoration_span = 0.0f;
    };

    Values values() const {
        return {
            .column_advance = column_advance(),
            .baseline_offset_x = baseline_offset_x(),
            .decoration_span = decoration_span(),
        };
    }

    void set_column_advance(float v) {
        metrics.column_advance = v;
    }

    void set_baseline_offset_x(float v) {
        metrics.baseline_offset_x = v;
    }

    void set_unknown_metrics_0x08(float v) {
        metrics.unknown_0x08 = v;
    }

    void set_unknown_metrics_0x0C(float v) {
        metrics.unknown_0x0C = v;
    }

    void set_decoration_span(float v) {
        decoration.decoration_span = v;
    }

    void set_unknown_decoration_0x08(float v) {
        decoration.unknown_0x08 = v;
    }

    void set_unknown_decoration_0x0C(float v) {
        decoration.unknown_0x0C = v;
    }
};
static_assert(sizeof(VerticalLayoutBlocks) == 0x20, "VerticalLayoutBlocks size");
static_assert(alignof(VerticalLayoutBlocks) == 16, "VerticalLayoutBlocks alignment");
static_assert(offsetof(VerticalLayoutBlocks, metrics) == 0x00,
              "VerticalLayoutBlocks metrics offset");
static_assert(offsetof(VerticalLayoutBlocks, decoration) == 0x10,
              "VerticalLayoutBlocks decoration offset");

struct HorizontalLayoutBlocksIo {
    u8 (*words)[16] = nullptr;

    struct F32Field {
        u8* base = nullptr;
        std::size_t offset = 0;

        float value() const {
            float v = 0.0f;
            std::memcpy(&v, base + offset, sizeof(v));
            return v;
        }

        F32Field& operator=(float v) {
            std::memcpy(base + offset, &v, sizeof(v));
            return *this;
        }

        operator float() const {
            return value();
        }
    };

    struct Fields {
        F32Field line_advance;
        F32Field baseline;
        F32Field x_bound_0;
        F32Field x_bound_1;
        F32Field effect_height;
        F32Field u13_i8_lane0;
        F32Field u13_i8_lane1;
    };

    Fields fields() const {
        return {
            .line_advance = {words ? words[0] : nullptr, 0x00},
            .baseline = {words ? words[0] : nullptr, 0x04},
            .x_bound_0 = {words ? words[0] : nullptr, 0x08},
            .x_bound_1 = {words ? words[0] : nullptr, 0x0C},
            .effect_height = {words ? words[1] : nullptr, 0x08},
            .u13_i8_lane0 = {words ? words[2] : nullptr, 0x00},
            .u13_i8_lane1 = {words ? words[1] : nullptr, 0x0C},
        };
    }
};

struct VerticalLayoutBlocksIo {
    u8 (*words)[16] = nullptr;

    struct F32Field {
        u8* base = nullptr;
        std::size_t offset = 0;

        float value() const {
            float v = 0.0f;
            std::memcpy(&v, base + offset, sizeof(v));
            return v;
        }

        F32Field& operator=(float v) {
            std::memcpy(base + offset, &v, sizeof(v));
            return *this;
        }

        operator float() const {
            return value();
        }
    };

    struct Fields {
        F32Field column_advance;
        F32Field baseline_offset_x;
        F32Field decoration_span;
        F32Field unknown_metrics_0x08;
        F32Field unknown_metrics_0x0C;
        F32Field unknown_decoration_0x08;
        F32Field unknown_decoration_0x0C;
    };

    Fields fields() const {
        return {
            .column_advance = {words ? words[0] : nullptr, 0x00},
            .baseline_offset_x = {words ? words[0] : nullptr, 0x04},
            .decoration_span = {words ? words[1] : nullptr, 0x04},
            .unknown_metrics_0x08 = {words ? words[0] : nullptr, 0x08},
            .unknown_metrics_0x0C = {words ? words[0] : nullptr, 0x0C},
            .unknown_decoration_0x08 = {words ? words[1] : nullptr, 0x08},
            .unknown_decoration_0x0C = {words ? words[1] : nullptr, 0x0C},
        };
    }
};

struct alignas(16) VerticalLayoutAltBlocks {
    struct MetricsWord {
        float unknown_0x00 = 0.0f;
        float baseline_offset_x_candidate = 0.0f;
        float unknown_0x08 = 0.0f;
        float unknown_0x0C = 0.0f;
    };

    struct ExtrasWord {
        float unknown_0x00 = 0.0f;
        float decoration_span_candidate = 0.0f;
        float unknown_0x08 = 0.0f;
        float unknown_0x0C = 0.0f;
    };

    MetricsWord metrics;
    ExtrasWord extras;

    constexpr void* words() {
        return &metrics;
    }
    constexpr const void* words() const {
        return &metrics;
    }

    struct Values {
        MetricsWord metrics{};
        ExtrasWord extras{};
    };

    Values values() const {
        return {
            .metrics = metrics,
            .extras = extras,
        };
    }
};
static_assert(sizeof(VerticalLayoutAltBlocks) == 0x20, "VerticalLayoutAltBlocks size");
static_assert(alignof(VerticalLayoutAltBlocks) == 16, "VerticalLayoutAltBlocks alignment");
static_assert(offsetof(VerticalLayoutAltBlocks, metrics) == 0x00,
              "VerticalLayoutAltBlocks metrics offset");
static_assert(offsetof(VerticalLayoutAltBlocks, extras) == 0x10,
              "VerticalLayoutAltBlocks extras offset");

s32 ComputeHorizontalLayoutBlocks(Libraries::Font::OrbisFontHandle fontHandle,
                                  const void* style_state_block, u8 (*out_words)[16]);
s32 ComputeVerticalLayoutBlocks(Libraries::Font::OrbisFontHandle fontHandle,
                                const void* style_state_block, u8 (*out_words)[16]);

inline u8 (*LayoutWordsBytes(void* bytes))[16] {
    return reinterpret_cast<u8(*)[16]>(bytes);
}
inline const u8 (*LayoutWordsBytes(const void* bytes))[16] {
    return reinterpret_cast<const u8(*)[16]>(bytes);
}

inline s32 ComputeHorizontalLayoutBlocksWords(Libraries::Font::OrbisFontHandle fontHandle,
                                              const void* style_state_block,
                                              LayoutWord* out_words) {
    return ComputeHorizontalLayoutBlocks(fontHandle, style_state_block,
                                         reinterpret_cast<u8(*)[16]>(out_words));
}

inline s32 ComputeHorizontalLayoutBlocksTyped(Libraries::Font::OrbisFontHandle fontHandle,
                                              const void* style_state_block,
                                              HorizontalLayoutBlocks* out_blocks) {
    return ComputeHorizontalLayoutBlocks(fontHandle, style_state_block,
                                         reinterpret_cast<u8(*)[16]>(out_blocks->words()));
}

inline s32 ComputeVerticalLayoutBlocksWords(Libraries::Font::OrbisFontHandle fontHandle,
                                            const void* style_state_block, LayoutWord* out_words) {
    return ComputeVerticalLayoutBlocks(fontHandle, style_state_block,
                                       reinterpret_cast<u8(*)[16]>(out_words));
}

inline s32 ComputeVerticalLayoutBlocksTyped(Libraries::Font::OrbisFontHandle fontHandle,
                                            const void* style_state_block,
                                            VerticalLayoutBlocks* out_blocks) {
    return ComputeVerticalLayoutBlocks(fontHandle, style_state_block,
                                       reinterpret_cast<u8(*)[16]>(out_blocks->words()));
}

inline s32 ComputeHorizontalLayoutBlocksToBytes(Libraries::Font::OrbisFontHandle fontHandle,
                                                const void* style_state_block, void* out_bytes) {
    return ComputeHorizontalLayoutBlocks(fontHandle, style_state_block,
                                         LayoutWordsBytes(out_bytes));
}

inline s32 ComputeVerticalLayoutBlocksToBytes(Libraries::Font::OrbisFontHandle fontHandle,
                                              const void* style_state_block, void* out_bytes) {
    return ComputeVerticalLayoutBlocks(fontHandle, style_state_block, LayoutWordsBytes(out_bytes));
}

struct SysFontDesc {
    float scale_factor;
    s32 shift_value;
};

SysFontDesc GetSysFontDesc(u32 font_id);

Libraries::Font::FontHandleOpaqueNative* GetNativeFont(Libraries::Font::OrbisFontHandle h);
bool AcquireFontLock(Libraries::Font::FontHandleOpaqueNative* font, u32& out_prev_lock_word);
void ReleaseFontLock(Libraries::Font::FontHandleOpaqueNative* font, u32 prev_lock_word);
bool AcquireCachedStyleLock(Libraries::Font::FontHandleOpaqueNative* font, u32& out_prev_lock_word);
void ReleaseCachedStyleLock(Libraries::Font::FontHandleOpaqueNative* font, u32 prev_lock_word);

std::uint8_t* AcquireFontCtxEntry(std::uint8_t* ctx, u32 idx, u32 mode_low, void** out_obj,
                                  u32* out_lock_word);

void ReleaseFontObjectsForHandle(Libraries::Font::OrbisFontHandle fontHandle, int entryCount);

s32 ComputeHorizontalLayoutBlocks(Libraries::Font::OrbisFontHandle fontHandle,
                                  const void* style_state_block, u8 (*out_words)[16]);

s32 ComputeVerticalLayoutBlocks(Libraries::Font::OrbisFontHandle fontHandle,
                                const void* style_state_block, u8 (*out_words)[16]);

s32 GetCharGlyphMetrics(Libraries::Font::OrbisFontHandle fontHandle, u32 code,
                        Libraries::Font::OrbisFontGlyphMetrics* metrics, bool use_cached_style);

const std::uint8_t* FindSysFontRangeRecord(u32 font_id, u32 codepoint);

u32 ResolveSysFontCodepoint(const void* record, u64 code, u32 flags, u32* out_font_id);

s32 RenderCharGlyphImageCore(Libraries::Font::OrbisFontHandle fontHandle, u32 code,
                             Libraries::Font::OrbisFontRenderSurface* surf, float x, float y,
                             Libraries::Font::OrbisFontGlyphMetrics* metrics,
                             Libraries::Font::OrbisFontRenderOutput* result);

s32 RenderGlyphImageCore(Libraries::Font::OrbisFontGlyph fontGlyph,
                         Libraries::Font::OrbisFontStyleFrame* fontStyleFrame,
                         Libraries::Font::OrbisFontRenderer fontRenderer,
                         Libraries::Font::OrbisFontRenderSurface* surface, float x, float y,
                         int mode, Libraries::Font::OrbisFontGlyphMetrics* metrics,
                         Libraries::Font::OrbisFontRenderOutput* result);

s32 StyleStateSetScalePixel(void* style_state_block, float w, float h);

s32 StyleStateSetScalePoint(void* style_state_block, float w, float h);

s32 StyleStateGetScalePixel(const void* style_state_block, float* w, float* h);

s32 StyleStateGetScalePoint(const void* style_state_block, float* w, float* h);

s32 StyleStateSetDpi(u32* dpi_pair, u32 h_dpi, u32 v_dpi);

s32 StyleStateSetSlantRatio(void* style_state_block, float slantRatio);

s32 StyleStateGetSlantRatio(const void* style_state_block, float* slantRatio);

s32 StyleStateSetWeightScale(void* style_state_block, float weightXScale, float weightYScale);

s32 StyleStateGetWeightScale(const void* style_state_block, float* weightXScale,
                             float* weightYScale, u32* mode);

} // namespace Libraries::Font::Internal

namespace Libraries::FontFt::Internal {

u32 PS4_SYSV_ABI LibraryGetPixelResolutionStub();
s32 PS4_SYSV_ABI LibraryInitStub(const void* memory, void* library);
s32 PS4_SYSV_ABI LibraryTermStub(void* library);
s32 PS4_SYSV_ABI LibrarySupportStub(void* library, u32 formats);
s32 PS4_SYSV_ABI LibraryOpenFontMemoryStub(void* library, u32 mode, const void* fontAddress,
                                           u32 fontSize, u32 subFontIndex, u32 uniqueWord,
                                           void** inoutFontObj);
s32 PS4_SYSV_ABI LibraryCloseFontObjStub(void* fontObj, u32 flags);
s32 PS4_SYSV_ABI LibraryGetFaceMetricStub(void* fontObj, u32 metricId, u16* outMetric);
s32 PS4_SYSV_ABI LibraryGetFaceScaleStub(void* fontObj, u16* outUnitsPerEm, float* outScale);
s32 PS4_SYSV_ABI LibraryGetGlyphIndexStub(void* fontObj, u32 codepoint_u16, u32* out_glyph_index);

s32 PS4_SYSV_ABI LibrarySetCharSizeWithDpiStub(void* fontObj, u32 dpi_x, u32 dpi_y, float scale_x,
                                               float scale_y, float* out_scale_x,
                                               float* out_scale_y);
s32 PS4_SYSV_ABI LibrarySetCharSizeDefaultDpiStub(void* fontObj, float scale_x, float scale_y,
                                                  float* out_scale_x, float* out_scale_y);
s32 PS4_SYSV_ABI LibraryComputeLayoutBlockStub(void* fontObj, const void* style_state_block,
                                               u8 (*out_words)[16]);
s32 PS4_SYSV_ABI LibraryComputeLayoutAltBlockStub(void* fontObj, const void* style_state_block,
                                                  u8 (*out_words)[16]);

s32 PS4_SYSV_ABI LibraryLoadGlyphCachedStub(void* fontObj, u32 glyphIndex, s32 mode,
                                            std::uint64_t* out_words);
s32 PS4_SYSV_ABI LibraryGetGlyphMetricsStub(void* fontObj, std::uint32_t* opt_param2,
                                            std::uint8_t mode, std::uint8_t* out_params,
                                            Libraries::Font::OrbisFontGlyphMetrics* out_metrics);
s32 PS4_SYSV_ABI LibraryApplyGlyphAdjustStub(void* fontObj, u32 p2, u32 glyphIndex, s32 p4, s32 p5,
                                             u32* inoutGlyphIndex);
s32 PS4_SYSV_ABI LibraryConfigureGlyphStub(void* fontObj, std::uint32_t* in_params, s32 mode,
                                           std::uint32_t* inout_state);

s32 PS4_SYSV_ABI FtRendererCreate(void* renderer);
u64 PS4_SYSV_ABI FtRendererQuery(void* renderer, u8* params, s64* out_ptr, u8 (*out_vec)[16]);
s32 PS4_SYSV_ABI FtRendererDestroy(void* renderer);

} // namespace Libraries::FontFt::Internal
