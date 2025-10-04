// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
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

#include "cheats_patches.h"
#include "common/config.h"
#include "common/logging/log.h"
#include "common/memory_patcher.h"
#include "common/path_util.h"
#include "core/module.h"

CheatsPatches::CheatsPatches(const QString& gameName, const QString& gameSerial,
                             const QString& gameVersion, const QString& gameSize,
                             const QPixmap& gameImage, QWidget* parent)
    : QWidget(parent), m_gameName(gameName), m_gameSerial(gameSerial), m_gameVersion(gameVersion),
      m_gameSize(gameSize), m_gameImage(gameImage), manager(new QNetworkAccessManager(this)) {
    setupUI();
    resize(500, 400);
    setWindowTitle(tr("Cheats / Patches for ") + m_gameName);
}

CheatsPatches::~CheatsPatches() {}

void CheatsPatches::setupUI() {

    // clang-format off
    defaultTextEdit_MSG = tr("Cheats/Patches are experimental.\\nUse with caution.\\n\\nDownload cheats individually by selecting the repository and clicking the download button.\\nIn the Patches tab, you can download all patches at once, choose which ones you want to use, and save your selection.\\n\\nSince we do not develop the Cheats/Patches,\\nplease report issues to the cheat author.\\n\\nCreated a new cheat? Visit:\\n").replace("\\n", "\n")+"https://github.com/shadps4-emu/ps4_cheats";
    CheatsNotFound_MSG = tr("No Cheats found for this game in this version of the selected repository,try another repository or a different version of the game.");
    CheatsDownloadedSuccessfully_MSG = tr("You have successfully downloaded the cheats for this version of the game from the selected repository. You can try downloading from another repository, if it is available it will also be possible to use it by selecting the file from the list.");
    DownloadComplete_MSG = tr("Patches Downloaded Successfully! All Patches available for all games have been downloaded, there is no need to download them individually for each game as happens in Cheats. If the patch does not appear, it may be that it does not exist for the specific serial and version of the game.");
    // clang-format on

    QString CHEATS_DIR_QString;
    Common::FS::PathToQString(CHEATS_DIR_QString,
                              Common::FS::GetUserPath(Common::FS::PathType::CheatsDir));
    QString PATCHS_DIR_QString;
    Common::FS::PathToQString(PATCHS_DIR_QString,
                              Common::FS::GetUserPath(Common::FS::PathType::PatchesDir));
    QString NameCheatJson = m_gameSerial + "_" + m_gameVersion + ".json";
    m_cheatFilePath = CHEATS_DIR_QString + "/" + NameCheatJson;

    QHBoxLayout* mainLayout = new QHBoxLayout(this);

    // Create the game info group box
    QGroupBox* gameInfoGroupBox = new QGroupBox();
    QVBoxLayout* gameInfoLayout = new QVBoxLayout(gameInfoGroupBox);
    gameInfoLayout->setAlignment(Qt::AlignTop);

    QLabel* gameImageLabel = new QLabel();
    if (!m_gameImage.isNull()) {
        gameImageLabel->setPixmap(
            m_gameImage.scaled(275, 275, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        gameImageLabel->setText(tr("No Image Available"));
    }
    gameImageLabel->setAlignment(Qt::AlignCenter);
    gameInfoLayout->addWidget(gameImageLabel, 0, Qt::AlignCenter);

    QLabel* gameNameLabel = new QLabel(m_gameName);
    gameNameLabel->setAlignment(Qt::AlignLeft);
    gameNameLabel->setWordWrap(true);
    gameInfoLayout->addWidget(gameNameLabel);

    QLabel* gameSerialLabel = new QLabel(tr("Serial: ") + m_gameSerial);
    gameSerialLabel->setAlignment(Qt::AlignLeft);
    gameInfoLayout->addWidget(gameSerialLabel);

    QLabel* gameVersionLabel = new QLabel(tr("Version: ") + m_gameVersion);
    gameVersionLabel->setAlignment(Qt::AlignLeft);
    gameInfoLayout->addWidget(gameVersionLabel);

    if (Config::GetLoadGameSizeEnabled()) {
        QLabel* gameSizeLabel = new QLabel(tr("Size: ") + m_gameSize);
        gameSizeLabel->setAlignment(Qt::AlignLeft);
        gameInfoLayout->addWidget(gameSizeLabel);
    }

    // Add a text area for instructions and 'Patch' descriptions
    instructionsTextEdit = new QTextEdit();
    instructionsTextEdit->setText(defaultTextEdit_MSG);
    instructionsTextEdit->setReadOnly(true);
    instructionsTextEdit->setFixedHeight(290);
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
    scrollArea->setMinimumHeight(490);
    cheatsLayout->addWidget(scrollArea);

    // QListView
    listView_selectFile = new QListView();
    listView_selectFile->setSelectionMode(QAbstractItemView::SingleSelection);
    listView_selectFile->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Add QListView to layout
    QVBoxLayout* fileListLayout = new QVBoxLayout();
    fileListLayout->addWidget(new QLabel(tr("Select Cheat File:")));
    fileListLayout->addWidget(listView_selectFile);
    cheatsLayout->addLayout(fileListLayout, 2);

    // Call the method to fill the list of cheat files
    populateFileListCheats();

    QLabel* repositoryLabel = new QLabel(tr("Repository:"));
    repositoryLabel->setAlignment(Qt::AlignLeft);
    repositoryLabel->setAlignment(Qt::AlignVCenter);

    // Add a combo box and a download button
    QHBoxLayout* controlLayout = new QHBoxLayout();
    controlLayout->addWidget(repositoryLabel);
    controlLayout->setAlignment(Qt::AlignLeft);
    QComboBox* downloadComboBox = new QComboBox();

    downloadComboBox->addItem("GoldHEN", "GoldHEN");
    downloadComboBox->addItem("shadPS4", "shadPS4");

    controlLayout->addWidget(downloadComboBox);

    QPushButton* downloadButton = new QPushButton(tr("Download Cheats"));
    connect(downloadButton, &QPushButton::clicked, [this, downloadComboBox]() {
        QString source = downloadComboBox->currentData().toString();
        downloadCheats(source, m_gameSerial, m_gameVersion, true);
    });

    QPushButton* deleteCheatButton = new QPushButton(tr("Delete File"));
    connect(deleteCheatButton, &QPushButton::clicked, [this, CHEATS_DIR_QString]() {
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
                this, tr("Delete File"),
                tr("No files selected.") + "\n" +
                    tr("You can delete the cheats you don't want after downloading them."));
            return;
        }
        QModelIndex selectedIndex = selectedIndexes.first();
        QString selectedFileName = model->data(selectedIndex).toString();

        int ret = QMessageBox::warning(
            this, tr("Delete File"),
            QString(tr("Do you want to delete the selected file?\\n%1").replace("\\n", "\n"))
                .arg(selectedFileName),
            QMessageBox::Yes | QMessageBox::No);

        if (ret == QMessageBox::Yes) {
            QString filePath = CHEATS_DIR_QString + "/" + selectedFileName;
            QFile::remove(filePath);
            populateFileListCheats();
        }
    });

    QPushButton* closeButton = new QPushButton(tr("Close"));
    connect(closeButton, &QPushButton::clicked, [this]() { QWidget::close(); });

    controlLayout->addWidget(downloadButton);
    controlLayout->addWidget(deleteCheatButton);
    controlLayout->addWidget(closeButton);

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
    patchesScrollArea->setMinimumHeight(490);
    patchesLayout->addWidget(patchesScrollArea);

    // List of files in patchesListView
    patchesListView = new QListView();
    patchesListView->setSelectionMode(QAbstractItemView::SingleSelection);
    patchesListView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Add new label "Select Patch File:" above the QListView
    QVBoxLayout* patchFileListLayout = new QVBoxLayout();
    patchFileListLayout->addWidget(new QLabel(tr("Select Patch File:")));
    patchFileListLayout->addWidget(patchesListView);
    patchesLayout->addLayout(patchFileListLayout, 2);

    QStringListModel* patchesModel = new QStringListModel();
    patchesListView->setModel(patchesModel);

    QHBoxLayout* patchesControlLayout = new QHBoxLayout();

    QLabel* patchesRepositoryLabel = new QLabel(tr("Repository:"));
    patchesRepositoryLabel->setAlignment(Qt::AlignLeft);
    patchesRepositoryLabel->setAlignment(Qt::AlignVCenter);
    patchesControlLayout->addWidget(patchesRepositoryLabel);

    // Add the combo box with options
    patchesComboBox = new QComboBox();
    patchesComboBox->addItem("shadPS4", "shadPS4");
    patchesComboBox->addItem("GoldHEN", "GoldHEN");
    patchesControlLayout->addWidget(patchesComboBox);

    QPushButton* patchesButton = new QPushButton(tr("Download Patches"));
    connect(patchesButton, &QPushButton::clicked, [this]() {
        QString selectedOption = patchesComboBox->currentData().toString();
        downloadPatches(selectedOption, true);
    });
    patchesControlLayout->addWidget(patchesButton);

    QPushButton* deletePatchButton = new QPushButton(tr("Delete File"));
    connect(deletePatchButton, &QPushButton::clicked, [this, PATCHS_DIR_QString]() {
        QStringListModel* model = qobject_cast<QStringListModel*>(patchesListView->model());
        if (!model) {
            return;
        }
        QItemSelectionModel* selectionModel = patchesListView->selectionModel();
        if (!selectionModel) {
            return;
        }
        QModelIndexList selectedIndexes = selectionModel->selectedIndexes();
        if (selectedIndexes.isEmpty()) {
            QMessageBox::warning(this, tr("Delete File"), tr("No files selected."));
            return;
        }
        QModelIndex selectedIndex = selectedIndexes.first();
        QString selectedFileName = model->data(selectedIndex).toString();

        int ret = QMessageBox::warning(
            this, tr("Delete File"),
            QString(tr("Do you want to delete the selected file?\\n%1").replace("\\n", "\n"))
                .arg(selectedFileName),
            QMessageBox::Yes | QMessageBox::No);

        if (ret == QMessageBox::Yes) {
            QString fileName = selectedFileName.split('|').first().trimmed();
            QString directoryName = selectedFileName.split('|').last().trimmed();
            QString filePath = PATCHS_DIR_QString + "/" + directoryName + "/" + fileName;

            QFile::remove(filePath);
            createFilesJson(directoryName);
            populateFileListPatches();
        }
    });

    QPushButton* saveButton = new QPushButton(tr("Save"));
    connect(saveButton, &QPushButton::clicked, this, &CheatsPatches::onSaveButtonClicked);

    patchesControlLayout->addWidget(deletePatchButton);
    patchesControlLayout->addWidget(saveButton);

    patchesLayout->addLayout(patchesControlLayout);
    patchesTab->setLayout(patchesLayout);

    tabWidget->addTab(cheatsTab, tr("Cheats"));
    tabWidget->addTab(patchesTab, tr("Patches"));

    connect(tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        if (index == 1) {
            populateFileListPatches();
        }
    });

    mainLayout->addWidget(gameInfoGroupBox, 1);
    mainLayout->addWidget(tabWidget, 3);

    manager = new QNetworkAccessManager(this);

    setLayout(mainLayout);
}

