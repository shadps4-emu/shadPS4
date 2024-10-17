// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fmt/core.h>
#include "common/config.h"
#include "common/memory_patcher.h"
#include "emulator.h"

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    // Parse command line arguments
    if (!Config::loadArgs(argc, argv)) {
        return 0;
    }

    MemoryPatcher::patchFile = Config::getPatchFile();

    if (argc == 1) {
        fmt::print("Usage: {} <elf or eboot.bin path>\n", argv[0]);
        return -1;
    }
    // check if eboot file exists
    if (!std::filesystem::exists(argv[1])) {
        fmt::print("Eboot.bin file not found\n");
        return -1;
    }

    Core::Emulator emulator;
    emulator.Run(argv[argc - 1]);

    return 0;
}
