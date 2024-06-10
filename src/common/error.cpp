// SPDX-FileCopyrightText: 2013 Dolphin Emulator Project
// SPDX-FileCopyrightText: 2014 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstddef>
#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#include <cstring>
#endif

#include "common/error.h"

namespace Common {

std::string NativeErrorToString(int e) {
#ifdef _WIN32
    LPSTR err_str;

    DWORD res = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                   FORMAT_MESSAGE_IGNORE_INSERTS,
                               nullptr, e, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                               reinterpret_cast<LPSTR>(&err_str), 1, nullptr);
    if (!res) {
        return "(FormatMessageA failed to format error)";
    }
    std::string ret(err_str);
    LocalFree(err_str);
    return ret;
#else
    char err_str[255];
#if defined(__GLIBC__) && (_GNU_SOURCE || (_POSIX_C_SOURCE < 200112L && _XOPEN_SOURCE < 600)) ||   \
    defined(ANDROID)
    // Thread safe (GNU-specific)
    const char* str = strerror_r(e, err_str, sizeof(err_str));
    return std::string(str);
#else
    // Thread safe (XSI-compliant)
    int second_err = strerror_r(e, err_str, sizeof(err_str));
    if (second_err != 0) {
        return "(strerror_r failed to format error)";
    }
    return std::string(err_str);
#endif // GLIBC etc.
#endif // _WIN32
}

std::string GetLastErrorMsg() {
#ifdef _WIN32
    return NativeErrorToString(GetLastError());
#else
    return NativeErrorToString(errno);
#endif
}

} // namespace Common
