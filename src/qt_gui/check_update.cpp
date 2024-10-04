// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <filesystem>
#include <QDateTime>
#include <QDir>
#include <QFile>
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
#include <common/version.h>
#include <qprogressbar.h>
#include "check_update.h"

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
    QString updateChannel;
    QUrl url;

    bool checkName = true;
    while (checkName) {
        updateChannel = QString::fromStdString(Config::getUpdateChannel());
        if (updateChannel == "Nightly") {
            url = QUrl("https://api.github.com/repos/shadps4-emu/shadPS4/releases");
            checkName = false;
        } else if (updateChannel == "Release") {
            url = QUrl("https://api.github.com/repos/shadps4-emu/shadPS4/releases/latest");
            checkName = false;
        } else {
            if (Common::isRelease) {
                Config::setUpdateChannel("Release");
            } else {
                Config::setUpdateChannel("Nightly");
            }
            const auto config_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
            Config::save(config_dir / "config.toml");
        }
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
        QString platformString;

#ifdef Q_OS_WIN
        platformString = "win64-qt";
#elif defined(Q_OS_LINUX)
        platformString = "linux-qt";
#elif defined(Q_OS_MAC)
        platformString = "macos-qt";
#endif

        QJsonObject jsonObj;
        if (updateChannel == "Nightly") {
            QJsonArray jsonArray = jsonDoc.array();
            for (const QJsonValue& value : jsonArray) {
                jsonObj = value.toObject();
                if (jsonObj.contains("prerelease") && jsonObj["prerelease"].toBool()) {
                    break;
                }
            }
            if (!jsonObj.isEmpty()) {
                latestVersion = jsonObj["tag_name"].toString();
            } else {
                QMessageBox::warning(this, tr("Error"), tr("No pre-releases found."));
                reply->deleteLater();
                return;
            }
        } else {
            jsonObj = jsonDoc.object();
            if (jsonObj.contains("tag_name")) {
                latestVersion = jsonObj["tag_name"].toString();
            } else {
                QMessageBox::warning(this, tr("Error"), tr("Invalid release data."));
                reply->deleteLater();
                return;
            }
        }

        latestRev = latestVersion.right(7);
        latestDate = jsonObj["published_at"].toString();

        QJsonArray assets = jsonObj["assets"].toArray();
        bool found = false;

        for (const QJsonValue& assetValue : assets) {
            QJsonObject assetObj = assetValue.toObject();
            if (assetObj["name"].toString().contains(platformString)) {
                downloadUrl = assetObj["browser_download_url"].toString();
                found = true;
                break;
            }
        }

        if (!found) {
            QMessageBox::warning(this, tr("Error"),
                                 tr("No download URL found for the specified asset."));
            reply->deleteLater();
            return;
        }

        QString currentRev = QString::fromStdString(Common::g_scm_rev).left(7);
        QString currentDate = Common::g_scm_date;

        QDateTime dateTime = QDateTime::fromString(latestDate, Qt::ISODate);
        latestDate = dateTime.isValid() ? dateTime.toString("yyyy-MM-dd HH:mm:ss") : "Unknown date";

        if (latestRev == currentRev) {
            if (showMessage) {
                QMessageBox::information(this, tr("Auto Updater"),
                                         tr("Your version is already up to date!"));
            }
            close();
            return;
        } else {
            setupUI(downloadUrl, latestDate, latestRev, currentDate, currentRev);
        }
        reply->deleteLater();
    });
}

