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
#include <QListWidget>

GameInstallDialog::GameInstallDialog() : m_gamesDirectory(nullptr) {
    auto layout = new QVBoxLayout(this);

    layout->addWidget(SetupGamesDirectory());
    
    layout->addWidget(GameDirectoryList());
    // call list dialog here later

    layout->addStretch();
    layout->addWidget(SetupDialogActions());

    setWindowTitle("shadPS4 - Choose directory");
    setWindowIcon(QIcon(":images/shadps4.ico"));
}

GameInstallDialog::~GameInstallDialog() {}

QWidget* GameInstallDialog::GameDirectoryList() {
    auto group = new QGroupBox("Gamepaths");
    auto layout = new QHBoxLayout(group);

    // Input.

    /*
    newItem->setText(path);
    listWidget->insertItem(1, newItem);
    */
    // Browse button.
    //auto browse = new QPushButton("Browse");

    //connect(browse, &QPushButton::clicked, this, &GameInstallDialog::Browse);

    QListWidget* listWidget = new QListWidget(this);
    QFile file("shadPS4-gamepaths.txt");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString path = in.readLine();
            if (!path.isEmpty() && path.endsWith(";")) {
                path.chop(1);
                listWidget->addItem(path);
            }
        }
        file.close();
    }
    layout->addWidget(listWidget);

    return group;
}

QWidget* GameInstallDialog::GameDirectoryListDialog() {
    auto actions = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(actions, &QDialogButtonBox::accepted, this, &GameInstallDialog::Save);
    return actions;
}

void GameInstallDialog::Browse() {
    auto path = QFileDialog::getExistingDirectory(this, "Directory to install games");

    QFile file("shadPS4-gamepaths.txt");
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << path << ";\n";
        file.close();
    }

    if (!path.isEmpty()) {
        m_gamesDirectory->setText(QDir::toNativeSeparators(path));
    }
}

QWidget* GameInstallDialog::SetupGamesDirectory() {
    auto group = new QGroupBox("Directory to install games");
    auto layout = new QHBoxLayout(group);

    // Input.
    m_gamesDirectory = new QLineEdit();
    m_gamesDirectory->setText(QString::fromStdString(Config::getGameInstallDir()));
    m_gamesDirectory->setMinimumWidth(400);

    layout->addWidget(m_gamesDirectory);

    // Browse button.
    auto browse = new QPushButton("Browse");

    connect(browse, &QPushButton::clicked, this, &GameInstallDialog::Browse);

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

    if (gamesDirectory.isEmpty() || !QDir(gamesDirectory).exists() ||
        !QDir::isAbsolutePath(gamesDirectory)) {
        QMessageBox::critical(this, "Error",
                              "The value for location to install games is not valid.");
        return;
    }

    Config::setGameInstallDir(gamesDirectory.toStdString());
    const auto config_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
    Config::save(config_dir / "config.toml");
    accept();
}
