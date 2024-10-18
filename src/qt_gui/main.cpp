// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/config.h"
#include "common/memory_patcher.h"
#include "core/file_sys/fs.h"
#include "emulator.h"
#include "game_install_dialog.h"
#include "main_window.h"

#ifdef _WIN32
#include <windows.h>
#endif

// Custom message handler to ignore Qt logs
void customMessageHandler(QtMsgType, const QMessageLogContext&, const QString&) {}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    QApplication a(argc, argv);

    // Load configurations and initialize Qt application
    const auto user_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
    Config::load(user_dir / "config.toml");

    // Check if elf or eboot.bin path was passed as a command line argument
    bool has_command_line_argument = argc > 1;

    // Check if the game install directory is set
    if (Config::getGameInstallDirs().empty() && !has_command_line_argument) {
        GameInstallDialog dlg;
        dlg.exec();
    }

    // Ignore Qt logs
    qInstallMessageHandler(customMessageHandler);

    // Initialize the main window
    MainWindow* m_main_window = new MainWindow(nullptr);
    m_main_window->Init();

    // Check for command line arguments
    if (has_command_line_argument) {
        Core::Emulator emulator;
        for (int i = 0; i < argc; i++) {
            std::string curArg = argv[i];
            if (curArg == "-p") {
                std::string patchFile = argv[i + 1];
                MemoryPatcher::patchFile = patchFile;
            }
        }
        emulator.Run(argv[1]);
    }

    // Run the Qt application
    return a.exec();
}
