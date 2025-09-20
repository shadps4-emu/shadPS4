//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include "debugger.h"

#include <iostream>
#include <thread>

#ifdef _WIN32
#include <Windows.h>
#include <debugapi.h>
#elif defined(__linux__)
#include <fstream>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>
#endif

bool Core::Debugger::IsDebuggerAttached() {
#ifdef _WIN32
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