void CheatsPatches::onSaveButtonClicked() {
    // Get the name of the selected folder in the patchesListView
    QString selectedPatchName;
    QModelIndexList selectedIndexes = patchesListView->selectionModel()->selectedIndexes();
    if (selectedIndexes.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("No patch selected."));
        return;
    }
    selectedPatchName = patchesListView->model()->data(selectedIndexes.first()).toString();
    int separatorIndex = selectedPatchName.indexOf(" | ");
    selectedPatchName = selectedPatchName.mid(separatorIndex + 3);

    QString patchDir;
    Common::FS::PathToQString(patchDir, Common::FS::GetUserPath(Common::FS::PathType::PatchesDir));
    patchDir += "/" + selectedPatchName;

    QString filesJsonPath = patchDir + "/files.json";
    QFile jsonFile(filesJsonPath);
    if (!jsonFile.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Error"), tr("Unable to open files.json for reading."));
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
        QMessageBox::critical(this, tr("Error"), tr("No patch file found for the current serial."));
        return;
    }

    QString filePath = patchDir + "/" + selectedFileName;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Error"), tr("Unable to open the file for reading."));
        return;
    }

    QByteArray xmlData = file.readAll();
    file.close();

    QString newXmlData;
    QXmlStreamWriter xmlWriter(&newXmlData);
    xmlWriter.setAutoFormatting(true);
    xmlWriter.writeStartDocument();

    QXmlStreamReader xmlReader(xmlData);

    while (!xmlReader.atEnd()) {
        xmlReader.readNext();

        if (xmlReader.isStartElement()) {
            if (xmlReader.name() == QStringLiteral("Metadata")) {
                xmlWriter.writeStartElement(xmlReader.name().toString());

                QString name = xmlReader.attributes().value("Name").toString();
                QString version = xmlReader.attributes().value("AppVer").toString();

                bool versionMatch = version == m_gameVersion;
                bool isEnabled = false;
                bool foundPatchInfo = false;

                // Check and update the isEnabled attribute
                for (const QXmlStreamAttribute& attr : xmlReader.attributes()) {
                    if (attr.name() == QStringLiteral("isEnabled"))
                        continue;
                    xmlWriter.writeAttribute(attr.name().toString(), attr.value().toString());
                }

                auto it = m_patchInfos.find(name);
                if (it != m_patchInfos.end()) {
                    QCheckBox* checkBox = findCheckBoxByName(it->name);
                    if (checkBox) {
                        foundPatchInfo = true;
                        isEnabled = checkBox->isChecked();
                    }
                }
                if (!foundPatchInfo) {
                    auto maskIt = m_patchInfos.find(name + " (any version)");
                    if (maskIt != m_patchInfos.end()) {
                        QCheckBox* checkBox = findCheckBoxByName(maskIt->name);
                        if (checkBox) {
                            versionMatch = true;
                            foundPatchInfo = true;
                            isEnabled = checkBox->isChecked();
                        }
                    }
                }
                if (foundPatchInfo) {
                    xmlWriter.writeAttribute("isEnabled",
                                             (isEnabled && versionMatch) ? "true" : "false");
                }
            } else {
                xmlWriter.writeStartElement(xmlReader.name().toString());
                for (const QXmlStreamAttribute& attr : xmlReader.attributes()) {
                    xmlWriter.writeAttribute(attr.name().toString(), attr.value().toString());
                }
            }
        } else if (xmlReader.isEndElement()) {
            xmlWriter.writeEndElement();
        } else if (xmlReader.isCharacters() && !xmlReader.isWhitespace()) {
            xmlWriter.writeCharacters(xmlReader.text().toString());
        }
    }

    xmlWriter.writeEndDocument();

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Error"), tr("Unable to open the file for writing."));
        return;
    }

    QTextStream textStream(&file);
    textStream << newXmlData;
    file.close();

    if (xmlReader.hasError()) {
        QMessageBox::critical(this, tr("Error"),
                              tr("Failed to parse XML: ") + "\n" + xmlReader.errorString());
    } else {
        QMessageBox::information(this, tr("Success"), tr("Options saved successfully."));
    }

    QWidget::close();
}

