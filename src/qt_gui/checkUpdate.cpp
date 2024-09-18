// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <filesystem>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QPushButton>
#include <QString>
#include <QStringList>
#include <QTextEdit>
#include <QVBoxLayout>
#include <common/config.h>
#include <common/path_util.h>
#include <common/scm_rev.h>
#include <zlib-ng.h>
#include "checkUpdate.h"

using namespace Common::FS;
namespace fs = std::filesystem;

CheckUpdate::CheckUpdate(const bool showMessage, QWidget* parent)
    : QDialog(parent), networkManager(new QNetworkAccessManager(this)) {
    setWindowTitle(tr("Auto Updater"));
    setFixedSize(0, 0);
    CheckForUpdates(showMessage);
}

CheckUpdate::~CheckUpdate() {}

void CheckUpdate::CheckForUpdates(const bool showMessage) {
    QString updateChannel = QString::fromStdString(Config::getUpdateChannel());
    QUrl url;

    if (updateChannel == "unstable") {
        url = QUrl("https://api.github.com/repos/DanielSvoboda/shadPS4/releases?per_page=1");
    } else if (updateChannel == "stable") {
        url = QUrl("https://api.github.com/repos/shadps4-emu/shadPS4/releases/latest");
    } else {
        QMessageBox::warning(
            this, tr("Error"),
            QString(tr("Invalid update channel: ") + updateChannel + "\n" +
                    tr("In updateChannel in config.tml file must contain 'stable' or 'unstable'")
                        .arg(updateChannel)));
        return;
    }

    QNetworkRequest request(url);
    QNetworkReply* reply = networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, showMessage, updateChannel]() {
        if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::warning(this, tr("Error"),
                                 QString(tr("Network error:") + "\n" + reply->errorString()));
            reply->deleteLater();
            return;
        }

        QByteArray response = reply->readAll();
        QJsonDocument jsonDoc(QJsonDocument::fromJson(response));

        if (jsonDoc.isNull()) {
            QMessageBox::warning(this, tr("Error"), tr("Failed to parse update information."));
            reply->deleteLater();
            return;
        }

        QString downloadUrl;
        QString latestVersion;
        QString latestRev;
        QString latestDate;

        QJsonObject jsonObj;
        if (jsonDoc.isArray()) {
            QJsonArray jsonArray = jsonDoc.array();
            if (!jsonArray.isEmpty()) {
                jsonObj = jsonArray.first().toObject();
            } else {
                QMessageBox::warning(this, tr("Error"), tr("No releases found."));
                reply->deleteLater();
                return;
            }
        } else {
            jsonObj = jsonDoc.object();
        }

        if (jsonObj.contains("tag_name")) {
            latestVersion = jsonObj["tag_name"].toString();
            latestRev = latestVersion.right(7);
            latestDate = jsonObj["published_at"].toString();
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Invalid release data."));
            reply->deleteLater();
            return;
        }

        QString currentRev = QString::fromStdString(Common::g_scm_rev).left(7);
        QString currentDate = Common::g_scm_date;

        QDateTime dateTime = QDateTime::fromString(latestDate, Qt::ISODate);
        latestDate = dateTime.isValid() ? dateTime.toString("yyyy-MM-dd HH:mm:ss") : "Unknown date";

        QJsonArray assets = jsonObj["assets"].toArray();
        bool found = false;
        for (const QJsonValue& assetValue : assets) {
            QJsonObject assetObj = assetValue.toObject();

            QString platformString;
#ifdef Q_OS_WIN
            platformString = "win64-qt";
#elif defined(Q_OS_LINUX)

                QString executablePath = QCoreApplication::applicationDirPath();
                QFileInfo fileInfo(executablePath);

                if (QProcessEnvironment::systemEnvironment().contains("APPIMAGE")) {
                    platformString = "linux-qt";
                } else {
                    platformString = "ubuntu64";
                }
#elif defined(Q_OS_MAC)
                platformString = "macos-qt";
#endif
            if (assetObj["name"].toString().contains(platformString)) {
                downloadUrl = assetObj["browser_download_url"].toString();
                found = true;
                break;
            }
        }
        if (!found) {
            QMessageBox::warning(this, tr("Error"),
                                 tr("No download URL found for the specified asset."));
        }

        if (latestRev == currentRev) {
            if (showMessage) {
                QMessageBox::information(this, tr("Auto Updater"),
                                         tr("Your version is already up to date!"));
            }
            close();
            return;
        } else {
            setFixedSize(420, 205);
            setupUI_UpdateAvailable(downloadUrl, latestDate, latestRev, currentDate, currentRev);
        }
        reply->deleteLater();
    });
}

