// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdarg>
#include <cstdio>

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "libc_internal_io.h"

namespace Libraries::LibcInternal {

s32 PS4_SYSV_ABI internal___vfprintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Printf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__WPrintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_asprintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fprintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fprintf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fwprintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fwprintf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_printf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_printf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_snprintf(char* s, size_t n, const char* format, ...) {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_snprintf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_snwprintf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_sprintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_sprintf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_swprintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_swprintf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_swscanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_swscanf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_vasprintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_vfprintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_vfprintf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_vfscanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_vfscanf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_vfwprintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_vfwprintf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_vfwscanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_vfwscanf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_vprintf(const char* format, va_list args) {
    // Copy the va_list because vsnprintf consumes it
    va_list args_copy;
    va_copy(args_copy, args);

    // Calculate the required buffer size
    int size = std::vsnprintf(nullptr, 0, format, args_copy);
    va_end(args_copy);

    if (size < 0) {
        // Handle vsnprintf error
        LOG_ERROR(Lib_LibcInternal, "vsnprintf failed to calculate size");
        return size;
    }

    // Create a string with the required size
    std::string buffer(size, '\0');

    // Format the string into the buffer
    int result =
        std::vsnprintf(buffer.data(), buffer.size() + 1, format, args); // +1 for null terminator
    if (result >= 0) {
        // Log the formatted result
        LOG_INFO(Lib_LibcInternal, "{}", buffer);
    } else {
        LOG_ERROR(Lib_LibcInternal, "vsnprintf failed during formatting");
    }

    return result;
}

s32 PS4_SYSV_ABI internal_vprintf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_vscanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_vscanf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_vsnprintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_vsnprintf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_vsnwprintf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_vsprintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_vsprintf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_vsscanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_vsscanf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_vswprintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_vswprintf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_vswscanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_vswscanf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_vwprintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_vwprintf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_vwscanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_vwscanf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wprintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wprintf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wscanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wscanf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Scanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__WScanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fscanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fscanf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fwscanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fwscanf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_scanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_scanf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_sscanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_sscanf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterlibSceLibcInternalIo(Core::Loader::SymbolsResolver* sym) {

    LIB_FUNCTION("yAZ5vOpmBus", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___vfprintf);
    LIB_FUNCTION("FModQzwn1-4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Printf);
    LIB_FUNCTION("kvEP5-KOG1U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__WPrintf);
    LIB_FUNCTION("cOYia2dE0Ik", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_asprintf);
    LIB_FUNCTION("fffwELXNVFA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fprintf);
    LIB_FUNCTION("-e-F9HjUFp8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fprintf_s);
    LIB_FUNCTION("ZRAcn3dPVmA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fwprintf);
    LIB_FUNCTION("9kOFELic7Pk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fwprintf_s);
    LIB_FUNCTION("a6CYO8YOzfw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fwscanf);
    LIB_FUNCTION("Bo5wtXSj4kc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fwscanf_s);
    LIB_FUNCTION("hcuQgD53UxM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_printf);
    LIB_FUNCTION("w1NxRBQqfmQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_printf_s);
    LIB_FUNCTION("eLdDw6l0-bU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_snprintf);
    LIB_FUNCTION("3BytPOQgVKc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_snprintf_s);
    LIB_FUNCTION("jbj2wBoiCyg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_snwprintf_s);
    LIB_FUNCTION("tcVi5SivF7Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_sprintf);
    LIB_FUNCTION("xEszJVGpybs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_sprintf_s);
    LIB_FUNCTION("1Pk0qZQGeWo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_sscanf);
    LIB_FUNCTION("24m4Z4bUaoY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_sscanf_s);
    LIB_FUNCTION("nJz16JE1txM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_swprintf);
    LIB_FUNCTION("Im55VJ-Bekc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_swprintf_s);
    LIB_FUNCTION("HNnWdT43ues", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_swscanf);
    LIB_FUNCTION("tQNolUV1q5A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_swscanf_s);
    LIB_FUNCTION("qjBlw2cVMAM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vasprintf);
    LIB_FUNCTION("pDBDcY6uLSA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vfprintf);
    LIB_FUNCTION("GhTZtaodo7o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vfprintf_s);
    LIB_FUNCTION("lckWSkHDBrY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vfscanf);
    LIB_FUNCTION("JjPXy-HX5dY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vfscanf_s);
    LIB_FUNCTION("M2bGWSqt764", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vfwprintf);
    LIB_FUNCTION("XX9KWzJvRf0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vfwprintf_s);
    LIB_FUNCTION("WF4fBmip+38", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vfwscanf);
    LIB_FUNCTION("Wvm90I-TGl0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vfwscanf_s);
    LIB_FUNCTION("GMpvxPFW924", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vprintf);
    LIB_FUNCTION("YfJUGNPkbK4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vprintf_s);
    LIB_FUNCTION("j7Jk3yd3yC8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vscanf);
    LIB_FUNCTION("fQYpcUzy3zo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vscanf_s);
    LIB_FUNCTION("Q2V+iqvjgC0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vsnprintf);
    LIB_FUNCTION("rWSuTWY2JN0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vsnprintf_s);
    LIB_FUNCTION("8SKVXgeK1wY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vsnwprintf_s);
    LIB_FUNCTION("jbz9I9vkqkk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vsprintf);
    LIB_FUNCTION("+qitMEbkSWk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vsprintf_s);
    LIB_FUNCTION("UTrpOVLcoOA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vsscanf);
    LIB_FUNCTION("tfNbpqL3D0M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vsscanf_s);
    LIB_FUNCTION("u0XOsuOmOzc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vswprintf);
    LIB_FUNCTION("oDoV9tyHTbA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vswprintf_s);
    LIB_FUNCTION("KGotca3AjYw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vswscanf);
    LIB_FUNCTION("39HHkIWrWNo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vswscanf_s);
    LIB_FUNCTION("QuF2rZGE-v8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vwprintf);
    LIB_FUNCTION("XPrliF5n-ww", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vwprintf_s);
    LIB_FUNCTION("QNwdOK7HfJU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vwscanf);
    LIB_FUNCTION("YgZ6qvFH3QI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vwscanf_s);
    LIB_FUNCTION("OGVdXU3E-xg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wprintf);
    LIB_FUNCTION("FEtOJURNey0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wprintf_s);
    LIB_FUNCTION("D8JBAR3RiZQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wscanf);
    LIB_FUNCTION("RV7X3FrWfTI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wscanf_s);
    LIB_FUNCTION("s+MeMHbB1Ro", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Scanf);
    LIB_FUNCTION("fzgkSILqRHE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__WScanf);
    LIB_FUNCTION("npLpPTaSuHg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fscanf);
    LIB_FUNCTION("vj2WUi2LrfE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fscanf_s);
    LIB_FUNCTION("7XEv6NnznWw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_scanf);
    LIB_FUNCTION("-B76wP6IeVA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_scanf_s);
}

} // namespace Libraries::LibcInternal
