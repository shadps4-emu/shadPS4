// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <cctype>
#include <mutex>
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "libc_internal_memory.h"

namespace Libraries::LibcInternal {

// Itanium C++ ABI guard variables protect function-local statics with
// non-trivial initialization (e.g. `static Foo x;`). A single recursive
// mutex serializes all such initializations; correctness (no torn/skipped
// construction across threads) matters far more than parallelism here, since
// each guard only takes this path once in the object's lifetime.
namespace {
std::recursive_mutex g_cxa_guard_mutex;
}

s32 PS4_SYSV_ABI __cxa_guard_acquire(u64* guard) {
    g_cxa_guard_mutex.lock();
    auto* initialized = reinterpret_cast<volatile u8*>(guard);
    if (*initialized != 0) {
        g_cxa_guard_mutex.unlock();
        return 0; // Already initialized, skip running the initializer.
    }
    // Mutex stays locked until the caller runs the initializer and calls
    // __cxa_guard_release (success) or __cxa_guard_abort (exception).
    return 1;
}

void PS4_SYSV_ABI __cxa_guard_release(u64* guard) {
    auto* initialized = reinterpret_cast<volatile u8*>(guard);
    *initialized = 1;
    g_cxa_guard_mutex.unlock();
}

void PS4_SYSV_ABI __cxa_guard_abort(u64* guard) {
    g_cxa_guard_mutex.unlock();
}

// MSVC-style ctype classification table used by isalpha/isdigit/etc. Indexed
// from -128 to 255 so it tolerates both signed-char and unsigned-char (and
// EOF) lookups without going out of bounds; classification wraps to the
// equivalent unsigned byte value.
namespace {
constexpr u16 CT_UPPER = 0x1;
constexpr u16 CT_LOWER = 0x2;
constexpr u16 CT_DIGIT = 0x4;
constexpr u16 CT_SPACE = 0x8;
constexpr u16 CT_PUNCT = 0x10;
constexpr u16 CT_CONTROL = 0x20;
constexpr u16 CT_BLANK = 0x40;
constexpr u16 CT_HEX = 0x80;

struct CTypeTable {
    std::array<u16, 384> data{};
    CTypeTable() {
        for (int i = 0; i < 384; i++) {
            const int byte_value = ((i - 128) % 256 + 256) % 256;
            u16 flags = 0;
            if (std::isupper(byte_value)) flags |= CT_UPPER;
            if (std::islower(byte_value)) flags |= CT_LOWER;
            if (std::isdigit(byte_value)) flags |= CT_DIGIT;
            if (std::isspace(byte_value)) flags |= CT_SPACE;
            if (std::ispunct(byte_value)) flags |= CT_PUNCT;
            if (std::iscntrl(byte_value)) flags |= CT_CONTROL;
            if (byte_value == ' ' || byte_value == '\t') flags |= CT_BLANK;
            if (std::isxdigit(byte_value)) flags |= CT_HEX;
            data[i] = flags;
        }
    }
};

const CTypeTable g_ctype_table;
} // namespace

const u16* PS4_SYSV_ABI _Getpctype() {
    return g_ctype_table.data.data() + 128;
}

void* PS4_SYSV_ABI internal_memset(void* s, int c, size_t n) {
    return std::memset(s, c, n);
}

void* PS4_SYSV_ABI internal_memcpy(void* dest, const void* src, size_t n) {
    return std::memcpy(dest, src, n);
}

s32 PS4_SYSV_ABI internal_memcpy_s(void* dest, size_t destsz, const void* src, size_t count) {
#ifdef _WIN64
    return memcpy_s(dest, destsz, src, count);
#else
    std::memcpy(dest, src, count);
    return 0; // ALL OK
#endif
}

s32 PS4_SYSV_ABI internal_memcmp(const void* s1, const void* s2, size_t n) {
    return std::memcmp(s1, s2, n);
}

static u64 g_mspace_atomic_id_mask = 0;
static u64 g_mstate_table[64] = {0};

// Minimal first-fit allocator implementing the sceLibcMspace* family. Real PS4
// titles use this to carve a private heap out of a caller-provided memory
// region (mirrors dlmalloc's mspace API, which the Sony wrapper is based on).
namespace {
struct MspaceBlock {
    u64 size; // usable size of this block, excluding the header
    bool used;
    MspaceBlock* next;
    MspaceBlock* prev;
};

struct MspaceControl {
    u8* base;
    u64 total_size;
    MspaceBlock* first;
};

constexpr u64 MspaceAlign = 16;

u64 AlignUp(u64 value, u64 align) {
    return (value + align - 1) & ~(align - 1);
}
} // namespace

void* PS4_SYSV_ABI sceLibcMspaceCreate(const char* name, void* base, u64 size, s32 flags) {
    if (!base || size < sizeof(MspaceControl) + sizeof(MspaceBlock)) {
        LOG_ERROR(Core, "sceLibcMspaceCreate: invalid base/size (base={}, size={:#x})", base,
                  size);
        return nullptr;
    }
    auto* ctrl = reinterpret_cast<MspaceControl*>(base);
    ctrl->base = reinterpret_cast<u8*>(base);
    ctrl->total_size = size;
    auto* first = reinterpret_cast<MspaceBlock*>(ctrl->base + sizeof(MspaceControl));
    first->size = size - sizeof(MspaceControl) - sizeof(MspaceBlock);
    first->used = false;
    first->next = nullptr;
    first->prev = nullptr;
    ctrl->first = first;
    LOG_INFO(Core, "sceLibcMspaceCreate: name={} base={} size={:#x}", name ? name : "(null)", base,
              size);
    return ctrl;
}

void* PS4_SYSV_ABI sceLibcMspaceMalloc(void* mspace, u64 size) {
    if (!mspace || size == 0) {
        return nullptr;
    }
    auto* ctrl = reinterpret_cast<MspaceControl*>(mspace);
    size = AlignUp(size, MspaceAlign);
    for (auto* block = ctrl->first; block; block = block->next) {
        if (block->used || block->size < size) {
            continue;
        }
        if (block->size >= size + sizeof(MspaceBlock) + MspaceAlign) {
            auto* remainder =
                reinterpret_cast<MspaceBlock*>(reinterpret_cast<u8*>(block) + sizeof(MspaceBlock) + size);
            remainder->size = block->size - size - sizeof(MspaceBlock);
            remainder->used = false;
            remainder->next = block->next;
            remainder->prev = block;
            if (block->next) {
                block->next->prev = remainder;
            }
            block->next = remainder;
            block->size = size;
        }
        block->used = true;
        return reinterpret_cast<u8*>(block) + sizeof(MspaceBlock);
    }
    LOG_ERROR(Core, "sceLibcMspaceMalloc: out of memory (requested {:#x})", size);
    return nullptr;
}

void PS4_SYSV_ABI sceLibcMspaceFree(void* mspace, void* ptr) {
    if (!mspace || !ptr) {
        return;
    }
    auto* block = reinterpret_cast<MspaceBlock*>(reinterpret_cast<u8*>(ptr) - sizeof(MspaceBlock));
    block->used = false;
    if (block->next && !block->next->used) {
        block->size += sizeof(MspaceBlock) + block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }
    if (block->prev && !block->prev->used) {
        block->prev->size += sizeof(MspaceBlock) + block->size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }
}

void* PS4_SYSV_ABI sceLibcMspaceCalloc(void* mspace, u64 nelem, u64 size) {
    const u64 total = nelem * size;
    void* ptr = sceLibcMspaceMalloc(mspace, total);
    if (ptr) {
        std::memset(ptr, 0, total);
    }
    return ptr;
}

void* PS4_SYSV_ABI sceLibcMspaceRealloc(void* mspace, void* ptr, u64 size) {
    if (!ptr) {
        return sceLibcMspaceMalloc(mspace, size);
    }
    if (size == 0) {
        sceLibcMspaceFree(mspace, ptr);
        return nullptr;
    }
    auto* block = reinterpret_cast<MspaceBlock*>(reinterpret_cast<u8*>(ptr) - sizeof(MspaceBlock));
    if (block->size >= size) {
        return ptr;
    }
    void* new_ptr = sceLibcMspaceMalloc(mspace, size);
    if (new_ptr) {
        std::memcpy(new_ptr, ptr, block->size);
        sceLibcMspaceFree(mspace, ptr);
    }
    return new_ptr;
}

s32 PS4_SYSV_ABI sceLibcMspaceDestroy(void* mspace) {
    // The backing region is owned by the caller (typically kernel flexible
    // memory); there is nothing extra for us to release here.
    return 0;
}

struct HeapInfoInfo {
    u64 size = sizeof(HeapInfoInfo);
    u32 flag;
    u32 getSegmentInfo;
    u64* mspace_atomic_id_mask;
    u64* mstate_table;
};

void PS4_SYSV_ABI sceLibcHeapGetTraceInfo(HeapInfoInfo* info) {
    info->mspace_atomic_id_mask = &g_mspace_atomic_id_mask;
    info->mstate_table = g_mstate_table;
    info->getSegmentInfo = 0;
}

void RegisterlibSceLibcInternalMemory(Core::Loader::SymbolsResolver* sym) {

    LIB_FUNCTION("NFLs+dRJGNg", "libSceLibcInternal", 1, "libSceLibcInternal", internal_memcpy_s);
    LIB_FUNCTION("Q3VBxCXhUHs", "libSceLibcInternal", 1, "libSceLibcInternal", internal_memcpy);
    LIB_FUNCTION("8zTFvBIAIN8", "libSceLibcInternal", 1, "libSceLibcInternal", internal_memset);
    LIB_FUNCTION("DfivPArhucg", "libSceLibcInternal", 1, "libSceLibcInternal", internal_memcmp);

    LIB_FUNCTION("NWtTN10cJzE", "libSceLibcInternalExt", 1, "libSceLibcInternal",
                 sceLibcHeapGetTraceInfo);

    LIB_FUNCTION("-hn1tcVHq5Q", "libSceLibcInternal", 1, "libSceLibcInternal", sceLibcMspaceCreate);
    LIB_FUNCTION("OJjm-QOIHlI", "libSceLibcInternal", 1, "libSceLibcInternal", sceLibcMspaceMalloc);
    LIB_FUNCTION("Vla-Z+eXlxo", "libSceLibcInternal", 1, "libSceLibcInternal", sceLibcMspaceFree);
    LIB_FUNCTION("W6SiVSiCDtI", "libSceLibcInternal", 1, "libSceLibcInternal", sceLibcMspaceDestroy);
    LIB_FUNCTION("LYo3GhIlB38", "libSceLibcInternal", 1, "libSceLibcInternal", sceLibcMspaceCalloc);
    LIB_FUNCTION("gigoVHZvVPE", "libSceLibcInternal", 1, "libSceLibcInternal", sceLibcMspaceRealloc);

    LIB_FUNCTION("3GPpjQdAMTw", "libSceLibcInternal", 1, "libSceLibcInternal", __cxa_guard_acquire);
    LIB_FUNCTION("9rAeANT2tyE", "libSceLibcInternal", 1, "libSceLibcInternal", __cxa_guard_release);
    LIB_FUNCTION("2emaaluWzUw", "libSceLibcInternal", 1, "libSceLibcInternal", __cxa_guard_abort);
    LIB_FUNCTION("sUP1hBaouOw", "libSceLibcInternal", 1, "libSceLibcInternal", _Getpctype);
}

} // namespace Libraries::LibcInternal
