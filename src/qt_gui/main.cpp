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

    // Check if an ELF or eboot.bin path was passed as a command line argument
    bool has_command_line_argument = argc > 1;

    if (has_command_line_argument) {
        for (int i = 1; i < argc; i++) {
            std::string curArg = argv[i];
            if (curArg == "-p" && i + 1 < argc) {
                std::string patchFile = argv[i + 1];
                MemoryPatcher::patchFile = patchFile;
                i++; // Skip the next argument as it’s the patch file
            }
        }
        // Run the emulator directly with the provided argument
        Core::Emulator emulator;
        emulator.Run(argv[1]);
        return 0;
    }

    // Initialize QApplication and run the GUI if no command-line argument is provided
    QApplication a(argc, argv);

    // Load configurations and initialize Qt application
    const auto user_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
    Config::load(user_dir / "config.toml");

    // Check if the game install directory is set
    if (Config::getGameInstallDirs().empty()) {
        GameInstallDialog dlg;
        dlg.exec();
    }

    // Ignore Qt logs
    qInstallMessageHandler(customMessageHandler);

    // Initialize the main window
    MainWindow* m_main_window = new MainWindow(nullptr);
    m_main_window->Init();

    // Run the Qt application
    return a.exec();
}
