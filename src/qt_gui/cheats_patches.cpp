﻿// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QComboBox>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHoverEvent>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QListView>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QString>
#include <QStringListModel>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QXmlStreamReader>
#include <common/logging/log.h>
#include "cheats_patches.h"
#include "common/memory_patcher.h"
#include "common/path_util.h"
#include "core/module.h"

using namespace Common::FS;

CheatsPatches::CheatsPatches(const QString& gameName, const QString& gameSerial,
                             const QString& gameVersion, const QString& gameSize,
                             const QPixmap& gameImage, QWidget* parent)
    : QWidget(parent), m_gameName(gameName), m_gameSerial(gameSerial), m_gameVersion(gameVersion),
      m_gameSize(gameSize), m_gameImage(gameImage), manager(new QNetworkAccessManager(this)) {
    setupUI();
    resize(700, 350);
    setWindowTitle("Cheats / Patches");
}

CheatsPatches::~CheatsPatches() {}

QString defaultTextEdit =
    ("Cheat/Patches are experimental. Use with caution.\n\n"
     "Download the cheats individually by selecting the repository and clicking the download "
     "button.\nIn the Patch tab you can download all Patches at once, choose which one you want "
     "to use and save the option.\n\n"
     "As we do not develop the Cheat/Patches,\nplease report issues related to the cheat author.");

