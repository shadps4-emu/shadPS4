// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QtWidgets/QApplication>

#include "common/config.h"
#include "core/file_sys/fs.h"
#include "qt_gui/game_install_dialog.h"
#include "qt_gui/main_window.h"

#include <emulator.h>
#include <fmt/core.h>

// Custom message handler to ignore Qt logs
void customMessageHandler(QtMsgType, const QMessageLogContext&, const QString&) {}

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);

    // Load configurations and initialize Qt application
    const auto user_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
    Config::load(user_dir / "config.toml");
    std::filesystem::create_directory(user_dir / "game_data");

    // Check if the game install directory is set
    if (Config::getGameInstallDir() == "") {
        GameInstallDialog dlg;
        dlg.exec();
    }

    // Ignore Qt logs
    qInstallMessageHandler(customMessageHandler);

    // Initialize the main window
    MainWindow* m_main_window = new MainWindow(nullptr);
    m_main_window->Init();

    // Check for command line arguments
    if (argc > 1) {
        Core::Emulator emulator;
        emulator.Run(argv[1]);
    }

    // Run the Qt application
    return a.exec();
}
