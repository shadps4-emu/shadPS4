// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef MSPACE_SELFTEST
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "libc_internal_memory.h"
#endif

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>

#ifndef MSPACE_SELFTEST
#define PS4_SYSV_ABI_ATTR PS4_SYSV_ABI
#else
#define PS4_SYSV_ABI_ATTR
#define LOG_INFO(...) ((void)0)
#define LOG_ERROR(...) ((void)0)
#endif

namespace Libraries::LibcInternal {

using u64 = std::uint64_t;
using u32 = std::uint32_t;
using s32 = std::int32_t;

void* PS4_SYSV_ABI_ATTR internal_memset(void* s, int c, size_t n) {
    return std::memset(s, c, n);
}

void* PS4_SYSV_ABI_ATTR internal_memcpy(void* dest, const void* src, size_t n) {
    return std::memcpy(dest, src, n);
}

s32 PS4_SYSV_ABI_ATTR internal_memcpy_s(void* dest, size_t destsz, const void* src, size_t count) {
    if (count > destsz) {
        return 22; // EINVAL
    }
    std::memcpy(dest, src, count);
    return 0;
}

s32 PS4_SYSV_ABI_ATTR internal_memcmp(const void* s1, const void* s2, size_t n) {
    return std::memcmp(s1, s2, n);
}

void* PS4_SYSV_ABI_ATTR internal_memmove(void* dest, const void* src, size_t n) {
    return std::memmove(dest, src, n);
}

// ---------------------------------------------------------------- mspace
//
// sceLibcMspaceCreate(name, base, capacity, flags) membuat heap kecil di dalam
// blok memori yang sudah dialokasikan game (mis. 16 MB 'SceNpToolkit2Lib').
//
// ponytail: bump allocator - free() hanya menandai, tidak memakai ulang blok.
// Cukup untuk heap toolkit (alokasi sedikit, hidup sampai shutdown).
// Kalau ada game yang churn besar di mspace, ganti ke dlmalloc mspace.

// deklarasi maju (Realloc memakai Free)
void* PS4_SYSV_ABI_ATTR sceLibcMspaceMalloc(void* mspace, size_t size);
void PS4_SYSV_ABI_ATTR sceLibcMspaceFree(void* mspace, void* ptr);

namespace {

constexpr size_t kAlign = 16;
constexpr u32 kMspaceMagic = 0x4D535043; // 'MSPC'
constexpr s32 kMspaceErrorInvalidArgument = 0xE0000022; // EINVAL di ruang PS4 libc
constexpr s32 kMspaceErrorNotEnoughMemory = 0xE000000C; // ENOMEM

struct Mspace {
    u32 magic;
    size_t capacity;
    size_t used;
    char name[32];
};

std::mutex g_mspace_mutex;

inline size_t AlignUp(size_t v, size_t a) {
    return (v + (a - 1)) & ~(a - 1);
}

inline Mspace* AsMspace(void* mspace) {
    auto* ms = static_cast<Mspace*>(mspace);
    if (ms == nullptr || ms->magic != kMspaceMagic) {
        return nullptr;
    }
    return ms;
}

} // namespace

void* PS4_SYSV_ABI_ATTR sceLibcMspaceCreate(const char* name, void* base, size_t capacity,
                                            u32 flags) {
    (void)flags; // ponytail: MAP flags mspace tidak dipakai bump allocator
    if (base == nullptr || capacity < sizeof(Mspace) + kAlign) {
        LOG_ERROR(Lib_LibcInternal, "sceLibcMspaceCreate: invalid base/capacity (cap={:#x})",
                  capacity);
        return nullptr;
    }

    std::lock_guard lock{g_mspace_mutex};

    auto* ms = static_cast<Mspace*>(base);
    ms->magic = kMspaceMagic;
    ms->capacity = capacity;
    ms->used = AlignUp(sizeof(Mspace), kAlign);
    ms->name[0] = '\0';
    if (name != nullptr) {
        std::strncpy(ms->name, name, sizeof(ms->name) - 1);
        ms->name[sizeof(ms->name) - 1] = '\0';
    }

    LOG_INFO(Lib_LibcInternal, "sceLibcMspaceCreate: name='{}', base={}, capacity={:#x} -> {}",
             ms->name, static_cast<const void*>(base), capacity, static_cast<const void*>(ms));
    return ms;
}

void* PS4_SYSV_ABI_ATTR sceLibcMspaceMalloc(void* mspace, size_t size) {
    std::lock_guard lock{g_mspace_mutex};
    auto* ms = AsMspace(mspace);
    if (ms == nullptr) {
        return nullptr;
    }
    if (size == 0) {
        size = 1;
    }

    const size_t offset = AlignUp(ms->used, kAlign);
    if (offset + size > ms->capacity) {
        return nullptr;
    }

    auto* ptr = reinterpret_cast<std::uint8_t*>(ms) + offset;
    ms->used = offset + size;
    return ptr;
}

void* PS4_SYSV_ABI_ATTR sceLibcMspaceMemalign(void* mspace, size_t alignment, size_t size) {
    std::lock_guard lock{g_mspace_mutex};
    auto* ms = AsMspace(mspace);
    if (ms == nullptr || alignment == 0 || (alignment & (alignment - 1)) != 0) {
        return nullptr;
    }
    if (size == 0) {
        size = 1;
    }

    const auto base = reinterpret_cast<std::uintptr_t>(ms);
    std::uintptr_t cursor = AlignUp(base + ms->used, alignment);
    const std::uintptr_t end = cursor + size;
    if (end - base > ms->capacity) {
        return nullptr;
    }

    ms->used = static_cast<size_t>(end - base);
    return reinterpret_cast<void*>(cursor);
}

void* PS4_SYSV_ABI_ATTR sceLibcMspaceRealloc(void* mspace, void* ptr, size_t size) {
    auto* ms = AsMspace(mspace);
    if (ms == nullptr) {
        return nullptr;
    }
    if (ptr == nullptr) {
        return sceLibcMspaceMalloc(mspace, size);
    }
    if (size == 0) {
        sceLibcMspaceFree(mspace, ptr);
        return nullptr;
    }

    // ponytail: bump allocator tidak tahu ukuran blok lama -> selalu pindah.
    auto* baru = sceLibcMspaceMalloc(mspace, size);
    if (baru == nullptr) {
        return nullptr;
    }
    std::memcpy(baru, ptr, size);
    return baru;
}

void PS4_SYSV_ABI_ATTR sceLibcMspaceFree(void* mspace, void* ptr) {
    // ponytail: no-op. Bump allocator, lihat catatan di atas.
    (void)mspace;
    (void)ptr;
}

s32 PS4_SYSV_ABI_ATTR sceLibcMspaceDestroy(void* mspace) {
    std::lock_guard lock{g_mspace_mutex};
    auto* ms = AsMspace(mspace);
    if (ms == nullptr) {
        return kMspaceErrorInvalidArgument;
    }
    LOG_INFO(Lib_LibcInternal, "sceLibcMspaceDestroy: name='{}', used={:#x}/{:#x}", ms->name,
             ms->used, ms->capacity);
    ms->magic = 0;
    return 0;
}

s32 PS4_SYSV_ABI_ATTR sceLibcMspaceIsHeapEmpty(void* mspace) {
    auto* ms = AsMspace(mspace);
    if (ms == nullptr) {
        return kMspaceErrorInvalidArgument;
    }
    return ms->used <= AlignUp(sizeof(Mspace), kAlign) ? 1 : 0;
}

size_t PS4_SYSV_ABI_ATTR sceLibcMspaceGetBlockSize(void* mspace, void* ptr) {
    (void)ptr;
    auto* ms = AsMspace(mspace);
    if (ms == nullptr) {
        return 0;
    }
    return ms->capacity - static_cast<size_t>(reinterpret_cast<std::uint8_t*>(ptr) -
                                              reinterpret_cast<std::uint8_t*>(ms));
}


// ------------------------------------------------------- __cxa_guard_*
//
// Game memakai guard C++ statis (libSceNpToolkit2). Stub bawaan mengembalikan 0
// ("sudah diinisialisasi"), sehingga ctor objeknya TIDAK pernah jalan ->
// crash saat dipakai. Itanium ABI: acquire() mengembalikan 1 kalau pemanggil
// HARUS menjalankan inisialisasi.

// ponytail: no-op. Kita tidak menjalankan destructor C++ statis saat exit;
// cukup kembalikan 0 (= berhasil didaftarkan) supaya toolkit tidak gagal.
s32 PS4_SYSV_ABI_ATTR sceCxaAtexit(void* func, void* arg, void* dso) {
    (void)func;
    (void)arg;
    (void)dso;
    return 0;
}

s32 PS4_SYSV_ABI_ATTR sceCxaGuardAcquire(u64* guard) {
    if (guard == nullptr) {
        return 0;
    }
    static std::mutex guard_mutex;
    std::lock_guard lock{guard_mutex};

    if ((__atomic_load_n(guard, __ATOMIC_ACQUIRE) & 1) != 0) {
        return 0; // sudah diinisialisasi
    }
    // ponytail: tandai sebelum ctor jalan (bukan setelah release) - cukup untuk
    // inisialisasi single-thread. Kalau ada ctor yang melempar exception,
    // tambahkan state "in progress" + abort().
    __atomic_store_n(guard, 1, __ATOMIC_RELEASE);
    LOG_INFO(Lib_LibcInternal, "__cxa_guard_acquire: init required, guard={}", static_cast<void*>(guard));
    return 1;
}

void PS4_SYSV_ABI_ATTR sceCxaGuardRelease(u64* guard) {
    (void)guard;
}

void PS4_SYSV_ABI_ATTR sceCxaGuardAbort(u64* guard) {
    if (guard != nullptr) {
        __atomic_store_n(guard, 0, __ATOMIC_RELEASE);
    }
}

static u64 g_mspace_atomic_id_mask = 0;
static u64 g_mstate_table[64] = {0};

struct HeapInfoInfo {
    u64 size = sizeof(HeapInfoInfo);
    u32 flag;
    u32 getSegmentInfo;
    u64* mspace_atomic_id_mask;
    u64* mstate_table;
};

void PS4_SYSV_ABI_ATTR sceLibcHeapGetTraceInfo(HeapInfoInfo* info) {
    info->mspace_atomic_id_mask = &g_mspace_atomic_id_mask;
    info->mstate_table = g_mstate_table;
    info->getSegmentInfo = 0;
}

#ifndef MSPACE_SELFTEST
void RegisterlibSceLibcInternalMemory(Core::Loader::SymbolsResolver* sym) {

    LIB_FUNCTION("NFLs+dRJGNg", "libSceLibcInternal", 1, "libSceLibcInternal", internal_memcpy_s);
    LIB_FUNCTION("Q3VBxCXhUHs", "libSceLibcInternal", 1, "libSceLibcInternal", internal_memcpy);
    LIB_FUNCTION("+P6FRGH4LfA", "libSceLibcInternal", 1, "libSceLibcInternal", internal_memmove);
    LIB_FUNCTION("8zTFvBIAIN8", "libSceLibcInternal", 1, "libSceLibcInternal", internal_memset);
    LIB_FUNCTION("DfivPArhucg", "libSceLibcInternal", 1, "libSceLibcInternal", internal_memcmp);

    LIB_FUNCTION("NWtTN10cJzE", "libSceLibcInternalExt", 1, "libSceLibcInternal",
                 sceLibcHeapGetTraceInfo);

    // mspace: NID dari log game CUSA44357 (sceLibcMspaceCreate).
    // NID fungsi mspace lain belum diketahui -> lihat log berikutnya lalu tambahkan baris serupa.
    LIB_FUNCTION("-hn1tcVHq5Q", "libSceLibcInternal", 1, "libSceLibcInternal",
                 sceLibcMspaceCreate);
    LIB_FUNCTION("OJjm-QOIHlI", "libSceLibcInternal", 1, "libSceLibcInternal",
                 sceLibcMspaceMalloc);
    LIB_FUNCTION("W6SiVSiCDtI", "libSceLibcInternal", 1, "libSceLibcInternal",
                 sceLibcMspaceDestroy);
    LIB_FUNCTION("9rAeANT2tyE", "libSceLibcInternal", 1, "libSceLibcInternal",
                 sceCxaGuardRelease);
    LIB_FUNCTION("tsvEmnenz48", "libSceLibcInternal", 1, "libSceLibcInternal",
                 sceCxaAtexit);
    LIB_FUNCTION("3GPpjQdAMTw", "libSceLibcInternal", 1, "libSceLibcInternal",
                 sceCxaGuardAcquire);

}
#endif

} // namespace Libraries::LibcInternal

