// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "game_install_dialog.h"

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
#include "gui_settings.h"

GameInstallDialog::GameInstallDialog(std::shared_ptr<GuiSettings> gui_settings)
    : m_gamesDirectory(nullptr), m_gui_settings(std::move(gui_settings)) {
    auto layout = new QVBoxLayout(this);

    layout->addWidget(SetupGamesDirectory());
    layout->addStretch();
    layout->addWidget(SetupDialogActions());

    setWindowTitle("Shadps4 - Choose directory");
}

GameInstallDialog::~GameInstallDialog() {}

void GameInstallDialog::Browse() {
    auto path = QFileDialog::getExistingDirectory(this, "Directory to install games");

    if (!path.isEmpty()) {
        m_gamesDirectory->setText(QDir::toNativeSeparators(path));
    }
}

QWidget* GameInstallDialog::SetupGamesDirectory() {
    auto group = new QGroupBox("Directory to install games");
    auto layout = new QHBoxLayout(group);

    // Input.
    m_gamesDirectory = new QLineEdit();
    m_gamesDirectory->setText(m_gui_settings->GetValue(gui::settings_install_dir).toString());
    m_gamesDirectory->setMinimumWidth(400);

    layout->addWidget(m_gamesDirectory);

    // Browse button.
    auto browse = new QPushButton("...");

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

    m_gui_settings->SetValue(gui::settings_install_dir, QDir::toNativeSeparators(gamesDirectory));

    accept();
}