void CheckUpdate::setupUI(const QString& downloadUrl, const QString& latestDate,
                          const QString& latestRev, const QString& currentDate,
                          const QString& currentRev) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    QHBoxLayout* titleLayout = new QHBoxLayout();

    QLabel* imageLabel = new QLabel(this);
    QPixmap pixmap(":/images/shadps4.ico");
    imageLabel->setPixmap(pixmap);
    imageLabel->setScaledContents(true);
    imageLabel->setFixedSize(50, 50);

    QLabel* titleLabel = new QLabel("<h1>" + tr("Update Available") + "</h1>", this);
    titleLayout->addWidget(imageLabel);
    titleLayout->addWidget(titleLabel);
    layout->addLayout(titleLayout);

    QString updateChannel = QString::fromStdString(Config::getUpdateChannel());

    QString updateText =
        QString("<p><b><br>" + tr("Update Channel") + ": </b>" + updateChannel + "<br><b>" +
                tr("Current Version") + ":</b> %1 (%2)<br><b>" + tr("Latest Version") +
                ":</b> %3 (%4)</p><p>" + tr("Do you want to update?") + "</p>")
            .arg(currentRev, currentDate, latestRev, latestDate);
    QLabel* updateLabel = new QLabel(updateText, this);
    layout->addWidget(updateLabel);

    // Setup bottom layout with action buttons
    QHBoxLayout* bottomLayout = new QHBoxLayout();
    autoUpdateCheckBox = new QCheckBox(tr("Check for Updates at Startup"), this);
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

    // Don't show changelog button if:
    // The current version is a pre-release and the version to be downloaded is a release.
    bool current_isRelease = currentRev.startsWith('v', Qt::CaseInsensitive);
    bool latest_isRelease = latestRev.startsWith('v', Qt::CaseInsensitive);
    if (!current_isRelease && latest_isRelease) {
    } else {
        QTextEdit* textField = new QTextEdit(this);
        textField->setReadOnly(true);
        textField->setFixedWidth(500);
        textField->setFixedHeight(200);
        textField->setVisible(false);
        layout->addWidget(textField);

        QPushButton* toggleButton = new QPushButton(tr("Show Changelog"), this);
        layout->addWidget(toggleButton);

        connect(toggleButton, &QPushButton::clicked,
                [this, textField, toggleButton, currentRev, latestRev, downloadUrl, latestDate,
                 currentDate]() {
                    QString updateChannel = QString::fromStdString(Config::getUpdateChannel());
                    if (!textField->isVisible()) {
                        requestChangelog(currentRev, latestRev, downloadUrl, latestDate,
                                         currentDate);
                        textField->setVisible(true);
                        toggleButton->setText(tr("Hide Changelog"));
                        adjustSize();
                    } else {
                        textField->setVisible(false);
                        toggleButton->setText(tr("Show Changelog"));
                        adjustSize();
                    }
                });
    }

    connect(yesButton, &QPushButton::clicked, this, [this, downloadUrl]() {
        yesButton->setEnabled(false);
        noButton->setEnabled(false);
        DownloadUpdate(downloadUrl);
    });

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
        QString("https://api.github.com/repos/shadps4-emu/shadPS4/compare/%1...%2")
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

                    // Remove texts after first line break, if any, to make it cleaner
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

void CheckUpdate::DownloadUpdate(const QString& url) {
    QProgressBar* progressBar = new QProgressBar(this);
    progressBar->setRange(0, 100);
    progressBar->setTextVisible(true);
    progressBar->setValue(0);

    layout()->addWidget(progressBar);

    QNetworkRequest request(url);
    QNetworkReply* reply = networkManager->get(request);

    connect(reply, &QNetworkReply::downloadProgress, this,
            [progressBar](qint64 bytesReceived, qint64 bytesTotal) {
                if (bytesTotal > 0) {
                    int percentage = static_cast<int>((bytesReceived * 100) / bytesTotal);
                    progressBar->setValue(percentage);
                }
            });

    connect(reply, &QNetworkReply::finished, this, [this, reply, progressBar, url]() {
        progressBar->setValue(100);
        if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::warning(this, tr("Error"),
                                 tr("Network error occurred while trying to access the URL") +
                                     ":\n" + url + "\n" + reply->errorString());
            reply->deleteLater();
            progressBar->deleteLater();
            return;
        }

        QString userPath;
        Common::FS::PathToQString(userPath, Common::FS::GetUserPath(Common::FS::PathType::UserDir));
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
            Install();
        } else {
            QMessageBox::warning(
                this, tr("Error"),
                QString(tr("Failed to save the update file at") + ":\n" + downloadPath));
        }

        reply->deleteLater();
        progressBar->deleteLater();
    });
}

