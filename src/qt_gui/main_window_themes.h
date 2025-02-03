// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QApplication>
#include <QLineEdit>
#include <QWidget>

enum class Theme : int { Dark, Light, Green, Blue, Violet, Gruvbox, TokyoNight };

class WindowThemes : public QObject {
    Q_OBJECT
public Q_SLOTS:
    void SetWindowTheme(Theme theme, QLineEdit* mw_searchbar);
};