void CheatsPatches::setupUI() {
    QString CHEATS_DIR_QString =
        QString::fromStdString(Common::FS::GetUserPath(Common::FS::PathType::CheatsDir).string());
    QString NameCheatJson = m_gameSerial + "_" + m_gameVersion + ".json";
    m_cheatFilePath = CHEATS_DIR_QString + "/" + NameCheatJson;

    QHBoxLayout* mainLayout = new QHBoxLayout(this);

    // Create the game info group box
    QGroupBox* gameInfoGroupBox = new QGroupBox();
    QVBoxLayout* gameInfoLayout = new QVBoxLayout(gameInfoGroupBox);
    gameInfoLayout->setAlignment(Qt::AlignTop);

    QLabel* gameImageLabel = new QLabel();
    if (!m_gameImage.isNull()) {
        gameImageLabel->setPixmap(m_gameImage.scaled(275, 275, Qt::KeepAspectRatio));
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

    // Add a text area for instructions and 'Patch' descriptions
    instructionsTextEdit = new QTextEdit();
    instructionsTextEdit->setText(defaultTextEdit);
    instructionsTextEdit->setReadOnly(true);
    instructionsTextEdit->setFixedHeight(170);
    gameInfoLayout->addWidget(instructionsTextEdit);

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
    rightLayout->setAlignment(Qt::AlignTop);

    cheatsGroupBox->setLayout(rightLayout);
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(cheatsGroupBox);
    scrollArea->setMinimumHeight(400);
    cheatsLayout->addWidget(scrollArea);

    // QListView
    listView_selectFile = new QListView();
    listView_selectFile->setMaximumHeight(66);
    listView_selectFile->setSelectionMode(QAbstractItemView::SingleSelection);
    listView_selectFile->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Add QListView to layout
    QVBoxLayout* fileListLayout = new QVBoxLayout();
    fileListLayout->addWidget(new QLabel("Select Cheat File:"));
    fileListLayout->addWidget(listView_selectFile);
    cheatsLayout->addLayout(fileListLayout, 2);

    // Call the method to fill the list of cheat files
    populateFileListCheats();

    QLabel* repositoryLabel = new QLabel("Repository:");
    repositoryLabel->setAlignment(Qt::AlignLeft);
    repositoryLabel->setAlignment(Qt::AlignVCenter);

    // Add a combo box and a download button
    QHBoxLayout* controlLayout = new QHBoxLayout();
    controlLayout->addWidget(repositoryLabel);
    controlLayout->setAlignment(Qt::AlignLeft);
    QComboBox* downloadComboBox = new QComboBox();

    downloadComboBox->addItem("wolf2022", "wolf2022");
    downloadComboBox->addItem("GoldHEN", "GoldHEN");
    downloadComboBox->addItem("shadPS4", "shadPS4");

    controlLayout->addWidget(downloadComboBox);

    QPushButton* downloadButton = new QPushButton("Download Cheats");
    connect(downloadButton, &QPushButton::clicked, [=]() {
        QString source = downloadComboBox->currentData().toString();
        downloadCheats(source, m_gameSerial, m_gameVersion, true);
    });

    QPushButton* deleteCheatButton = new QPushButton("Delete File");
    connect(deleteCheatButton, &QPushButton::clicked, [=]() {
        QStringListModel* model = qobject_cast<QStringListModel*>(listView_selectFile->model());
        if (!model) {
            return;
        }
        QItemSelectionModel* selectionModel = listView_selectFile->selectionModel();
        if (!selectionModel) {
            return;
        }
        QModelIndexList selectedIndexes = selectionModel->selectedIndexes();
        if (selectedIndexes.isEmpty()) {
            QMessageBox::warning(
                this, "Delete Cheat File",
                "No files selected.\n"
                "You can delete the cheats you don't want after downloading them.");
            return;
        }
        QModelIndex selectedIndex = selectedIndexes.first();
        QString selectedFileName = model->data(selectedIndex).toString();

        int ret = QMessageBox::warning(
            this, "Delete Cheat File",
            QString("Do you want to delete the selected file?\n%1").arg(selectedFileName),
            QMessageBox::Yes | QMessageBox::No);

        if (ret == QMessageBox::Yes) {
            QString filePath = CHEATS_DIR_QString + "/" + selectedFileName;
            QFile::remove(filePath);
            populateFileListCheats();
        }
    });

    controlLayout->addWidget(downloadButton);
    controlLayout->addWidget(deleteCheatButton);

    cheatsLayout->addLayout(controlLayout);
    cheatsTab->setLayout(cheatsLayout);

    // Setup the patches tab
    QGroupBox* patchesGroupBox = new QGroupBox();
    patchesGroupBoxLayout = new QVBoxLayout(patchesGroupBox);
    patchesGroupBoxLayout->setAlignment(Qt::AlignTop);
    patchesGroupBox->setLayout(patchesGroupBoxLayout);

    QScrollArea* patchesScrollArea = new QScrollArea();
    patchesScrollArea->setWidgetResizable(true);
    patchesScrollArea->setWidget(patchesGroupBox);
    patchesLayout->addWidget(patchesScrollArea);

    QHBoxLayout* patchesControlLayout = new QHBoxLayout();
    QComboBox* patchesComboBox = new QComboBox();

    QPushButton* patchesButton = new QPushButton("Download All Available Patches");
    connect(patchesButton, &QPushButton::clicked, [=]() { downloadPatches(); });
    patchesControlLayout->addWidget(patchesButton);

    QPushButton* saveButton = new QPushButton("Save");
    connect(saveButton, &QPushButton::clicked, this, &CheatsPatches::onSaveButtonClicked);

    patchesControlLayout->addWidget(saveButton);

    patchesLayout->addLayout(patchesControlLayout);
    patchesTab->setLayout(patchesLayout);

    tabWidget->addTab(cheatsTab, "CHEATS");
    tabWidget->addTab(patchesTab, "PATCHES");

    // Connect the currentChanged signal to the onTabChanged slot
    connect(tabWidget, &QTabWidget::currentChanged, this, &CheatsPatches::onTabChanged);

    mainLayout->addWidget(gameInfoGroupBox, 1);
    mainLayout->addWidget(tabWidget, 3);

    manager = new QNetworkAccessManager(this);

    setLayout(mainLayout);
}

void CheatsPatches::onSaveButtonClicked() {
    QString patchDir =
        QString::fromStdString(Common::FS::GetUserPath(Common::FS::PathType::PatchesDir).string());

    QString filesJsonPath = patchDir + "/files.json";
    QFile jsonFile(filesJsonPath);
    if (!jsonFile.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Error", "Unable to open files.json for reading.");
        return;
    }

    QByteArray jsonData = jsonFile.readAll();
    jsonFile.close();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    QJsonObject jsonObject = jsonDoc.object();

    QString selectedFileName;
    QString serial = m_gameSerial;

    for (auto it = jsonObject.constBegin(); it != jsonObject.constEnd(); ++it) {
        QString filePath = it.key();
        QJsonArray idsArray = it.value().toArray();

        if (idsArray.contains(QJsonValue(serial))) {
            selectedFileName = filePath;
            break;
        }
    }

    if (selectedFileName.isEmpty()) {
        QMessageBox::critical(this, "Error", "No patch file found for the current serial.");
        return;
    }

    QString filePath = patchDir + "/" + selectedFileName;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Unable to open the file for reading.");
        return;
    }

    QByteArray xmlData = file.readAll();
    file.close();

    QString newXmlData;
    QXmlStreamWriter xmlWriter(&newXmlData);
    xmlWriter.setAutoFormatting(true);
    xmlWriter.writeStartDocument();

    QXmlStreamReader xmlReader(xmlData);
    bool insideMetadata = false;

    while (!xmlReader.atEnd()) {
        xmlReader.readNext();

        if (xmlReader.isStartElement()) {
            if (xmlReader.name() == QStringLiteral("Metadata")) {
                insideMetadata = true;
                xmlWriter.writeStartElement(xmlReader.name().toString());

                QString name = xmlReader.attributes().value("Name").toString();
                bool isEnabled = false;
                bool hasIsEnabled = false;

                // Check and update the isEnabled attribute
                for (const QXmlStreamAttribute& attr : xmlReader.attributes()) {
                    if (attr.name() == QStringLiteral("isEnabled")) {
                        hasIsEnabled = true;
                        auto it = m_patchInfos.find(name);
                        if (it != m_patchInfos.end()) {
                            QCheckBox* checkBox = findCheckBoxByName(it->name);
                            if (checkBox) {
                                isEnabled = checkBox->isChecked();
                                xmlWriter.writeAttribute("isEnabled", isEnabled ? "true" : "false");
                            }
                        }
                    } else {
                        xmlWriter.writeAttribute(attr.name().toString(), attr.value().toString());
                    }
                }

                if (!hasIsEnabled) {
                    auto it = m_patchInfos.find(name);
                    if (it != m_patchInfos.end()) {
                        QCheckBox* checkBox = findCheckBoxByName(it->name);
                        if (checkBox) {
                            isEnabled = checkBox->isChecked();
                        }
                    }
                    xmlWriter.writeAttribute("isEnabled", isEnabled ? "true" : "false");
                }
            } else {
                xmlWriter.writeStartElement(xmlReader.name().toString());
                for (const QXmlStreamAttribute& attr : xmlReader.attributes()) {
                    xmlWriter.writeAttribute(attr.name().toString(), attr.value().toString());
                }
            }
        } else if (xmlReader.isEndElement()) {
            if (xmlReader.name() == QStringLiteral("Metadata")) {
                insideMetadata = false;
            }
            xmlWriter.writeEndElement();
        } else if (xmlReader.isCharacters() && !xmlReader.isWhitespace()) {
            xmlWriter.writeCharacters(xmlReader.text().toString());
        }
    }

    xmlWriter.writeEndDocument();

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Unable to open the file for writing.");
        return;
    }

    QTextStream textStream(&file);
    textStream << newXmlData;
    file.close();

    if (xmlReader.hasError()) {
        QMessageBox::critical(this, "Error", "Failed to parse XML: " + xmlReader.errorString());
    } else {
        QMessageBox::information(this, "Success", "Options saved successfully.");
    }
}

