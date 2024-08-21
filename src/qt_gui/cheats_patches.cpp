// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCheckBox>
#include <QFile>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QString>
#include <QTabWidget>
#include <QVBoxLayout>
#include <common/logging/log.h>
#include "cheats_patches.h"
#include "common/path_util.h"
#include "core/module.h"

CheatsPatches::CheatsPatches(const QString& gameName, const QString& gameSerial,
                             const QString& gameVersion, const QString& gameSize,
                             const QPixmap& gameImage, QWidget* parent)
    : QWidget(parent), m_gameName(gameName), m_gameSerial(gameSerial), m_gameVersion(gameVersion),
      m_gameSize(gameSize), m_gameImage(gameImage) {
    setupUI();
    resize(850, 400);
    setWindowTitle("Cheats / Patches");
}

CheatsPatches::~CheatsPatches() {}

void CheatsPatches::setupUI() {
    const auto& CHEATS_DIR = Common::FS::GetUserPath(Common::FS::PathType::CheatsDir);
    QString CHEATS_DIR_QString = QString::fromStdString(CHEATS_DIR.string());
    QString NameCheatJson = m_gameSerial + "_" + m_gameVersion + ".json";
    m_cheatFilePath = CHEATS_DIR_QString + "/" + NameCheatJson;

    QHBoxLayout* mainLayout = new QHBoxLayout(this);

    // Create the game info group box
    QGroupBox* gameInfoGroupBox = new QGroupBox("Game Information");
    QVBoxLayout* gameInfoLayout = new QVBoxLayout(gameInfoGroupBox);
    gameInfoLayout->setAlignment(Qt::AlignTop);

    QLabel* gameImageLabel = new QLabel();
    if (!m_gameImage.isNull()) {
        gameImageLabel->setPixmap(m_gameImage.scaled(250, 250, Qt::KeepAspectRatio));
    } else {
        gameImageLabel->setText("No Image Available");
    }
    gameImageLabel->setAlignment(Qt::AlignCenter);
    gameInfoLayout->addWidget(gameImageLabel, 0, Qt::AlignCenter);

    QLabel* gameNameLabel = new QLabel(m_gameName);
    gameNameLabel->setAlignment(Qt::AlignLeft);
    gameNameLabel->setWordWrap(true);
    gameInfoLayout->addWidget(gameNameLabel);

    QLabel* gameSerialLabel = new QLabel("Serial: " + m_gameSerial);
    gameSerialLabel->setAlignment(Qt::AlignLeft);
    gameInfoLayout->addWidget(gameSerialLabel);

    QLabel* gameVersionLabel = new QLabel("Version: " + m_gameVersion);
    gameVersionLabel->setAlignment(Qt::AlignLeft);
    gameInfoLayout->addWidget(gameVersionLabel);

    QLabel* gameSizeLabel = new QLabel("Size: " + m_gameSize);
    gameSizeLabel->setAlignment(Qt::AlignLeft);
    gameInfoLayout->addWidget(gameSizeLabel);

    // Create the tab widget
    QTabWidget* tabWidget = new QTabWidget();
    QWidget* cheatsTab = new QWidget();
    QWidget* patchesTab = new QWidget();

    // Layouts for the tabs
    QVBoxLayout* cheatsLayout = new QVBoxLayout();
    QVBoxLayout* patchesLayout = new QVBoxLayout();

    // Setup the cheats tab
    QGroupBox* cheatsGroupBox = new QGroupBox();
    rightLayout = new QVBoxLayout(cheatsGroupBox);
    checkBoxStyle = "QCheckBox { font-size: 17px; }";
    buttonStyle = "QPushButton { font-size: 17px; }";
    rightLayout->setAlignment(Qt::AlignTop);

    loadCheats(m_cheatFilePath);
    cheatsGroupBox->setLayout(rightLayout);
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(cheatsGroupBox);
    cheatsLayout->addWidget(scrollArea);

    // Add a check for updates button
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* checkUpdateButton = new QPushButton("Download Cheats");
    checkUpdateButton->setStyleSheet(buttonStyle);
    connect(checkUpdateButton, &QPushButton::clicked, [=]() {
        if (QFile::exists(m_cheatFilePath)) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(this, "File Exists",
                                          "File already exists. Do you want to replace it?",
                                          QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::No) {
                return;
            }
        }

        const QString url =
            "https://raw.githubusercontent.com/GoldHEN/GoldHEN_Cheat_Repository/main/json/" +
            m_gameSerial + "_" + m_gameVersion + ".json";

        QNetworkAccessManager* manager = new QNetworkAccessManager(this);
        QNetworkRequest request(url);
        QNetworkReply* reply = manager->get(request);

        connect(reply, &QNetworkReply::finished, [=]() {
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray jsonData = reply->readAll();
                QFile cheatFile(m_cheatFilePath);
                if (cheatFile.open(QIODevice::WriteOnly)) {
                    cheatFile.write(jsonData);
                    cheatFile.close();
                    loadCheats(m_cheatFilePath);
                }
            } else {
                QMessageBox::warning(this, "Cheats not found",
                                     "No Cheats found for this game in this version.");
            }
            reply->deleteLater();
        });
    });
    buttonLayout->addWidget(checkUpdateButton);
    cheatsLayout->addLayout(buttonLayout);
    cheatsTab->setLayout(cheatsLayout);
    patchesTab->setLayout(patchesLayout);
    tabWidget->addTab(cheatsTab, "Cheats");
    tabWidget->addTab(patchesTab, "Patches");
    mainLayout->addWidget(gameInfoGroupBox, 1);
    mainLayout->addWidget(tabWidget, 3);
    setLayout(mainLayout);
}