QCheckBox* CheatsPatches::findCheckBoxByName(const QString& name) {
    for (int i = 0; i < patchesGroupBoxLayout->count(); ++i) {
        QLayoutItem* item = patchesGroupBoxLayout->itemAt(i);
        if (item) {
            QWidget* widget = item->widget();
            QCheckBox* checkBox = qobject_cast<QCheckBox*>(widget);
            if (checkBox) {
                const auto patchName = checkBox->property("patchName");
                if (patchName.isValid() && patchName.toString().toStdString().find(
                                               name.toStdString()) != std::string::npos) {
                    return checkBox;
                }
            }
        }
    }
    return nullptr;
}

void CheatsPatches::downloadCheats(const QString& source, const QString& gameSerial,
                                   const QString& gameVersion, const bool showMessageBox) {
    QDir dir(Common::FS::GetUserPath(Common::FS::PathType::CheatsDir));
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString url;
    if (source == "GoldHEN") {
        url = "https://raw.githubusercontent.com/GoldHEN/GoldHEN_Cheat_Repository/main/json.txt";
    } else if (source == "shadPS4") {
        url = "https://raw.githubusercontent.com/shadps4-emu/ps4_cheats/main/CHEATS_JSON.txt";
    } else {
        QMessageBox::warning(this, tr("Invalid Source"),
                             QString(tr("The selected source is invalid.") + "\n%1").arg(source));
        return;
    }

    QNetworkRequest request(url);
    QNetworkReply* reply = manager->get(request);

    connect(reply, &QNetworkReply::finished, [=, this]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray jsonData = reply->readAll();
            bool foundFiles = false;

            if (source == "GoldHEN" || source == "shadPS4") {
                QString textContent(jsonData);
                QRegularExpression regex(
                    QString("%1_%2[^=]*\\.json").arg(gameSerial).arg(gameVersion));
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
                                this, tr("File Exists"),
                                tr("File already exists. Do you want to replace it?") + "\n" +
                                    newFileName,
                                QMessageBox::Yes | QMessageBox::No);
                            if (reply == QMessageBox::No) {
                                continue;
                            }
                        }
                        QNetworkRequest fileRequest(fileUrl);
                        QNetworkReply* fileReply = manager->get(fileRequest);

                        connect(fileReply, &QNetworkReply::finished, [=, this]() {
                            if (fileReply->error() == QNetworkReply::NoError) {
                                QByteArray fileData = fileReply->readAll();
                                QFile localFile(localFilePath);
                                if (localFile.open(QIODevice::WriteOnly)) {
                                    localFile.write(fileData);
                                    localFile.close();
                                } else {
                                    QMessageBox::warning(
                                        this, tr("Error"),
                                        QString(tr("Failed to save file:") + "\n%1")
                                            .arg(localFilePath));
                                }
                            } else {
                                QMessageBox::warning(this, tr("Error"),
                                                     QString(tr("Failed to download file:") +
                                                             "%1\n\n" + tr("Error") + ":%2")
                                                         .arg(fileUrl)
                                                         .arg(fileReply->errorString()));
                            }
                            fileReply->deleteLater();
                        });

                        foundFiles = true;
                    }
                }
                if (!foundFiles && showMessageBox) {
                    QMessageBox::warning(this, tr("Cheats Not Found"), CheatsNotFound_MSG);
                }
            }
            if (foundFiles && showMessageBox) {
                QMessageBox::information(this, tr("Cheats Downloaded Successfully"),
                                         CheatsDownloadedSuccessfully_MSG);
                populateFileListCheats();
            }

        } else {
            if (showMessageBox) {
                QMessageBox::warning(this, tr("Cheats Not Found"), CheatsNotFound_MSG);
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

void CheatsPatches::populateFileListPatches() {
    QLayoutItem* item;
    while ((item = patchesGroupBoxLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    m_patchInfos.clear();

    QString patchesDir;
    Common::FS::PathToQString(patchesDir,
                              Common::FS::GetUserPath(Common::FS::PathType::PatchesDir));
    QDir dir(patchesDir);

    QStringList folders = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    QStringList matchingFiles;
    QString shadPS4entry = "";

    foreach (const QString& folder, folders) {
        QString folderPath = dir.filePath(folder);
        QDir subDir(folderPath);

        QString filesJsonPath = subDir.filePath("files.json");
        QFile file(filesJsonPath);

        if (file.open(QIODevice::ReadOnly)) {
            QByteArray fileData = file.readAll();
            file.close();

            QJsonDocument jsonDoc(QJsonDocument::fromJson(fileData));
            QJsonObject jsonObj = jsonDoc.object();

            for (auto it = jsonObj.constBegin(); it != jsonObj.constEnd(); ++it) {
                QString fileName = it.key();
                QJsonArray serials = it.value().toArray();

                if (serials.contains(QJsonValue(m_gameSerial))) {
                    QString fileEntry = fileName + " | " + folder;
                    if (!matchingFiles.contains(fileEntry)) {
                        if (folder == "shadPS4") {
                            shadPS4entry = fileEntry;
                        }
                        matchingFiles << fileEntry;
                    }
                }
            }
        }
    }
    QStringListModel* model = new QStringListModel(matchingFiles, this);
    if (shadPS4entry != "") {
        QModelIndexList matches = model->match(model->index(0, 0), Qt::DisplayRole, shadPS4entry, 1,
                                               Qt::MatchExactly | Qt::MatchCaseSensitive);
        QModelIndex shadPS4Index = matches.first();
        model->moveRow(QModelIndex(), shadPS4Index.row(), QModelIndex(), 0);
    }
    patchesListView->setModel(model);

    connect(
        patchesListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
            QModelIndexList selectedIndexes = patchesListView->selectionModel()->selectedIndexes();
            if (!selectedIndexes.isEmpty()) {
                QString selectedText = selectedIndexes.first().data().toString();
                addPatchesToLayout(selectedText);
            }
        });

    if (!matchingFiles.isEmpty()) {
        QModelIndex firstIndex = model->index(0, 0);
        patchesListView->selectionModel()->select(firstIndex, QItemSelectionModel::Select |
                                                                  QItemSelectionModel::Rows);
        patchesListView->setCurrentIndex(firstIndex);
    }
}

void CheatsPatches::downloadPatches(const QString repository, const bool showMessageBox) {
    QString url;
    if (repository == "shadPS4") {
        url = "https://api.github.com/repos/shadps4-emu/ps4_cheats/contents/PATCHES";
    }
    if (repository == "GoldHEN") {
        url = "https://api.github.com/repos/illusion0001/PS4-PS5-Game-Patch/contents/patches/xml";
    }
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/vnd.github.v3+json");
    QNetworkReply* reply = manager->get(request);

    connect(reply, &QNetworkReply::finished, [=, this]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray jsonData = reply->readAll();
            reply->deleteLater();

            QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
            QJsonArray itemsArray = jsonDoc.array();

            if (itemsArray.isEmpty()) {
                if (showMessageBox) {
                    QMessageBox::warning(this, tr("Error"),
                                         tr("Failed to parse JSON data from HTML."));
                }
                return;
            }

            QDir dir(Common::FS::GetUserPath(Common::FS::PathType::PatchesDir));
            QString fullPath = dir.filePath(repository);
            if (!dir.exists(fullPath)) {
                dir.mkpath(fullPath);
            }
            dir.setPath(fullPath);

            foreach (const QJsonValue& value, itemsArray) {
                QJsonObject fileObj = value.toObject();
                QString fileName = fileObj["name"].toString();
                QString filePath = fileObj["path"].toString();
                QString downloadUrl = fileObj["download_url"].toString();

                if (fileName.endsWith(".xml")) {
                    QNetworkRequest fileRequest(downloadUrl);
                    QNetworkReply* fileReply = manager->get(fileRequest);

                    connect(fileReply, &QNetworkReply::finished, [=, this]() {
                        if (fileReply->error() == QNetworkReply::NoError) {
                            QByteArray fileData = fileReply->readAll();
                            QFile localFile(dir.filePath(fileName));
                            if (localFile.open(QIODevice::WriteOnly)) {
                                localFile.write(fileData);
                                localFile.close();
                            } else {
                                if (showMessageBox) {
                                    QMessageBox::warning(
                                        this, tr("Error"),
                                        QString(tr("Failed to save:") + "\n%1").arg(fileName));
                                }
                            }
                        } else {
                            if (showMessageBox) {
                                QMessageBox::warning(
                                    this, tr("Error"),
                                    QString(tr("Failed to download:") + "\n%1").arg(downloadUrl));
                            }
                        }
                        fileReply->deleteLater();
                    });
                }
            }
            if (showMessageBox) {
                QMessageBox::information(this, tr("Download Complete"),
                                         QString(DownloadComplete_MSG));
            }
            // Create the files.json file with the identification of which file to open
            createFilesJson(repository);
            populateFileListPatches();
            compatibleVersionNotice(repository);
        } else {
            if (showMessageBox) {
                QMessageBox::warning(this, tr("Error"),
                                     QString(tr("Failed to retrieve HTML page.") + "\n%1")
                                         .arg(reply->errorString()));
            }
        }
        emit downloadFinished();
    });
}

void CheatsPatches::compatibleVersionNotice(const QString repository) {
    QDir patchesDir(Common::FS::GetUserPath(Common::FS::PathType::PatchesDir));
    QDir dir = patchesDir.filePath(repository);

    QStringList xmlFiles = dir.entryList(QStringList() << "*.xml", QDir::Files);
    QStringList incompatMessages;

    foreach (const QString& xmlFile, xmlFiles) {
        QFile file(dir.filePath(xmlFile));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::warning(this, tr("Error"),
                                 QString(tr("Failed to open file:") + "\n%1").arg(xmlFile));
            continue;
        }

        QXmlStreamReader xmlReader(&file);
        QSet<QString> appVersionsSet;
        bool foundMatchingID = false;

        while (!xmlReader.atEnd() && !xmlReader.hasError()) {
            QXmlStreamReader::TokenType token = xmlReader.readNext();
            if (token == QXmlStreamReader::StartElement) {
                if (xmlReader.name() == QStringLiteral("ID")) {
                    QString id = xmlReader.readElementText();
                    if (id == m_gameSerial) {
                        foundMatchingID = true;
                    }
                } else if (xmlReader.name() == QStringLiteral("Metadata")) {
                    if (foundMatchingID) {
                        QString appVer = xmlReader.attributes().value("AppVer").toString();
                        if (!appVer.isEmpty()) {
                            appVersionsSet.insert(appVer);
                        }
                    }
                }
            }
        }

        if (xmlReader.hasError()) {
            QMessageBox::warning(this, tr("Error"),
                                 QString(tr("XML ERROR:") + "\n%1").arg(xmlReader.errorString()));
        }

        if (!foundMatchingID) {
            continue;
        }

        for (const QString& appVer : appVersionsSet) {
            if (appVer == QLatin1String("mask") || appVer == m_gameVersion) {
                return;
            }
        }

        if (!appVersionsSet.isEmpty()) {
            QStringList versionsList;
            for (const QString& v : appVersionsSet) {
                versionsList << v;
            }
            QString versions = versionsList.join(", ");
            QString message =
                QString(tr("The game is in version: %1")).arg(m_gameVersion) + "\n" +
                QString(tr("The downloaded patch only works on version: %1")).arg(versions) +
                QString("\n" + tr("You may need to update your game."));
            incompatMessages << message;
        }
    }

    if (!incompatMessages.isEmpty()) {
        QString finalMsg = incompatMessages.join("\n\n---\n\n");
        QMessageBox::information(this, tr("Incompatibility Notice"), finalMsg);
    }
}