QCheckBox* CheatsPatches::findCheckBoxByName(const QString& name) {
    for (int i = 0; i < patchesGroupBoxLayout->count(); ++i) {
        QLayoutItem* item = patchesGroupBoxLayout->itemAt(i);
        if (item) {
            QWidget* widget = item->widget();
            QCheckBox* checkBox = qobject_cast<QCheckBox*>(widget);
            if (checkBox) {
                if (checkBox->text() == name) {
                    return checkBox;
                }
            }
        }
    }
    return nullptr;
}

void CheatsPatches::downloadCheats(const QString& source, const QString& m_gameSerial,
                                   const QString& m_gameVersion, const bool showMessageBox) {
    QDir dir(Common::FS::GetUserPath(Common::FS::PathType::CheatsDir));
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString url;
    if (source == "GoldHEN") {
        url = "https://raw.githubusercontent.com/GoldHEN/GoldHEN_Cheat_Repository/main/json.txt";
    } else if (source == "wolf2022") {
        url = "https://wolf2022.ir/trainer/" + m_gameSerial + "_" + m_gameVersion + ".json";
    } else if (source == "shadPS4") {
        url = "https://raw.githubusercontent.com/shadps4-emu/ps4_cheats/main/"
              "CHEATS_JSON.txt";
    } else {
        QMessageBox::warning(this, "Invalid Source", "The selected source is invalid.");
        return;
    }

    QNetworkRequest request(url);
    QNetworkReply* reply = manager->get(request);

    connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray jsonData = reply->readAll();
            bool foundFiles = false;

            if (source == "GoldHEN" || source == "shadPS4") {
                QString textContent(jsonData);
                QRegularExpression regex(
                    QString("%1_%2[^=]*\.json").arg(m_gameSerial).arg(m_gameVersion));
                QRegularExpressionMatchIterator matches = regex.globalMatch(textContent);
                QString baseUrl;

                if (source == "GoldHEN") {
                    baseUrl = "https://raw.githubusercontent.com/GoldHEN/GoldHEN_Cheat_Repository/"
                              "main/json/";
                } else {
                    baseUrl = "https://raw.githubusercontent.com/shadps4-emu/ps4_cheats/"
                              "main/CHEATS/";
                }

                while (matches.hasNext()) {
                    QRegularExpressionMatch match = matches.next();
                    QString fileName = match.captured(0);

                    if (!fileName.isEmpty()) {
                        QString newFileName = fileName;
                        int dotIndex = newFileName.lastIndexOf('.');
                        if (dotIndex != -1) {

                            if (source == "GoldHEN") {
                                newFileName.insert(dotIndex, "_GoldHEN");
                            } else {
                                newFileName.insert(dotIndex, "_shadPS4");
                            }
                        }
                        QString fileUrl = baseUrl + fileName;
                        QString localFilePath = dir.filePath(newFileName);

                        if (QFile::exists(localFilePath) && showMessageBox) {
                            QMessageBox::StandardButton reply;
                            reply = QMessageBox::question(
                                this, "File Exists",
                                "File already exists. Do you want to replace it?",
                                QMessageBox::Yes | QMessageBox::No);
                            if (reply == QMessageBox::No) {
                                continue;
                            }
                        }
                        QNetworkRequest fileRequest(fileUrl);
                        QNetworkReply* fileReply = manager->get(fileRequest);

                        connect(fileReply, &QNetworkReply::finished, [=]() {
                            if (fileReply->error() == QNetworkReply::NoError) {
                                QByteArray fileData = fileReply->readAll();
                                QFile localFile(localFilePath);
                                if (localFile.open(QIODevice::WriteOnly)) {
                                    localFile.write(fileData);
                                    localFile.close();
                                } else {
                                    QMessageBox::warning(
                                        this, "Error Saving",
                                        QString("Failed to save file: \n %1").arg(localFilePath));
                                }
                            } else {
                                QMessageBox::warning(
                                    this, "Failed to Download",
                                    QString("Failed to download file: %1\n\nError: %2")
                                        .arg(fileUrl)
                                        .arg(fileReply->errorString()));
                            }
                            fileReply->deleteLater();
                        });

                        foundFiles = true;
                    }
                }
                if (!foundFiles && showMessageBox) {
                    QMessageBox::warning(
                        this, "Cheats Not Found1",
                        "No Cheats found for this game in this version of the selected "
                        "repository,\n"
                        "try another repository or a different version of the game.");
                }
            } else if (source == "wolf2022") {
                QString fileName = QFileInfo(QUrl(url).path()).fileName();
                QString baseFileName = fileName;
                int dotIndex = baseFileName.lastIndexOf('.');
                if (dotIndex != -1) {
                    baseFileName.insert(dotIndex, "_wolf2022");
                }
                QString filePath =
                    QString::fromStdString(
                        Common::FS::GetUserPath(Common::FS::PathType::CheatsDir).string()) +
                    "/" + baseFileName;
                if (QFile::exists(filePath) && showMessageBox) {
                    QMessageBox::StandardButton reply2;
                    reply2 = QMessageBox::question(
                        this, "File Exists", "File already exists. Do you want to replace it?",
                        QMessageBox::Yes | QMessageBox::No);
                    if (reply2 == QMessageBox::No) {
                        reply->deleteLater();
                        return;
                    }
                }
                QFile cheatFile(filePath);
                if (cheatFile.open(QIODevice::WriteOnly)) {
                    cheatFile.write(jsonData);
                    cheatFile.close();
                    foundFiles = true;
                    populateFileListCheats();
                } else {
                    QMessageBox::warning(this, "Error Saving",
                                         QString("Failed to save file:\n %1").arg(filePath));
                }
            }
            if (foundFiles && showMessageBox) {
                QMessageBox::information(
                    this, "Cheats Downloaded Successfully",
                    "You have successfully downloaded the cheats\n"
                    "for this version of the game from the selected repository.\n\n"
                    "You can try downloading from another repository, if it is available "
                    "it will also be possible to use it by selecting the file from the list.");
                populateFileListCheats();
            }

        } else {
            if (showMessageBox) {
                QMessageBox::warning(
                    this, "Cheats Not Found",
                    "No Cheats found for this game in this version of the selected "
                    "repository,\n"
                    "try another repository or a different version of the game.");
            }
        }
        reply->deleteLater();
        emit downloadFinished();
    });

    // connect(reply, &QNetworkReply::errorOccurred, [=](QNetworkReply::NetworkError code) {
    //     if (showMessageBox)
    //         QMessageBox::warning(this, "Download Error",
    //                              QString("Error in response: %1").arg(reply->errorString()));
    // });
}

