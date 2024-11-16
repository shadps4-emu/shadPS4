// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDesktopServices>
#include <QEvent>
#include <QGraphicsDropShadowEffect>
#include <QImage>
#include <QLabel>
#include <QPixmap>
#include <common/config.h>
#include "about_dialog.h"
#include "main_window_themes.h"
#include "ui_about_dialog.h"

AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent), ui(new Ui::AboutDialog) {
    ui->setupUi(this);
    preloadImages();

    ui->image_1->setAttribute(Qt::WA_Hover, true);
    ui->image_2->setAttribute(Qt::WA_Hover, true);
    ui->image_3->setAttribute(Qt::WA_Hover, true);
    ui->image_4->setAttribute(Qt::WA_Hover, true);
    ui->image_5->setAttribute(Qt::WA_Hover, true);

    ui->image_1->installEventFilter(this);
    ui->image_2->installEventFilter(this);
    ui->image_3->installEventFilter(this);
    ui->image_4->installEventFilter(this);
    ui->image_5->installEventFilter(this);
}

AboutDialog::~AboutDialog() {
    delete ui;
}

void AboutDialog::preloadImages() {
    originalImages[0] = ui->image_1->pixmap().copy();
    originalImages[1] = ui->image_2->pixmap().copy();
    originalImages[2] = ui->image_3->pixmap().copy();
    originalImages[3] = ui->image_4->pixmap().copy();
    originalImages[4] = ui->image_5->pixmap().copy();

    for (int i = 0; i < 5; ++i) {
        QImage image = originalImages[i].toImage();
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                QColor color = image.pixelColor(x, y);
                color.setRed(255 - color.red());
                color.setGreen(255 - color.green());
                color.setBlue(255 - color.blue());
                image.setPixelColor(x, y, color);
            }
        }
        invertedImages[i] = QPixmap::fromImage(image);
    }
    updateImagesForCurrentTheme();
}

void AboutDialog::updateImagesForCurrentTheme() {
    Theme currentTheme = static_cast<Theme>(Config::getMainWindowTheme());
    bool isDarkTheme = (currentTheme == Theme::Dark || currentTheme == Theme::Green ||
                        currentTheme == Theme::Blue || currentTheme == Theme::Violet);
    if (isDarkTheme) {
        ui->image_1->setPixmap(invertedImages[0]);
        ui->image_2->setPixmap(invertedImages[1]);
        ui->image_3->setPixmap(invertedImages[2]);
        ui->image_4->setPixmap(invertedImages[3]);
        ui->image_5->setPixmap(invertedImages[4]);
    } else {
        ui->image_1->setPixmap(originalImages[0]);
        ui->image_2->setPixmap(originalImages[1]);
        ui->image_3->setPixmap(originalImages[2]);
        ui->image_4->setPixmap(originalImages[3]);
        ui->image_5->setPixmap(originalImages[4]);
    }
}

bool AboutDialog::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::Enter) {
        if (obj == ui->image_1) {
            if (isDarkTheme()) {
                ui->image_1->setPixmap(originalImages[0]);
            } else {
                ui->image_1->setPixmap(invertedImages[0]);
            }
            applyHoverEffect(ui->image_1);
        } else if (obj == ui->image_2) {
            if (isDarkTheme()) {
                ui->image_2->setPixmap(originalImages[1]);
            } else {
                ui->image_2->setPixmap(invertedImages[1]);
            }
            applyHoverEffect(ui->image_2);
        } else if (obj == ui->image_3) {
            if (isDarkTheme()) {
                ui->image_3->setPixmap(originalImages[2]);
            } else {
                ui->image_3->setPixmap(invertedImages[2]);
            }
            applyHoverEffect(ui->image_3);
        } else if (obj == ui->image_4) {
            if (isDarkTheme()) {
                ui->image_4->setPixmap(originalImages[3]);
            } else {
                ui->image_4->setPixmap(invertedImages[3]);
            }
            applyHoverEffect(ui->image_4);
        } else if (obj == ui->image_5) {
            if (isDarkTheme()) {
                ui->image_5->setPixmap(originalImages[4]);
            } else {
                ui->image_5->setPixmap(invertedImages[4]);
            }
            applyHoverEffect(ui->image_5);
        }
    } else if (event->type() == QEvent::Leave) {
        if (obj == ui->image_1) {
            if (isDarkTheme()) {
                ui->image_1->setPixmap(invertedImages[0]);
            } else {
                ui->image_1->setPixmap(originalImages[0]);
            }
            removeHoverEffect(ui->image_1);
        } else if (obj == ui->image_2) {
            if (isDarkTheme()) {
                ui->image_2->setPixmap(invertedImages[1]);
            } else {
                ui->image_2->setPixmap(originalImages[1]);
            }
            removeHoverEffect(ui->image_2);
        } else if (obj == ui->image_3) {
            if (isDarkTheme()) {
                ui->image_3->setPixmap(invertedImages[2]);
            } else {
                ui->image_3->setPixmap(originalImages[2]);
            }
            removeHoverEffect(ui->image_3);
        } else if (obj == ui->image_4) {
            if (isDarkTheme()) {
                ui->image_4->setPixmap(invertedImages[3]);
            } else {
                ui->image_4->setPixmap(originalImages[3]);
            }
            removeHoverEffect(ui->image_4);
        } else if (obj == ui->image_5) {
            if (isDarkTheme()) {
                ui->image_5->setPixmap(invertedImages[4]);
            } else {
                ui->image_5->setPixmap(originalImages[4]);
            }
            removeHoverEffect(ui->image_5);
        }
    } else if (event->type() == QEvent::MouseButtonPress) {
        if (obj == ui->image_1) {
            QDesktopServices::openUrl(QUrl("https://github.com/shadps4-emu/shadPS4"));
        } else if (obj == ui->image_2) {
            QDesktopServices::openUrl(QUrl("https://discord.gg/bFJxfftGW6"));
        } else if (obj == ui->image_3) {
            QDesktopServices::openUrl(QUrl("https://www.youtube.com/@shadPS4/videos"));
        } else if (obj == ui->image_4) {
            QDesktopServices::openUrl(QUrl("https://ko-fi.com/shadps4"));
        } else if (obj == ui->image_5) {
            QDesktopServices::openUrl(QUrl("https://shadps4.net"));
        }
        return true;
    }
    return QDialog::eventFilter(obj, event);
}

void AboutDialog::applyHoverEffect(QLabel* label) {
    QColor shadowColor = isDarkTheme() ? QColor(0, 0, 0) : QColor(169, 169, 169);
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect;
    shadow->setBlurRadius(5);
    shadow->setXOffset(2);
    shadow->setYOffset(2);
    shadow->setColor(shadowColor);
    label->setGraphicsEffect(shadow);
}

void AboutDialog::removeHoverEffect(QLabel* label) {
    QColor shadowColor = isDarkTheme() ? QColor(50, 50, 50) : QColor(169, 169, 169);
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect;
    shadow->setBlurRadius(3);
    shadow->setXOffset(0);
    shadow->setYOffset(0);
    shadow->setColor(shadowColor);
    label->setGraphicsEffect(shadow);
}

bool AboutDialog::isDarkTheme() const {
    Theme currentTheme = static_cast<Theme>(Config::getMainWindowTheme());
    return currentTheme == Theme::Dark || currentTheme == Theme::Green ||
           currentTheme == Theme::Blue || currentTheme == Theme::Violet;
}
