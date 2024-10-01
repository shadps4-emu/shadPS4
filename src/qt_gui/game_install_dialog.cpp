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
    layout->addWidget(SetupUserDirectory());
    layout->addStretch();
    layout->addWidget(SetupDialogActions());

    setWindowTitle(tr("shadPS4 - Choose directory"));
    setWindowIcon(QIcon(":images/shadps4.ico"));
}

GameInstallDialog::~GameInstallDialog() {}

void GameInstallDialog::Browse(const QString& browseTitle, QLineEdit* browseDir) {
    auto path = QFileDialog::getExistingDirectory(this, browseTitle);

    if (!path.isEmpty()) {
        browseDir->setText(QDir::toNativeSeparators(path));
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

    connect(browse, &QPushButton::clicked, this,
            [this]() { Browse(tr("Directory to install games"), m_gamesDirectory); });

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

    connect(browse, &QPushButton::clicked, this,
            [this]() { Browse(tr("Directory to install DLC"), m_addonsDirectory); });

    layout->addWidget(browse);

    return group;
}

QWidget* GameInstallDialog::SetupUserDirectory() {
    auto group = new QGroupBox(tr("Location of user directory"));
    auto layout = new QHBoxLayout(group);

    m_userDirectory = new QLineEdit();
    QString user_dir;
    std::filesystem::path default_path =
        Config::getEmulatorUserDir().empty()
            ? Common::FS::GetUserPath(Common::FS::PathType::ConfigDir)
            : Config::getEmulatorUserDir();
    Common::FS::PathToQString(user_dir, default_path);
    m_userDirectory->setText(user_dir);
    m_userDirectory->setMinimumWidth(400);

    layout->addWidget(m_userDirectory);

    // Browse button.
    auto browse = new QPushButton(tr("Browse"));

    connect(browse, &QPushButton::clicked, this,
            [this]() { Browse(tr("Location of user directory"), m_userDirectory); });

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
    auto userDirectory = m_userDirectory->text();

    if (gamesDirectory.isEmpty() || !QDir(gamesDirectory).exists() ||
        !QDir::isAbsolutePath(gamesDirectory)) {
        QMessageBox::critical(this, tr("Error"),
                              tr("The value for location to install games is not valid."));
        return;
    }

    if (addonsDirectory.isEmpty() || !QDir(addonsDirectory).exists() ||
        !QDir::isAbsolutePath(addonsDirectory)) {
        QMessageBox::critical(this, tr("Error"),
                              "The value for location to install DLC is not valid.");
        return;
    }

    if (!userDirectory.endsWith("user", Qt::CaseInsensitive) || !QDir(userDirectory).exists() ||
        !QDir::isAbsolutePath(userDirectory)) {
        QMessageBox::critical(this, tr("Error"),
                              tr("The value for location of user directory is not valid."));
        return;
    }
    Config::setEmulatorUserDir(Common::FS::PathFromQString(userDirectory));
    Common::FS::SetUserPath(Common::FS::PathType::UserDir, Config::getEmulatorUserDir());

    Config::setGameInstallDir(Common::FS::PathFromQString(gamesDirectory));
    Config::setAddonInstallDir(Common::FS::PathFromQString(addonsDirectory));
    const auto config_dir = Common::FS::GetUserPath(Common::FS::PathType::ConfigDir);
    Config::save(config_dir / "config.toml");
    accept();
}