void CheatsPatches::createFilesJson(const QString& repository) {

    QDir dir(Common::FS::GetUserPath(Common::FS::PathType::PatchesDir));
    QString fullPath = dir.filePath(repository);
    if (!dir.exists(fullPath)) {
        dir.mkpath(fullPath);
    }
    dir.setPath(fullPath);

    QJsonObject filesObject;
    QStringList xmlFiles = dir.entryList(QStringList() << "*.xml", QDir::Files);

    foreach (const QString& xmlFile, xmlFiles) {
        QFile file(dir.filePath(xmlFile));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::warning(this, tr("Error"),
                                 QString(tr("Failed to open file:") + "\n%1").arg(xmlFile));
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
            QMessageBox::warning(this, tr("Error"),
                                 QString(tr("XML ERROR:") + "\n%1").arg(xmlReader.errorString()));
        }
        filesObject[xmlFile] = titleIdsArray;
    }

    QFile jsonFile(dir.absolutePath() + "/files.json");
    if (!jsonFile.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to open files.json for writing"));
        return;
    }

    QJsonDocument jsonDoc(filesObject);
    jsonFile.write(jsonDoc.toJson());
    jsonFile.close();
}

void CheatsPatches::clearListCheats() {
    QLayoutItem* item;
    while ((item = rightLayout->takeAt(0)) != nullptr) {
        QWidget* widget = item->widget();
        if (widget) {
            delete widget;
        } else {
            QLayout* layout = item->layout();
            if (layout) {
                QLayoutItem* innerItem;
                while ((innerItem = layout->takeAt(0)) != nullptr) {
                    QWidget* innerWidget = innerItem->widget();
                    if (innerWidget) {
                        delete innerWidget;
                    }
                    delete innerItem;
                }
                delete layout;
            }
        }
    }
    m_cheats.clear();
    m_cheatCheckBoxes.clear();
}