void CheatsPatches::loadCheats(const QString& filePath) {
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray jsonData = file.readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        QJsonObject jsonObject = jsonDoc.object();
        QJsonArray modsArray = jsonObject["mods"].toArray();
        addMods(modsArray);
    }
}

void CheatsPatches::addMods(const QJsonArray& modsArray) {
    QLayoutItem* item;
    while ((item = rightLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    m_cheats.clear();

    int maxWidthButton = 0;

    for (const QJsonValue& modValue : modsArray) {
        QJsonObject modObject = modValue.toObject();
        QString modName = modObject["name"].toString();
        QString modType = modObject["type"].toString();

        Cheat cheat;
        cheat.name = modName;
        cheat.type = modType;

        QJsonArray memoryArray = modObject["memory"].toArray();
        for (const QJsonValue& memoryValue : memoryArray) {
            QJsonObject memoryObject = memoryValue.toObject();
            MemoryMod memoryMod;
            memoryMod.offset = memoryObject["offset"].toString();
            memoryMod.on = memoryObject["on"].toString();
            memoryMod.off = memoryObject["off"].toString();
            cheat.memoryMods.append(memoryMod);
        }

        m_cheats[modName] = cheat;

        if (modType == "checkbox") {
            QCheckBox* cheatCheckBox = new QCheckBox(modName);
            cheatCheckBox->setStyleSheet(checkBoxStyle);
            rightLayout->addWidget(cheatCheckBox);
            connect(cheatCheckBox, &QCheckBox::toggled,
                    [=](bool checked) { applyCheat(modName, checked); });
        } else if (modType == "button") {
            QPushButton* cheatButton = new QPushButton(modName);
            cheatButton->setStyleSheet(buttonStyle);
            cheatButton->adjustSize();
            int buttonWidth = cheatButton->sizeHint().width();
            if (buttonWidth > maxWidthButton) {
                maxWidthButton = buttonWidth;
            }

            // Create a horizontal layout for buttons
            QHBoxLayout* buttonLayout = new QHBoxLayout();
            buttonLayout->setContentsMargins(0, 0, 0, 0);
            buttonLayout->addWidget(cheatButton);
            buttonLayout->addStretch();

            rightLayout->addLayout(buttonLayout); // Add the layout to the main layout
            connect(cheatButton, &QPushButton::clicked, [=]() { applyCheat(modName, true); });
        }
    }

    // Set minimum and fixed size for all buttons
    for (int i = 0; i < rightLayout->count(); ++i) {
        QLayoutItem* layoutItem = rightLayout->itemAt(i);
        QWidget* widget = layoutItem->widget();
        if (widget) {
            QPushButton* button = qobject_cast<QPushButton*>(widget);
            if (button) {
                button->setMinimumWidth(maxWidthButton);
                button->setFixedWidth(maxWidthButton);
            }
        } else {
            QLayout* layout = layoutItem->layout();
            if (layout) {
                for (int j = 0; j < layout->count(); ++j) {
                    QLayoutItem* innerItem = layout->itemAt(j);
                    QWidget* innerWidget = innerItem->widget();
                    if (innerWidget) {
                        QPushButton* button = qobject_cast<QPushButton*>(innerWidget);
                        if (button) {
                            button->setMinimumWidth(maxWidthButton);
                            button->setFixedWidth(maxWidthButton);
                        }
                    }
                }
            }
        }
    }

    QLabel* creditsLabel = new QLabel();
    QString creditsText = "Author: ";
    QFile file(m_cheatFilePath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray jsonData = file.readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        QJsonObject jsonObject = jsonDoc.object();
        QJsonArray creditsArray = jsonObject["credits"].toArray();
        for (const QJsonValue& creditValue : creditsArray) {
            creditsText += creditValue.toString() + ", ";
        }
        if (creditsText.endsWith(", ")) {
            creditsText.chop(2);
        }
    }
    creditsLabel->setText(creditsText);
    creditsLabel->setAlignment(Qt::AlignLeft);
    rightLayout->addWidget(creditsLabel);
}

void CheatsPatches::applyCheat(const QString& modName, bool enabled) {
    if (!m_cheats.contains(modName))
        return;

    if (Core::g_eboot_address == 0) {
        QMessageBox::warning(this, "Cheats not found",
                             "Can't apply mod until a game has been started.");
        return;
    }

    Cheat cheat = m_cheats[modName];

    for (const MemoryMod& memoryMod : cheat.memoryMods) {
        QString value = enabled ? memoryMod.on : memoryMod.off;

        std::string modNameStr = modName.toStdString();
        std::string offsetStr = memoryMod.offset.toStdString();
        std::string valueStr = value.toStdString();

        LOG_INFO(Loader, "Cheat applied:{}, Offset:{}, Value:{}", modNameStr, offsetStr, valueStr);

        // Send a request to modify the process memory.
        void* cheatAddress =
            reinterpret_cast<void*>(Core::g_eboot_address + std::stoi(offsetStr, 0, 16));

        std::vector<unsigned char> bytePatch;

        for (size_t i = 0; i < valueStr.length(); i += 2) {
            unsigned char byte =
                static_cast<unsigned char>(std::strtol(valueStr.substr(i, 2).c_str(), nullptr, 16));

            bytePatch.push_back(byte);
        }
        std::memcpy(cheatAddress, bytePatch.data(), bytePatch.size());
    }
}