// --------------------------------------------------------------------- test
#ifdef MSPACE_SELFTEST
#include <cassert>
#include <cstdio>

int main() {
    using namespace Libraries::LibcInternal;

    alignas(16) static std::uint8_t region[64 * 1024];
    std::memset(region, 0, sizeof(region));

    void* ms = sceLibcMspaceCreate("SceNpToolkit2Lib", region, sizeof(region), 0);
    assert(ms != nullptr);

    void* a = sceLibcMspaceMalloc(ms, 100);
    void* b = sceLibcMspaceMalloc(ms, 4096);
    assert(a != nullptr && b != nullptr);
    assert(a != b);
    assert((reinterpret_cast<std::uintptr_t>(a) % 16) == 0);
    assert((reinterpret_cast<std::uintptr_t>(b) % 16) == 0);

    std::memset(a, 0xAB, 100);
    std::memset(b, 0xCD, 4096);
    assert(static_cast<std::uint8_t*>(a)[99] == 0xAB);
    assert(static_cast<std::uint8_t*>(b)[4095] == 0xCD);

    // blok tidak saling tumpang tindih
    assert(static_cast<std::uint8_t*>(a) + 100 <= static_cast<std::uint8_t*>(b));

    void* c = sceLibcMspaceMemalign(ms, 256, 32);
    assert(c != nullptr && (reinterpret_cast<std::uintptr_t>(c) % 256) == 0);

    // habis: minta jauh melebihi kapasitas -> harus nullptr, bukan crash
    void* tooBig = sceLibcMspaceMalloc(ms, sizeof(region) * 2);
    assert(tooBig == nullptr);

    assert(sceLibcMspaceIsHeapEmpty(ms) == 0);
    assert(sceLibcMspaceDestroy(ms) == 0);
    assert(sceLibcMspaceDestroy(ms) != 0); // dobel destroy -> error, bukan crash

    // selftest guard
    alignas(8) static u64 g = 0;
    assert(sceCxaGuardAcquire(&g) == 1);  // harus init
    assert(sceCxaGuardAcquire(&g) == 0);  // sudah init
    sceCxaGuardAbort(&g);
    assert(sceCxaGuardAcquire(&g) == 1);  // abort -> boleh init lagi
    assert(sceCxaGuardAcquire(nullptr) == 0);

    std::printf("mspace selftest: OK\n");
    return 0;
}
#endif
