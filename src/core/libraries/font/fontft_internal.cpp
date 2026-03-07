// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/font/fontft_internal.h"

#include <algorithm>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_MODULE_H
#include FT_SYSTEM_H
#include FT_TRUETYPE_TABLES_H

#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/file_sys/fs.h"
#include "core/libraries/font/font_internal.h"
#include "core/libraries/kernel/kernel.h"
#include "core/tls.h"
#include "font_error.h"

namespace Libraries::Font::Internal {

using Libraries::Font::OrbisFontGlyph;
using Libraries::Font::OrbisFontGlyphMetrics;
using Libraries::Font::OrbisFontHandle;
using Libraries::Font::OrbisFontRenderer;
using Libraries::Font::OrbisFontRenderOutput;
using Libraries::Font::OrbisFontRenderSurface;
using Libraries::Font::OrbisFontStyleFrame;

namespace {
static thread_local GetCharGlyphMetricsFailLogState g_get_char_metrics_fail;

static void LogGetCharGlyphMetricsFailOnce(std::string_view stage, s32 rc, FT_Error ft_err,
                                           bool is_system, u32 code, FT_UInt glyph_index,
                                           FT_Face face, float scale_w, float scale_h) {
    constexpr u32 kMaxDetailedLogs = 16;
    if (g_get_char_metrics_fail.count < kMaxDetailedLogs) {
        u32 enc = 0;
        u16 platform_id = 0;
        u16 encoding_id = 0;
        u32 num_glyphs = 0;
        u32 num_charmaps = 0;
        if (face) {
            num_glyphs = static_cast<u32>(face->num_glyphs);
            num_charmaps = static_cast<u32>(face->num_charmaps);
            if (face->charmap) {
                enc = static_cast<u32>(face->charmap->encoding);
                platform_id = face->charmap->platform_id;
                encoding_id = face->charmap->encoding_id;
            }
        }

        LOG_WARNING(
            Lib_Font,
            "GetCharGlyphMetricsFail: rc={} stage={} ft_err={} is_system={} code={} glyph_index={} "
            "num_glyphs={} cmap(enc={},pid={},eid={}) num_charmaps={} scale_w={} scale_h={}",
            rc, stage, static_cast<int>(ft_err), is_system, code, glyph_index, num_glyphs, enc,
            platform_id, encoding_id, num_charmaps, scale_w, scale_h);

        ++g_get_char_metrics_fail.count;
        return;
    }

    if (!g_get_char_metrics_fail.suppression_logged) {
        LOG_WARNING(Lib_Font, "GetCharGlyphMetricsFail: further failures suppressed");
        g_get_char_metrics_fail.suppression_logged = true;
    }
}
} // namespace

static void UpdateFtFontObjShiftCache(u8* font_obj) {
    if (!font_obj) {
        return;
    }

    auto* obj = reinterpret_cast<FontObj*>(font_obj);
    obj->shift_cache_x = static_cast<s64>(obj->shift_units_x);
    obj->shift_cache_y = static_cast<s64>(obj->shift_units_y);

    const s32 seed_low = static_cast<s32>(static_cast<u32>(obj->layout_seed_pair & 0xFFFFFFFFu));
    const s32 seed_high =
        static_cast<s32>(static_cast<u32>((obj->layout_seed_pair >> 32) & 0xFFFFFFFFu));
    obj->layout_seed_vec[0] = static_cast<std::uint64_t>(static_cast<std::int64_t>(seed_low));
    obj->layout_seed_vec[1] = static_cast<std::uint64_t>(static_cast<std::int64_t>(seed_high));
    obj->layout_scale_vec[0] = 0x0000000000010000ull;
    obj->layout_scale_vec[1] = 0x0000000000010000ull;
}

SysFontDesc GetSysFontDesc(u32 font_id) {
    (void)font_id;
    return SysFontDesc{
        .scale_factor = 1.0f,
        .shift_value = 0,
    };
}

Libraries::Font::FontHandleOpaqueNative* GetNativeFont(Libraries::Font::OrbisFontHandle h) {
    return h ? reinterpret_cast<Libraries::Font::FontHandleOpaqueNative*>(h) : nullptr;
}

bool AcquireFontLock(Libraries::Font::FontHandleOpaqueNative* font, u32& out_prev_lock_word) {
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

void ReleaseFontLock(Libraries::Font::FontHandleOpaqueNative* font, u32 prev_lock_word) {
    if (!font) {
        return;
    }
    font->lock_word = prev_lock_word & 0x7fffffff;
}

bool AcquireCachedStyleLock(Libraries::Font::FontHandleOpaqueNative* font,
                            u32& out_prev_lock_word) {
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

void ReleaseCachedStyleLock(Libraries::Font::FontHandleOpaqueNative* font, u32 prev_lock_word) {
    if (!font) {
        return;
    }
    font->cached_style.cache_lock_word = prev_lock_word & 0x7fffffff;
}

std::uint8_t* AcquireFontCtxEntry(std::uint8_t* ctx, u32 idx, u32 mode_low, void** out_obj,
                                  u32* out_lock_word) {
    if (!ctx || !out_obj || !out_lock_word) {
        return nullptr;
    }

    auto* header = reinterpret_cast<FontCtxHeader*>(ctx);
    const u32 max = header->max_entries;
    if (max < idx || max == idx) {
        *out_lock_word = 0;
        *out_obj = nullptr;
        return nullptr;
    }

    auto* base = static_cast<std::uint8_t*>(header->base);
    const std::size_t entry_off = static_cast<std::size_t>(idx) * sizeof(FontCtxEntry);
    auto* entry_u8 = base ? (base + entry_off) : nullptr;
    auto* entry = entry_u8 ? reinterpret_cast<FontCtxEntry*>(entry_u8) : nullptr;
    if (!entry) {
        *out_lock_word = 0;
        *out_obj = nullptr;
        return nullptr;
    }

    u32* lock_word = nullptr;
    void** obj_slot = nullptr;
    if (mode_low == 3) {
        lock_word = &entry->lock_mode3;
        obj_slot = &entry->obj_mode3;
    } else if (mode_low == 2) {
        lock_word = &entry->lock_mode2;
        obj_slot = &entry->obj_mode2;
    } else if (mode_low == 1) {
        lock_word = &entry->lock_mode1;
        obj_slot = &entry->obj_mode1;
    } else {
        *out_lock_word = 0;
        *out_obj = nullptr;
        return entry_u8;
    }

    for (;;) {
        const u32 cur = *lock_word;
        if (static_cast<s32>(cur) >= 0) {
            std::atomic_ref<u32> ref(*lock_word);
            u32 expected = cur;
            if (ref.compare_exchange_weak(expected, cur | 0x80000000u, std::memory_order_acq_rel)) {
                *out_lock_word = cur | 0x80000000u;
                *out_obj = obj_slot ? *obj_slot : nullptr;
                return entry_u8;
            }
        }
        Libraries::Kernel::sceKernelUsleep(0x1e);
    }
}

static std::uint8_t* AcquireFontCtxEntryAndSelectNode(std::uint8_t* ctx, u32 idx, u32 mode_low,
                                                      u32 sub_font_index, void** out_obj,
                                                      u32* out_lock_word) {
    if (!out_obj) {
        return nullptr;
    }
    std::uint8_t* entry_u8 = AcquireFontCtxEntry(ctx, idx, mode_low, out_obj, out_lock_word);
    auto* node = static_cast<FontObj*>(*out_obj);
    while (node) {
        if (node->sub_font_index == sub_font_index) {
            break;
        }
        node = node->next;
    }
    *out_obj = node;
    return entry_u8;
}

static void ReleaseFontCtxEntryLock(std::uint8_t* entry_u8, u32 mode_low, u32 lock_word) {
    if (!entry_u8) {
        return;
    }
    auto* entry = reinterpret_cast<FontCtxEntry*>(entry_u8);
    u32* lock_word_ptr = nullptr;
    if (mode_low == 3) {
        if ((lock_word & 0x0FFFFFFFu) == 0) {
            entry->obj_mode3 = nullptr;
        }
        lock_word_ptr = &entry->lock_mode3;
    } else if (mode_low == 1) {
        if ((lock_word & 0x0FFFFFFFu) == 0) {
            entry->obj_mode1 = nullptr;
        }
        lock_word_ptr = &entry->lock_mode1;
    } else {
        if ((lock_word & 0x0FFFFFFFu) == 0) {
            entry->obj_mode2 = nullptr;
        }
        lock_word_ptr = &entry->lock_mode2;
    }

    if (((lock_word & 0x0FFFFFFFu) == 0) && entry->obj_mode1 == nullptr &&
        entry->obj_mode3 == nullptr && entry->obj_mode2 == nullptr) {
        entry->unique_id = 0;
        entry->active = 0;
    }
    *lock_word_ptr = lock_word & 0x7FFFFFFFu;
}

static void ApplyGlyphSpecialCaseAdjust(int font_id, int glyph_index, int* inout_seed_words) {
    if (!inout_seed_words) {
        return;
    }
    if (((font_id == 10) && (static_cast<u32>(glyph_index - 0x23d1U) < 0x5e)) &&
        ((0x5c0000000017ULL >>
          (static_cast<u64>(static_cast<u8>((static_cast<u8>(glyph_index - 0x23d1U) >> 1) & 0x3f)) &
           0x3fULL)) &
         1ULL) != 0) {
        inout_seed_words[0] = inout_seed_words[0] + 500;
    }
}

static std::uintptr_t AcquireRendererSelectionLock(RendererOpaque* renderer) {
    if (!renderer) {
        return 0;
    }

    constexpr std::uintptr_t kLocked = std::numeric_limits<std::uintptr_t>::max();
    for (;;) {
        const std::uintptr_t cur = reinterpret_cast<std::uintptr_t>(renderer->selection);
        if (cur != kLocked) {
            std::atomic_ref<void*> ref(renderer->selection);
            void* expected = reinterpret_cast<void*>(cur);
            if (ref.compare_exchange_weak(expected, reinterpret_cast<void*>(kLocked),
                                          std::memory_order_acq_rel)) {
                return cur;
            }
        }
        Libraries::Kernel::sceKernelUsleep(0x1e);
    }
}

static std::uint64_t ResolveGposTagForCode(u32 codepoint) {
    (void)codepoint;
    return 0;
}

static float SysFontScaleFactor(u32 font_id) {
    (void)font_id;
    return 1.0f;
}

static float SysFontShiftValueF32(u32 font_id) {
    (void)font_id;
    return 0.0f;
}

static u8 SysFontHasAltFlag(u32 font_id) {
    (void)font_id;
    return 0;
}

const std::uint8_t* FindSysFontRangeRecord(u32 font_id, u32 codepoint) {
    (void)font_id;
    (void)codepoint;
    return nullptr;
}

static u32 ResolveSysFontMapping(const void* record, int table_id, u32 codepoint,
                                 u32* out_codepoint) {
    (void)record;
    (void)table_id;
    if (out_codepoint) {
        *out_codepoint = codepoint;
    }
    return 0;
}

u32 ResolveSysFontCodepoint(const void* record, u64 code, u32 flags, u32* out_font_id) {
    if (!record) {
        if (out_font_id) {
            *out_font_id = 0;
        }
        return static_cast<u32>(code);
    }

    const u32 code_u32 = static_cast<u32>(code);

    const auto* rec = static_cast<const FontSetRecordHeader*>(record);
    if (rec->magic == Libraries::Font::Internal::FontSetSelector::kMagic) {
        const auto view = MakeFontSetRecordView(rec);
        const u32 entry_count = view.header ? view.header->entry_count : 0u;
        const u32 record_primary_id =
            (entry_count && view.entry_indices) ? view.entry_indices[0] : 0u;
        const auto* selector = view.selector;
        if (!selector || selector->magic != Libraries::Font::Internal::FontSetSelector::kMagic) {
            if (out_font_id) {
                *out_font_id = record_primary_id;
            }
            return code_u32;
        }

        const u32 primary_id = selector->primary_font_id != 0xffffffffu ? selector->primary_font_id
                                                                        : record_primary_id;
        const u32 roman_id =
            selector->roman_font_id != 0xffffffffu ? selector->roman_font_id : record_primary_id;
        const u32 arabic_id =
            selector->arabic_font_id != 0xffffffffu ? selector->arabic_font_id : roman_id;

        auto is_cjk_like = [](u32 cp) -> bool {
            if ((cp >= 0x3040u && cp <= 0x30FFu) || (cp >= 0x31F0u && cp <= 0x31FFu) ||
                (cp >= 0x3400u && cp <= 0x4DBFu) || (cp >= 0x4E00u && cp <= 0x9FFFu) ||
                (cp >= 0xAC00u && cp <= 0xD7AFu) || (cp >= 0xF900u && cp <= 0xFAFFu) ||
                (cp >= 0x20000u && cp <= 0x2FA1Fu)) {
                return true;
            }
            return false;
        };
        auto is_arabic_like = [](u32 cp) -> bool {
            return (cp >= 0x0600u && cp <= 0x06FFu) || (cp >= 0x0750u && cp <= 0x077Fu) ||
                   (cp >= 0x08A0u && cp <= 0x08FFu) || (cp >= 0xFB50u && cp <= 0xFDFFu) ||
                   (cp >= 0xFE70u && cp <= 0xFEFFu);
        };
        auto is_symbol_like = [](u32 cp) -> bool {
            return (cp >= 0x25A0u && cp <= 0x25FFu) || (cp >= 0x2600u && cp <= 0x26FFu) ||
                   (cp >= 0x2700u && cp <= 0x27BFu);
        };

        u32 selected_font_id = roman_id;
        if (is_arabic_like(code_u32)) {
            selected_font_id = arabic_id;
        } else if (is_cjk_like(code_u32) || is_symbol_like(code_u32)) {
            selected_font_id = primary_id;
        }

        if (view.entry_indices && entry_count) {
            bool found = false;
            for (u32 i = 0; i < entry_count; ++i) {
                if (view.entry_indices[i] == selected_font_id) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                selected_font_id = record_primary_id;
            }
        }

        if (out_font_id) {
            *out_font_id = selected_font_id;
        }
        (void)flags;
        return code_u32;
    }

    (void)flags;
    if (out_font_id) {
        *out_font_id = 0;
    }
    return code_u32;
}

void ReleaseFontObjectsForHandle(Libraries::Font::OrbisFontHandle fontHandle, int entryCount) {
    if (!fontHandle) {
        return;
    }
    auto* font = GetNativeFont(fontHandle);
    if (!font) {
        return;
    }
    auto* lib = static_cast<Libraries::Font::Internal::FontLibOpaque*>(font->library);
    if (!lib || lib->magic != 0x0F01 || !lib->sys_driver) {
        return;
    }

    const auto* fontset_record =
        static_cast<const FontSetRecordHeader*>(font->open_info.fontset_record);

    const u32* entry_indices = nullptr;
    u32 count = 0;
    u8* ctx = nullptr;
    if (!fontset_record) {
        entry_indices = &font->open_info.ctx_entry_index;
        count = 1;
        ctx = static_cast<u8*>(lib->external_fonts_ctx);
    } else {
        if (entryCount < 0) {
            entryCount = static_cast<int>(fontset_record->entry_count);
        }
        entry_indices = reinterpret_cast<const u32*>(fontset_record + 1);
        count = static_cast<u32>(entryCount);
        ctx = static_cast<u8*>(lib->sysfonts_ctx);
    }

    if (!ctx || !entry_indices || count == 0) {
        return;
    }

    const auto* driver = reinterpret_cast<const SysDriver*>(lib->sys_driver);
    const auto close_fn = driver ? driver->close : nullptr;
    if (!close_fn) {
        return;
    }

    const u32 mode_low = static_cast<u32>(font->flags) & 0x0Fu;
    const u32 sub_font_index = font->open_info.sub_font_index;

    for (u32 n = count; n != 0; --n) {
        const u32 font_id = entry_indices[n - 1];
        if (static_cast<s32>(font_id) < 0) {
            continue;
        }

        void* head = nullptr;
        u32 lock_word = 0;
        u8* entry_u8 = AcquireFontCtxEntry(ctx, font_id, mode_low, &head, &lock_word);
        auto* entry = entry_u8 ? reinterpret_cast<FontCtxEntry*>(entry_u8) : nullptr;
        if (!entry) {
            continue;
        }

        FontObj* match = nullptr;
        for (auto* node = static_cast<FontObj*>(head); node != nullptr; node = node->next) {
            if (node->sub_font_index == sub_font_index) {
                match = node;
                break;
            }
        }

        const u32 open_flag = lock_word & 0x40000000u;
        const u32 count_bits = lock_word & 0x0FFFFFFFu;
        if (open_flag != 0 && count_bits != 0 && match) {
            const u32 refcount = match->refcount;
            const u32 dec_lock = lock_word - 1;
            lock_word = dec_lock;
            if (refcount != 0) {
                if (refcount == 1) {
                    const u64 next = reinterpret_cast<u64>(match->next);
                    const u64 prev = reinterpret_cast<u64>(match->prev);
                    (void)close_fn(match, 0);
                    if (prev == 0) {
                        if (mode_low == 1) {
                            entry->obj_mode1 = reinterpret_cast<void*>(next);
                        } else if (mode_low == 2) {
                            entry->obj_mode2 = reinterpret_cast<void*>(next);
                        } else if (mode_low == 3) {
                            entry->obj_mode3 = reinterpret_cast<void*>(next);
                        }
                    }
                } else {
                    match->refcount = refcount - 1;
                }
            }
            if (count_bits == 1) {
                lock_word = (~open_flag) & dec_lock;
            }
        }

        const u32 remaining = lock_word & 0x0FFFFFFFu;
        if (mode_low == 3) {
            if (remaining == 0) {
                entry->obj_mode3 = nullptr;
            }
            entry->lock_mode3 = lock_word & 0x7fffffffu;
        } else if (mode_low == 1) {
            if (remaining == 0) {
                entry->obj_mode1 = nullptr;
            }
            entry->lock_mode1 = lock_word & 0x7fffffffu;
        } else {
            if (remaining == 0) {
                entry->obj_mode2 = nullptr;
            }
            entry->lock_mode2 = lock_word & 0x7fffffffu;
        }

        if (remaining == 0 && entry->obj_mode1 == nullptr && entry->obj_mode3 == nullptr &&
            entry->obj_mode2 == nullptr) {
            entry->unique_id = 0;
            entry->active = 0;
        }
    }
}

s32 ComputeHorizontalLayoutBlocks(OrbisFontHandle fontHandle, const void* style_state_block,
                                  u8 (*out_words)[16]) {
    if (out_words) {
        std::memset(out_words[0], 0, 16);
        std::memset(out_words[1] + 8, 0, 8);
        std::memset(out_words[2], 0, 4);
    }

    const auto fail_return = [&]() -> s32 {
        if (out_words) {
            std::memset(out_words[1] + 8, 0, 8);
            std::memset(out_words[2], 0, 4);
            std::memset(out_words[0], 0, 16);
        }
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    };

    auto* font = GetNativeFont(fontHandle);
    if (!font) {
        return fail_return();
    }

    auto* lib = static_cast<FontLibOpaque*>(font->library);
    if (!lib) {
        return fail_return();
    }

    const auto* fontset_record =
        static_cast<const FontSetRecordHeader*>(font->open_info.fontset_record);

    void* ctx_void = nullptr;
    const u32* entry_indices = nullptr;
    int entry_count = 0;
    if (!fontset_record) {
        ctx_void = lib->external_fonts_ctx;
        entry_indices = &font->open_info.ctx_entry_index;
        entry_count = 1;
    } else {
        ctx_void = lib->sysfonts_ctx;
        entry_count = static_cast<int>(fontset_record->entry_count);
        entry_indices = reinterpret_cast<const u32*>(fontset_record + 1);
        if (entry_count == 0) {
            return fail_return();
        }
    }

    auto* ctx = static_cast<u8*>(ctx_void);
    if (!ctx || !entry_indices) {
        return fail_return();
    }

    const u32 mode_low = static_cast<u32>(font->flags) & 0x0Fu;
    const u32 sub_font_index = font->open_info.sub_font_index;
    const auto* driver = reinterpret_cast<const SysDriver*>(lib->sys_driver);
    if (!driver) {
        return fail_return();
    }
    const auto set_char_with_dpi = driver->set_char_with_dpi;
    const auto set_char_default_dpi = driver->set_char_default_dpi;
    const auto compute_layout = driver->compute_layout;
    if (!set_char_with_dpi || !set_char_default_dpi || !compute_layout) {
        return fail_return();
    }

    const auto* style = static_cast<const StyleStateBlock*>(style_state_block);
    if (!style) {
        return fail_return();
    }

    float baseline_max = 0.0f;
    float delta_max = 0.0f;
    float effect_for_baseline = 0.0f;
    float effect_for_delta = 0.0f;

    F32x2 acc_u13_i8{};
    F32x2 acc_x_bounds{};

    const auto load_f32x2_from_u64 = [](u64 bits) -> F32x2 {
        const u32 lo = static_cast<u32>(bits & 0xFFFFFFFFull);
        const u32 hi = static_cast<u32>(bits >> 32);
        return {
            .lo = std::bit_cast<float>(lo),
            .hi = std::bit_cast<float>(hi),
        };
    };

    const auto flip_sign_bit = [](float v) -> float {
        u32 bits = std::bit_cast<u32>(v);
        bits ^= 0x80000000u;
        return std::bit_cast<float>(bits);
    };

    const auto ordered_lt = [](float a, float b) -> bool {
        return !std::isnan(a) && !std::isnan(b) && (a < b);
    };

    const auto maxss = [](float a, float b) -> float {
        if (std::isnan(a) || std::isnan(b)) {
            return b;
        }
        if (a > b) {
            return a;
        }
        if (a < b) {
            return b;
        }
        if (a == 0.0f && b == 0.0f) {
            const bool a_neg = std::signbit(a);
            const bool b_neg = std::signbit(b);
            if (a_neg != b_neg) {
                return a_neg ? b : a;
            }
        }
        return a;
    };

    for (int n = entry_count; n != 0; --n) {
        const u32 font_id = *entry_indices;
        ++entry_indices;
        if (static_cast<s32>(font_id) < 0) {
            continue;
        }

        float fontset_scale_factor = 1.0f;
        int fontset_shift_value = 0;
        if (fontset_record &&
            fontset_record->magic == Libraries::Font::Internal::FontSetSelector::kMagic) {
            if (auto* st = Internal::TryGetState(fontHandle)) {
                if (st->system_requested) {
                    if (font_id == st->system_font_id) {
                        fontset_scale_factor = st->system_font_scale_factor;
                        fontset_shift_value = st->system_font_shift_value;
                    } else {
                        for (const auto& fb : st->system_fallback_faces) {
                            if (fb.font_id == font_id) {
                                fontset_scale_factor = fb.scale_factor;
                                fontset_shift_value = fb.shift_value;
                                break;
                            }
                        }
                    }
                }
            }
        } else {
            (void)fontset_record;
        }

        void* head = nullptr;
        u32 lock_word = 0;
        u8* entry_u8 = AcquireFontCtxEntry(ctx, font_id, mode_low, &head, &lock_word);
        auto* entry = entry_u8 ? reinterpret_cast<FontCtxEntry*>(entry_u8) : nullptr;
        if (!entry) {
            return fail_return();
        }

        FontObj* match = nullptr;
        for (auto* node = static_cast<FontObj*>(head); node != nullptr; node = node->next) {
            if (node->sub_font_index == sub_font_index) {
                match = node;
                break;
            }
        }

        s32 rc = ORBIS_FONT_ERROR_FATAL;
        HorizontalLayoutBlocks layout_tmp{};
        if (((lock_word & 0x40000000u) != 0) && ((lock_word & 0x0FFFFFFFu) != 0) && match) {
            float scale_x = 0.0f;
            float scale_y = 0.0f;
            {
                scale_x = fontset_scale_factor * style->scale_w;
                scale_y = fontset_scale_factor * style->scale_h;
            }

            match->font_handle = fontHandle;
            match->shift_units_x = 0;
            match->shift_units_y = fontset_shift_value;

            if (style->scale_unit == 0) {
                rc = set_char_default_dpi(match, scale_x, scale_y, &scale_x, &scale_y);
            } else {
                rc = set_char_with_dpi(match, style->dpi_x, style->dpi_y, scale_x, scale_y,
                                       &scale_x, &scale_y);
            }
            if (rc == ORBIS_OK) {
                UpdateFtFontObjShiftCache(reinterpret_cast<u8*>(match));
                rc = compute_layout(match, style_state_block, LayoutWordsBytes(layout_tmp.words()));
            }
        }

        u32* lock_word_ptr = nullptr;
        if (mode_low == 3) {
            if ((lock_word & 0x0FFFFFFFu) == 0) {
                entry->obj_mode3 = nullptr;
            }
            lock_word_ptr = &entry->lock_mode3;
        } else if (mode_low == 1) {
            if ((lock_word & 0x0FFFFFFFu) == 0) {
                entry->obj_mode1 = nullptr;
            }
            lock_word_ptr = &entry->lock_mode1;
        } else {
            if ((lock_word & 0x0FFFFFFFu) == 0) {
                entry->obj_mode2 = nullptr;
            }
            lock_word_ptr = &entry->lock_mode2;
        }

        if (((lock_word & 0x0FFFFFFFu) == 0) && entry->obj_mode1 == nullptr &&
            entry->obj_mode3 == nullptr && entry->obj_mode2 == nullptr) {
            entry->unique_id = 0;
            entry->active = 0;
        }
        *lock_word_ptr = lock_word & 0x7FFFFFFFu;

        if (rc != ORBIS_OK) {
            return fail_return();
        }

        const auto layout = layout_tmp.values();
        const float line_advance = layout.line_advance;
        const float baseline = layout.baseline;
        const float effect_h = layout.effect_height;
        const u64 x_bounds_bits = layout.x_bounds_bits;
        const u64 i8_adj_u13_bits = layout.i8_adj_u13_bits;

        const F32x2 prev_u13_i8 = acc_u13_i8;
        const F32x2 prev_bounds = acc_x_bounds;

        const float prev_baseline_max = baseline_max;
        const float prev_effect_for_baseline = effect_for_baseline;
        const bool baseline_update = ordered_lt(prev_baseline_max, baseline);
        baseline_max = maxss(baseline, prev_baseline_max);
        effect_for_baseline = baseline_update ? effect_h : prev_effect_for_baseline;

        const float prev_delta_max = delta_max;
        const float prev_effect_for_delta = effect_for_delta;
        const float new_delta = line_advance - baseline;
        const bool delta_update = ordered_lt(prev_delta_max, new_delta);
        delta_max = maxss(new_delta, prev_delta_max);
        effect_for_delta = delta_update ? effect_h : prev_effect_for_delta;

        const F32x2 v_i8_adj_u13 = load_f32x2_from_u64(i8_adj_u13_bits);
        const F32x2 v_x_bounds = load_f32x2_from_u64(x_bounds_bits);

        const F32x2 perm = {.lo = v_i8_adj_u13.hi, .hi = v_i8_adj_u13.lo};
        const F32x2 diff = {.lo = v_x_bounds.lo - perm.lo, .hi = v_x_bounds.hi - perm.hi};
        const F32x2 sum = {
            .lo = prev_u13_i8.lo + prev_bounds.lo,
            .hi = prev_u13_i8.hi + prev_bounds.hi,
        };

        const F32x2 inserted = {.lo = flip_sign_bit(v_i8_adj_u13.hi), .hi = v_i8_adj_u13.lo};

        const bool mask_lo = ordered_lt(diff.lo, sum.lo);
        const bool mask_hi = ordered_lt(diff.hi, sum.hi);
        if (mask_lo) {
            acc_x_bounds.lo = v_x_bounds.lo;
            acc_u13_i8.lo = inserted.lo;
        }
        if (mask_hi) {
            acc_x_bounds.hi = v_x_bounds.hi;
            acc_u13_i8.hi = inserted.hi;
        }
    }

    if (!out_words) {
        return fail_return();
    }

    const float line_h = baseline_max + delta_max;
    const float effect_h = maxss(effect_for_baseline, effect_for_delta);

    if ((reinterpret_cast<std::uintptr_t>(out_words) & (alignof(HorizontalLayoutBlocks) - 1)) ==
        0) {
        auto* out_blocks = reinterpret_cast<HorizontalLayoutBlocks*>(out_words);
        out_blocks->set_baseline(baseline_max);
        out_blocks->set_line_advance(line_h);
        out_blocks->set_x_bounds(acc_x_bounds.lo, acc_x_bounds.hi);
        out_blocks->set_effect_height(effect_h);
        out_blocks->set_i8_adj_u13_lanes(acc_u13_i8.lo, acc_u13_i8.hi);
    } else {
        auto out = HorizontalLayoutBlocksIo{out_words}.fields();
        out.baseline = baseline_max;
        out.line_advance = line_h;
        out.x_bound_0 = acc_x_bounds.lo;
        out.x_bound_1 = acc_x_bounds.hi;
        out.effect_height = effect_h;
        out.u13_i8_lane0 = acc_u13_i8.lo;
        out.u13_i8_lane1 = acc_u13_i8.hi;
    }

    return ORBIS_OK;
}

s32 ComputeVerticalLayoutBlocks(OrbisFontHandle fontHandle, const void* style_state_block,
                                u8 (*out_words)[16]) {
    auto* font = GetNativeFont(fontHandle);
    auto* lib = font ? reinterpret_cast<FontLibOpaque*>(font->library) : nullptr;
    if (!lib) {
        if (out_words) {
            std::memset(out_words[0], 0, 16);
            std::memset(out_words[1] + 4, 0, 8);
            std::memset(out_words[1] + 0x0C, 0, 4);
        }
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    const void* fontset_record = font->open_info.fontset_record;

    const u32 mode_low = static_cast<u32>(font->flags) & 0x0Fu;

    const u32* font_ids = nullptr;
    int entry_count = 0;
    u32 rc = ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;

    if (!fontset_record) {
        entry_count = 1;
        font_ids = &font->open_info.ctx_entry_index;
        rc = ORBIS_OK;
    } else {
        entry_count =
            *reinterpret_cast<const int*>(reinterpret_cast<const u8*>(fontset_record) + 0x20);
        font_ids = reinterpret_cast<const u32*>(reinterpret_cast<const u8*>(fontset_record) + 0x24);
        rc = (entry_count != 0) ? ORBIS_OK : ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    u8* ctx = fontset_record ? static_cast<u8*>(lib->sysfonts_ctx)
                             : static_cast<u8*>(lib->external_fonts_ctx);
    if (!ctx) {
        if (out_words) {
            std::memset(out_words[0], 0, 16);
            std::memset(out_words[1] + 4, 0, 8);
            std::memset(out_words[1] + 0x0C, 0, 4);
        }
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    if (static_cast<s32>(rc) != ORBIS_OK) {
        if (out_words) {
            std::memset(out_words[0], 0, 16);
            std::memset(out_words[1] + 4, 0, 8);
            std::memset(out_words[1] + 0x0C, 0, 4);
        }
        return static_cast<s32>(rc);
    }

    float acc_neg_local64_max = 0.0f;
    float acc_sum_max = 0.0f;
    float assoc_for_neg_local64_max = 0.0f;
    float assoc_for_sum_max = 0.0f;
    float acc_local60_max = 0.0f;
    float acc_local5c_max = 0.0f;
    float acc_diff_min = 0.0f;
    float acc_temp_min = 0.0f;

    const SysDriver* driver = static_cast<const SysDriver*>(lib->sys_driver);
    if (!driver) {
        if (out_words) {
            std::memset(out_words[0], 0, 16);
            std::memset(out_words[1] + 4, 0, 8);
            std::memset(out_words[1] + 0x0C, 0, 4);
        }
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    for (int i = 0; i < entry_count; ++i) {
        const u32 font_id = font_ids ? font_ids[i] : 0u;
        if (static_cast<s32>(font_id) < 0) {
            continue;
        }

        float scale_factor = 1.0f;
        s32 shift_value = 0;
        if (fontset_record && static_cast<const FontSetRecordHeader*>(fontset_record)->magic ==
                                  Libraries::Font::Internal::FontSetSelector::kMagic) {
            if (auto* st = Internal::TryGetState(fontHandle)) {
                if (st->system_requested) {
                    if (font_id == st->system_font_id) {
                        scale_factor = st->system_font_scale_factor;
                        shift_value = st->system_font_shift_value;
                    } else {
                        for (const auto& fb : st->system_fallback_faces) {
                            if (fb.font_id == font_id) {
                                scale_factor = fb.scale_factor;
                                shift_value = fb.shift_value;
                                break;
                            }
                        }
                    }
                }
            }
        } else if (fontset_record) {
            const auto desc = GetSysFontDesc(font_id);
            scale_factor = desc.scale_factor;
            shift_value = desc.shift_value;
        }

        void* head = nullptr;
        u32 lock_word = 0;
        u8* entry_u8 = AcquireFontCtxEntry(ctx, font_id, mode_low, &head, &lock_word);
        auto* entry = entry_u8 ? reinterpret_cast<FontCtxEntry*>(entry_u8) : nullptr;

        const u32 sub_font_index = font->open_info.sub_font_index;
        auto* font_obj = static_cast<FontObj*>(head);
        while (font_obj && font_obj->sub_font_index != sub_font_index) {
            font_obj = font_obj->next;
        }

        if (!entry || !font_obj) {
            rc = ORBIS_FONT_ERROR_FATAL;
            break;
        }

        rc = ORBIS_FONT_ERROR_FATAL;
        if (((lock_word & 0x40000000u) != 0) && ((lock_word & 0x0FFFFFFFu) != 0)) {
            const auto* style = static_cast<const StyleStateBlock*>(style_state_block);
            const float base_scale_x = style ? style->scale_w : 0.0f;
            const float base_scale_y = style ? style->scale_h : 0.0f;

            float scale_x = scale_factor * base_scale_x;
            float scale_y = scale_factor * base_scale_y;

            font_obj->font_handle = fontHandle;
            font_obj->shift_units_x = 0;
            font_obj->shift_units_y = shift_value;

            s32 call_rc = ORBIS_FONT_ERROR_FATAL;
            if (style && style->scale_unit == 0) {
                call_rc = driver->set_char_default_dpi
                              ? driver->set_char_default_dpi(font_obj, scale_x, scale_y, &scale_x,
                                                             &scale_y)
                              : ORBIS_FONT_ERROR_FATAL;
            } else {
                call_rc = driver->set_char_with_dpi
                              ? driver->set_char_with_dpi(font_obj, style ? style->dpi_x : 0u,
                                                          style ? style->dpi_y : 0u, scale_x,
                                                          scale_y, &scale_x, &scale_y)
                              : ORBIS_FONT_ERROR_FATAL;
            }

            VerticalLayoutAltBlocks layout_alt{};
            if (call_rc == ORBIS_OK) {
                UpdateFtFontObjShiftCache(reinterpret_cast<u8*>(font_obj));
                call_rc = driver->compute_layout_alt
                              ? driver->compute_layout_alt(font_obj, style_state_block,
                                                           LayoutWordsBytes(layout_alt.words()))
                              : ORBIS_FONT_ERROR_FATAL;
            }

            if (call_rc == ORBIS_OK) {
                const auto v = layout_alt.values();
                const float primary_0x00 = v.metrics.unknown_0x00;
                const float baseline_offset_x_candidate = v.metrics.baseline_offset_x_candidate;
                const float primary_0x08 = v.metrics.unknown_0x08;
                const float primary_0x0C = v.metrics.unknown_0x0C;
                const float decoration_span_candidate = v.extras.decoration_span_candidate;
                const float extra_0x08 = v.extras.unknown_0x08;
                const float extra_0x0C = v.extras.unknown_0x0C;

                const float neg_baseline_offset_x_candidate = -baseline_offset_x_candidate;
                const float sum_primary_0x00_and_baseline_offset_x_candidate =
                    baseline_offset_x_candidate + primary_0x00;

                if (acc_neg_local64_max < neg_baseline_offset_x_candidate) {
                    assoc_for_neg_local64_max = decoration_span_candidate;
                }
                acc_neg_local64_max =
                    std::max(acc_neg_local64_max, neg_baseline_offset_x_candidate);

                if (acc_sum_max < sum_primary_0x00_and_baseline_offset_x_candidate) {
                    assoc_for_sum_max = decoration_span_candidate;
                }
                acc_sum_max =
                    std::max(acc_sum_max, sum_primary_0x00_and_baseline_offset_x_candidate);

                const float diff = extra_0x08 - baseline_offset_x_candidate;
                acc_diff_min = std::min(diff, acc_diff_min);

                acc_local60_max = std::max(primary_0x08, acc_local60_max);

                const float sum2 = sum_primary_0x00_and_baseline_offset_x_candidate + extra_0x0C;
                acc_temp_min = std::min(sum2, acc_diff_min);

                acc_local5c_max = std::max(primary_0x0C, acc_local5c_max);
                rc = ORBIS_OK;
            } else {
                rc = call_rc;
            }
        }

        const u32 low_bits = lock_word & 0x0FFFFFFFu;
        u32* lock_word_store = nullptr;
        if (mode_low == 3) {
            if (low_bits == 0) {
                entry->obj_mode3 = nullptr;
            }
            lock_word_store = &entry->lock_mode3;
        } else if (mode_low == 1) {
            if (low_bits == 0) {
                entry->obj_mode1 = nullptr;
            }
            lock_word_store = &entry->lock_mode1;
        } else {
            if (low_bits == 0) {
                entry->obj_mode2 = nullptr;
            }
            lock_word_store = &entry->lock_mode2;
        }

        if ((low_bits == 0) && entry->obj_mode1 == nullptr && entry->obj_mode3 == nullptr &&
            entry->obj_mode2 == nullptr) {
            entry->unique_id = 0;
            entry->active = 0;
        }
        if (lock_word_store) {
            *lock_word_store = lock_word & 0x7FFFFFFFu;
        }

        if (static_cast<s32>(rc) != ORBIS_OK) {
            break;
        }
    }

    if (static_cast<s32>(rc) != ORBIS_OK) {
        if (out_words) {
            std::memset(out_words[0], 0, 16);
            std::memset(out_words[1] + 4, 0, 8);
            std::memset(out_words[1] + 0x0C, 0, 4);
        }
        return static_cast<s32>(rc);
    }

    if (out_words) {
        const float column_advance = acc_neg_local64_max + acc_sum_max;
        const float baseline_offset_x = -acc_neg_local64_max;
        const float decoration_span = std::max(assoc_for_sum_max, assoc_for_neg_local64_max);
        const float unknown_decoration_0x08 = acc_temp_min - acc_neg_local64_max;
        const float unknown_decoration_0x0C = -acc_sum_max;

        if ((reinterpret_cast<std::uintptr_t>(out_words) & (alignof(VerticalLayoutBlocks) - 1)) ==
            0) {
            auto* out_blocks = reinterpret_cast<VerticalLayoutBlocks*>(out_words);
            out_blocks->set_column_advance(column_advance);
            out_blocks->set_baseline_offset_x(baseline_offset_x);
            out_blocks->set_unknown_metrics_0x08(acc_local60_max);
            out_blocks->set_unknown_metrics_0x0C(acc_local5c_max);
            out_blocks->set_decoration_span(decoration_span);
            out_blocks->set_unknown_decoration_0x08(unknown_decoration_0x08);
            out_blocks->set_unknown_decoration_0x0C(unknown_decoration_0x0C);
        } else {
            auto out = VerticalLayoutBlocksIo{out_words}.fields();
            out.column_advance = column_advance;
            out.baseline_offset_x = baseline_offset_x;
            out.unknown_metrics_0x08 = acc_local60_max;
            out.unknown_metrics_0x0C = acc_local5c_max;
            out.decoration_span = decoration_span;
            out.unknown_decoration_0x08 = unknown_decoration_0x08;
            out.unknown_decoration_0x0C = unknown_decoration_0x0C;
        }
    }
    return ORBIS_OK;
}

s32 GetCharGlyphMetrics(OrbisFontHandle fontHandle, u32 code, OrbisFontGlyphMetrics* metrics,
                        bool use_cached_style) {
    auto clear_metrics = [&] {
        if (metrics) {
            *metrics = {};
        }
    };

    auto* font = GetNativeFont(fontHandle);
    if (!font || font->magic != 0x0F02) {
        clear_metrics();
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }
    if (code == 0) {
        clear_metrics();
        return ORBIS_FONT_ERROR_NO_SUPPORT_CODE;
    }
    if (!metrics) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    u32 prev_font_lock = 0;
    if (!AcquireFontLock(font, prev_font_lock)) {
        clear_metrics();
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    u32 prev_cached_lock = 0;
    if (use_cached_style) {
        if (!AcquireCachedStyleLock(font, prev_cached_lock)) {
            ReleaseFontLock(font, prev_font_lock);
            clear_metrics();
            return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
        }
    }

    auto* st = TryGetState(fontHandle);
    if (!st) {
        if (use_cached_style) {
            font->cached_style.cache_lock_word = prev_cached_lock;
        }
        font->lock_word = prev_font_lock;
        clear_metrics();
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    if (use_cached_style && font->renderer_binding.renderer == nullptr) {
        font->cached_style.cache_lock_word = prev_cached_lock;
        font->lock_word = prev_font_lock;
        clear_metrics();
        return ORBIS_FONT_ERROR_NOT_BOUND_RENDERER;
    }

    const void* style_state_block = use_cached_style ? static_cast<const void*>(&font->cached_style)
                                                     : static_cast<const void*>(font->style_frame);
    float scale_w = 0.0f;
    float scale_h = 0.0f;
    const s32 scale_rc = StyleStateGetScalePixel(style_state_block, &scale_w, &scale_h);
    if (scale_rc != ORBIS_OK) {
        if (use_cached_style) {
            font->cached_style.cache_lock_word = prev_cached_lock;
        }
        font->lock_word = prev_font_lock;
        clear_metrics();
        return scale_rc;
    }

    FT_Face resolved_face = nullptr;
    if (st->ext_face_ready) {
        resolved_face = st->ext_ft_face;
    } else {
        bool system_attached = false;
        (void)ReportSystemFaceRequest(*st, fontHandle, system_attached);
        if (system_attached && st->ext_face_ready) {
            resolved_face = st->ext_ft_face;
        }
    }

    if (!resolved_face) {
        if (use_cached_style) {
            font->cached_style.cache_lock_word = prev_cached_lock;
        }
        font->lock_word = prev_font_lock;
        clear_metrics();
        return ORBIS_FONT_ERROR_NO_SUPPORT_FUNCTION;
    }

    u32 resolved_code = code;
    u32 resolved_font_id = 0;
    float resolved_scale_factor = 1.0f;
    const auto* fontset_record =
        static_cast<const FontSetRecordHeader*>(font->open_info.fontset_record);
    const bool is_internal_fontset_record =
        fontset_record &&
        fontset_record->magic == Libraries::Font::Internal::FontSetSelector::kMagic;
    if (font->open_info.fontset_record) {
        u32 mapped_font_id = 0;
        resolved_code =
            ResolveSysFontCodepoint(font->open_info.fontset_record, static_cast<u64>(code),
                                    font->open_info.fontset_flags, &mapped_font_id);
        if (resolved_code == 0) {
            if (use_cached_style) {
                font->cached_style.cache_lock_word = prev_cached_lock;
            }
            font->lock_word = prev_font_lock;
            clear_metrics();
            return ORBIS_FONT_ERROR_NO_SUPPORT_CODE;
        }

        resolved_font_id = mapped_font_id;
        resolved_scale_factor = st->system_font_scale_factor;
        if (is_internal_fontset_record) {
            auto* lib = static_cast<FontLibOpaque*>(font->library);
            auto* ctx = lib ? static_cast<u8*>(lib->sysfonts_ctx) : nullptr;
            const u32 mode_low = static_cast<u32>(font->flags) & 0x0Fu;
            if (ctx && (mode_low == 1 || mode_low == 2 || mode_low == 3)) {
                void* head = nullptr;
                u32 lock_word = 0;
                u8* entry_u8 =
                    AcquireFontCtxEntry(ctx, mapped_font_id, mode_low, &head, &lock_word);
                auto* entry = entry_u8 ? reinterpret_cast<FontCtxEntry*>(entry_u8) : nullptr;
                auto* node = static_cast<FontObj*>(head);
                const u32 sub_font_index = font->open_info.sub_font_index;
                while (node) {
                    if (node->sub_font_index == sub_font_index) {
                        break;
                    }
                    node = node->next;
                }
                resolved_face = node ? static_cast<FT_Face>(node->ft_face) : nullptr;
                if (entry) {
                    if (mode_low == 3) {
                        entry->lock_mode3 = lock_word & 0x7fffffffu;
                    } else if (mode_low == 2) {
                        entry->lock_mode2 = lock_word & 0x7fffffffu;
                    } else {
                        entry->lock_mode1 = lock_word & 0x7fffffffu;
                    }
                }
            }
        } else if (st->system_requested) {
            if (mapped_font_id == st->system_font_id) {
                resolved_face = st->ext_ft_face;
            } else {
                resolved_face = nullptr;
                for (const auto& fb : st->system_fallback_faces) {
                    if (fb.ready && fb.ft_face && fb.font_id == mapped_font_id) {
                        resolved_face = fb.ft_face;
                        resolved_scale_factor = fb.scale_factor;
                        break;
                    }
                }
            }
        }
        if (!resolved_face) {
            if (use_cached_style) {
                font->cached_style.cache_lock_word = prev_cached_lock;
            }
            font->lock_word = prev_font_lock;
            clear_metrics();
            return ORBIS_FONT_ERROR_NO_SUPPORT_GLYPH;
        }
    }

    FT_UInt resolved_glyph_index =
        FT_Get_Char_Index(resolved_face, static_cast<FT_ULong>(resolved_code));
    if (resolved_glyph_index == 0) {
        if (st->ext_face_ready && st->ext_ft_face && resolved_face != st->ext_ft_face) {
            const FT_UInt gi =
                FT_Get_Char_Index(st->ext_ft_face, static_cast<FT_ULong>(resolved_code));
            if (gi != 0) {
                resolved_face = st->ext_ft_face;
                resolved_glyph_index = gi;
                resolved_scale_factor = st->system_font_scale_factor;
            }
        }
    }
    if (resolved_glyph_index == 0 && !font->open_info.fontset_record) {
        for (const auto& fb : st->system_fallback_faces) {
            if (!fb.ready || !fb.ft_face) {
                continue;
            }
            const FT_UInt gi = FT_Get_Char_Index(fb.ft_face, static_cast<FT_ULong>(resolved_code));
            if (gi == 0) {
                continue;
            }
            resolved_face = fb.ft_face;
            resolved_glyph_index = gi;
            resolved_scale_factor = fb.scale_factor;
            break;
        }
    }

    if (resolved_glyph_index == 0) {
        LogGetCharGlyphMetricsFailOnce("no_glyph", ORBIS_FONT_ERROR_NO_SUPPORT_GLYPH, 0,
                                       font->open_info.fontset_record != nullptr, code,
                                       resolved_glyph_index, resolved_face, scale_w, scale_h);
        if (use_cached_style) {
            font->cached_style.cache_lock_word = prev_cached_lock;
        }
        font->lock_word = prev_font_lock;
        clear_metrics();
        return ORBIS_FONT_ERROR_NO_SUPPORT_GLYPH;
    }

    float range_scale = 1.0f;
    s32 shift_x_units = 0;
    s32 shift_y_units = 0;
    if (font->open_info.fontset_record && !is_internal_fontset_record) {
        if (st->system_requested) {
            if (resolved_font_id == st->system_font_id) {
                shift_y_units = st->system_font_shift_value;
            } else {
                for (const auto& fb : st->system_fallback_faces) {
                    if (fb.ready && fb.ft_face && fb.font_id == resolved_font_id) {
                        shift_y_units = fb.shift_value;
                        break;
                    }
                }
            }
        }
        if (shift_y_units == 0) {
            shift_y_units = GetSysFontDesc(resolved_font_id).shift_value;
        }
        if (const std::uint8_t* range_rec =
                FindSysFontRangeRecord(resolved_font_id, resolved_code)) {
            alignas(4) SysFontRangeRecord rec{};
            std::memcpy(&rec, range_rec, sizeof(rec));
            const s16 range_shift_x = rec.shift_x;
            const s16 range_shift_y = rec.shift_y;
            range_scale = rec.scale_mul;
            (void)rec.reserved_0x10;
            (void)rec.reserved_0x14;
            (void)rec.reserved_0x18;

            shift_x_units = static_cast<s32>(range_shift_x);
            shift_y_units += static_cast<s32>(range_shift_y);
        }
        if (range_scale == 0.0f) {
            range_scale = 1.0f;
        }
    }

    const float scaled_w = scale_w * resolved_scale_factor * range_scale;
    const float scaled_h = scale_h * resolved_scale_factor * range_scale;

    const auto char_w = static_cast<FT_F26Dot6>(static_cast<s32>(scaled_w * 64.0f));
    const auto char_h = static_cast<FT_F26Dot6>(static_cast<s32>(scaled_h * 64.0f));
    if (FT_Set_Char_Size(resolved_face, char_w, char_h, 72, 72) != 0) {
        if (use_cached_style) {
            font->cached_style.cache_lock_word = prev_cached_lock;
        }
        font->lock_word = prev_font_lock;
        clear_metrics();
        return ORBIS_FONT_ERROR_NO_SUPPORT_GLYPH;
    }

    if (shift_x_units != 0 || shift_y_units != 0) {
        if (!resolved_face->size) {
            if (use_cached_style) {
                font->cached_style.cache_lock_word = prev_cached_lock;
            }
            font->lock_word = prev_font_lock;
            clear_metrics();
            return ORBIS_FONT_ERROR_NO_SUPPORT_GLYPH;
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
        static_cast<FT_Int32>(FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP | FT_LOAD_VERTICAL_LAYOUT);

    if (FT_Load_Glyph(resolved_face, resolved_glyph_index, kFtLoadFlagsBase) != 0) {
        FT_Set_Transform(resolved_face, nullptr, nullptr);
        if (use_cached_style) {
            font->cached_style.cache_lock_word = prev_cached_lock;
        }
        font->lock_word = prev_font_lock;
        clear_metrics();
        return ORBIS_FONT_ERROR_NO_SUPPORT_GLYPH;
    }
    FT_Set_Transform(resolved_face, nullptr, nullptr);

    const FT_GlyphSlot slot = resolved_face->glyph;
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

    if (use_cached_style) {
        font->cached_style.cache_lock_word = prev_cached_lock;
    }
    font->lock_word = prev_font_lock;
    return ORBIS_OK;
}

static StyleFrameScaleState ResolveStyleFrameScaleFromCachedStyle(const OrbisFontStyleFrame* style,
                                                                  float base_scale_w,
                                                                  float base_scale_h,
                                                                  u32 base_dpi_x, u32 base_dpi_y) {
    StyleFrameScaleState resolved{
        .scale_w = base_scale_w,
        .scale_h = base_scale_h,
        .dpi_x = base_dpi_x,
        .dpi_y = base_dpi_y,
        .scale_overridden = false,
        .dpi_overridden = false,
    };
    if (!style || style->magic != kStyleFrameMagic) {
        return resolved;
    }
    if (style->hDpi != 0) {
        resolved.dpi_x = style->hDpi;
        resolved.dpi_overridden = true;
    }
    if (style->vDpi != 0) {
        resolved.dpi_y = style->vDpi;
        resolved.dpi_overridden = true;
    }
    if ((style->flags1 & kStyleFrameFlagScale) == 0) {
        return resolved;
    }
    resolved.scale_overridden = true;

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

namespace {

static inline RenderSurfaceSystemUse* GetSurfaceSystemUse(
    Libraries::Font::OrbisFontRenderSurface* surf) {
    return surf ? reinterpret_cast<RenderSurfaceSystemUse*>(surf->reserved_q) : nullptr;
}
static inline const RenderSurfaceSystemUse* GetSurfaceSystemUse(
    const Libraries::Font::OrbisFontRenderSurface* surf) {
    return surf ? reinterpret_cast<const RenderSurfaceSystemUse*>(surf->reserved_q) : nullptr;
}

static const StyleStateBlock* ResolveSurfaceStyleState(
    const StyleStateBlock* cached_style_state, const Libraries::Font::OrbisFontRenderSurface* surf,
    const RenderSurfaceSystemUse* sys, StyleStateBlock& tmp_out) {
    if (!cached_style_state) {
        return nullptr;
    }
    if (!surf || (surf->styleFlag & 0x1) == 0 || !sys || !ValidateStyleFramePtr(sys->styleframe)) {
        return cached_style_state;
    }

    const auto* frame = sys->styleframe;
    tmp_out = *cached_style_state;

    const u8 flags1 = frame->flags1;
    if ((flags1 & kStyleFrameFlagScale) != 0) {
        tmp_out.scale_unit = frame->scaleUnit;
        tmp_out.scale_w = frame->scalePixelW;
        tmp_out.scale_h = frame->scalePixelH;
        tmp_out.dpi_x = frame->hDpi ? frame->hDpi : 0x48;
        tmp_out.dpi_y = frame->vDpi ? frame->vDpi : 0x48;
    } else {
        const u32 want_x = (frame->hDpi == 0) ? tmp_out.dpi_x : frame->hDpi;
        const u32 want_y = (frame->vDpi == 0) ? tmp_out.dpi_y : frame->vDpi;
        tmp_out.dpi_x = (want_x != 0) ? 0x48 : 0;
        tmp_out.dpi_y = (want_y != 0) ? 0x48 : 0;
    }

    if ((flags1 & kStyleFrameFlagSlant) != 0) {
        tmp_out.slant_ratio = frame->slantRatio;
    }
    if ((flags1 & kStyleFrameFlagWeight) != 0) {
        tmp_out.effect_weight_x = frame->effectWeightX;
        tmp_out.effect_weight_y = frame->effectWeightY;
    }
    return &tmp_out;
}

static s32 RenderGlyphIndexToSurface(FontObj& font_obj, u32 glyph_index,
                                     Libraries::Font::OrbisFontRenderSurface* surf, float x,
                                     float y, Libraries::Font::OrbisFontGlyphMetrics* metrics,
                                     Libraries::Font::OrbisFontRenderOutput* result) {
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

    FT_Face face = static_cast<FT_Face>(font_obj.ft_face);
    if (!face || !face->size) {
        return ORBIS_FONT_ERROR_NO_SUPPORT_GLYPH;
    }

    const float frac_x = x - std::floor(x);
    const float frac_y = y - std::floor(y);
    FT_Vector delta{};
    delta.x = static_cast<FT_Pos>(static_cast<s32>(frac_x * 64.0f));
    delta.y = static_cast<FT_Pos>(-static_cast<s32>(frac_y * 64.0f));

    if ((font_obj.shift_units_x != 0 || font_obj.shift_units_y != 0) && face->size) {
        const long x_scale = static_cast<long>(face->size->metrics.x_scale);
        const long y_scale = static_cast<long>(face->size->metrics.y_scale);

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

        delta.x += static_cast<FT_Pos>(round_fixed_mul(x_scale, font_obj.shift_units_x));
        delta.y += static_cast<FT_Pos>(round_fixed_mul(y_scale, font_obj.shift_units_y));
    }

    FT_Set_Transform(face, nullptr, &delta);

    constexpr FT_Int32 kFtLoadFlagsRender = static_cast<FT_Int32>(
        FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP | FT_LOAD_VERTICAL_LAYOUT | FT_LOAD_RENDER);
    if (FT_Load_Glyph(face, static_cast<FT_UInt>(glyph_index), kFtLoadFlagsRender) != 0) {
        FT_Set_Transform(face, nullptr, nullptr);
        return ORBIS_FONT_ERROR_NO_SUPPORT_GLYPH;
    }
    FT_Set_Transform(face, nullptr, nullptr);

    const FT_GlyphSlot slot = face->glyph;
    if (!slot) {
        return ORBIS_FONT_ERROR_NO_SUPPORT_GLYPH;
    }

    const int glyph_w = static_cast<int>(slot->bitmap.width);
    const int glyph_h = static_cast<int>(slot->bitmap.rows);
    const int x0 = static_cast<int>(slot->bitmap_left);
    const int y0 = -static_cast<int>(slot->bitmap_top);

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

    std::vector<unsigned char> glyph_bitmap;
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
            if (src_y < 0 || src_y >= glyph_h) {
                continue;
            }
            const std::uint8_t* src_row =
                glyph_bitmap.data() + static_cast<std::size_t>(src_y) * glyph_w;
            std::uint8_t* dst_row = dst_base + static_cast<std::size_t>(row) *
                                                   static_cast<std::size_t>(surf->widthByte);
            for (int col = start_col; col < end_col; ++col) {
                const int src_x = col - dest_x;
                if (src_x < 0 || src_x >= glyph_w) {
                    continue;
                }
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
    const int bottom_i = floor_int(bottom_f);

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

static StyleFrameScaleState ResolveMergedStyleFrameScale(
    const Libraries::Font::OrbisFontStyleFrame* base,
    const Libraries::Font::OrbisFontStyleFrame* over, const FontState& st) {
    StyleFrameScaleState resolved = ResolveStyleFrameScale(base, st);
    if (!ValidateStyleFramePtr(over)) {
        return resolved;
    }
    if ((over->flags1 & kStyleFrameFlagScale) != 0) {
        resolved.scale_overridden = true;
        const bool unit_is_pixel = (over->scaleUnit == 0);
        const u32 dpi_x = over->hDpi;
        const u32 dpi_y = over->vDpi;
        resolved.scale_w =
            unit_is_pixel ? over->scalePixelW
                          : (dpi_x ? PointsToPixels(over->scalePixelW, dpi_x) : over->scalePixelW);
        resolved.scale_h =
            unit_is_pixel ? over->scalePixelH
                          : (dpi_y ? PointsToPixels(over->scalePixelH, dpi_y) : over->scalePixelH);
    }
    if (!resolved.scale_overridden && st.scale_point_active) {
        resolved.scale_w = PointsToPixels(st.scale_point_w, resolved.dpi_x);
        resolved.scale_h = PointsToPixels(st.scale_point_h, resolved.dpi_y);
    }
    return resolved;
}

} // namespace

s32 RenderCharGlyphImageCore(Libraries::Font::OrbisFontHandle fontHandle, u32 code,
                             Libraries::Font::OrbisFontRenderSurface* surf, float x, float y,
                             Libraries::Font::OrbisFontGlyphMetrics* metrics,
                             Libraries::Font::OrbisFontRenderOutput* result) {
    if (!fontHandle) {
        ClearRenderOutputs(metrics, result);
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }
    if (code == 0) {
        ClearRenderOutputs(metrics, result);
        return ORBIS_FONT_ERROR_NO_SUPPORT_CODE;
    }
    if (!surf || !metrics || !result) {
        ClearRenderOutputs(metrics, result);
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    auto* font = GetNativeFont(fontHandle);
    if (!font || font->magic != 0x0F02) {
        ClearRenderOutputs(metrics, result);
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }
    if (font->renderer_binding.renderer == nullptr) {
        ClearRenderOutputs(metrics, result);
        return ORBIS_FONT_ERROR_NOT_BOUND_RENDERER;
    }

    auto* lib = static_cast<FontLibOpaque*>(font->library);
    if (!lib || lib->magic != 0x0F01) {
        ClearRenderOutputs(metrics, result);
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    const auto* driver = reinterpret_cast<const SysDriver*>(lib->sys_driver);
    if (!driver || !driver->glyph_index || !driver->set_char_with_dpi ||
        !driver->set_char_default_dpi) {
        ClearRenderOutputs(metrics, result);
        return ORBIS_FONT_ERROR_FATAL;
    }

    const auto mode_low = static_cast<u32>(font->flags) & 0x0Fu;
    const u32 sub_font_index = font->open_info.sub_font_index;

    const auto* cached_style_state = reinterpret_cast<const StyleStateBlock*>(&font->cached_style);
    StyleStateBlock surface_style_tmp{};
    const RenderSurfaceSystemUse* surf_sys = GetSurfaceSystemUse(surf);
    const StyleStateBlock* style_state =
        ResolveSurfaceStyleState(cached_style_state, surf, surf_sys, surface_style_tmp);
    if (!style_state) {
        ClearRenderOutputs(metrics, result);
        return ORBIS_FONT_ERROR_FATAL;
    }

    float scale_x_in = style_state->scale_w;
    float scale_y_in = style_state->scale_h;
    u32 dpi_x = style_state->dpi_x;
    u32 dpi_y = style_state->dpi_y;
    const u32 scale_unit = style_state->scale_unit;

    u32 mapped_code = code;
    u32 font_id = font->open_info.ctx_entry_index;
    std::uint8_t* ctx = static_cast<std::uint8_t*>(lib->external_fonts_ctx);
    float sys_scale_factor = 1.0f;
    s32 shift_x_units = 0;
    s32 shift_y_units = 0;

    u32 special_case_font_id = 0xffffffffu;
    if (font->open_info.fontset_record) {
        const auto* fontset_record =
            static_cast<const FontSetRecordHeader*>(font->open_info.fontset_record);
        const bool is_internal_fontset_record =
            fontset_record &&
            fontset_record->magic == Libraries::Font::Internal::FontSetSelector::kMagic;
        ctx = static_cast<std::uint8_t*>(lib->sysfonts_ctx);
        mapped_code =
            ResolveSysFontCodepoint(font->open_info.fontset_record, static_cast<u64>(code),
                                    font->open_info.fontset_flags, &font_id);
        if (mapped_code == 0) {
            ClearRenderOutputs(metrics, result);
            return ORBIS_FONT_ERROR_NO_SUPPORT_CODE;
        }

        if (is_internal_fontset_record) {
            if (auto* st = Internal::TryGetState(fontHandle)) {
                if (st->system_requested) {
                    if (font_id == st->system_font_id) {
                        sys_scale_factor = st->system_font_scale_factor;
                        shift_y_units = st->system_font_shift_value;
                    } else {
                        for (const auto& fb : st->system_fallback_faces) {
                            if (fb.font_id == font_id) {
                                sys_scale_factor = fb.scale_factor;
                                shift_y_units = fb.shift_value;
                                break;
                            }
                        }
                    }
                }
            }
        } else {
            const SysFontDesc desc = GetSysFontDesc(font_id);
            sys_scale_factor = desc.scale_factor;
            shift_y_units = desc.shift_value;

            float range_scale = 1.0f;
            if (const std::uint8_t* range_rec = FindSysFontRangeRecord(font_id, mapped_code)) {
                alignas(4) SysFontRangeRecord rec{};
                std::memcpy(&rec, range_rec, sizeof(rec));
                shift_x_units = static_cast<s32>(rec.shift_x);
                shift_y_units += static_cast<s32>(rec.shift_y);
                range_scale = rec.scale_mul;
            }
            if (range_scale != 0.0f) {
                sys_scale_factor *= range_scale;
            }
        }

        if (font_id == 10) {
            special_case_font_id = font_id;
        }
    }

    {
        static std::mutex s_mutex;
        static std::unordered_map<Libraries::Font::OrbisFontHandle, int> s_counts;
        std::scoped_lock lock(s_mutex);
        int& count = s_counts[fontHandle];
        const bool force_diag = (code == 0x000Au) || (code == 0x000Du) || (code == 0x25B3u);
        if (force_diag || count < 5) {
            if (!force_diag) {
                ++count;
            }
            LOG_DEBUG(Lib_Font,
                      "SysfontRender: handle={} code=U+{:04X} mapped=U+{:04X} font_id={} "
                      "scale_unit={} dpi_x={} dpi_y={} scale_w={} scale_h={} sys_scale_factor={} "
                      "shift_y_units={}",
                      static_cast<const void*>(fontHandle), code, mapped_code, font_id, scale_unit,
                      dpi_x, dpi_y, scale_x_in, scale_y_in, sys_scale_factor, shift_y_units);
        }
    }

    void* font_obj_void = nullptr;
    u32 lock_word = 0;
    std::uint8_t* entry_u8 = AcquireFontCtxEntryAndSelectNode(
        ctx, font_id, mode_low, sub_font_index, &font_obj_void, &lock_word);
    auto* font_obj = static_cast<FontObj*>(font_obj_void);
    if (!entry_u8 || !font_obj) {
        ClearRenderOutputs(metrics, result);
        return ORBIS_FONT_ERROR_FATAL;
    }

    s32 rc = ORBIS_FONT_ERROR_FATAL;
    const auto cleanup_return = [&](s32 ret) -> s32 {
        ReleaseFontCtxEntryLock(entry_u8, mode_low, lock_word);
        if (ret != ORBIS_OK) {
            ClearRenderOutputs(metrics, result);
        }
        return ret;
    };

    if (((lock_word & 0x40000000u) == 0) || ((lock_word & 0x0FFFFFFFu) == 0)) {
        return cleanup_return(rc);
    }

    font_obj->reserved_0x04 = static_cast<u32>(font->flags >> 15);
    font_obj->font_handle = fontHandle;
    font_obj->shift_units_x = shift_x_units;
    font_obj->shift_units_y = shift_y_units;
    font_obj->layout_seed_pair = 0;

    (void)StyleStateGetScalePixel(style_state, &font_obj->scale_x_0x50, &font_obj->scale_y_0x54);

    scale_x_in *= sys_scale_factor;
    scale_y_in *= sys_scale_factor;

    float out_scale_x = 0.0f;
    float out_scale_y = 0.0f;
    if (scale_unit == 0) {
        rc = driver->set_char_default_dpi(font_obj, scale_x_in, scale_y_in, &out_scale_x,
                                          &out_scale_y);
    } else {
        if (dpi_x == 0) {
            dpi_x = 0x48;
        }
        if (dpi_y == 0) {
            dpi_y = 0x48;
        }
        rc = driver->set_char_with_dpi(font_obj, dpi_x, dpi_y, scale_x_in, scale_y_in, &out_scale_x,
                                       &out_scale_y);
    }
    if (rc != ORBIS_OK) {
        return cleanup_return(ORBIS_FONT_ERROR_NO_SUPPORT_GLYPH);
    }

    u32 glyph_index = 0;
    rc = driver->glyph_index(font_obj, mapped_code, &glyph_index);
    if (rc != ORBIS_OK || glyph_index == 0) {
        return cleanup_return(ORBIS_FONT_ERROR_NO_SUPPORT_GLYPH);
    }

    if (special_case_font_id != 0xffffffffu) {
        ApplyGlyphSpecialCaseAdjust(static_cast<int>(special_case_font_id),
                                    static_cast<int>(glyph_index),
                                    reinterpret_cast<int*>(&font_obj->layout_seed_pair));
    }

    rc = RenderGlyphIndexToSurface(*font_obj, glyph_index, surf, x, y, metrics, result);
    return cleanup_return(rc);
}

s32 RenderGlyphImageCore(Libraries::Font::OrbisFontGlyph fontGlyph,
                         Libraries::Font::OrbisFontStyleFrame* fontStyleFrame,
                         Libraries::Font::OrbisFontRenderer fontRenderer,
                         Libraries::Font::OrbisFontRenderSurface* surface, float x, float y,
                         int mode, Libraries::Font::OrbisFontGlyphMetrics* metrics,
                         Libraries::Font::OrbisFontRenderOutput* result) {
    if (!fontRenderer) {
        ClearRenderOutputs(metrics, result);
        return ORBIS_FONT_ERROR_INVALID_RENDERER;
    }
    if (!surface || !metrics || !result) {
        ClearRenderOutputs(metrics, result);
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    if (!fontGlyph || fontGlyph->magic != 0x0F03) {
        ClearRenderOutputs(metrics, result);
        return ORBIS_FONT_ERROR_INVALID_GLYPH;
    }

    auto* gg = TryGetGeneratedGlyph(fontGlyph);
    if (!gg) {
        ClearRenderOutputs(metrics, result);
        return ORBIS_FONT_ERROR_INVALID_GLYPH;
    }
    auto* st = TryGetState(gg->owner_handle);
    if (!st) {
        ClearRenderOutputs(metrics, result);
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }

    const auto previous_renderer = st->bound_renderer;
    st->bound_renderer = fontRenderer;

    RenderSurfaceSystemUse* sys = GetSurfaceSystemUse(surface);
    const OrbisFontStyleFrame* surface_frame = nullptr;
    if ((surface->styleFlag & 0x1) != 0 && sys && ValidateStyleFramePtr(sys->styleframe)) {
        surface_frame = sys->styleframe;
    }
    const OrbisFontStyleFrame* call_frame =
        ValidateStyleFramePtr(fontStyleFrame) ? fontStyleFrame : nullptr;

    const StyleFrameScaleState style_scale =
        ResolveMergedStyleFrameScale(surface_frame, call_frame, *st);

    float x_used = x;
    float y_used = y;
    const s16 baseline = static_cast<s16>(fontGlyph->baseline);
    char baseline_mode = 0;
    s16 metrics_axis = 1;
    float baseline_x = 0.0f;
    float baseline_y = 0.0f;

    if (mode == 2) {
        metrics_axis = 2;
        baseline_mode = static_cast<char>(-0x56);
        if (baseline < 0) {
            baseline_mode = 0;
        }
    } else if (mode == 1) {
        metrics_axis = 1;
        baseline_mode = static_cast<char>(-0x78);
        if (baseline >= 0) {
            baseline_mode = 0;
        }
    } else {
        baseline_mode = (baseline < 0) ? '`' : '\x06';
        metrics_axis = static_cast<s16>(1 - (baseline >> 15));

        if ((surface->styleFlag & 0x1) != 0 && sys && surface_frame) {
            const float cached = sys->catchedScale;
            if (std::abs(cached) > kScaleEpsilon) {
                if (baseline >= 1) {
                    y_used += cached;
                    baseline_mode = 0;
                } else if (baseline < 0) {
                    x_used += cached;
                    metrics_axis = 2;
                    baseline_mode = 0;
                }
            }
        }
    }

    if (baseline_mode == '`') {
        baseline_x = static_cast<float>(baseline);
    } else if (baseline_mode == '\x06') {
        baseline_y = static_cast<float>(baseline);
    }

    FT_Face face = nullptr;
    if (!ResolveFace(*st, gg->owner_handle, face)) {
        ClearRenderOutputs(metrics, result);
        st->bound_renderer = previous_renderer;
        return ORBIS_FONT_ERROR_NO_SUPPORT_GLYPH;
    }

    float scale_y = gg->scale_y_used;
    if (scale_y <= kScaleEpsilon) {
        const float pixel_h =
            (fontGlyph->base_scale > kScaleEpsilon) ? fontGlyph->base_scale : st->scale_h;
        scale_y = ComputeScaleExtForState(*st, face, pixel_h);
    }
    float scale_x = gg->scale_x_used > kScaleEpsilon ? gg->scale_x_used : scale_y;

    if (style_scale.scale_overridden) {
        float style_scale_y = 0.0f;
        if (ResolveFaceAndScale(*st, gg->owner_handle, style_scale.scale_h, face, style_scale_y)) {
            scale_y = style_scale_y;
            scale_x = (style_scale.scale_h > kScaleEpsilon)
                          ? style_scale_y * (style_scale.scale_w / style_scale.scale_h)
                          : style_scale_y;
        }
    } else if (fontGlyph->base_scale > kScaleEpsilon && fontGlyph->scale_x > kScaleEpsilon) {
        scale_x = scale_y * (fontGlyph->scale_x / fontGlyph->base_scale);
    }

    (void)metrics_axis;
    (void)baseline_x;
    (void)baseline_y;

    const s32 rc =
        RenderCodepointToSurfaceWithScale(*st, gg->owner_handle, face, scale_x, scale_y, surface,
                                          gg->codepoint, x_used, y_used, metrics, result);

    st->bound_renderer = previous_renderer;
    return rc;
}

s32 StyleStateSetScalePixel(void* style_state_block, float w, float h) {
    auto* st = static_cast<StyleStateBlock*>(style_state_block);
    if (!st) {
        return 0;
    }
    if (st->scale_w == w) {
        if (st->scale_h == h && st->scale_unit == 0) {
            return 0;
        }
    }
    st->scale_unit = 0;
    st->scale_w = w;
    st->scale_h = h;
    return 1;
}

s32 StyleStateSetScalePoint(void* style_state_block, float w, float h) {
    auto* st = static_cast<StyleStateBlock*>(style_state_block);
    if (!st) {
        return 0;
    }
    if (st->scale_w == w) {
        if (st->scale_h == h && st->scale_unit == 1) {
            return 0;
        }
    }
    st->scale_unit = 1;
    st->scale_w = w;
    st->scale_h = h;
    return 1;
}

s32 StyleStateGetScalePixel(const void* style_state_block, float* w, float* h) {
    const auto* st = static_cast<const StyleStateBlock*>(style_state_block);
    if (!st || (!w && !h)) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    const u32 flag = st->scale_unit;
    if (w) {
        float out = st->scale_w;
        const u32 dpi = st->dpi_x;
        if (flag != 0 && dpi != 0) {
            out *= static_cast<float>(dpi) / kPointsPerInch;
        }
        *w = out;
        if (!h) {
            return ORBIS_OK;
        }
    }

    float out = st->scale_h;
    const u32 dpi = st->dpi_y;
    if (flag != 0 && dpi != 0) {
        out *= static_cast<float>(dpi) / kPointsPerInch;
    }
    *h = out;
    return ORBIS_OK;
}

s32 StyleStateGetScalePoint(const void* style_state_block, float* w, float* h) {
    const auto* st = static_cast<const StyleStateBlock*>(style_state_block);
    if (!st || (!w && !h)) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    const u32 flag = st->scale_unit;
    if (w) {
        float out = st->scale_w;
        const u32 dpi = st->dpi_x;
        if (flag == 0 && dpi != 0) {
            out *= kPointsPerInch / static_cast<float>(dpi);
        }
        *w = out;
        if (!h) {
            return ORBIS_OK;
        }
    }

    float out = st->scale_h;
    const u32 dpi = st->dpi_y;
    if (flag == 0 && dpi != 0) {
        out *= kPointsPerInch / static_cast<float>(dpi);
    }
    *h = out;
    return ORBIS_OK;
}

s32 StyleStateSetDpi(u32* dpi_pair, u32 h_dpi, u32 v_dpi) {
    if (!dpi_pair) {
        return 0;
    }
    if (h_dpi == 0) {
        h_dpi = 0x48;
    }
    if (v_dpi == 0) {
        v_dpi = 0x48;
    }
    if (dpi_pair[0] == h_dpi && dpi_pair[1] == v_dpi) {
        return 0;
    }
    dpi_pair[0] = h_dpi;
    dpi_pair[1] = v_dpi;
    return 1;
}

s32 StyleStateSetSlantRatio(void* style_state_block, float slantRatio) {
    auto* st = static_cast<StyleStateBlock*>(style_state_block);
    if (!st) {
        return 0;
    }
    const float prev = st->slant_ratio;
    if (prev == slantRatio) {
        return 0;
    }

    float clamped = slantRatio;
    if (clamped > 1.0f) {
        clamped = 1.0f;
    } else if (clamped < -1.0f) {
        clamped = -1.0f;
    }
    st->slant_ratio = clamped;
    return 1;
}

s32 StyleStateGetSlantRatio(const void* style_state_block, float* slantRatio) {
    const auto* st = static_cast<const StyleStateBlock*>(style_state_block);
    if (!st || !slantRatio) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    *slantRatio = st->slant_ratio;
    return ORBIS_OK;
}

s32 StyleStateSetWeightScale(void* style_state_block, float weightXScale, float weightYScale) {
    auto* st = static_cast<StyleStateBlock*>(style_state_block);
    if (!st) {
        return 0;
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

    bool changed = false;
    const float dx = clamp_delta(weightXScale - 1.0f);
    const float prev_x = st->effect_weight_x;
    if (!(prev_x == dx)) {
        st->effect_weight_x = dx;
        changed = true;
    }

    const float dy = clamp_delta(weightYScale - 1.0f);
    const float prev_y = st->effect_weight_y;
    if (!(prev_y == dy)) {
        st->effect_weight_y = dy;
        changed = true;
    }

    return changed ? 1 : 0;
}

s32 StyleStateGetWeightScale(const void* style_state_block, float* weightXScale,
                             float* weightYScale, u32* mode) {
    if ((reinterpret_cast<std::uintptr_t>(weightXScale) |
         reinterpret_cast<std::uintptr_t>(weightYScale) | reinterpret_cast<std::uintptr_t>(mode)) ==
        0) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    const auto* st = static_cast<const StyleStateBlock*>(style_state_block);
    if (!st) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    if (weightXScale) {
        *weightXScale = st->effect_weight_x + 1.0f;
    }
    if (weightYScale) {
        *weightYScale = st->effect_weight_y + 1.0f;
    }
    if (mode) {
        *mode = 0;
    }
    return ORBIS_OK;
}

} // namespace Libraries::Font::Internal

namespace Libraries::FontFt::Internal {

namespace {

static std::once_flag g_driver_table_once;
alignas(Libraries::Font::Internal::SysDriver) static Libraries::Font::Internal::SysDriver
    g_driver_table{};

static std::once_flag g_renderer_table_once;
alignas(Libraries::FontFt::OrbisFontRendererSelection) static Libraries::FontFt::
    OrbisFontRendererSelection g_renderer_table{};

using Libraries::Font::Internal::FontLibOpaque;
using Libraries::Font::Internal::FontObj;

static constexpr float kOneOver64 = 1.0f / 64.0f;

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

static constexpr u32 MakeTag(char a, char b, char c, char d) {
    return (static_cast<u32>(static_cast<u8>(a)) << 24) |
           (static_cast<u32>(static_cast<u8>(b)) << 16) |
           (static_cast<u32>(static_cast<u8>(c)) << 8) | static_cast<u32>(static_cast<u8>(d));
}

static bool ResolveSfntBaseOffset(const u8* data, std::size_t size, u32 subFontIndex,
                                  u32& out_base) {
    out_base = 0;
    if (!data || size < 4) {
        return false;
    }

    const u32 sig = static_cast<const BeU32*>(static_cast<const void*>(data))->value();
    if (sig != MakeTag('t', 't', 'c', 'f')) {
        return true;
    }

    if (size < sizeof(TtcHeader)) {
        return false;
    }

    const auto* header = static_cast<const TtcHeader*>(static_cast<const void*>(data));
    const u32 num_fonts = header->num_fonts.value();
    if (num_fonts == 0 || subFontIndex >= num_fonts) {
        return false;
    }

    const std::size_t offsets_off = 0x0C;
    const std::size_t want_off = offsets_off + static_cast<std::size_t>(subFontIndex) * 4;
    if (want_off + 4 > size) {
        return false;
    }

    const u32 base = static_cast<const BeU32*>(static_cast<const void*>(data + want_off))->value();
    if (base > size || (size - base) < 0x0C) {
        return false;
    }
    out_base = base;
    return true;
}

static bool FindSfntTable(const u8* data, std::size_t size, u32 base, u32 tag, u32& out_off,
                          u32& out_len) {
    out_off = 0;
    out_len = 0;
    if (!data || base > size || (size - base) < 0x0C) {
        return false;
    }

    const u8* sfnt = data + base;
    const std::size_t sfnt_size = size - base;

    if (sfnt_size < sizeof(SfntOffsetTable)) {
        return false;
    }
    const auto* offset_table = static_cast<const SfntOffsetTable*>(static_cast<const void*>(sfnt));
    const u16 num_tables = offset_table->num_tables.value();
    const std::size_t dir_off = sizeof(SfntOffsetTable);
    const std::size_t record_size = sizeof(SfntTableRecord);
    const std::size_t dir_size = dir_off + static_cast<std::size_t>(num_tables) * record_size;
    if (dir_size > sfnt_size) {
        return false;
    }

    for (u16 i = 0; i < num_tables; i++) {
        const auto* rec = static_cast<const SfntTableRecord*>(
            static_cast<const void*>(sfnt + dir_off + static_cast<std::size_t>(i) * record_size));
        const u32 rec_tag = rec->tag.value();
        if (rec_tag != tag) {
            continue;
        }
        const u32 off = rec->offset.value();
        const u32 len = rec->length.value();
        if (off > sfnt_size || len > (sfnt_size - off)) {
            return false;
        }
        out_off = base + off;
        out_len = len;
        return true;
    }
    return false;
}

static bool ReadUnitsPerEm(const u8* data, std::size_t size, u32 base, u16& out_units) {
    out_units = 0;
    u32 head_off = 0;
    u32 head_len = 0;
    if (!FindSfntTable(data, size, base, MakeTag('h', 'e', 'a', 'd'), head_off, head_len)) {
        return false;
    }
    if (head_len < 0x14 || head_off + 0x14 > size) {
        return false;
    }
    out_units =
        static_cast<const BeU16*>(static_cast<const void*>(data + head_off + 0x12))->value();
    return out_units != 0;
}

static std::mutex g_font_obj_sidecars_mutex;
static std::unordered_map<const FontObj*, FontObjSidecar> g_font_obj_sidecars;

static void SetFontObjSidecar(const FontObj* obj, FontObjSidecar sidecar) {
    if (!obj) {
        return;
    }
    std::lock_guard lock(g_font_obj_sidecars_mutex);
    g_font_obj_sidecars.insert_or_assign(obj, std::move(sidecar));
}

static std::optional<FontObjSidecar> TakeFontObjSidecar(const FontObj* obj) {
    if (!obj) {
        return std::nullopt;
    }
    std::lock_guard lock(g_font_obj_sidecars_mutex);
    auto it = g_font_obj_sidecars.find(obj);
    if (it == g_font_obj_sidecars.end()) {
        return std::nullopt;
    }
    FontObjSidecar out = std::move(it->second);
    g_font_obj_sidecars.erase(it);
    return out;
}

static void* FtAlloc(FT_Memory memory, long size) {
    if (!memory || size <= 0) {
        return nullptr;
    }
    auto* ctx = static_cast<FtLibraryCtx*>(memory->user);
    if (!ctx || !ctx->alloc_vtbl) {
        return nullptr;
    }
    const auto alloc_fn = reinterpret_cast<GuestAllocFn>(ctx->alloc_vtbl[0]);
    return alloc_fn ? Core::ExecuteGuest(alloc_fn, ctx->alloc_ctx, static_cast<u32>(size))
                    : nullptr;
}

static void FtFree(FT_Memory memory, void* block) {
    if (!memory || !block) {
        return;
    }
    auto* ctx = static_cast<FtLibraryCtx*>(memory->user);
    if (!ctx || !ctx->alloc_vtbl) {
        return;
    }
    const auto free_fn = reinterpret_cast<GuestFreeFn>(ctx->alloc_vtbl[1]);
    if (free_fn) {
        Core::ExecuteGuest(free_fn, ctx->alloc_ctx, block);
    }
}

static void* FtRealloc(FT_Memory memory, long cur_size, long new_size, void* block) {
    if (!memory) {
        return nullptr;
    }
    auto* ctx = static_cast<FtLibraryCtx*>(memory->user);
    if (!ctx || !ctx->alloc_vtbl) {
        return nullptr;
    }
    const auto realloc_fn = reinterpret_cast<GuestReallocFn>(ctx->alloc_vtbl[2]);
    if (realloc_fn) {
        return Core::ExecuteGuest(realloc_fn, ctx->alloc_ctx, block, static_cast<u32>(new_size));
    }

    if (new_size <= 0) {
        FtFree(memory, block);
        return nullptr;
    }
    void* out = FtAlloc(memory, new_size);
    if (!out) {
        return nullptr;
    }
    if (block && cur_size > 0) {
        std::memcpy(out, block, static_cast<std::size_t>(std::min(cur_size, new_size)));
    }
    FtFree(memory, block);
    return out;
}

} // namespace

const Libraries::FontFt::OrbisFontLibrarySelection* GetDriverTable() {
    std::call_once(g_driver_table_once, [] {
        auto* selection =
            reinterpret_cast<Libraries::FontFt::OrbisFontLibrarySelection*>(&g_driver_table);
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

    return reinterpret_cast<const Libraries::FontFt::OrbisFontLibrarySelection*>(&g_driver_table);
}

const Libraries::FontFt::OrbisFontRendererSelection* GetRendererSelectionTable() {
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

u32 PS4_SYSV_ABI LibraryGetPixelResolutionStub() {
    return 0x40;
}

s32 PS4_SYSV_ABI LibraryInitStub(const void* memory, void* library) {
    if (!memory || !library) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    const auto* mem = static_cast<const Libraries::Font::OrbisFontMem*>(memory);
    if (!mem->iface || !mem->iface->alloc || !mem->iface->dealloc) {
        return ORBIS_FONT_ERROR_INVALID_MEMORY;
    }

    const auto alloc_fn = reinterpret_cast<GuestAllocFn>(mem->iface->alloc);
    const auto free_fn = reinterpret_cast<GuestFreeFn>(mem->iface->dealloc);
    if (!alloc_fn || !free_fn) {
        return ORBIS_FONT_ERROR_INVALID_MEMORY;
    }

    void* alloc_ctx = mem->mspace_handle;
    void** alloc_vtbl =
        reinterpret_cast<void**>(const_cast<Libraries::Font::OrbisFontMemInterface*>(mem->iface));

    auto* ctx =
        static_cast<FtLibraryCtx*>(Core::ExecuteGuest(alloc_fn, alloc_ctx, sizeof(FtLibraryCtx)));
    if (!ctx) {
        return ORBIS_FONT_ERROR_ALLOCATION_FAILED;
    }
    std::memset(ctx, 0, sizeof(FtLibraryCtx));
    ctx->alloc_ctx = alloc_ctx;
    ctx->alloc_vtbl = alloc_vtbl;

    FT_Memory ft_mem =
        static_cast<FT_Memory>(Core::ExecuteGuest(alloc_fn, alloc_ctx, sizeof(FT_MemoryRec_)));
    if (!ft_mem) {
        Core::ExecuteGuest(free_fn, alloc_ctx, ctx);
        return ORBIS_FONT_ERROR_ALLOCATION_FAILED;
    }
    std::memset(ft_mem, 0, sizeof(*ft_mem));
    ft_mem->user = ctx;
    ft_mem->alloc = &FtAlloc;
    ft_mem->free = &FtFree;
    ft_mem->realloc = &FtRealloc;
    ctx->ft_memory = ft_mem;

    FT_Library ft_lib = nullptr;
    const FT_Error ft_err = FT_New_Library(ft_mem, &ft_lib);
    if (ft_err != 0 || !ft_lib) {
        Core::ExecuteGuest(free_fn, alloc_ctx, ft_mem);
        Core::ExecuteGuest(free_fn, alloc_ctx, ctx);
        return ORBIS_FONT_ERROR_ALLOCATION_FAILED;
    }
    FT_Add_Default_Modules(ft_lib);
    ctx->ft_lib = ft_lib;

    auto* lib = static_cast<FontLibOpaque*>(library);
    lib->fontset_registry = ctx;
    lib->flags = 0x60000000;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI FtRendererCreate(void* renderer) {
    if (!renderer) {
        return ORBIS_FONT_ERROR_INVALID_RENDERER;
    }

    auto* r = static_cast<Libraries::Font::Internal::RendererFtOpaque*>(renderer);
    r->ft_backend.renderer_header_0x10 = static_cast<void*>(&r->base.mem_kind);
    r->ft_backend.unknown_0x08 = 0;
    r->ft_backend.unknown_0x10 = 0;
    r->ft_backend.unknown_0x18 = 0;
    r->ft_backend.initialized_marker = r;
    return ORBIS_OK;
}

u64 PS4_SYSV_ABI FtRendererQuery(void* /*renderer*/, u8* /*params*/, s64* out_ptr,
                                 u8 (*out_vec)[16]) {
    if (out_vec) {
        std::memset(out_vec, 0, sizeof(*out_vec));
    }
    if (out_ptr) {
        *out_ptr = 0;
    }
    return 0;
}

s32 PS4_SYSV_ABI FtRendererDestroy(void* renderer) {
    if (!renderer) {
        return ORBIS_FONT_ERROR_INVALID_RENDERER;
    }
    auto* r = static_cast<Libraries::Font::Internal::RendererFtOpaque*>(renderer);
    r->ft_backend.initialized_marker = nullptr;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI LibraryTermStub(void* library) {
    if (!library) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    auto* lib = static_cast<FontLibOpaque*>(library);
    auto* alloc_ctx = lib->alloc_ctx;
    auto* alloc_vtbl = lib->alloc_vtbl;
    const auto free_fn = alloc_vtbl ? reinterpret_cast<GuestFreeFn>(alloc_vtbl[1]) : nullptr;
    if (!free_fn) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    auto* ctx = static_cast<FtLibraryCtx*>(lib->fontset_registry);
    if (!ctx) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    if (ctx->ft_lib) {
        FT_Done_Library(ctx->ft_lib);
        ctx->ft_lib = nullptr;
    }
    if (ctx->ft_memory) {
        Core::ExecuteGuest(free_fn, alloc_ctx, ctx->ft_memory);
        ctx->ft_memory = nullptr;
    }
    Core::ExecuteGuest(free_fn, alloc_ctx, ctx);
    lib->fontset_registry = nullptr;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI LibrarySupportStub(void* library, u32 formats) {
    if (!library) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto* lib = static_cast<FontLibOpaque*>(library);
    if (!lib->fontset_registry) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    u32 mapped = formats * 2u & 0x100u;
    const u32 inv_formats = ~formats;
    u32 tmp = mapped + 0x4CC0u;
    if ((inv_formats & 0x52u) != 0) {
        tmp = mapped;
    }
    mapped = tmp | 0x4C40u;
    if ((inv_formats & 0x42u) != 0) {
        mapped = tmp;
    }
    tmp = mapped | 0x4C80u;
    if ((inv_formats & 0x50u) != 0) {
        tmp = mapped;
    }
    mapped = tmp | 0x0C80u;
    if ((formats & 0x10u) == 0) {
        mapped = tmp;
    }
    tmp = mapped | 0x8C80u;
    if ((inv_formats & 0x30u) != 0) {
        tmp = mapped;
    }
    mapped = tmp | 0x1820u;
    if ((formats & 8u) == 0) {
        mapped = tmp;
    }
    tmp = mapped | 0x1808u;
    if ((formats & 1u) == 0) {
        tmp = mapped;
    }
    mapped = tmp | 0x1C90u;
    if ((inv_formats & 0x14u) != 0) {
        mapped = tmp;
    }
    (void)mapped;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI LibraryOpenFontMemoryStub(void* library, u32 mode, const void* fontAddress,
                                           u32 fontSize, u32 subFontIndex, u32 /*uniqueWord*/,
                                           void** inoutFontObj) {
    if (!library || !fontAddress || !inoutFontObj) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    auto* lib = static_cast<FontLibOpaque*>(library);
    void* alloc_ctx = lib->alloc_ctx;
    void** alloc_vtbl = lib->alloc_vtbl;
    const auto alloc_fn = alloc_vtbl ? reinterpret_cast<GuestAllocFn>(alloc_vtbl[0]) : nullptr;
    const auto free_fn = alloc_vtbl ? reinterpret_cast<GuestFreeFn>(alloc_vtbl[1]) : nullptr;
    if (!alloc_fn || !free_fn) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    const u8* data = nullptr;
    u32 size = 0;
    void* owned_data = nullptr;
    std::string open_path;
    std::filesystem::path host_path_fs{};

    if (mode == 1) {
        if (fontSize == 0) {
            return ORBIS_FONT_ERROR_INVALID_PARAMETER;
        }
        data = static_cast<const u8*>(fontAddress);
        size = fontSize;
    } else if (mode == 5 || mode == 6 || mode == 7) {
        const char* path = static_cast<const char*>(fontAddress);
        if (!path || path[0] == '\0') {
            return ORBIS_FONT_ERROR_INVALID_PARAMETER;
        }

        open_path = path;
        if (path[0] == '/') {
            auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
            host_path_fs = mnt ? mnt->GetHostPath(path) : std::filesystem::path{};
            if (!host_path_fs.empty()) {
                open_path = host_path_fs.string();
            }
        }
    } else {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    auto* ctx = static_cast<FtLibraryCtx*>(lib->fontset_registry);
    if (!ctx || !ctx->ft_lib) {
        if (owned_data) {
            Core::ExecuteGuest(free_fn, alloc_ctx, owned_data);
        }
        return ORBIS_FONT_ERROR_INVALID_LIBRARY;
    }

    FT_Face face = nullptr;
    FT_Error ft_err = 0;
    if (mode == 1) {
        ft_err = FT_New_Memory_Face(ctx->ft_lib, reinterpret_cast<const FT_Byte*>(data),
                                    static_cast<FT_Long>(size), static_cast<FT_Long>(subFontIndex),
                                    &face);
    } else {
        std::vector<std::string> candidates;
        candidates.emplace_back(open_path);

        if (!host_path_fs.empty()) {
            const auto sysfonts_dir = host_path_fs.parent_path();
            const auto ps4_name = host_path_fs.filename().string();
            const std::filesystem::path file_path{ps4_name};
            candidates.emplace_back((sysfonts_dir / "font" / file_path).string());
            candidates.emplace_back((sysfonts_dir / "font2" / file_path).string());
            if (const auto alias = ResolveKnownSysFontAlias(sysfonts_dir, ps4_name)) {
                candidates.emplace_back(alias->string());
            }
        }

        std::error_code ec;
        FT_Error last_ft_err = 0;
        bool attempted_open = false;
        for (const auto& cand : candidates) {
            if (cand.empty()) {
                continue;
            }
            if (!std::filesystem::exists(std::filesystem::path{cand}, ec) || ec) {
                continue;
            }
            attempted_open = true;
            ft_err =
                FT_New_Face(ctx->ft_lib, cand.c_str(), static_cast<FT_Long>(subFontIndex), &face);
            last_ft_err = ft_err;
            if (ft_err == 0 && face) {
                break;
            }
        }
        if (ft_err != 0) {
            ft_err = last_ft_err;
        } else if (!attempted_open) {
            ft_err = FT_Err_Cannot_Open_Resource;
        }
    }
    if (ft_err != 0 || !face) {
        if (owned_data) {
            Core::ExecuteGuest(free_fn, alloc_ctx, owned_data);
        }
        if (mode == 1) {
            return ORBIS_FONT_ERROR_NO_SUPPORT_FORMAT;
        }
        if (ft_err == FT_Err_Unknown_File_Format) {
            return ORBIS_FONT_ERROR_NO_SUPPORT_FORMAT;
        }
        return ORBIS_FONT_ERROR_FS_OPEN_FAILED;
    }

    (void)FT_Select_Charmap(face, FT_ENCODING_UNICODE);

    auto* obj = static_cast<FontObj*>(Core::ExecuteGuest(alloc_fn, alloc_ctx, sizeof(FontObj)));
    if (!obj) {
        FT_Done_Face(face);
        if (owned_data) {
            Core::ExecuteGuest(free_fn, alloc_ctx, owned_data);
        }
        return ORBIS_FONT_ERROR_ALLOCATION_FAILED;
    }

    std::memset(obj, 0, sizeof(FontObj));
    obj->refcount = 1;
    obj->sub_font_index = subFontIndex;
    obj->prev = nullptr;
    obj->next = static_cast<FontObj*>(*inoutFontObj);
    if (obj->next) {
        obj->next->prev = obj;
    }
    obj->open_ctx_0x28 = nullptr;
    obj->ft_face = face;
    obj->font_handle = nullptr;
    obj->shift_units_x = 0;
    obj->shift_units_y = 0;
    obj->layout_seed_pair = 0;
    obj->ft_ctx_0x58 = &ctx->ft_lib;

    FontObjSidecar sidecar{};
    sidecar.font_data = data;
    sidecar.font_size = size;
    sidecar.owned_data = owned_data;
    if (mode == 1) {
        (void)ResolveSfntBaseOffset(data, size, subFontIndex, sidecar.sfnt_base);
    }
    SetFontObjSidecar(obj, std::move(sidecar));

    *inoutFontObj = obj;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI LibraryCloseFontObjStub(void* fontObj, u32 /*flags*/) {
    if (!fontObj) {
        return ORBIS_FONT_ERROR_FATAL;
    }

    auto* obj = static_cast<FontObj*>(fontObj);
    if (obj->refcount > 1) {
        obj->refcount--;
        return ORBIS_OK;
    }

    FT_Face face = static_cast<FT_Face>(obj->ft_face);
    FtLibraryCtx* ctx = nullptr;
    if (face && face->memory) {
        ctx = static_cast<FtLibraryCtx*>(face->memory->user);
    }

    const auto free_fn =
        (ctx && ctx->alloc_vtbl) ? reinterpret_cast<GuestFreeFn>(ctx->alloc_vtbl[1]) : nullptr;
    void* owned_data = nullptr;
    if (const auto sidecar = TakeFontObjSidecar(obj)) {
        owned_data = sidecar->owned_data;
    }

    if (face) {
        FT_Done_Face(face);
        obj->ft_face = nullptr;
    }
    if (owned_data && free_fn) {
        Core::ExecuteGuest(free_fn, ctx->alloc_ctx, owned_data);
    }
    if (free_fn) {
        FontObj* next = obj->next;
        if (obj->prev == nullptr) {
            if (next != nullptr) {
                next->prev = nullptr;
            }
        } else {
            obj->prev->next = next;
        }
        Core::ExecuteGuest(free_fn, ctx->alloc_ctx, obj);
        return ORBIS_OK;
    }
    return ORBIS_FONT_ERROR_FATAL;
}

s32 PS4_SYSV_ABI LibraryGetFaceScaleStub(void* fontObj, u16* outUnitsPerEm, float* outScale) {
    if (!fontObj || !outUnitsPerEm || !outScale) {
        return ORBIS_FONT_ERROR_FATAL;
    }
    auto* obj = static_cast<FontObj*>(fontObj);
    const auto* face = static_cast<const FT_Face>(obj->ft_face);
    if (!face) {
        return ORBIS_FONT_ERROR_FATAL;
    }
    const u16 units = face->units_per_EM;
    *outUnitsPerEm = units;
    *outScale = static_cast<float>(static_cast<u32>(units)) * kOneOver64;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI LibraryGetFaceMetricStub(void* fontObj, u32 metricId, u16* outMetric) {
    if (!fontObj || !outMetric) {
        return ORBIS_FONT_ERROR_FATAL;
    }
    auto* obj = static_cast<FontObj*>(fontObj);
    const auto* face = static_cast<const FT_Face>(obj->ft_face);
    if (!face) {
        return ORBIS_FONT_ERROR_FATAL;
    }

    const u16 units = face->units_per_EM;
    if (metricId == 0x0e00) {
        *outMetric = units;
        return ORBIS_OK;
    }
    if (metricId == 0xea00) {
        const TT_OS2* os2 =
            static_cast<const TT_OS2*>(FT_Get_Sfnt_Table(const_cast<FT_Face>(face), ft_sfnt_os2));
        if (os2) {
            *outMetric = static_cast<u16>(os2->sTypoAscender);
            return ORBIS_OK;
        }
        *outMetric = units;
        return ORBIS_FONT_ERROR_NO_SUPPORT_FUNCTION;
    }

    *outMetric = 0;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI LibraryGetGlyphIndexStub(void* fontObj, u32 codepoint_u16, u32* out_glyph_index) {
    if (!fontObj || !out_glyph_index) {
        return ORBIS_FONT_ERROR_FATAL;
    }
    auto* obj = static_cast<FontObj*>(fontObj);
    const FT_Face face = static_cast<FT_Face>(obj->ft_face);
    if (!face) {
        *out_glyph_index = 0;
        return ORBIS_FONT_ERROR_FATAL;
    }

    const auto glyph_index =
        static_cast<u32>(FT_Get_Char_Index(face, static_cast<FT_ULong>(codepoint_u16)));
    *out_glyph_index = glyph_index;
    return glyph_index ? ORBIS_OK : ORBIS_FONT_ERROR_NO_SUPPORT_GLYPH;
}

s32 PS4_SYSV_ABI LibrarySetCharSizeWithDpiStub(void* fontObj, u32 dpi_x, u32 dpi_y, float scale_x,
                                               float scale_y, float* out_scale_x,
                                               float* out_scale_y) {
    if (!fontObj || !out_scale_x || !out_scale_y) {
        return ORBIS_FONT_ERROR_FATAL;
    }
    auto* obj = static_cast<FontObj*>(fontObj);
    const FT_Face face = static_cast<FT_Face>(obj->ft_face);
    if (!face) {
        return ORBIS_FONT_ERROR_FATAL;
    }

    const auto char_w = static_cast<FT_F26Dot6>(static_cast<s32>(scale_x * 64.0f));
    const auto char_h = static_cast<FT_F26Dot6>(static_cast<s32>(scale_y * 64.0f));
    if (FT_Set_Char_Size(face, char_w, char_h, dpi_x, dpi_y) != 0) {
        return ORBIS_FONT_ERROR_FATAL;
    }

    const FT_Size size = face->size;
    if (!size) {
        return ORBIS_FONT_ERROR_FATAL;
    }

    const u16 units = face->units_per_EM;
    const long x_scale = static_cast<long>(size->metrics.x_scale);
    const long y_scale = static_cast<long>(size->metrics.y_scale);

    auto fixed_mul_units_to_f26dot6 = [](long fixed_16_16, u16 units_per_em) -> float {
        const long prod = static_cast<long>(static_cast<long long>(fixed_16_16) *
                                            static_cast<long long>(units_per_em));
        long rounded = prod + 0xFFFF;
        if (prod >= 0) {
            rounded = prod;
        }
        const long v = rounded >> 16;
        return static_cast<float>(v) * kOneOver64;
    };

    *out_scale_x = fixed_mul_units_to_f26dot6(x_scale, units);
    *out_scale_y = fixed_mul_units_to_f26dot6(y_scale, units);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI LibrarySetCharSizeDefaultDpiStub(void* fontObj, float scale_x, float scale_y,
                                                  float* out_scale_x, float* out_scale_y) {
    if (!fontObj || !out_scale_x || !out_scale_y) {
        return ORBIS_FONT_ERROR_FATAL;
    }
    return LibrarySetCharSizeWithDpiStub(fontObj, 0x48, 0x48, scale_x, scale_y, out_scale_x,
                                         out_scale_y);
}

s32 PS4_SYSV_ABI LibraryComputeLayoutBlockStub(void* fontObj, const void* style_state_block,
                                               u8 (*out_words)[16]) {
    if (!fontObj || !style_state_block || !out_words) {
        return ORBIS_FONT_ERROR_FATAL;
    }
    auto* obj = static_cast<FontObj*>(fontObj);
    const FT_Face face = static_cast<FT_Face>(obj->ft_face);
    if (!face || !face->size) {
        return ORBIS_FONT_ERROR_FATAL;
    }

    const auto* style =
        static_cast<const Libraries::Font::Internal::StyleStateBlock*>(style_state_block);
    const float effect_width = style->effect_weight_x;
    const float effect_height = style->effect_weight_y;
    const float slant = style->slant_ratio;

    const s64 y_scale = static_cast<s64>(face->size->metrics.y_scale);
    const s64 x_scale = static_cast<s64>(face->size->metrics.x_scale);
    const s64 y_shift = obj->shift_cache_y;
    const s64 x_shift = obj->shift_cache_x;
    const s64 units_per_em = static_cast<s64>(static_cast<u16>(face->units_per_EM));

    auto cvttss2si = [](float v) -> s32 {
        if (!std::isfinite(v) || v > 2147483647.0f || v < -2147483648.0f) {
            return std::numeric_limits<s32>::min();
        }
        return static_cast<s32>(v);
    };

    auto round_mul_16_16 = [](s64 value, s64 fixed_16_16) -> s32 {
        const s64 prod = value * fixed_16_16;
        const s64 sign_adj = (value >= 0) ? 0x10000LL : 0LL;
        s64 tmp = sign_adj + prod - 0x8000LL;
        if (tmp < 0) {
            tmp = sign_adj + prod + 0x7FFFLL;
        }
        return static_cast<s32>(static_cast<u64>(tmp) >> 16);
    };

    auto trunc_fixed_16_16_to_int = [](s64 fixed_16_16) -> s32 {
        s64 tmp = fixed_16_16;
        if (fixed_16_16 < 0) {
            tmp = fixed_16_16 + 0xFFFFLL;
        }
        return static_cast<s32>(tmp >> 16);
    };

    s32 y_min_px = round_mul_16_16(static_cast<s64>(face->bbox.yMin) + y_shift, y_scale);
    s32 y_max_px = round_mul_16_16(static_cast<s64>(face->bbox.yMax) + y_shift, y_scale);

    s32 half_effect_w_px = 0;
    s32 left_adjust_px = 0;
    if (effect_width != 0.0f) {
        const s32 units_scaled_x = trunc_fixed_16_16_to_int(units_per_em * x_scale);
        half_effect_w_px = cvttss2si(effect_width * static_cast<float>(units_scaled_x)) / 2;
        left_adjust_px = -half_effect_w_px;
    }

    s32 half_effect_h_px = 0;
    float out_effect_h = 0.0f;
    if (effect_height != 0.0f) {
        const s32 units_scaled_y = trunc_fixed_16_16_to_int(units_per_em * y_scale);
        half_effect_h_px = cvttss2si(effect_height * static_cast<float>(units_scaled_y)) / 2;
        out_effect_h = static_cast<float>(half_effect_h_px) * kOneOver64;
        y_min_px -= half_effect_h_px;
        y_max_px += half_effect_h_px;
    }

    if (slant != 0.0f) {
        const s64 shear_16_16 = static_cast<s64>(cvttss2si(slant * 65536.0f));
        left_adjust_px += round_mul_16_16(static_cast<s64>(y_min_px), shear_16_16);
        half_effect_w_px += round_mul_16_16(static_cast<s64>(y_max_px), shear_16_16);
    }

    auto out = LayoutOutIo{out_words}.fields();
    out.effect_height = out_effect_h;
    out.left_adjust = static_cast<float>(left_adjust_px) * kOneOver64;
    out.half_effect_width = static_cast<float>(half_effect_w_px) * kOneOver64;

    const s32 x_min_px = round_mul_16_16(static_cast<s64>(face->bbox.xMin) + x_shift, x_scale);
    const s32 x_max_px = round_mul_16_16(static_cast<s64>(face->bbox.xMax) + x_shift, x_scale);

    out.line_advance = static_cast<float>(y_max_px - y_min_px) * kOneOver64;
    out.baseline = static_cast<float>(y_max_px) * kOneOver64;
    out.x_bound_0 = static_cast<float>(x_min_px + left_adjust_px) * kOneOver64;
    out.x_bound_1 = static_cast<float>(x_max_px + half_effect_w_px) * kOneOver64;

    const s64 max_adv_w_units = static_cast<s64>(face->max_advance_width) + x_shift;
    const s32 max_adv_w_px = round_mul_16_16(max_adv_w_units, x_scale);
    out.max_advance_width = static_cast<float>(max_adv_w_px) * kOneOver64;

    float hhea_out = 0.0f;
    if (const TT_HoriHeader* hhea =
            static_cast<const TT_HoriHeader*>(FT_Get_Sfnt_Table(face, ft_sfnt_hhea))) {
        const s64 caret_rise_units = x_shift + static_cast<s64>(hhea->caret_Slope_Rise);
        const s32 caret_rise_px = trunc_fixed_16_16_to_int(caret_rise_units * x_scale);
        hhea_out = static_cast<float>(caret_rise_px - half_effect_w_px) * kOneOver64;
    }
    out.hhea_caret_rise_adjust = hhea_out;

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI LibraryComputeLayoutAltBlockStub(void* fontObj, const void* style_state_block,
                                                  u8 (*out_words)[16]) {
    if (!fontObj || !style_state_block || !out_words) {
        return ORBIS_FONT_ERROR_FATAL;
    }
    auto* obj = static_cast<FontObj*>(fontObj);
    const FT_Face face = static_cast<FT_Face>(obj->ft_face);
    if (!face || !face->size) {
        return ORBIS_FONT_ERROR_FATAL;
    }

    const auto* style =
        static_cast<const Libraries::Font::Internal::StyleStateBlock*>(style_state_block);
    const float p18 = style->effect_weight_x;
    const float p1c = style->effect_weight_y;
    const float p20 = style->slant_ratio;

    const long x_scale = static_cast<long>(face->size->metrics.x_scale);
    const long y_scale = static_cast<long>(face->size->metrics.y_scale);

    auto round_fixed_mul = [](long value, long fixed_16_16) -> s32 {
        const long long prod = static_cast<long long>(value) * static_cast<long long>(fixed_16_16);
        const long long sign_adj = (value >= 0) ? 0x10000LL : 0LL;
        long long tmp = sign_adj + prod - 0x8000LL;
        if (tmp < 0) {
            tmp = sign_adj + prod + 0x7FFFLL;
        }
        return static_cast<s32>(static_cast<u64>(tmp) >> 16);
    };

    const auto* vhea = static_cast<const TT_VertHeader*>(FT_Get_Sfnt_Table(face, ft_sfnt_vhea));
    const long y_shift = static_cast<long>(obj->shift_cache_y);
    const long x_shift = static_cast<long>(obj->shift_cache_x);
    const long units = static_cast<long>(static_cast<u16>(face->units_per_EM));

    long y_ascender = 0;
    long y_descender = 0;
    if (vhea) {
        y_ascender = static_cast<long>(vhea->Ascender) + y_shift;
        y_descender = static_cast<long>(vhea->Descender) + y_shift;
    }

    const long left_in = static_cast<long>(face->bbox.xMin) + x_shift;
    const long right_in = static_cast<long>(face->bbox.xMax) + x_shift;

    const s32 scaled_left = round_fixed_mul(left_in, x_scale);
    const s32 scaled_right = round_fixed_mul(right_in, x_scale);

    s32 x_min_scaled = scaled_left;
    s32 x_max_scaled = scaled_right;
    s32 x_abs_max = -x_min_scaled;
    if (x_abs_max < x_max_scaled) {
        x_abs_max = x_max_scaled;
        x_min_scaled = -x_max_scaled;
    }

    float out_effect_width = 0.0f;
    if (p18 != 0.0f) {
        const long long prod = static_cast<long long>(x_scale) * static_cast<long long>(units);
        long long rounded = prod + 0xFFFFLL;
        if (prod >= 0) {
            rounded = prod;
        }
        const s32 units_scaled = static_cast<s32>(rounded >> 16);
        const s32 effect = static_cast<s32>(std::trunc(p18 * static_cast<float>(units_scaled))) / 2;
        out_effect_width = static_cast<float>(effect) * kOneOver64;
    }

    if (p1c != 0.0f) {
        const long long prod = static_cast<long long>(y_scale) * static_cast<long long>(units);
        long long rounded = prod + 0xFFFFLL;
        if (prod >= 0) {
            rounded = prod;
        }
        const s32 units_scaled = static_cast<s32>(rounded >> 16);
        const s32 delta = static_cast<s32>(std::trunc(p1c * static_cast<float>(units_scaled))) / 2;
        y_ascender -= delta;
        y_descender += delta;
    }

    float out_slant_a = 0.0f;
    float out_slant_b = 0.0f;
    if (p20 != 0.0f) {
        const long shear = static_cast<long>(static_cast<s32>(std::trunc(p20 * 65536.0f)));
        const long long sign_adj_min = (y_ascender < 1) ? 0x10000LL : 0LL;
        long long tmp_min = sign_adj_min +
                            static_cast<long long>(-y_ascender) * static_cast<long long>(shear) -
                            0x8000LL;
        if (tmp_min < 0) {
            tmp_min = sign_adj_min + 0x7FFFLL +
                      static_cast<long long>(-y_ascender) * static_cast<long long>(shear);
        }
        const long long sign_adj_max = (y_descender >= 0) ? 0x10000LL : 0LL;
        long long tmp_max = sign_adj_max +
                            static_cast<long long>(y_descender) * static_cast<long long>(shear) -
                            0x8000LL;
        if (tmp_max < 0) {
            tmp_max = sign_adj_max + 0x7FFFLL +
                      static_cast<long long>(y_descender) * static_cast<long long>(shear);
        }

        const u64 tmp_max_u64 = static_cast<u64>(tmp_max);
        const u64 tmp_min_u64 = static_cast<u64>(tmp_min);
        const u64 tmp_max_hi = tmp_max_u64 >> 16;
        const bool max_round_bit_clear = ((tmp_max_u64 >> 15) & 1u) == 0;
        u64 selected_hi = tmp_max_hi;
        s32 selected_lo = static_cast<s32>(tmp_min_u64 >> 16);
        if (max_round_bit_clear && tmp_max_hi != 0) {
            selected_hi = tmp_min_u64 >> 16;
            selected_lo = static_cast<s32>(tmp_max_u64 >> 16);
        }
        out_slant_a = static_cast<float>(selected_lo) * kOneOver64;
        out_slant_b = static_cast<float>(static_cast<s32>(selected_hi)) * kOneOver64;
    }

    const s32 lane0 = x_abs_max - x_min_scaled;
    const s32 lane1 = -x_abs_max;
    const s32 lane2 = round_fixed_mul(y_ascender, x_scale);
    const s32 lane3 = round_fixed_mul(y_descender, x_scale);

    auto out = LayoutAltOutIo{out_words}.fields();
    out.metrics_0x00 = static_cast<float>(lane0) * kOneOver64;
    out.metrics_0x04 = static_cast<float>(lane1) * kOneOver64;
    out.metrics_0x08 = static_cast<float>(lane2) * kOneOver64;
    out.metrics_0x0C = static_cast<float>(lane3) * kOneOver64;

    const s32 adv_h_scaled =
        round_fixed_mul(static_cast<long>(face->max_advance_height) + y_shift, y_scale);
    out.adv_height = static_cast<float>(adv_h_scaled) * kOneOver64;
    out.effect_width = out_effect_width;
    out.slant_b = out_slant_b;
    out.slant_a = out_slant_a;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI LibraryLoadGlyphCachedStub(void* fontObj, u32 glyphIndex, s32 mode,
                                            std::uint64_t* out_words) {
    if (out_words) {
        out_words[0] = 0;
        out_words[1] = 0;
    }
    if (!fontObj || !out_words) {
        return ORBIS_FONT_ERROR_FATAL;
    }

    auto* obj = static_cast<Libraries::Font::Internal::FontObj*>(fontObj);
    FT_Face face = static_cast<FT_Face>(obj->ft_face);
    if (!face) {
        return ORBIS_FONT_ERROR_FATAL;
    }

    FT_GlyphSlot slot = face->glyph;
    if (!slot) {
        return ORBIS_FONT_ERROR_FATAL;
    }

    const u16 units_per_em = static_cast<u16>(face->units_per_EM);
    const bool cached_match = (obj->cached_glyph_index_0x64 == static_cast<s32>(glyphIndex)) &&
                              (obj->cached_units_x_0x68 == static_cast<u64>(units_per_em)) &&
                              (obj->cached_units_y_0x70 == obj->cached_units_x_0x68) && (mode == 0);

    auto write_vec88_from_seed_pair_unscaled = [&]() {
        const s32 seed_low =
            static_cast<s32>(static_cast<u32>(obj->layout_seed_pair & 0xFFFFFFFFu));
        const s32 seed_high =
            static_cast<s32>(static_cast<u32>((obj->layout_seed_pair >> 32) & 0xFFFFFFFFu));
        obj->layout_seed_vec[0] = static_cast<u64>(static_cast<s64>(seed_low));
        obj->layout_seed_vec[1] = static_cast<u64>(static_cast<s64>(seed_high));
    };

    auto write_out_words_from_slot = [&]() {
        const s32 n_contours = static_cast<s32>(slot->outline.n_contours);
        const s32 n_points = static_cast<s32>(slot->outline.n_points);

        const u32 computed_4 = static_cast<u32>(n_points * 0x10 + 0x68 +
                                                ((n_points + 0x0F + n_contours * 2) & 0xFFFFFFF0u));
        const u32 computed_8 = static_cast<u32>(((n_points + 0x2B + n_contours * 2) & 0xFFFFFFFCu) +
                                                0x10 + n_points * 4);

        auto* out_u8 = reinterpret_cast<std::uint8_t*>(out_words);
        *reinterpret_cast<u16*>(out_u8 + 0) = static_cast<u16>(n_contours);
        *reinterpret_cast<u16*>(out_u8 + 2) = static_cast<u16>(n_points);
        *reinterpret_cast<u32*>(out_u8 + 4) = computed_4;
        *reinterpret_cast<u32*>(out_u8 + 8) = computed_8;
    };

    if (cached_match) {
        obj->shift_cache_x = static_cast<s64>(obj->shift_units_x);
        obj->shift_cache_y = static_cast<s64>(obj->shift_units_y);
        write_vec88_from_seed_pair_unscaled();
        write_out_words_from_slot();
        return ORBIS_OK;
    }

    const FT_Int32 load_flags = static_cast<FT_Int32>(((mode == 0) ? 1 : 0) | 0x1A);
    const FT_Error ft_err = FT_Load_Glyph(face, static_cast<FT_UInt>(glyphIndex), load_flags);
    if (ft_err != 0) {
        obj->cached_glyph_index_0x64 = 0;
        obj->cached_units_x_0x68 = 0;
        obj->cached_units_y_0x70 = 0;
        obj->layout_seed_vec = {};
        obj->layout_scale_vec = {};
        return (ft_err == 0x40) ? ORBIS_FONT_ERROR_ALLOCATION_FAILED
                                : ORBIS_FONT_ERROR_NO_SUPPORT_GLYPH;
    }

    slot = face->glyph;
    if (!slot) {
        return ORBIS_FONT_ERROR_FATAL;
    }

    obj->cached_glyph_index_0x64 = static_cast<s32>(glyphIndex);

    if (mode == 0) {
        obj->cached_units_x_0x68 = static_cast<u64>(units_per_em);
        obj->cached_units_y_0x70 = static_cast<u64>(units_per_em);

        obj->shift_cache_x = static_cast<s64>(obj->shift_units_x);
        obj->shift_cache_y = static_cast<s64>(obj->shift_units_y);
        write_vec88_from_seed_pair_unscaled();
        obj->layout_scale_vec[0] = 0x0000000000010000ull;
        obj->layout_scale_vec[1] = 0x0000000000010000ull;
    } else {
        if (!face->size) {
            return ORBIS_FONT_ERROR_FATAL;
        }
        const long x_scale = static_cast<long>(face->size->metrics.x_scale);
        const long y_scale = static_cast<long>(face->size->metrics.y_scale);

        const auto trunc_mul_units = [](long fixed_16_16, u16 units) -> s64 {
            const s64 prod = static_cast<s64>(fixed_16_16) * static_cast<s64>(units);
            s64 rounded = prod;
            if (prod < 0) {
                rounded = prod + 0xFFFFLL;
            }
            return rounded >> 16;
        };

        const auto round_fixed_mul = [](long fixed_16_16, s32 value) -> s64 {
            const long long prod =
                static_cast<long long>(fixed_16_16) * static_cast<long long>(value);
            const long long sign_adj =
                (static_cast<long long>(~static_cast<long long>(value)) >> 63) * -0x10000LL;
            const long long base = sign_adj + prod;
            long long tmp = base - 0x8000LL;
            if (tmp < 0) {
                tmp = base + 0x7FFFLL;
            }
            return static_cast<s64>(static_cast<s32>(static_cast<u64>(tmp) >> 16));
        };

        obj->cached_units_x_0x68 = static_cast<u64>(trunc_mul_units(x_scale, units_per_em));
        obj->cached_units_y_0x70 = static_cast<u64>(trunc_mul_units(y_scale, units_per_em));
        obj->shift_cache_x = round_fixed_mul(x_scale, obj->shift_units_x);
        obj->shift_cache_y = round_fixed_mul(y_scale, obj->shift_units_y);

        const s32 seed_low =
            static_cast<s32>(static_cast<u32>(obj->layout_seed_pair & 0xFFFFFFFFu));
        const s32 seed_high =
            static_cast<s32>(static_cast<u32>((obj->layout_seed_pair >> 32) & 0xFFFFFFFFu));
        const s64 v0 = round_fixed_mul(x_scale, seed_low);
        const s64 v1 = round_fixed_mul(y_scale, seed_high);
        obj->layout_seed_vec[0] = static_cast<u64>(v0);
        obj->layout_seed_vec[1] = static_cast<u64>(v1);

        u64 lane0 = obj->layout_scale_vec[0];
        u64 lane1 = obj->layout_scale_vec[1];
        std::memcpy(&lane0, &x_scale, sizeof(x_scale));
        std::memcpy(&lane1, &y_scale, sizeof(y_scale));
        obj->layout_scale_vec[0] = lane0;
        obj->layout_scale_vec[1] = lane1;
    }

    write_out_words_from_slot();
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI LibraryGetGlyphMetricsStub(void* fontObj, std::uint32_t* /*opt_param2*/,
                                            std::uint8_t /*mode*/, std::uint8_t* out_params,
                                            Libraries::Font::OrbisFontGlyphMetrics* out_metrics) {
    if (!fontObj || !out_params || !out_metrics) {
        return ORBIS_FONT_ERROR_FATAL;
    }

    auto* obj = static_cast<Libraries::Font::Internal::FontObj*>(fontObj);
    FT_Face face = static_cast<FT_Face>(obj->ft_face);
    if (!face || !face->glyph) {
        return ORBIS_FONT_ERROR_FATAL;
    }

    const FT_GlyphSlot slot = face->glyph;
    out_metrics->width = static_cast<float>(slot->metrics.width) * kOneOver64;
    out_metrics->height = static_cast<float>(slot->metrics.height) * kOneOver64;
    out_metrics->Horizontal.bearingX = static_cast<float>(slot->metrics.horiBearingX) * kOneOver64;
    out_metrics->Horizontal.bearingY = static_cast<float>(slot->metrics.horiBearingY) * kOneOver64;
    out_metrics->Horizontal.advance = static_cast<float>(slot->metrics.horiAdvance) * kOneOver64;
    out_metrics->Vertical.bearingX = static_cast<float>(slot->metrics.vertBearingX) * kOneOver64;
    out_metrics->Vertical.bearingY = static_cast<float>(slot->metrics.vertBearingY) * kOneOver64;
    out_metrics->Vertical.advance = static_cast<float>(slot->metrics.vertAdvance) * kOneOver64;

    out_params[0] = 0xF2;
    out_params[1] = 0;

    const u8 face_flag_bit = static_cast<u8>((static_cast<u32>(face->face_flags) >> 5) & 1u);
    const u64 vec88_a = obj->layout_seed_vec[0];
    const u64 vec88_b = obj->layout_seed_vec[1];
    const u8 has_vec88 = (vec88_a != 0 || vec88_b != 0) ? 8 : 0;
    out_params[2] = static_cast<u8>(0xF0 | has_vec88 | face_flag_bit);
    out_params[3] = 0;

    const u16 units_per_em = static_cast<u16>(face->units_per_EM);
    std::memcpy(out_params + 4, &units_per_em, sizeof(units_per_em));

    void* outline_ptr = static_cast<void*>(&slot->outline);
    std::memcpy(out_params + 0x10, &outline_ptr, sizeof(outline_ptr));

    void* metrics_ptr = static_cast<void*>(out_metrics);
    std::memcpy(out_params + 0x18, &metrics_ptr, sizeof(metrics_ptr));

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI LibraryApplyGlyphAdjustStub(void* fontObj, u32 /*p2*/, u32 glyphIndex, s32 p4,
                                             s32 p5, u32* inoutGlyphIndex) {
    if (!fontObj || !inoutGlyphIndex) {
        return ORBIS_FONT_ERROR_FATAL;
    }
    auto* obj = static_cast<Libraries::Font::Internal::FontObj*>(fontObj);
    *inoutGlyphIndex = glyphIndex;
    obj->shift_units_x = p4;
    obj->shift_units_y = p5;
    obj->layout_seed_pair = 0;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI LibraryConfigureGlyphStub(void* fontObj, std::uint32_t* in_params, s32 mode,
                                           std::uint32_t* inout_state) {
    if (!fontObj || !in_params || !inout_state) {
        return ORBIS_FONT_ERROR_FATAL;
    }

    auto* obj = static_cast<Libraries::Font::Internal::FontObj*>(fontObj);

    if (*in_params != 0) {
        if (mode == 0) {
            reinterpret_cast<std::uint8_t*>(inout_state)[3] |= 0x80;
        } else {
            reinterpret_cast<std::uint8_t*>(inout_state)[3] |= 0x80;
        }
    }

    obj->glyph_cfg_word_0x130 = *in_params;
    obj->glyph_cfg_mode_0x134 = static_cast<std::uint8_t>(mode);
    obj->glyph_cfg_byte_0x136 = 0;
    obj->glyph_cfg_byte_0x135 = 0;
    return ORBIS_OK;
}

} // namespace Libraries::FontFt::Internal
