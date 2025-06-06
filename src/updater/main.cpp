// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QApplication>
#include "common/config.h"
#include "common/path_util.h"
#include "core/file_sys/fs.h"
#include "qt_gui/check_update.h"
#include "qt_gui/main_window_themes.h"

#ifdef _WIN32
#include <windows.h>
#endif

// Custom message handler to ignore Qt logs
void customMessageHandler(QtMsgType, const QMessageLogContext&, const QString&) {}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    QApplication u(argc, argv);

    QApplication::setDesktopFileName("net.shadps4.shadPS4_updater");
    QApplication::setStyle("Fusion");

    // Load configurations
    const auto user_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
    Config::load(user_dir / "config.toml");

    WindowThemes m_window_themes;

    Theme lastTheme = static_cast<Theme>(Config::getMainWindowTheme());
    m_window_themes.SetWindowTheme(lastTheme, nullptr);

    if (Config::startupUpdate()) {
        auto checkUpdate = new CheckUpdate(false);
        checkUpdate->exec();
    }

    // Ignore Qt logs
    qInstallMessageHandler(customMessageHandler);

    return 0;
}
