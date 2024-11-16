// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDesktopServices>
#include <QDialog>
#include <QLabel>
#include <QPixmap>
#include <QUrl>

namespace Ui {
class AboutDialog;
}

class AboutDialog : public QDialog {
    Q_OBJECT

public:
    explicit AboutDialog(QWidget* parent = nullptr);
    ~AboutDialog();
    bool eventFilter(QObject* obj, QEvent* event);

private:
    Ui::AboutDialog* ui;

    void preloadImages();
    void updateImagesForCurrentTheme();
    void applyHoverEffect(QLabel* label);
    void removeHoverEffect(QLabel* label);

    bool isDarkTheme() const;

    QPixmap originalImages[5];
    QPixmap invertedImages[5];
};