void CheckUpdate::setupUI_UpdateAvailable(const QString& downloadUrl, const QString& latestDate,
                                          const QString& latestRev, const QString& currentDate,
                                          const QString& currentRev) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    QHBoxLayout* titleLayout = new QHBoxLayout();

    QLabel* imageLabel = new QLabel(this);
    QPixmap pixmap(":/images/shadps4.ico");
    imageLabel->setPixmap(pixmap);
    imageLabel->setScaledContents(true);
    imageLabel->setFixedSize(40, 40);

    QLabel* titleLabel = new QLabel("<h1>" + tr("Update Available") + "</h1>", this);
    titleLayout->addWidget(imageLabel);
    titleLayout->addWidget(titleLabel);
    layout->addLayout(titleLayout);

    QString updateText = QString("<p><b><br>" + tr("Current Version") + ":</b> %1 (%2)<br><b>" +
                                 tr("Latest Version") + ":</b> %3 (%4)</p><p>" +
                                 tr("Do you want to update?") + "</p>")
                             .arg(currentRev, currentDate, latestRev, latestDate);
    QLabel* updateLabel = new QLabel(updateText, this);
    layout->addWidget(updateLabel);

    // Create text field for changelog
    QTextEdit* textField = new QTextEdit(this);
    textField->setReadOnly(true);
    textField->setFixedWidth(400);
    textField->setFixedHeight(200);
    textField->setVisible(false);
    layout->addWidget(textField);

    // Create toggle button
    QPushButton* toggleButton = new QPushButton(tr("Show Changelog"), this);
    layout->addWidget(toggleButton);

    // Setup bottom layout with action buttons
    QHBoxLayout* bottomLayout = new QHBoxLayout();
    autoUpdateCheckBox = new QCheckBox(tr("Auto Update (Check at Startup)"), this);
    yesButton = new QPushButton(tr("Update"), this);
    noButton = new QPushButton(tr("No"), this);
    yesButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    noButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    bottomLayout->addWidget(autoUpdateCheckBox);

    QSpacerItem* spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    bottomLayout->addItem(spacer);

    bottomLayout->addWidget(yesButton);
    bottomLayout->addWidget(noButton);
    layout->addLayout(bottomLayout);

    // Connect the toggle button to the slot to show/hide changelog
    connect(toggleButton, &QPushButton::clicked,
            [this, textField, toggleButton, currentRev, latestRev, downloadUrl, latestDate,
             currentDate]() {
                QString updateChannel = QString::fromStdString(Config::getUpdateChannel());
                if (updateChannel == "unstable") {
                    if (!textField->isVisible()) {
                        requestChangelog(currentRev, latestRev, downloadUrl, latestDate,
                                         currentDate);
                        setFixedSize(420, 410);
                        textField->setVisible(true);
                        toggleButton->setText(tr("Hide Changelog"));
                    } else {
                        setFixedSize(420, 205);
                        textField->setVisible(false);
                        toggleButton->setText(tr("Show Changelog"));
                    }
                } else {
                    QMessageBox::information(
                        this, tr("Changelog Unavailable"),
                        tr("Viewing changelog is only available for the 'unstable' channel."));
                }
            });

    connect(yesButton, &QPushButton::clicked, this,
            [this, downloadUrl]() { DownloadAndInstallUpdate(downloadUrl); });
    connect(noButton, &QPushButton::clicked, this, [this]() { close(); });

    autoUpdateCheckBox->setChecked(Config::autoUpdate());
    connect(autoUpdateCheckBox, &QCheckBox::stateChanged, this, [](int state) {
        const auto user_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
        Config::setAutoUpdate(state == Qt::Checked);
        Config::save(user_dir / "config.toml");
    });

    setLayout(layout);
}