void CheatsPatches::addCheatsToLayout(const QJsonArray& modsArray, const QJsonArray& creditsArray) {
    clearListCheats();
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
                    [this, modName](bool checked) { applyCheat(modName, checked); });
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
            connect(cheatButton, &QPushButton::clicked,
                    [this, modName]() { applyCheat(modName, true); });
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
    QString creditsText = tr("Author: ");
    if (!creditsArray.isEmpty()) {
        QStringList authors;
        for (const QJsonValue& credit : creditsArray) {
            authors << credit.toString();
        }
        creditsText += authors.join(", ");
    }
    creditsLabel->setText(creditsText);
    creditsLabel->setAlignment(Qt::AlignLeft);
    rightLayout->addWidget(creditsLabel);
}

void CheatsPatches::populateFileListCheats() {
    clearListCheats();

    QString cheatsDir;
    Common::FS::PathToQString(cheatsDir, Common::FS::GetUserPath(Common::FS::PathType::CheatsDir));

    QString fullGameVersion = m_gameVersion;
    QString modifiedGameVersion = m_gameVersion.mid(1);

    QString patternWithFirstChar = m_gameSerial + "_" + fullGameVersion + "*.json";
    QString patternWithoutFirstChar = m_gameSerial + "_" + modifiedGameVersion + "*.json";

    QDir dir(cheatsDir);
    QStringList filters;
    filters << patternWithFirstChar << patternWithoutFirstChar;
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

                    QString selectedFileName = selectedIndexes.first().data().toString();
                    QString cheatsDir;
                    Common::FS::PathToQString(
                        cheatsDir, Common::FS::GetUserPath(Common::FS::PathType::CheatsDir));

                    QFile file(cheatsDir + "/" + selectedFileName);
                    if (file.open(QIODevice::ReadOnly)) {
                        QByteArray jsonData = file.readAll();
                        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
                        QJsonObject jsonObject = jsonDoc.object();
                        QJsonArray modsArray = jsonObject["mods"].toArray();
                        QJsonArray creditsArray = jsonObject["credits"].toArray();
                        addCheatsToLayout(modsArray, creditsArray);
                    }
                }
            });

    if (!fileNames.isEmpty()) {
        QModelIndex firstIndex = model->index(0, 0);
        listView_selectFile->selectionModel()->select(firstIndex, QItemSelectionModel::Select |
                                                                      QItemSelectionModel::Rows);
        listView_selectFile->setCurrentIndex(firstIndex);
    }
}

