// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CHECKUPDATE_H
#define CHECKUPDATE_H
#pragma once

#include <QCheckBox>
#include <QDialog>
#include <QNetworkAccessManager>
#include <QPushButton>
#include "main_window.h"

class CheckUpdate : public QDialog {
    Q_OBJECT

public:
    explicit CheckUpdate(MainWindow* mainWindow, bool showMessage, QWidget* parent = nullptr);
    CheckUpdate(const bool showMessage, QWidget* parent);
    ~CheckUpdate();

private slots:
    void CheckForUpdates(const bool showMessage);
    void DownloadUpdate(const QString& url);
    void Install();

private:
    void setupUI(const QString& downloadUrl, const QString& latestDate, const QString& latestRev,
                 const QString& currentDate, const QString& currentRev);

    void requestChangelog(const QString& currentRev, const QString& latestRev,
                          const QString& downloadUrl, const QString& latestDate,
                          const QString& currentDate);

    QCheckBox* autoUpdateCheckBox;
    QPushButton* yesButton;
    QPushButton* noButton;
    QString updateDownloadUrl;

    QNetworkAccessManager* networkManager;
    MainWindow* m_mainWindow;
};

#endif // CHECKUPDATE_H