void CheatsPatches::onTabChanged(int index) {
    if (index == 1) {
        loadPatches(m_gameSerial);
    }
}

void CheatsPatches::downloadPatches() {
    QString url = "https://github.com/GoldHEN/GoldHEN_Patch_Repository/tree/main/"
                  "patches/xml";
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);

    QNetworkRequest request(url);
    QNetworkReply* reply = manager->get(request);

    connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray htmlData = reply->readAll();
            reply->deleteLater();

            // Parsear HTML e extrair JSON usando QRegularExpression
            QString htmlString = QString::fromUtf8(htmlData);
            QRegularExpression jsonRegex(
                R"(<script type="application/json" data-target="react-app.embeddedData">(.+?)</script>)");
            QRegularExpressionMatch match = jsonRegex.match(htmlString);

            if (match.hasMatch()) {
                QByteArray jsonData = match.captured(1).toUtf8();
                QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
                QJsonObject jsonObj = jsonDoc.object();
                QJsonArray itemsArray =
                    jsonObj["payload"].toObject()["tree"].toObject()["items"].toArray();

                QDir dir(Common::FS::GetUserPath(Common::FS::PathType::PatchesDir));

                if (!dir.exists()) {
                    std::filesystem::create_directory(
                        Common::FS::GetUserPath(Common::FS::PathType::PatchesDir));
                }
                foreach (const QJsonValue& value, itemsArray) {
                    QJsonObject fileObj = value.toObject();
                    QString fileName = fileObj["name"].toString();
                    QString filePath = fileObj["path"].toString();

                    if (fileName.endsWith(".xml")) {
                        QString fileUrl = QString("https://raw.githubusercontent.com/GoldHEN/"
                                                  "GoldHEN_Patch_Repository/main/%1")
                                              .arg(filePath);
                        QNetworkRequest fileRequest(fileUrl);
                        QNetworkReply* fileReply = manager->get(fileRequest);

                        connect(fileReply, &QNetworkReply::finished, [=]() {
                            if (fileReply->error() == QNetworkReply::NoError) {
                                QByteArray fileData = fileReply->readAll();
                                QFile localFile(dir.filePath(fileName));
                                if (localFile.open(QIODevice::WriteOnly)) {
                                    localFile.write(fileData);
                                    localFile.close();
                                } else {
                                    QMessageBox::warning(
                                        this, "File Error",
                                        QString("Failed to save: %1").arg(fileName));
                                }
                            } else {
                                QMessageBox::warning(
                                    this, "Download Error",
                                    QString("Failed to download: %1").arg(fileUrl));
                            }
                            fileReply->deleteLater();
                        });
                    }
                }

                QMessageBox::information(
                    this, "Download Complete",
                    QString(
                        "Patches Downloaded Successfully!\n"
                        "All Patches available for all games have been downloaded, there is no "
                        "need to download them individually for each game as happens in Cheats."));

                // Create the files.json file with the identification of which file to open
                createFilesJson();
                loadPatches(m_gameSerial);

            } else {
                QMessageBox::warning(this, "Data Error", "Failed to parse JSON data from HTML.");
            }
        } else {
            QMessageBox::warning(this, "Network Error", "Failed to retrieve HTML page.");
        }
    });
}