void CheatsPatches::addPatchesToLayout(const QString& filePath) {
    if (filePath == "") {
        return;
    }
    QString folderPath = filePath.section(" | ", 1, 1);

    // Clear existing layout items
    QLayoutItem* item;
    while ((item = patchesGroupBoxLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    m_patchInfos.clear();

    QDir dir(Common::FS::GetUserPath(Common::FS::PathType::PatchesDir));
    QString fullPath = dir.filePath(folderPath);

    if (!dir.exists(fullPath)) {
        QMessageBox::warning(this, tr("Error"),
                             QString(tr("Directory does not exist:") + "\n%1").arg(fullPath));
        return;
    }
    dir.setPath(fullPath);

    QString filesJsonPath = dir.filePath("files.json");

    QFile jsonFile(filesJsonPath);
    if (!jsonFile.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to open files.json for reading."));
        return;
    }

    QByteArray jsonData = jsonFile.readAll();
    jsonFile.close();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    QJsonObject jsonObject = jsonDoc.object();

    bool patchAdded = false;

    // Iterate over each entry in the JSON file
    for (auto it = jsonObject.constBegin(); it != jsonObject.constEnd(); ++it) {
        QString xmlFileName = it.key();
        QJsonArray idsArray = it.value().toArray();

        // Check if the serial is in the ID list
        if (idsArray.contains(QJsonValue(m_gameSerial))) {
            QString xmlFilePath = dir.filePath(xmlFileName);
            QFile xmlFile(xmlFilePath);

            if (!xmlFile.open(QIODevice::ReadOnly)) {
                QMessageBox::warning(
                    this, tr("Error"),
                    QString(tr("Failed to open file:") + "\n%1").arg(xmlFile.fileName()));
                continue;
            }
            QXmlStreamReader xmlReader(&xmlFile);
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
                            isEnabled =
                                attributes.value("isEnabled").toString() == QStringLiteral("true");
                        }
                        if (appVer == "mask") {
                            patchName = attributes.value("Name").toString() + " (any version)";
                            patchAuthor = attributes.value("Author").toString();
                            patchNote = attributes.value("Note").toString();
                            isEnabled =
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
                    QCheckBox* patchCheckBox = new QCheckBox(patchName);
                    patchCheckBox->setProperty("patchName", patchName);
                    patchCheckBox->setChecked(isEnabled);
                    patchesGroupBoxLayout->addWidget(patchCheckBox);

                    PatchInfo patchInfo;
                    patchInfo.name = patchName;
                    patchInfo.author = patchAuthor;
                    patchInfo.note = patchNote;
                    patchInfo.linesArray = patchLines;
                    patchInfo.serial = m_gameSerial;
                    m_patchInfos[patchName] = patchInfo;

                    patchCheckBox->installEventFilter(this);

                    connect(patchCheckBox, &QCheckBox::toggled,
                            [this, patchName](bool checked) { applyPatch(patchName, checked); });

                    patchName.clear();
                    patchAuthor.clear();
                    patchNote.clear();
                    patchLines = QJsonArray();
                    patchAdded = true;
                }
            }
            xmlFile.close();
        }
    }

    // Remove the item from the list view if no patches were added
    // (the game has patches, but not for the current version)
    if (!patchAdded) {
        QStringListModel* model = qobject_cast<QStringListModel*>(patchesListView->model());
        if (model) {
            QStringList items = model->stringList();
            int index = items.indexOf(filePath);
            if (index != -1) {
                items.removeAt(index);
                model->setStringList(items);
            }
        }
    }
}

