// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/libs.h"//TODO
#include "shadps4_app.h"
#ifdef _WIN32
#include <windows.h>
#endif

std::shared_ptr<ShadPs4App> ShadPs4App::instance = std::make_shared<ShadPs4App>();

static void DeleteInstance() {
    ShadPs4App::instance.reset();
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    auto& shadps4_app = *ShadPs4App::instance;

    Common::Log::Setup(argc, argv);
    // register Shutdwon to run before registry dtor
    static struct LogDestroyer {
        ~LogDestroyer() {
            Common::Log::Shutdown();
        }
    } s_log_destroyer;

    // register instance deleter after log one
    //std::atexit(DeleteInstance);
    //std::at_quick_exit(DeleteInstance);

    if (auto code = shadps4_app.parse(argc, argv); code.has_value()) {
        //DeleteInstance();
        //Common::Log::Shutdown();

        return *code;
    }

    shadps4_app.init();

    // should be the first to be destroyed (in particular before vulkan/log)
    static struct AppDestroyer {
        ~AppDestroyer() {
            //DeleteInstance();
        }
    } s_app_destroyer;

    //if (!Common::Log::g_should_append)
    //    Common::Log::Truncate();

    auto r = shadps4_app.run();

    DeleteInstance();

    return r;
}