void CheatsPatches::createFilesJson() {
    QString patchesDir =
        QString::fromStdString(Common::FS::GetUserPath(Common::FS::PathType::PatchesDir).string());
    QDir dir(patchesDir);

    if (!dir.exists()) {
        QMessageBox::warning(this, "ERROR Directory",
                             QString("Directory does not exist:\n %1").arg(patchesDir));
        return;
    }

    QJsonObject filesObject;
    QStringList xmlFiles = dir.entryList(QStringList() << "*.xml", QDir::Files);

    foreach (const QString& xmlFile, xmlFiles) {
        QFile file(dir.filePath(xmlFile));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "ERROR File",
                                 QString("Failed to open file: %1").arg(xmlFile));
            continue;
        }

        QXmlStreamReader xmlReader(&file);
        QJsonArray titleIdsArray;

        while (!xmlReader.atEnd() && !xmlReader.hasError()) {
            QXmlStreamReader::TokenType token = xmlReader.readNext();
            if (token == QXmlStreamReader::StartElement) {
                if (xmlReader.name() == QStringLiteral("ID")) {
                    titleIdsArray.append(xmlReader.readElementText());
                }
            }
        }

        if (xmlReader.hasError()) {
            QMessageBox::warning(this, "XML ERROR",
                                 QString("XML ERROR:\n %1").arg(xmlReader.errorString()));
        }
        filesObject[xmlFile] = titleIdsArray;
    }

    QFile jsonFile(patchesDir + "/files.json");
    if (!jsonFile.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "ERROR files.json", "Failed to open files.json for writing");
        return;
    }

    QJsonDocument jsonDoc(filesObject);
    jsonFile.write(jsonDoc.toJson());
    jsonFile.close();
}

void CheatsPatches::addCheatsToLayout(const QJsonArray& modsArray, const QJsonArray& creditsArray) {
    QLayoutItem* item;
    while ((item = rightLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    m_cheats.clear();
    m_cheatCheckBoxes.clear();

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

        // Check for the presence of 'hint' field
        cheat.hasHint = modObject.contains("hint");

        m_cheats[modName] = cheat;

        if (modType == "checkbox") {
            QCheckBox* cheatCheckBox = new QCheckBox(modName);
            rightLayout->addWidget(cheatCheckBox);
            m_cheatCheckBoxes.append(cheatCheckBox);
            connect(cheatCheckBox, &QCheckBox::toggled,
                    [=](bool checked) { applyCheat(modName, checked); });
        } else if (modType == "button") {
            QPushButton* cheatButton = new QPushButton(modName);
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

            rightLayout->addLayout(buttonLayout);
            connect(cheatButton, &QPushButton::clicked, [=]() { applyCheat(modName, true); });
        }
    }

    // Set minimum and fixed size for all buttons + 20
    for (int i = 0; i < rightLayout->count(); ++i) {
        QLayoutItem* layoutItem = rightLayout->itemAt(i);
        QWidget* widget = layoutItem->widget();
        if (widget) {
            QPushButton* button = qobject_cast<QPushButton*>(widget);
            if (button) {
                button->setMinimumWidth(maxWidthButton);
                button->setFixedWidth(maxWidthButton + 20);
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
                            button->setFixedWidth(maxWidthButton + 20);
                        }
                    }
                }
            }
        }
    }

    // Set credits label
    QLabel* creditsLabel = new QLabel();
    QString creditsText = "Author: ";
    if (!creditsArray.isEmpty()) {
        creditsText += creditsArray[0].toString();
    }
    creditsLabel->setText(creditsText);
    creditsLabel->setAlignment(Qt::AlignLeft);
    rightLayout->addWidget(creditsLabel);
}

