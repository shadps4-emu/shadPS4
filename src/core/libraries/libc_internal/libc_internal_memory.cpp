// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "libc_internal_memory.h"

namespace Libraries::LibcInternal {

s32 PS4_SYSV_ABI internal__malloc_finalize_lv2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__malloc_fini() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__malloc_init() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__malloc_init_lv2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__malloc_postfork() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__malloc_prefork() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__malloc_thread_cleanup() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__sceLibcGetMallocParam() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_operator_new(size_t size) {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_malloc(size_t size) {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_malloc_check_memory_bounds() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_malloc_finalize() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_malloc_get_footer_value() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_malloc_get_malloc_state() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_malloc_initialize() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_malloc_report_memory_blocks() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_malloc_stats() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_malloc_stats_fast() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_malloc_usable_size() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_memalign() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_memchr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_memcmp(const void* s1, const void* s2, size_t n) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::memcmp(s1, s2, n);
}

void* PS4_SYSV_ABI internal_memcpy(void* dest, const void* src, size_t n) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::memcpy(dest, src, n);
}

s32 PS4_SYSV_ABI internal_memcpy_s(void* dest, size_t destsz, const void* src, size_t count) {
    LOG_DEBUG(Lib_LibcInternal, "called");
#ifdef _WIN64
    return memcpy_s(dest, destsz, src, count);
#else
    std::memcpy(dest, src, count);
    return 0; // ALL OK
#endif
}

s32 PS4_SYSV_ABI internal_memmove(void* d, void* s, size_t n) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    std::memmove(d, s, n);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_memmove_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_memrchr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

void* PS4_SYSV_ABI internal_memset(void* s, int c, size_t n) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::memset(s, c, n);
}

s32 PS4_SYSV_ABI internal_memset_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_posix_memalign() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_realloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_reallocalign() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_reallocf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wmemchr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wmemcmp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wmemcpy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wmemcpy_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wmemmove() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wmemmove_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wmemset() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

void PS4_SYSV_ABI internal_operator_delete(void* ptr) {
    if (ptr) {
        std::free(ptr);
    }
}

void PS4_SYSV_ABI internal_free(void* ptr) {
    std::free(ptr);
}

void RegisterlibSceLibcInternalMemory(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("RnqlvEmvkdw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__malloc_finalize_lv2);
    LIB_FUNCTION("21KFhEQDJ3s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__malloc_fini);
    LIB_FUNCTION("z8GPiQwaAEY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__malloc_init);
    LIB_FUNCTION("20cUk0qX9zo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__malloc_init_lv2);
    LIB_FUNCTION("V94pLruduLg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__malloc_postfork);
    LIB_FUNCTION("aLYyS4Kx6rQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__malloc_prefork);
    LIB_FUNCTION("Sopthb9ztZI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__malloc_thread_cleanup);
    LIB_FUNCTION("1nZ4Xfnyp38", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__sceLibcGetMallocParam);
    LIB_FUNCTION("fJnpuVVBbKk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_operator_new);
    LIB_FUNCTION("cVSk9y8URbc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_posix_memalign);
    LIB_FUNCTION("Ujf3KzMvRmI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_memalign);
    LIB_FUNCTION("8u8lPzUEq+U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_memchr);
    LIB_FUNCTION("DfivPArhucg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_memcmp);
    LIB_FUNCTION("Q3VBxCXhUHs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_memcpy);
    LIB_FUNCTION("NFLs+dRJGNg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_memcpy_s);
    LIB_FUNCTION("+P6FRGH4LfA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_memmove);
    LIB_FUNCTION("B59+zQQCcbU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_memmove_s);
    LIB_FUNCTION("5G2ONUzRgjY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_memrchr);
    LIB_FUNCTION("8zTFvBIAIN8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_memset);
    LIB_FUNCTION("h8GwqPFbu6I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_memset_s);
    LIB_FUNCTION("Y7aJ1uydPMo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_realloc);
    LIB_FUNCTION("OGybVuPAhAY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_reallocalign);
    LIB_FUNCTION("YMZO9ChZb0E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_reallocf);
    LIB_FUNCTION("fnUEjBCNRVU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wmemchr);
    LIB_FUNCTION("QJ5xVfKkni0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wmemcmp);
    LIB_FUNCTION("fL3O02ypZFE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wmemcpy);
    LIB_FUNCTION("BTsuaJ6FxKM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wmemcpy_s);
    LIB_FUNCTION("Noj9PsJrsa8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wmemmove);
    LIB_FUNCTION("F8b+Wb-YQVs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wmemmove_s);
    LIB_FUNCTION("Al8MZJh-4hM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wmemset);
    LIB_FUNCTION("gQX+4GDQjpM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_malloc);
    LIB_FUNCTION("ECOPpUQEch0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_malloc_check_memory_bounds);
    LIB_FUNCTION("J6FoFNydpFI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_malloc_finalize);
    LIB_FUNCTION("SlG1FN-y0N0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_malloc_get_footer_value);
    LIB_FUNCTION("Nmezc1Lh7TQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_malloc_get_malloc_state);
    LIB_FUNCTION("owT6zLJxrTs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_malloc_initialize);
    LIB_FUNCTION("0F08WOP8G3s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_malloc_report_memory_blocks);
    LIB_FUNCTION("CC-BLMBu9-I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_malloc_stats);
    LIB_FUNCTION("KuOuD58hqn4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_malloc_stats_fast);
    LIB_FUNCTION("NDcSfcYZRC8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_malloc_usable_size);
    LIB_FUNCTION("MLWl90SFWNE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_operator_delete);
    LIB_FUNCTION("tIhsqj0qsFE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_free);
}

} // namespace Libraries::LibcInternal