void CheckUpdate::requestChangelog(const QString& currentRev, const QString& latestRev,
                                   const QString& downloadUrl, const QString& latestDate,
                                   const QString& currentDate) {
    QString compareUrlString =
        QString("https://api.github.com/repos/DanielSvoboda/shadPS4/compare/%1...%2")
            .arg(currentRev)
            .arg(latestRev);

    QUrl compareUrl(compareUrlString);
    QNetworkRequest compareRequest(compareUrl);
    QNetworkReply* compareReply = networkManager->get(compareRequest);

    connect(compareReply, &QNetworkReply::finished, this,
            [this, compareReply, downloadUrl, latestDate, latestRev, currentDate, currentRev]() {
                if (compareReply->error() != QNetworkReply::NoError) {
                    QMessageBox::warning(
                        this, tr("Error"),
                        QString(tr("Network error:") + "\n%1").arg(compareReply->errorString()));
                    compareReply->deleteLater();
                    return;
                }

                QByteArray compareResponse = compareReply->readAll();
                QJsonDocument compareJsonDoc(QJsonDocument::fromJson(compareResponse));
                QJsonObject compareJsonObj = compareJsonDoc.object();
                QJsonArray commits = compareJsonObj["commits"].toArray();

                QString changes;
                for (const QJsonValue& commitValue : commits) {
                    QJsonObject commitObj = commitValue.toObject();
                    QString message = commitObj["commit"].toObject()["message"].toString();

                    // Remove texts after the first line break, if any
                    int newlineIndex = message.indexOf('\n');
                    if (newlineIndex != -1) {
                        message = message.left(newlineIndex);
                    }
                    if (!changes.isEmpty()) {
                        changes += "<br>";
                    }
                    changes += "&nbsp;&nbsp;&nbsp;&nbsp;• " + message;
                }

                // Update the text field with the changelog
                QTextEdit* textField = findChild<QTextEdit*>();
                if (textField) {
                    textField->setHtml("<h2>" + tr("Changes") + ":</h2>" + changes);
                }

                compareReply->deleteLater();
            });
}

void CheckUpdate::DownloadAndInstallUpdate(const QString& url) {
    QNetworkRequest request(url);
    QNetworkReply* reply = networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, url]() {
        if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::warning(this, tr("Error"),
                                 tr("Network error occurred while trying to access the URL") +
                                     ":\n" + url + "\n" + reply->errorString());
            reply->deleteLater();
            return;
        }

        QString userPath =
            QString::fromStdString(Common::FS::GetUserPath(Common::FS::PathType::UserDir).string());
        QString tempDownloadPath = userPath + "/temp_download_update";
        QDir dir(tempDownloadPath);
        if (!dir.exists()) {
            dir.mkpath(".");
        }

        QString downloadPath = tempDownloadPath + "/temp_download_update.zip";
        QFile file(downloadPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(reply->readAll());
            file.close();
            QMessageBox::information(this, tr("Download Complete"),
                                     tr("The update has been downloaded, press OK to install."));
            Unzip();
            Install();
        } else {
            QMessageBox::warning(
                this, tr("Error"),
                QString(tr("Failed to save the update file at") + ":\n" + downloadPath));
        }

        reply->deleteLater();
    });
}

void CheckUpdate::Unzip() {
    QString userPath =
        QString::fromStdString(Common::FS::GetUserPath(Common::FS::PathType::UserDir).string());
    QString tempDirPath = userPath + "/temp_download_update";
    QString zipFilePath = tempDirPath + "/temp_download_update.zip";

    QFile zipFile(zipFilePath);
    if (!zipFile.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Error"),
                             tr("Failed to open the ZIP file") + ":\n" + zipFilePath);
        return;
    }

    QByteArray zipData = zipFile.readAll();
    zipFile.close();

    const uint8_t* data = reinterpret_cast<const uint8_t*>(zipData.constData());
    size_t size = zipData.size();
    size_t offset = 0;

#pragma pack(push, 1)
    struct ZipLocalFileHeader {
        uint32_t signature;
        uint16_t version;
        uint16_t flags;
        uint16_t method;
        uint16_t time;
        uint16_t date;
        uint32_t crc32;
        uint32_t compressedSize;
        uint32_t uncompressedSize;
        uint16_t filenameLength;
        uint16_t extraFieldLength;
    };
