// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CHECKUPDATE_H
#define CHECKUPDATE_H

#include <QCheckBox>
#include <QDialog>

class CheckUpdate : public QDialog {
    Q_OBJECT

public:
    explicit CheckUpdate(QWidget* parent = nullptr);

private slots:
    void CheckForUpdates();
    void DownloadAndInstallUpdate(const QString& url);
    void Unzip();

private:
    void setupUI_NoUpdate();
    void setupUI_UpdateAvailable(const QString& downloadUrl);

    QCheckBox* autoUpdateCheckBox;
    QPushButton* yesButton;
    QPushButton* noButton;
    QString updateDownloadUrl;
};

#endif // CHECKUPDATE_H