void CheatsPatches::updateNoteTextEdit(const QString& patchName) {
    if (m_patchInfos.contains(patchName)) {
        const PatchInfo& patchInfo = m_patchInfos[patchName];
        QString text = QString(tr("Name:") + " %1\n" + tr("Author: ") + "%2\n\n%3")
                           .arg(patchInfo.name)
                           .arg(patchInfo.author)
                           .arg(patchInfo.note);

        foreach (const QJsonValue& value, patchInfo.linesArray) {
            QJsonObject lineObject = value.toObject();
            QString type = lineObject["Type"].toString();
            QString address = lineObject["Address"].toString();
            QString patchValue = lineObject["Value"].toString();
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

    if (MemoryPatcher::g_eboot_address == 0 && enabled) {
        QMessageBox::critical(this, tr("Error"),
                              tr("Can't apply cheats before the game is started"));
        uncheckAllCheatCheckBoxes();
        return;
    }

    Cheat cheat = m_cheats[modName];

    for (const MemoryMod& memoryMod : cheat.memoryMods) {
        QString value = enabled ? memoryMod.on : memoryMod.off;

        std::string modNameStr = modName.toStdString();
        std::string offsetStr = memoryMod.offset.toStdString();
        std::string valueStr = value.toStdString();

        if (MemoryPatcher::g_eboot_address == 0)
            return;

        // Determine if the hint field is present
        bool isHintPresent = m_cheats[modName].hasHint;
        MemoryPatcher::PatchMemory(modNameStr, offsetStr, valueStr, "", "", !isHintPresent, false);
    }
}

void CheatsPatches::applyPatch(const QString& patchName, bool enabled) {
    if (!enabled)
        return;
    if (m_patchInfos.contains(patchName)) {
        const PatchInfo& patchInfo = m_patchInfos[patchName];

        foreach (const QJsonValue& value, patchInfo.linesArray) {
            QJsonObject lineObject = value.toObject();
            QString type = lineObject["Type"].toString();
            QString address = lineObject["Address"].toString();
            QString patchValue = lineObject["Value"].toString();
            QString maskOffsetStr = lineObject["Offset"].toString();

            patchValue = QString::fromStdString(
                MemoryPatcher::convertValueToHex(type.toStdString(), patchValue.toStdString()));

            bool littleEndian = false;

            if (type == "bytes16" || type == "bytes32" || type == "bytes64") {
                littleEndian = true;
            }

            MemoryPatcher::PatchMask patchMask = MemoryPatcher::PatchMask::None;
            int maskOffsetValue = 0;

            if (type == "mask")
                patchMask = MemoryPatcher::PatchMask::Mask;

            if (type == "mask_jump32")
                patchMask = MemoryPatcher::PatchMask::Mask_Jump32;

            if ((type == "mask" || type == "mask_jump32") && !maskOffsetStr.toStdString().empty()) {
                maskOffsetValue = std::stoi(maskOffsetStr.toStdString(), 0, 10);
            }

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
                                       patchValue.toStdString(), "", "", false, littleEndian,
                                       patchMask);
        }
    }
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
        const auto patchName = checkBox->property("patchName");
        if (patchName.isValid()) {
            updateNoteTextEdit(patchName.toString());
        }
    } else {
        instructionsTextEdit->setText(defaultTextEdit_MSG);
    }
}
