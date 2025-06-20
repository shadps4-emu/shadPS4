// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#ifdef _WIN32

#include "ntapi.h"
#include "logging/log.h"

NtClose_t NtClose = nullptr;
NtSetInformationFile_t NtSetInformationFile = nullptr;
NtCreateThread_t NtCreateThread = nullptr;
NtTerminateThread_t NtTerminateThread = nullptr;
NtQueueApcThreadEx_t NtQueueApcThreadEx = nullptr;

namespace Common::NtApi {

void Initialize() {
    HMODULE nt_handle = GetModuleHandleA("ntdll.dll");
    if(nt_handle == NULL){
        LPVOID message_buffer;
        DWORD buffer_len = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR) &message_buffer,
                0, NULL );
        if(buffer_len){
            std::string message((LPCSTR)message_buffer, (LPCSTR)message_buffer + buffer_len);
            LocalFree(message_buffer);
            LOG_WARNING(Common_Ntapi, "{}", message);
        }
    }

    // http://stackoverflow.com/a/31411628/4725495
    NtClose = (NtClose_t)GetProcAddress(nt_handle, "NtClose");
    if(NtClose == NULL){
        LPVOID message_buffer;
        DWORD buffer_len = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR) &message_buffer,
                0, NULL );
        if(buffer_len){
            std::string message((LPCSTR)message_buffer, (LPCSTR)message_buffer + buffer_len);
            LocalFree(message_buffer);
            LOG_WARNING(Common_Ntapi, "{}", message);
        }
    }

    NtSetInformationFile = (NtSetInformationFile_t)GetProcAddress(nt_handle, "NtSetInformationFile");
    if(NtSetInformationFile == NULL){
        LPVOID message_buffer;
        DWORD buffer_len = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR) &message_buffer,
                0, NULL );
        if(buffer_len){
            std::string message((LPCSTR)message_buffer, (LPCSTR)message_buffer + buffer_len);
            LocalFree(message_buffer);
            LOG_WARNING(Common_Ntapi, "{}", message);
        }
    }

    NtCreateThread = (NtCreateThread_t)GetProcAddress(nt_handle, "NtCreateThread");
    if(NtCreateThread == NULL){
        LPVOID message_buffer;
        DWORD buffer_len = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR) &message_buffer,
                0, NULL );
        if(buffer_len){
            std::string message((LPCSTR)message_buffer, (LPCSTR)message_buffer + buffer_len);
            LocalFree(message_buffer);
            LOG_WARNING(Common_Ntapi, "{}", message);
        }
    }

    NtTerminateThread = (NtTerminateThread_t)GetProcAddress(nt_handle, "NtTerminateThread");
    if(NtTerminateThread == NULL){
        LPVOID message_buffer;
        DWORD buffer_len = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR) &message_buffer,
                0, NULL );
        if(buffer_len){
            std::string message((LPCSTR)message_buffer, (LPCSTR)message_buffer + buffer_len);
            LocalFree(message_buffer);
            LOG_WARNING(Common_Ntapi, "{}", message);
        }
    }

    NtQueueApcThreadEx = (NtQueueApcThreadEx_t)GetProcAddress(nt_handle, "NtQueueApcThreadEx");
    if(NtQueueApcThreadEx == NULL){
        LPVOID message_buffer;
        DWORD buffer_len = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR) &message_buffer,
                0, NULL );
        if(buffer_len){
            std::string message((LPCSTR)message_buffer, (LPCSTR)message_buffer + buffer_len);
            LocalFree(message_buffer);
            LOG_WARNING(Common_Ntapi, "{}", message);
        }
    }
}

} // namespace Common::NtApi

#endif
