// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "libc_internal_str.h"

namespace Libraries::LibcInternal {

s32 PS4_SYSV_ABI internal__CStrftime() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__CStrxfrm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Getstr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Stod() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Stodx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Stof() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Stoflt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Stofx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Stold() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Stoldx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Stoll() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Stollx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Stolx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Stopfx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Stoul() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Stoull() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Stoullx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Stoulx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Stoxflt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Strcollx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Strerror() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Strxfrmx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_strcasecmp(const char* str1, const char* str2) {
    LOG_DEBUG(Lib_LibcInternal, "called");
#ifdef _WIN32
    return _stricmp(str1, str2);
#else
    return strcasecmp(str1, str2);
#endif
}

char* PS4_SYSV_ABI internal_strcat(char* dest, const char* src) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::strcat(dest, src);
}

s32 PS4_SYSV_ABI internal_strcat_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

const char* PS4_SYSV_ABI internal_strchr(const char* str, int c) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::strchr(str, c);
}

s32 PS4_SYSV_ABI internal_strcmp(const char* str1, const char* str2) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::strcmp(str1, str2);
}

s32 PS4_SYSV_ABI internal_strcoll() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

char* PS4_SYSV_ABI internal_strcpy(char* dest, const char* src) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::strcpy(dest, src);
}

char* PS4_SYSV_ABI internal_strcpy_s(char* dest, u64 len, const char* src) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::strncpy(dest, src, len);
}

s32 PS4_SYSV_ABI internal_strcspn(const char* str1, const char* str2) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::strcspn(str1, str2);
}

char* PS4_SYSV_ABI internal_strdup() {
    LOG_DEBUG(Lib_LibcInternal, "called");
    char* dup = (char*)std::malloc(std::strlen(str1) + 1);
    if (dup != NULL)
        strcpy(dup, str1);
    return dup;
}

s32 PS4_SYSV_ABI internal_strerror() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_strerror_r() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_strerror_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_strerrorlen_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_strftime() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_strlcat() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_strlcpy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

size_t PS4_SYSV_ABI internal_strlen(const char* str) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::strlen(str);
}

s32 PS4_SYSV_ABI internal_strncasecmp(const char* str1, const char* str2, size_t num) {
    LOG_DEBUG(Lib_LibcInternal, "called");
#ifdef _WIN32
    return _strnicmp(str1, str2, num);
#else
    return strncasecmp(str1, str2, num);
#endif
}

s32 PS4_SYSV_ABI internal_strncat() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_strncat_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_strncmp(const char* str1, const char* str2, size_t num) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::strncmp(str1, str2, num);
}

char* PS4_SYSV_ABI internal_strncpy(char* dest, const char* src, std::size_t count) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::strncpy(dest, src, count);
}

s32 PS4_SYSV_ABI internal_strncpy_s(char* dest, size_t destsz, const char* src, size_t count) {
    LOG_DEBUG(Lib_LibcInternal, "called");
#ifdef _WIN64
    return strncpy_s(dest, destsz, src, count);
#else
    std::strcpy(dest, src);
    return 0;
#endif
}

s32 PS4_SYSV_ABI internal_strndup() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_strnlen(const char* str, size_t maxlen) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::min(std::strlen(str), maxlen);
}

s32 PS4_SYSV_ABI internal_strnlen_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_strnstr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

const char* PS4_SYSV_ABI internal_strpbrk(const char* str1, const char* str2) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::strpbrk(str1, str2);
}

const char* PS4_SYSV_ABI internal_strrchr(const char* str, int c) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::strrchr(str, c);
}

char* PS4_SYSV_ABI internal_strsep(char** strp, const char* delim) {
    LOG_DEBUG(Lib_LibcInternal, "called");
#ifdef _GNU_SOURCE
    return strsep(strp, delim);
#else
    if (!*strp)
        return nullptr;
    char* token = *strp;
    *strp = std::strpbrk(token, delim);
    if (*strp)
        *(*strp)++ = '\0';
    return token;
#endif
}

s32 PS4_SYSV_ABI internal_strspn(const char* str1, const char* str2) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::strspn(str1, str2);
}

char* PS4_SYSV_ABI internal_strstr(char* h, char* n) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::strstr(h, n);
}

double PS4_SYSV_ABI internal_strtod(const char* str, char** endptr) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::strtod(str, endptr);
}

float PS4_SYSV_ABI internal_strtof(const char* str, char** endptr) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::strtof(str, endptr);
}

