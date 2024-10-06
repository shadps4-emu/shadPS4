// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "game_install_dialog.h"

GameInstallDialog::GameInstallDialog() : m_gamesDirectory(nullptr) {
    auto layout = new QVBoxLayout(this);

    layout->addWidget(SetupGamesDirectory());
    layout->addWidget(SetupAddonsDirectory());
    layout->addStretch();
    layout->addWidget(SetupDialogActions());

    setWindowTitle(tr("shadPS4 - Choose directory"));
    setWindowIcon(QIcon(":images/shadps4.ico"));
}

GameInstallDialog::~GameInstallDialog() {}

void GameInstallDialog::BrowseGamesDirectory() {
    auto path = QFileDialog::getExistingDirectory(this, tr("Directory to install games"));

    if (!path.isEmpty()) {
        m_gamesDirectory->setText(QDir::toNativeSeparators(path));
    }
}

void GameInstallDialog::BrowseAddonsDirectory() {
    auto path = QFileDialog::getExistingDirectory(this, tr("Directory to install DLC"));

    if (!path.isEmpty()) {
        m_addonsDirectory->setText(QDir::toNativeSeparators(path));
    }
}

QWidget* GameInstallDialog::SetupGamesDirectory() {
    auto group = new QGroupBox(tr("Directory to install games"));
    auto layout = new QHBoxLayout(group);

    // Input.
    m_gamesDirectory = new QLineEdit();
    QString install_dir;
    Common::FS::PathToQString(install_dir, Config::getGameInstallDir());
    m_gamesDirectory->setText(install_dir);
    m_gamesDirectory->setMinimumWidth(400);

    layout->addWidget(m_gamesDirectory);

    // Browse button.
    auto browse = new QPushButton(tr("Browse"));

    connect(browse, &QPushButton::clicked, this, &GameInstallDialog::BrowseGamesDirectory);

    layout->addWidget(browse);

    return group;
}

QWidget* GameInstallDialog::SetupAddonsDirectory() {
    auto group = new QGroupBox(tr("Directory to install DLC"));
    auto layout = new QHBoxLayout(group);

    // Input.
    m_addonsDirectory = new QLineEdit();
    QString install_dir;
    Common::FS::PathToQString(install_dir, Config::getAddonInstallDir());
    m_addonsDirectory->setText(install_dir);
    m_addonsDirectory->setMinimumWidth(400);

    layout->addWidget(m_addonsDirectory);

    // Browse button.
    auto browse = new QPushButton(tr("Browse"));

    connect(browse, &QPushButton::clicked, this, &GameInstallDialog::BrowseAddonsDirectory);

    layout->addWidget(browse);

    return group;
}

QWidget* GameInstallDialog::SetupDialogActions() {
    auto actions = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    connect(actions, &QDialogButtonBox::accepted, this, &GameInstallDialog::Save);
    connect(actions, &QDialogButtonBox::rejected, this, &GameInstallDialog::reject);

    return actions;
}

void GameInstallDialog::Save() {
    // Check games directory.
    auto gamesDirectory = m_gamesDirectory->text();
    auto addonsDirectory = m_addonsDirectory->text();

    if (gamesDirectory.isEmpty() || !QDir(gamesDirectory).exists() ||
        !QDir::isAbsolutePath(gamesDirectory)) {
        QMessageBox::critical(this, tr("Error"),
                              "The value for location to install games is not valid.");
        return;
    }

    if (addonsDirectory.isEmpty() || !QDir::isAbsolutePath(addonsDirectory)) {
        QMessageBox::critical(this, tr("Error"),
                              "The value for location to install DLC is not valid.");
        return;
    }
    QDir addonsDir(addonsDirectory);
    if (!addonsDir.exists()) {
        if (!addonsDir.mkpath(".")) {
            QMessageBox::critical(this, tr("Error"),
                                  "The DLC install location could not be created.");
            return;
        }
    }

    Config::setGameInstallDir(Common::FS::PathFromQString(gamesDirectory));
    Config::setAddonInstallDir(Common::FS::PathFromQString(addonsDirectory));
    const auto config_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
    Config::save(config_dir / "config.toml");
    accept();
}
