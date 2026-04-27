// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#ifdef _WIN32

#include "ntapi.h"

NtSetInformationFile_t NtSetInformationFile = nullptr;
NtQueueApcThreadEx_t NtQueueApcThreadEx = nullptr;

namespace Common::NtApi {

void Initialize() {
    HMODULE nt_handle = GetModuleHandleA("ntdll.dll");

    // http://stackoverflow.com/a/31411628/4725495
    NtSetInformationFile =
        (NtSetInformationFile_t)GetProcAddress(nt_handle, "NtSetInformationFile");
    NtQueueApcThreadEx = (NtQueueApcThreadEx_t)GetProcAddress(nt_handle, "NtQueueApcThreadEx");
}

} // namespace Common::NtApi

#endif
