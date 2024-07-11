// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#ifdef _WIN32

#include "ntapi.h"

NtDelayExecution_t NtDelayExecution = nullptr;
NtSetInformationFile_t NtSetInformationFile = nullptr;

namespace Common::NtApi {

void Initialize() {
    HMODULE nt_handle = GetModuleHandleA("ntdll.dll");

    // http://stackoverflow.com/a/31411628/4725495
    NtDelayExecution = (NtDelayExecution_t)GetProcAddress(nt_handle, "NtDelayExecution");
    NtSetInformationFile =
        (NtSetInformationFile_t)GetProcAddress(nt_handle, "NtSetInformationFile");
}

} // namespace Common::NtApi

#endif