#pragma pack(pop)

    auto readLocalFileHeader = [&](const uint8_t* data, size_t offset,
                                   ZipLocalFileHeader& header) -> bool {
        memcpy(&header, data + offset, sizeof(header));
        return header.signature == 0x04034b50;
    };

    auto decompressData = [&](const std::vector<uint8_t>& compressedData,
                              std::vector<uint8_t>& decompressedData) -> bool {
        zng_stream strm = {};
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        strm.avail_in = compressedData.size();
        strm.next_in = reinterpret_cast<Bytef*>(const_cast<uint8_t*>(compressedData.data()));

        if (zng_inflateInit2(&strm, -MAX_WBITS) != Z_OK) {
            return false;
        }

        strm.avail_out = decompressedData.size();
        strm.next_out = decompressedData.data();

        int result = zng_inflate(&strm, Z_NO_FLUSH);
        if (result != Z_STREAM_END) {
            zng_inflateEnd(&strm);
            return false;
        }

        zng_inflateEnd(&strm);
        return true;
    };

    while (offset < size) {
        ZipLocalFileHeader header;
        if (readLocalFileHeader(data, offset, header)) {
            uint16_t fileNameLength = header.filenameLength;
            std::string fileName(reinterpret_cast<const char*>(data + offset + sizeof(header)),
                                 fileNameLength);

            if (fileName.empty()) {
                QMessageBox::warning(this, tr("Error"),
                                     tr("File name is empty. Possibly corrupted ZIP."));
                break;
            }

            offset += sizeof(header) + fileNameLength + header.extraFieldLength;

            size_t compressedDataOffset = offset;
            size_t compressedDataSize = header.compressedSize;
            size_t uncompressedSize = header.uncompressedSize;

            if (header.method == 0) {
                // 0 = No need to decompress, just copy the data
                std::vector<uint8_t> decompressedData(
                    data + compressedDataOffset, data + compressedDataOffset + compressedDataSize);

                QString filePath = QString::fromUtf8(fileName.c_str());
                QString fullPath = tempDirPath + "/" + filePath;

                QFileInfo fileInfo(fullPath);
                QString dirPath = fileInfo.path();

                QDir dir(dirPath);
                if (!dir.exists()) {
                    if (!dir.mkpath(dirPath)) {
                        QMessageBox::warning(this, tr("Error"),
                                             tr("Failed to create directory") + ":\n" + dirPath);
                        continue;
                    }
                }

                QFile outFile(fullPath);
                outFile.write(reinterpret_cast<const char*>(decompressedData.data()),
                              decompressedData.size());
                outFile.close();

                offset += compressedDataSize;
            } else if (header.method == 8) {
                // 8 = Decompression Deflate
                std::vector<uint8_t> compressedData(
                    data + compressedDataOffset, data + compressedDataOffset + compressedDataSize);
                std::vector<uint8_t> decompressedData(uncompressedSize);

                if (!decompressData(compressedData, decompressedData)) {
                    QMessageBox::warning(this, tr("Error"),
                                         tr("Error decompressing file") + ":\n" +
                                             QString::fromStdString(fileName));
                    continue;
                }

                QString filePath = QString::fromUtf8(fileName.c_str());
                QString fullPath = tempDirPath + "/" + filePath;

                QFileInfo fileInfo(fullPath);
                QString dirPath = fileInfo.path();

                QDir dir(dirPath);
                if (!dir.exists()) {
                    if (!dir.mkpath(dirPath)) {
                        QMessageBox::warning(this, tr("Error"),
                                             tr("Failed to create directory") + ":\n" + dirPath);
                        continue;
                    }
                }

                QFile outFile(fullPath);
                if (!outFile.open(QIODevice::WriteOnly)) {
                    QMessageBox::warning(this, tr("Error"),
                                         tr("Failed to open output file") + ":\n" + fullPath);
                    continue;
                }
                outFile.write(reinterpret_cast<const char*>(decompressedData.data()),
                              decompressedData.size());
                outFile.close();

                offset += compressedDataSize;
            } else {
                QMessageBox::warning(this, tr("Error"),
                                     tr("Unsupported compression method for file:") +
                                         header.method + "\n" + QString::fromStdString(fileName));
                break;
            }
#if defined(Q_OS_MAC)
            if (filePath == "shadps4-macos-qt.tar.gz") {
                // Unpack the tar.gz file
                QString tarGzFilePath = tempDirPath + "/" + filePath;
                QString tarExtractDirPath = tempDirPath + "/tar_extracted";
                QDir tarExtractDir(tarExtractDirPath);
                if (!tarExtractDir.exists()) {
                    if (!tarExtractDir.mkpath(tarExtractDirPath)) {
                        QMessageBox::warning(this, tr("Error"),
                                             tr("Failed to create TAR extraction directory") +
                                                 ":\n" + tarExtractDirPath);
                        return;
                    }
                }

                QString tarCommand =
                    QString("tar -xzf %1 -C %2").arg(tarGzFilePath, tarExtractDirPath);
                QProcess tarProcess;
                tarProcess.start(tarCommand);
                tarProcess.waitForFinished();

                // Check if tar was successful
                if (tarProcess.exitStatus() != QProcess::NormalExit || tarProcess.exitCode() != 0) {
                    QMessageBox::warning(this, tr("Error"),
                                         tr("Failed to extract the TAR file") + ":\n" +
                                             tarProcess.errorString());
                    return;
                }
                // Remove .tar.gz file after extraction
                QFile::remove(tarGzFilePath);
            }
#endif
        } else {
            offset++;
        }
    }
}

