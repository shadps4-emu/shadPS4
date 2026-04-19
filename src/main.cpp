// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>
#include <optional>

#ifdef _WIN32
#include <windows.h>
#endif
#include <core/user_settings.h>

#include "shadps4_app.h"

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    auto& shadps4_app = *ShadPs4App::GetInstance();

    if (auto code = shadps4_app.parse(argc, argv); code.has_value()) {
        return *code;
    }

    shadps4_app.init();

    return shadps4_app.run();
}
