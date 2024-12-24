// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#ifdef _WIN32

#include "ntapi.h"

NtClose_t NtClose = nullptr;
NtSetInformationFile_t NtSetInformationFile = nullptr;
NtCreateThread_t NtCreateThread = nullptr;
NtTerminateThread_t NtTerminateThread = nullptr;
NtQueueApcThreadEx_t NtQueueApcThreadEx = nullptr;

namespace Common::NtApi {

void Initialize() {
    HMODULE nt_handle = GetModuleHandleA("ntdll.dll");

    // http://stackoverflow.com/a/31411628/4725495
    NtClose = (NtClose_t)GetProcAddress(nt_handle, "NtClose");
    NtSetInformationFile =
        (NtSetInformationFile_t)GetProcAddress(nt_handle, "NtSetInformationFile");
    NtCreateThread = (NtCreateThread_t)GetProcAddress(nt_handle, "NtCreateThread");
    NtTerminateThread = (NtTerminateThread_t)GetProcAddress(nt_handle, "NtTerminateThread");
    NtQueueApcThreadEx = (NtQueueApcThreadEx_t)GetProcAddress(nt_handle, "NtQueueApcThreadEx");
}

} // namespace Common::NtApi

#endif
