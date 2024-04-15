// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QtWidgets/QApplication>

#include "qt_gui/game_install_dialog.h"
#include "qt_gui/gui_settings.h"
#include "qt_gui/main_window.h"
#include "src/sdl_window.h"

Frontend::WindowSDL* g_window;

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);
    auto m_gui_settings = std::make_shared<GuiSettings>();
    if (m_gui_settings->GetValue(gui::settings_install_dir) == "") {
        GameInstallDialog dlg(m_gui_settings);
        dlg.exec();
    }
    MainWindow* m_main_window = new MainWindow(m_gui_settings, nullptr);
    m_main_window->Init();

    return a.exec();
}