s32 PS4_SYSV_ABI internal_strtoimax() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_strtok() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_strtok_r() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_strtok_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_strtol() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_strtold() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_strtoll() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_strtoul() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_strtoull() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_strtoumax() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_strtouq() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_strxfrm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcsstr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterlibSceLibcInternalStr(Core::Loader::SymbolsResolver* sym) {

    LIB_FUNCTION("ykNF6P3ZsdA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__CStrftime);
    LIB_FUNCTION("we-vQBAugV8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__CStrxfrm);
    LIB_FUNCTION("i2yN6xBwooo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Getstr);
    LIB_FUNCTION("c41UEHVtiEA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Stod);
    LIB_FUNCTION("QlcJbyd6jxM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Stodx);
    LIB_FUNCTION("CpWcnrEZbLA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Stof);
    LIB_FUNCTION("wO1-omboFjo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Stoflt);
    LIB_FUNCTION("7dlAxeH-htg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Stofx);
    LIB_FUNCTION("iNbtyJKM0iQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Stold);
    LIB_FUNCTION("BKidCxmLC5w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Stoldx);
    LIB_FUNCTION("7pNKcscKrf8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Stoll);
    LIB_FUNCTION("mOnfZ5aNDQE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Stollx);
    LIB_FUNCTION("Ecwid6wJMhY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Stolx);
    LIB_FUNCTION("yhbF6MbVuYc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Stopfx);
    LIB_FUNCTION("zlfEH8FmyUA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Stoul);
    LIB_FUNCTION("q+9E0X3aWpU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Stoull);
    LIB_FUNCTION("pSpDCDyxkaY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Stoullx);
    LIB_FUNCTION("YDnLaav6W6Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Stoulx);
    LIB_FUNCTION("Ouz5Q8+SUq4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Stoxflt);
    LIB_FUNCTION("v6rXYSx-WGA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Strcollx);
    LIB_FUNCTION("4F11tHMpJa0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Strerror);
    LIB_FUNCTION("CpiD2ZXrhNo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Strxfrmx);
    LIB_FUNCTION("AV6ipCNa4Rw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strcasecmp);
    LIB_FUNCTION("Ls4tzzhimqQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strcat);
    LIB_FUNCTION("K+gcnFFJKVc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strcat_s);
    LIB_FUNCTION("ob5xAW4ln-0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strchr);
    LIB_FUNCTION("Ovb2dSJOAuE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strcmp);
    LIB_FUNCTION("gjbmYpP-XJQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strcoll);
    LIB_FUNCTION("kiZSXIWd9vg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strcpy);
    LIB_FUNCTION("5Xa2ACNECdo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strcpy_s);
    LIB_FUNCTION("q0F6yS-rCms", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strcspn);
    LIB_FUNCTION("g7zzzLDYGw0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strdup);
    LIB_FUNCTION("RIa6GnWp+iU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strerror);
    LIB_FUNCTION("RBcs3uut1TA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strerror_r);
    LIB_FUNCTION("o+ok6Y+DtgY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strerror_s);
    LIB_FUNCTION("-g26XITGVgE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strerrorlen_s);
    LIB_FUNCTION("Av3zjWi64Kw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strftime);
    LIB_FUNCTION("ByfjUZsWiyg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strlcat);
    LIB_FUNCTION("SfQIZcqvvms", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strlcpy);
    LIB_FUNCTION("j4ViWNHEgww", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strlen);
    LIB_FUNCTION("pXvbDfchu6k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strncasecmp);
    LIB_FUNCTION("kHg45qPC6f0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strncat);
    LIB_FUNCTION("NC4MSB+BRQg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strncat_s);
    LIB_FUNCTION("aesyjrHVWy4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strncmp);
    LIB_FUNCTION("6sJWiWSRuqk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strncpy);
    LIB_FUNCTION("YNzNkJzYqEg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strncpy_s);
    LIB_FUNCTION("XGnuIBmEmyk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strndup);
    LIB_FUNCTION("5jNubw4vlAA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strnlen);
    LIB_FUNCTION("DQbtGaBKlaw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strnlen_s);
    LIB_FUNCTION("Xnrfb2-WhVw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strnstr);
    LIB_FUNCTION("kDZvoVssCgQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strpbrk);
    LIB_FUNCTION("9yDWMxEFdJU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strrchr);
    LIB_FUNCTION("cJWGxiQPmDQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strsep);
    LIB_FUNCTION("-kU6bB4M-+k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strspn);
    LIB_FUNCTION("viiwFMaNamA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strstr);
    LIB_FUNCTION("2vDqwBlpF-o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strtod);
    LIB_FUNCTION("xENtRue8dpI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strtof);
    LIB_FUNCTION("q5MWYCDfu3c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strtoimax);
    LIB_FUNCTION("oVkZ8W8-Q8A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strtok);
    LIB_FUNCTION("enqPGLfmVNU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strtok_r);
    LIB_FUNCTION("-vXEQdRADLI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strtok_s);
    LIB_FUNCTION("mXlxhmLNMPg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strtol);
    LIB_FUNCTION("nW9JRkciRk4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strtold);
    LIB_FUNCTION("VOBg+iNwB-4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strtoll);
    LIB_FUNCTION("QxmSHBCuKTk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strtoul);
    LIB_FUNCTION("5OqszGpy7Mg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strtoull);
    LIB_FUNCTION("QNyUWGXmXNc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strtoumax);
    LIB_FUNCTION("g-McpZfseZo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strtouq);
    LIB_FUNCTION("zogPrkd46DY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_strxfrm);
    LIB_FUNCTION("WDpobjImAb4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcsstr);
}

} // namespace Libraries::LibcInternal
