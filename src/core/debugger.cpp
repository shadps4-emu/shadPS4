//  SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include "debugger.h"

#include <iostream>
#include <thread>

#if defined(_WIN32)
#include <Windows.h>
#include <debugapi.h>
#elif defined(__linux__)
#include <filesystem>
#include <fstream>
#elif defined(__APPLE__)
#include <errno.h>
#include <signal.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>
#endif

bool Core::Debugger::IsDebuggerAttached() {
#if defined(_WIN32)
    return IsDebuggerPresent();
#elif defined(__linux__)
    std::ifstream status_file("/proc/self/status");
    std::string line;
    while (std::getline(status_file, line)) {
        if (line.starts_with("TracerPid:")) {
            std::string tracer_pid = line.substr(10);
            tracer_pid.erase(0, tracer_pid.find_first_not_of(" \t"));
            return tracer_pid != "0";
        }
    }
    return false;
#elif defined(__APPLE__)
    int mib[4];
    struct kinfo_proc info;
    size_t size = sizeof(info);

    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PID;
    mib[3] = getpid();

    if (sysctl(mib, 4, &info, &size, nullptr, 0) == 0) {
        return (info.kp_proc.p_flag & P_TRACED) != 0;
    }
    return false;
#else
#error "Unsupported platform"
#endif
}

void Core::Debugger::WaitForDebuggerAttach() {
    int count = 0;
    while (!IsDebuggerAttached()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        if (--count <= 0) {
            count = 10;
            std::cerr << "Waiting for debugger to attach..." << std::endl;
        }
    }
}
int Core::Debugger::GetCurrentPid() {
#if defined(_WIN32)
    return GetCurrentProcessId();
#elif defined(__APPLE__) || defined(__linux__)
    return getpid();
#else
#error "Unsupported platform"
#endif
}

void Core::Debugger::WaitForPid(int pid) {
#if defined(_WIN32)
    HANDLE process_handle = OpenProcess(SYNCHRONIZE, FALSE, pid);
    if (process_handle != nullptr) {
        std::cerr << "Waiting for process " << pid << " to exit..." << std::endl;
        WaitForSingleObject(process_handle, INFINITE);
        CloseHandle(process_handle);
    }
#elif defined(__linux__)
    std::string proc_path = "/proc/" + std::to_string(pid);

    while (std::filesystem::exists(proc_path)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cerr << "Waiting for process " << pid << " to exit..." << std::endl;
    }
#elif defined(__APPLE__)
    while (kill(pid, 0) == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cerr << "Waiting for process " << pid << " to exit..." << std::endl;
    }
#else
#error "Unsupported platform"
#endif
}