void CheatsPatches::populateFileListCheats() {
    QString cheatsDir =
        QString::fromStdString(Common::FS::GetUserPath(Common::FS::PathType::CheatsDir).string());
    QString pattern = m_gameSerial + "_" + m_gameVersion + "*.json";

    QDir dir(cheatsDir);
    QStringList filters;
    filters << pattern;
    dir.setNameFilters(filters);

    QFileInfoList fileList = dir.entryInfoList(QDir::Files);
    QStringList fileNames;

    for (const QFileInfo& fileInfo : fileList) {
        fileNames << fileInfo.fileName();
    }

    QStringListModel* model = new QStringListModel(fileNames, this);
    listView_selectFile->setModel(model);

    connect(listView_selectFile->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            [this]() {
                QModelIndexList selectedIndexes =
                    listView_selectFile->selectionModel()->selectedIndexes();
                if (!selectedIndexes.isEmpty()) {
                    onFileSelected(selectedIndexes.first());
                }
            });

    if (!fileNames.isEmpty()) {
        QModelIndex firstIndex = model->index(0, 0);
        listView_selectFile->selectionModel()->select(firstIndex, QItemSelectionModel::Select |
                                                                      QItemSelectionModel::Rows);
        listView_selectFile->setCurrentIndex(firstIndex);
    }
}

void CheatsPatches::onFileSelected(const QModelIndex& index) {
    QString selectedFileName = index.data().toString();
    QString cheatsDir =
        QString::fromStdString(Common::FS::GetUserPath(Common::FS::PathType::CheatsDir).string());
    QString filePath = cheatsDir + "/" + selectedFileName;

    loadCheats(filePath);
}

void CheatsPatches::loadCheats(const QString& filePath) {
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray jsonData = file.readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        QJsonObject jsonObject = jsonDoc.object();
        QJsonArray modsArray = jsonObject["mods"].toArray();
        QJsonArray creditsArray = jsonObject["credits"].toArray();
        addCheatsToLayout(modsArray, creditsArray);
    }
}

