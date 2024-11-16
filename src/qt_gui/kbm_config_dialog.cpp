// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "kbm_config_dialog.h"
#include "kbm_help_dialog.h"

#include <fstream>
#include <iostream>
#include "common/config.h"
#include "common/path_util.h"
#include "game_info.h"
#include "src/sdl_window.h"

#include <QCloseEvent>
#include <QComboBox>
#include <QFile>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QTextStream>
#include <QVBoxLayout>

EditorDialog::EditorDialog(QWidget* parent) : QDialog(parent) {

    setWindowTitle("Edit Config File");
    resize(600, 400);

    // Create the editor widget
    editor = new QPlainTextEdit(this);
    editorFont.setPointSize(10); // Set default text size
    editor->setFont(editorFont); // Apply font to the editor

    // Create the game selection combo box
    gameComboBox = new QComboBox(this);
    gameComboBox->addItem("default"); // Add default option
                                      /*
                                          gameComboBox = new QComboBox(this);
                                          layout->addWidget(gameComboBox); // Add the combobox for selecting game configurations
                                  
                                          // Populate the combo box with game configurations
                                          QStringList gameConfigs = GameInfoClass::GetGameInfo(this);
                                          gameComboBox->addItems(gameConfigs);
                                          gameComboBox->setCurrentText("default.ini"); // Set the default selection
                                      */
    // Load all installed games
    loadInstalledGames();

    // Create Save, Cancel, and Help buttons
    QPushButton* saveButton = new QPushButton("Save", this);
    QPushButton* cancelButton = new QPushButton("Cancel", this);
    QPushButton* helpButton = new QPushButton("Help", this);

    // Layout for the game selection and buttons
    QHBoxLayout* topLayout = new QHBoxLayout();
    topLayout->addWidget(gameComboBox);
    topLayout->addStretch();
    topLayout->addWidget(saveButton);
    topLayout->addWidget(cancelButton);
    topLayout->addWidget(helpButton);

    // Main layout with editor and buttons
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addLayout(topLayout);
    layout->addWidget(editor);

    // Load the default config file content into the editor
    loadFile(gameComboBox->currentText());

    // Connect button and combo box signals
    connect(saveButton, &QPushButton::clicked, this, &EditorDialog::onSaveClicked);
    connect(cancelButton, &QPushButton::clicked, this, &EditorDialog::onCancelClicked);
    connect(helpButton, &QPushButton::clicked, this, &EditorDialog::onHelpClicked);
    connect(gameComboBox, &QComboBox::currentTextChanged, this,
            &EditorDialog::onGameSelectionChanged);
}

void EditorDialog::loadFile(QString game) {

    // to make sure the files and the directory do exist
    const auto config_file = Config::GetFoolproofKbmConfigFile(game.toStdString());
    QFile file(config_file);

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        editor->setPlainText(in.readAll());
        originalConfig = editor->toPlainText();
        file.close();
    } else {
        QMessageBox::warning(this, "Error", "Could not open the file for reading");
    }
}

void EditorDialog::saveFile(QString game) {

    // to make sure the files and the directory do exist
    const auto config_file = Config::GetFoolproofKbmConfigFile(game.toStdString());
    QFile file(config_file);

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << editor->toPlainText();
        file.close();
    } else {
        QMessageBox::warning(this, "Error", "Could not open the file for writing");
    }
}

// Override the close event to show the save confirmation dialog only if changes were made
void EditorDialog::closeEvent(QCloseEvent* event) {
    if (hasUnsavedChanges()) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Save Changes", "Do you want to save changes?",
                                      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if (reply == QMessageBox::Yes) {
            saveFile(gameComboBox->currentText());
            event->accept(); // Close the dialog
        } else if (reply == QMessageBox::No) {
            event->accept(); // Close the dialog without saving
        } else {
            event->ignore(); // Cancel the close event
        }
    } else {
        event->accept(); // No changes, close the dialog without prompting
    }
}
void EditorDialog::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        close(); // Trigger the close action, same as pressing the close button
    } else {
        QDialog::keyPressEvent(event); // Call the base class implementation for other keys
    }
}

void EditorDialog::onSaveClicked() {
    saveFile(gameComboBox->currentText());
    reject(); // Close the dialog
}

void EditorDialog::onCancelClicked() {
    reject(); // Close the dialog
}
bool isHelpOpen = false;
HelpDialog* helpDialog;
void EditorDialog::onHelpClicked() {
    if (!isHelpOpen) {
        helpDialog = new HelpDialog(this);
        helpDialog->setWindowTitle("Help");
        helpDialog->setAttribute(Qt::WA_DeleteOnClose); // Clean up on close
        // Get the position and size of the Config window
        QRect configGeometry = this->geometry();
        int helpX = configGeometry.x() + configGeometry.width() + 10; // 10 pixels offset
        int helpY = configGeometry.y();
        // Move the Help dialog to the right side of the Config window
        helpDialog->move(helpX, helpY);
        helpDialog->show();
        isHelpOpen = true;
    } else {
        helpDialog->close();
        isHelpOpen = false;
    }
}

bool EditorDialog::hasUnsavedChanges() {
    // Compare the current content with the original content to check if there are unsaved changes
    return editor->toPlainText() != originalConfig;
}
void EditorDialog::loadInstalledGames() {
    QStringList filePaths;
    for (const auto& installLoc : Config::getGameInstallDirs()) {
        QString installDir;
        Common::FS::PathToQString(installDir, installLoc);
        QDir parentFolder(installDir);
        QFileInfoList fileList = parentFolder.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const auto& fileInfo : fileList) {
            if (fileInfo.isDir() && !fileInfo.filePath().endsWith("-UPDATE")) {
                gameComboBox->addItem(fileInfo.fileName()); // Add game name to combo box
            }
        }
    }
}
QString previousGame = "default";
void EditorDialog::onGameSelectionChanged(const QString& game) {
    saveFile(previousGame);
    loadFile(gameComboBox->currentText()); // Reload file based on the selected game
    previousGame = gameComboBox->currentText();
}
