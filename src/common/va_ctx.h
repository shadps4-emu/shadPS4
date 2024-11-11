//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <xmmintrin.h>
#include "common/types.h"

#define VA_ARGS                                                                                    \
    uint64_t rdi, uint64_t rsi, uint64_t rdx, uint64_t rcx, uint64_t r8, uint64_t r9,              \
        uint64_t overflow_arg_area, __m128 xmm0, __m128 xmm1, __m128 xmm2, __m128 xmm3,            \
        __m128 xmm4, __m128 xmm5, __m128 xmm6, __m128 xmm7, ...

#define VA_CTX(ctx)                                                                                \
    alignas(16)::Common::VaCtx ctx{};                                                              \
    (ctx).reg_save_area.gp[0] = rdi;                                                               \
    (ctx).reg_save_area.gp[1] = rsi;                                                               \
    (ctx).reg_save_area.gp[2] = rdx;                                                               \
    (ctx).reg_save_area.gp[3] = rcx;                                                               \
    (ctx).reg_save_area.gp[4] = r8;                                                                \
    (ctx).reg_save_area.gp[5] = r9;                                                                \
    (ctx).reg_save_area.fp[0] = xmm0;                                                              \
    (ctx).reg_save_area.fp[1] = xmm1;                                                              \
    (ctx).reg_save_area.fp[2] = xmm2;                                                              \
    (ctx).reg_save_area.fp[3] = xmm3;                                                              \
    (ctx).reg_save_area.fp[4] = xmm4;                                                              \
    (ctx).reg_save_area.fp[5] = xmm5;                                                              \
    (ctx).reg_save_area.fp[6] = xmm6;                                                              \
    (ctx).reg_save_area.fp[7] = xmm7;                                                              \
    (ctx).va_list.reg_save_area = &(ctx).reg_save_area;                                            \
    (ctx).va_list.gp_offset = offsetof(::Common::VaRegSave, gp);                                   \
    (ctx).va_list.fp_offset = offsetof(::Common::VaRegSave, fp);                                   \
    (ctx).va_list.overflow_arg_area = &overflow_arg_area;

namespace Common {

// https://stackoverflow.com/questions/4958384/what-is-the-format-of-the-x86-64-va-list-structure

struct VaList {
    u32 gp_offset;
    u32 fp_offset;
    void* overflow_arg_area;
    void* reg_save_area;
};

struct VaRegSave {
    u64 gp[6];
    __m128 fp[8];
};

struct VaCtx {
    VaRegSave reg_save_area;
    VaList va_list;
};

template <class T, uint32_t Size>
T vaArgRegSaveAreaGp(VaList* l) {
    auto* addr = reinterpret_cast<T*>(static_cast<u8*>(l->reg_save_area) + l->gp_offset);
    l->gp_offset += Size;
    return *addr;
}
template <class T, u64 Align, u64 Size>
T vaArgOverflowArgArea(VaList* l) {
    auto ptr = ((reinterpret_cast<u64>(l->overflow_arg_area) + (Align - 1)) & ~(Align - 1));
    auto* addr = reinterpret_cast<T*>(ptr);
    l->overflow_arg_area = reinterpret_cast<void*>(ptr + Size);
    return *addr;
}

template <class T, uint32_t Size>
T vaArgRegSaveAreaFp(VaList* l) {
    auto* addr = reinterpret_cast<T*>(static_cast<u8*>(l->reg_save_area) + l->fp_offset);
    l->fp_offset += Size;
    return *addr;
}

inline int vaArgInteger(VaList* l) {
    if (l->gp_offset <= 40) {
        return vaArgRegSaveAreaGp<int, 8>(l);
    }
    return vaArgOverflowArgArea<int, 1, 8>(l);
}

inline long long vaArgLongLong(VaList* l) {
    if (l->gp_offset <= 40) {
        return vaArgRegSaveAreaGp<long long, 8>(l);
    }
    return vaArgOverflowArgArea<long long, 1, 8>(l);
}
inline long vaArgLong(VaList* l) {
    if (l->gp_offset <= 40) {
        return vaArgRegSaveAreaGp<long, 8>(l);
    }
    return vaArgOverflowArgArea<long, 1, 8>(l);
}

inline double vaArgDouble(VaList* l) {
    if (l->fp_offset <= 160) {
        return vaArgRegSaveAreaFp<double, 16>(l);
    }
    return vaArgOverflowArgArea<double, 1, 8>(l);
}

template <class T>
T* vaArgPtr(VaList* l) {
    if (l->gp_offset <= 40) {
        return vaArgRegSaveAreaGp<T*, 8>(l);
    }
    return vaArgOverflowArgArea<T*, 1, 8>(l);
}

} // namespace Common