void CheatsPatches::loadPatches(const QString& serial) {
    QLayoutItem* item;
    while ((item = patchesGroupBoxLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    m_patchInfos.clear();

    QString patchDir =
        QString::fromStdString(Common::FS::GetUserPath(Common::FS::PathType::PatchesDir).string());
    QString filesJsonPath = patchDir + "/files.json";

    QFile jsonFile(filesJsonPath);
    if (!jsonFile.open(QIODevice::ReadOnly)) {
        // QMessageBox::warning(this, "ERRO", "Failed to open files.json for reading.");
        return;
    }

    QByteArray jsonData = jsonFile.readAll();
    jsonFile.close();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    QJsonObject jsonObject = jsonDoc.object();

    for (auto it = jsonObject.constBegin(); it != jsonObject.constEnd(); ++it) {
        QString filePath = it.key();
        QFile file(patchDir + "/" + filePath);
        QJsonArray idsArray = it.value().toArray();

        if (idsArray.contains(QJsonValue(serial))) {
            if (!file.open(QIODevice::ReadOnly)) {
                QMessageBox::warning(this, "ERRO",
                                     QString("Failed to open file:: %1").arg(file.fileName()));
                continue;
            }

            QXmlStreamReader xmlReader(&file);
            QString patchName;
            QString patchAuthor;
            QString patchNote;
            QJsonArray patchLines;
            bool isEnabled = false;

            while (!xmlReader.atEnd() && !xmlReader.hasError()) {
                xmlReader.readNext();

                if (xmlReader.tokenType() == QXmlStreamReader::StartElement) {
                    if (xmlReader.name() == QStringLiteral("Metadata")) {
                        QXmlStreamAttributes attributes = xmlReader.attributes();
                        QString appVer = attributes.value("AppVer").toString();
                        if (appVer == m_gameVersion) {
                            patchName = attributes.value("Name").toString();
                            patchAuthor = attributes.value("Author").toString();
                            patchNote = attributes.value("Note").toString();
                            isEnabled = isEnabled =
                                attributes.value("isEnabled").toString() == QStringLiteral("true");
                        }
                        if (appVer == "mask") {
                            patchName = attributes.value("Name").toString() + " (any version)";
                            patchAuthor = attributes.value("Author").toString();
                            patchNote = attributes.value("Note").toString();
                            isEnabled = isEnabled =
                                attributes.value("isEnabled").toString() == QStringLiteral("true");
                        }
                    } else if (xmlReader.name() == QStringLiteral("PatchList")) {
                        QJsonArray linesArray;
                        while (!xmlReader.atEnd() &&
                               !(xmlReader.tokenType() == QXmlStreamReader::EndElement &&
                                 xmlReader.name() == QStringLiteral("PatchList"))) {
                            xmlReader.readNext();
                            if (xmlReader.tokenType() == QXmlStreamReader::StartElement &&
                                xmlReader.name() == QStringLiteral("Line")) {
                                QXmlStreamAttributes attributes = xmlReader.attributes();
                                QJsonObject lineObject;
                                lineObject["Type"] = attributes.value("Type").toString();
                                lineObject["Address"] = attributes.value("Address").toString();
                                lineObject["Value"] = attributes.value("Value").toString();
                                linesArray.append(lineObject);
                            }
                        }
                        patchLines = linesArray;
                    }
                }

                if (!patchName.isEmpty() && !patchLines.isEmpty()) {
                    addPatchToLayout(patchName, patchAuthor, patchNote, patchLines, serial,
                                     isEnabled);
                    patchName.clear();
                    patchAuthor.clear();
                    patchNote.clear();
                    patchLines = QJsonArray();
                }
            }
            file.close();
        }
    }
}

void CheatsPatches::addPatchToLayout(const QString& name, const QString& author,
                                     const QString& note, const QJsonArray& linesArray,
                                     const QString& serial, bool isEnabled) {
    QCheckBox* patchCheckBox = new QCheckBox(name);
    patchCheckBox->setChecked(
        isEnabled); // Configura o estado do checkbox com base no valor isEnabled
    patchesGroupBoxLayout->addWidget(patchCheckBox);

    PatchInfo patchInfo;
    patchInfo.name = name;
    patchInfo.author = author;
    patchInfo.note = note;
    patchInfo.linesArray = linesArray;
    patchInfo.serial = serial;
    m_patchInfos[name] = patchInfo;

    patchCheckBox->installEventFilter(this);

    connect(patchCheckBox, &QCheckBox::toggled, [=](bool checked) { applyPatch(name, checked); });
}

void CheatsPatches::updateNoteTextEdit(const QString& patchName) {
    if (m_patchInfos.contains(patchName)) {
        const PatchInfo& patchInfo = m_patchInfos[patchName];
        QString text = QString("Name: %1\nAuthor: %2\n\n%3")
                           .arg(patchInfo.name)
                           .arg(patchInfo.author)
                           .arg(patchInfo.note);

        foreach (const QJsonValue& value, patchInfo.linesArray) {
            QJsonObject lineObject = value.toObject();
            QString type = lineObject["Type"].toString();
            QString address = lineObject["Address"].toString();
            QString patchValue = lineObject["Value"].toString();

            // add the values ​​to be modified in instructionsTextEdit
            // text.append(QString("\nType: %1\nAddress: %2\n\nValue: %3")
            //                .arg(type)
            //                .arg(address)
            //                .arg(patchValue));
        }
        text.replace("\\n", "\n");
        instructionsTextEdit->setText(text);
    }
}

bool showErrorMessage = true;
void CheatsPatches::uncheckAllCheatCheckBoxes() {
    for (auto& cheatCheckBox : m_cheatCheckBoxes) {
        cheatCheckBox->setChecked(false);
    }
    showErrorMessage = true;
}

void CheatsPatches::applyCheat(const QString& modName, bool enabled) {
    if (!m_cheats.contains(modName))
        return;

    Cheat cheat = m_cheats[modName];

    for (const MemoryMod& memoryMod : cheat.memoryMods) {
        QString value = enabled ? memoryMod.on : memoryMod.off;

        std::string modNameStr = modName.toStdString();
        std::string offsetStr = memoryMod.offset.toStdString();
        std::string valueStr = value.toStdString();

        if (MemoryPatcher::g_eboot_address == 0) {
            MemoryPatcher::patchInfo addingPatch;
            addingPatch.modNameStr = modNameStr;
            addingPatch.offsetStr = offsetStr;
            addingPatch.valueStr = valueStr;
            addingPatch.isOffset = true;

            MemoryPatcher::AddPatchToQueue(addingPatch);
            continue;
        }

        // Determine if the hint field is present
        bool isHintPresent = m_cheats[modName].hasHint;
        MemoryPatcher::PatchMemory(modNameStr, offsetStr, valueStr, !isHintPresent, false);
    }
}

void CheatsPatches::applyPatch(const QString& patchName, bool enabled) {
    if (m_patchInfos.contains(patchName)) {
        const PatchInfo& patchInfo = m_patchInfos[patchName];

        foreach (const QJsonValue& value, patchInfo.linesArray) {
            QJsonObject lineObject = value.toObject();
            QString type = lineObject["Type"].toString();
            QString address = lineObject["Address"].toString();
            QString patchValue = lineObject["Value"].toString();
            QString maskOffsetStr = lineObject["Offset"].toString();

            patchValue = convertValueToHex(type, patchValue);

            bool littleEndian = false;

            if (type == "bytes16") {
                littleEndian = true;
            } else if (type == "bytes32") {
                littleEndian = true;
            } else if (type == "bytes64") {
                littleEndian = true;
            }

            MemoryPatcher::PatchMask patchMask = MemoryPatcher::PatchMask::None;
            int maskOffsetValue = 0;

            if (type == "mask") {
                patchMask = MemoryPatcher::PatchMask::Mask;

                // im not sure if this works, there is no games to test the mask offset on yet
                if (!maskOffsetStr.toStdString().empty())
                    maskOffsetValue = std::stoi(maskOffsetStr.toStdString(), 0, 10);
            }

            if (type == "mask_jump32")
                patchMask = MemoryPatcher::PatchMask::Mask_Jump32;

            if (MemoryPatcher::g_eboot_address == 0) {
                MemoryPatcher::patchInfo addingPatch;
                addingPatch.gameSerial = patchInfo.serial.toStdString();
                addingPatch.modNameStr = patchName.toStdString();
                addingPatch.offsetStr = address.toStdString();
                addingPatch.valueStr = patchValue.toStdString();
                addingPatch.isOffset = false;
                addingPatch.littleEndian = littleEndian;
                addingPatch.patchMask = patchMask;
                addingPatch.maskOffset = maskOffsetValue;

                MemoryPatcher::AddPatchToQueue(addingPatch);
                continue;
            }

            MemoryPatcher::PatchMemory(patchName.toStdString(), address.toStdString(),
                                       patchValue.toStdString(), false, littleEndian, patchMask);
        }
    }
}
QString toHex(unsigned long long value, size_t byteSize) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(byteSize * 2) << value;
    return QString::fromStdString(ss.str());
}

QString CheatsPatches::convertValueToHex(const QString& type, const QString& valueStr) {
    QString result;
    std::string typeStr = type.toStdString();
    std::string valueStrStd = valueStr.toStdString();

    if (typeStr == "byte") {
        unsigned int value = std::stoul(valueStrStd, nullptr, 16);
        result = toHex(value, 1);
    } else if (typeStr == "bytes16") {
        unsigned int value = std::stoul(valueStrStd, nullptr, 16);
        result = toHex(value, 2);
    } else if (typeStr == "bytes32") {
        unsigned long value = std::stoul(valueStrStd, nullptr, 16);
        result = toHex(value, 4);
    } else if (typeStr == "bytes64") {
        unsigned long long value = std::stoull(valueStrStd, nullptr, 16);
        result = toHex(value, 8);
    } else if (typeStr == "float32") {
        union {
            float f;
            uint32_t i;
        } floatUnion;
        floatUnion.f = std::stof(valueStrStd);
        result = toHex(floatUnion.i, sizeof(floatUnion.i));
    } else if (typeStr == "float64") {
        union {
            double d;
            uint64_t i;
        } doubleUnion;
        doubleUnion.d = std::stod(valueStrStd);
        result = toHex(doubleUnion.i, sizeof(doubleUnion.i));
    } else if (typeStr == "utf8") {
        QByteArray byteArray = QString::fromStdString(valueStrStd).toUtf8();
        byteArray.append('\0');
        std::stringstream ss;
        for (unsigned char c : byteArray) {
            ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(c);
        }
        result = QString::fromStdString(ss.str());
    } else if (typeStr == "utf16") {
        QByteArray byteArray(
            reinterpret_cast<const char*>(QString::fromStdString(valueStrStd).utf16()),
            QString::fromStdString(valueStrStd).size() * 2);
        byteArray.append('\0');
        byteArray.append('\0');
        std::stringstream ss;
        for (unsigned char c : byteArray) {
            ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(c);
        }
        result = QString::fromStdString(ss.str());
    } else if (typeStr == "bytes") {
        result = valueStr;
    } else if (typeStr == "mask" || typeStr == "mask_jump32") {
        result = valueStr;
    } else {
        LOG_INFO(Loader, "Error applying Patch, unknown type: {}", typeStr);
    }
    return result;
}

bool CheatsPatches::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::HoverEnter || event->type() == QEvent::HoverLeave) {
        QCheckBox* checkBox = qobject_cast<QCheckBox*>(obj);
        if (checkBox) {
            bool hovered = (event->type() == QEvent::HoverEnter);
            onPatchCheckBoxHovered(checkBox, hovered);
            return true;
        }
    }
    // Pass the event on to base class
    return QWidget::eventFilter(obj, event);
}

void CheatsPatches::onPatchCheckBoxHovered(QCheckBox* checkBox, bool hovered) {
    if (hovered) {
        QString text = checkBox->text();
        updateNoteTextEdit(text);
    } else {
        instructionsTextEdit->setText(defaultTextEdit);
    }
}