void CheckUpdate::Install() {
    QString userPath;
    Common::FS::PathToQString(userPath, Common::FS::GetUserPath(Common::FS::PathType::UserDir));

    QString startingUpdate = tr("Starting Update...");
    QString tempDirPath = userPath + "/temp_download_update";
    QString rootPath;
    Common::FS::PathToQString(rootPath, std::filesystem::current_path());

    QString scriptContent;
    QString scriptFileName;
    QStringList arguments;
    QString processCommand;

#ifdef Q_OS_WIN
    // Windows Batch Script
    scriptFileName = tempDirPath + "/update.ps1";
    scriptContent = QStringLiteral(
        "Set-ExecutionPolicy Bypass -Scope Process -Force\n"
        "Write-Output '%1'\n"
        "Expand-Archive -Path '%2\\temp_download_update.zip' -DestinationPath '%2' -Force\n"
        "Start-Sleep -Seconds 3\n"
        "Copy-Item -Recurse -Force '%2\\*' '%3\\'\n"
        "Start-Sleep -Seconds 2\n"
        "Remove-Item -Force '%3\\update.ps1'\n"
        "Remove-Item -Force '%3\\temp_download_update.zip'\n"
        "Start-Process '%3\\shadps4.exe'\n"
        "Remove-Item -Recurse -Force '%2'\n");
    arguments << "-ExecutionPolicy"
              << "Bypass"
              << "-File" << scriptFileName;
    processCommand = "powershell.exe";

#elif defined(Q_OS_LINUX)
    // Linux Shell Script
    scriptFileName = tempDirPath + "/update.sh";
    scriptContent = QStringLiteral(
        "#!/bin/bash\n"
        "check_unzip() {\n"
        "    if ! command -v unzip &> /dev/null && ! command -v 7z &> /dev/null; then\n"
        "        echo \"Neither 'unzip' nor '7z' is installed.\"\n"
        "        read -p \"Would you like to install 'unzip'? (y/n): \" response\n"
        "        if [[ \"$response\" == \"y\" || \"$response\" == \"Y\" ]]; then\n"
        "            if [[ -f /etc/os-release ]]; then\n"
        "                . /etc/os-release\n"
        "                case \"$ID\" in\n"
        "                    ubuntu|debian)\n"
        "                        sudo apt-get install unzip -y\n"
        "                        ;;\n"
        "                    fedora|redhat)\n"
        "                        sudo dnf install unzip -y\n"
        "                        ;;\n"
        "                    *)\n"
        "                        echo \"Unsupported distribution for automatic installation.\"\n"
        "                        exit 1\n"
        "                        ;;\n"
        "                esac\n"
        "            else\n"
        "                echo \"Could not identify the distribution.\"\n"
        "                exit 1\n"
        "            fi\n"
        "        else\n"
        "            echo \"At least one of 'unzip' or '7z' is required to continue. The process "
        "will be terminated.\"\n"
        "            exit 1\n"
        "        fi\n"
        "    fi\n"
        "}\n"
        "extract_file() {\n"
        "    if command -v unzip &> /dev/null; then\n"
        "        unzip -o \"%2/temp_download_update.zip\" -d \"%2/\"\n"
        "    elif command -v 7z &> /dev/null; then\n"
        "        7z x \"%2/temp_download_update.zip\" -o\"%2/\" -y\n"
        "    else\n"
        "        echo \"No suitable extraction tool found.\"\n"
        "        exit 1\n"
        "    fi\n"
        "}\n"
        "main() {\n"
        "    check_unzip\n"
        "    echo \"%1\"\n"
        "    sleep 2\n"
        "    extract_file\n"
        "    sleep 2\n"
        "    cp -r \"%2/\"* \"%3/\"\n"
        "    sleep 2\n"
        "    rm \"%3/update.sh\"\n"
        "    rm \"%3/temp_download_update.zip\"\n"
        "    chmod +x \"%3/Shadps4-qt.AppImage\"\n"
        "    rm -r \"%2\"\n"
        "    cd \"%3\" && ./Shadps4-qt.AppImage\n"
        "}\n"
        "main\n");
    arguments << scriptFileName;
    processCommand = "bash";

#elif defined(Q_OS_MAC)
    // macOS Shell Script
    scriptFileName = tempDirPath + "/update.sh";
    scriptContent = QStringLiteral(
        "#!/bin/bash\n"
        "check_tools() {\n"
        "    if ! command -v unzip &> /dev/null && ! command -v tar &> /dev/null; then\n"
        "        echo \"Neither 'unzip' nor 'tar' is installed.\"\n"
        "        read -p \"Would you like to install 'unzip'? (y/n): \" response\n"
        "        if [[ \"$response\" == \"y\" || \"$response\" == \"Y\" ]]; then\n"
        "            echo \"Please install 'unzip' using Homebrew or another package manager.\"\n"
        "            exit 1\n"
        "        else\n"
        "            echo \"At least one of 'unzip' or 'tar' is required to continue. The process "
        "will be terminated.\"\n"
        "            exit 1\n"
        "        fi\n"
        "    fi\n"
        "}\n"
        "check_tools\n"
        "echo \"%1\"\n"
        "sleep 2\n"
        "unzip -o \"%2/temp_download_update.zip\" -d \"%2/\"\n"
        "sleep 2\n"
        "tar -xzf \"%2/shadps4-macos-qt.tar.gz\" -C \"%3\"\n"
        "sleep 2\n"
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
        scriptFile.setPermissions(QFileDevice::ExeOwner | QFileDevice::ReadOwner |
                                  QFileDevice::WriteOwner);
#endif

        QProcess::startDetached(processCommand, arguments);

        exit(EXIT_SUCCESS);
    } else {
        QMessageBox::warning(
            this, tr("Error"),
            QString(tr("Failed to create the update script file") + ":\n" + scriptFileName));
    }
}