void CheckUpdate::Install() {
    QString userPath =
        QString::fromStdString(Common::FS::GetUserPath(Common::FS::PathType::UserDir).string());
    QString tempDirPath = userPath + "/temp_download_update";
    QString rootPath = QString::fromStdString(std::filesystem::current_path().string());

    QString startingUpdate = tr("Starting Update...");

    QString scriptContent;
    QString scriptFileName;
    QStringList arguments;
    QString processCommand;

#ifdef Q_OS_WIN
    // Windows Batch Script
    scriptFileName = tempDirPath + "/update.bat";
    scriptContent = QStringLiteral("@echo off\n"
                                   "chcp 65001\n"
                                   "echo %1\n"
                                   "timeout /t 3 /nobreak\n"
                                   "xcopy /E /I /Y \"%2\\*\" \"%3\\\"\n"
                                   "timeout /t 3 /nobreak\n"
                                   "del /Q \"%3\\update.bat\"\n"
                                   "del /Q \"%3\\temp_download_update.zip\"\n"
                                   "start \"\" \"%3\\shadps4.exe\"\n"
                                   "rmdir /S /Q \"%2\"\n");
    arguments << "/C" << scriptFileName;
    processCommand = "cmd.exe";

#elif defined(Q_OS_LINUX)
    // Linux Shell Script
    scriptFileName = tempDirPath + "/update.sh";
    scriptContent = QStringLiteral("#!/bin/bash\n"
                                   "echo \"%1\"\n"
                                   "sleep 3\n"
                                   "cp -r \"%2/\"* \"%3/\"\n"
                                   "sleep 3\n"
                                   "rm \"%3/update.sh\"\n"
                                   "rm \"%3/Shadps4-qt.AppImage\"\n"
                                   "chmod +x \"%3/Shadps4-qt.AppImage\"\n"
                                   "cd \"%3\" && ./Shadps4-qt.AppImage\n"
                                   "rm -r \"%2\"\n");
    arguments << scriptFileName;
    processCommand = "bash";

#elif defined(Q_OS_MAC)
    // macOS Shell Script
    scriptFileName = tempDirPath + "/update.sh";
    scriptContent = QStringLiteral("#!/bin/bash\n"
                                   "echo \"%1\"\n"
                                   "sleep 3\n"
                                   "tar -xzf \"%2/temp_download_update.tar.gz\" -C \"%3\"\n"
                                   "sleep 3\n"
                                   "rm \"%3/update.sh\"\n"
                                   "chmod +x \"%3/shadps4.app/Contents/MacOS/shadps4\"\n"
                                   "open \"%3/shadps4.app\"\n"
                                   "rm -r \"%2\"\n");
    arguments << scriptFileName;
    processCommand = "bash";
#else
    QMessageBox::warning(this, tr("Error"), "Unsupported operating system.");
    return;
#endif

    QFile scriptFile(scriptFileName);
    if (scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&scriptFile);
        out << scriptContent.arg(startingUpdate).arg(tempDirPath).arg(rootPath);
        scriptFile.close();

// Make the script executable on Unix-like systems
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
        std::filesystem::permissions(scriptFileName, std::filesystem::perms::owner_exec);
#endif
        QProcess::startDetached(processCommand, arguments);

        exit(EXIT_SUCCESS);
    } else {
        QMessageBox::warning(
            this, tr("Error"),
            QString(tr("Failed to create the update script file") + ":\n" + scriptFileName));
    }
}