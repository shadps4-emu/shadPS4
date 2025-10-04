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
#include <QProgressBar>
#include <QPushButton>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <common/config.h>
#include <common/path_util.h>
#include <common/scm_rev.h>
#include "check_update.h"

using namespace Common::FS;

CheckUpdate::CheckUpdate(std::shared_ptr<gui_settings> gui_settings, const bool showMessage,
                         QWidget* parent)
    : QDialog(parent), m_gui_settings(std::move(gui_settings)),
      networkManager(new QNetworkAccessManager(this)) {
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
        updateChannel = m_gui_settings->GetValue(gui::gen_updateChannel).toString();
        if (updateChannel == "Nightly") {
            url = QUrl("https://api.github.com/repos/shadps4-emu/shadPS4/releases");
            checkName = false;
        } else if (updateChannel == "Release") {
            url = QUrl("https://api.github.com/repos/shadps4-emu/shadPS4/releases/latest");
            checkName = false;
        } else {
            if (Common::g_is_release) {
                m_gui_settings->SetValue(gui::gen_updateChannel, "Release");
            } else {
                m_gui_settings->SetValue(gui::gen_updateChannel, "Nightly");
            }
        }
    }

    QNetworkRequest request(url);
    QNetworkReply* reply = networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, showMessage, updateChannel]() {
        if (reply->error() != QNetworkReply::NoError) {
            if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 403) {
                QString response = reply->readAll();
                if (response.startsWith("{\"message\":\"API rate limit exceeded for")) {
                    QMessageBox::warning(
                        this, tr("Auto Updater"),
                        // clang-format off
tr("The Auto Updater allows up to 60 update checks per hour.\\nYou have reached this limit. Please try again later.").replace("\\n", "\n"));
                    // clang-format on
                } else {
                    QMessageBox::warning(
                        this, tr("Error"),
                        QString(tr("Network error:") + "\n" + reply->errorString()));
                }
            } else {
                QMessageBox::warning(this, tr("Error"),
                                     QString(tr("Network error:") + "\n" + reply->errorString()));
            }
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

        latestRev = latestVersion.right(40);
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

        QString currentRev = (updateChannel == "Nightly")
                                 ? QString::fromStdString(Common::g_scm_rev)
                                 : "v." + QString::fromStdString(Common::g_version);
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
    QPixmap pixmap(":/images/shadps4.png");
    imageLabel->setPixmap(pixmap);
    imageLabel->setScaledContents(true);
    imageLabel->setFixedSize(50, 50);

    QLabel* titleLabel = new QLabel("<h1>" + tr("Update Available") + "</h1>", this);
    titleLayout->addWidget(imageLabel);
    titleLayout->addWidget(titleLabel);
    layout->addLayout(titleLayout);

    QString updateChannel = m_gui_settings->GetValue(gui::gen_updateChannel).toString();

    QString updateText =
        QString("<p><b>" + tr("Update Channel") + ": </b>" + updateChannel +
                "<br>"
                "<table><tr>"
                "<td><b>" +
                tr("Current Version") +
                ":</b></td>"
                "<td>%1</td>"
                "<td>(%2)</td>"
                "</tr><tr>"
                "<td><b>" +
                tr("Latest Version") +
                ":</b></td>"
                "<td>%3</td>"
                "<td>(%4)</td>"
                "</tr></table></p>")
            .arg(updateChannel == "Nightly" ? currentRev.left(7) : currentRev.left(8), currentDate,
                 updateChannel == "Nightly" ? latestRev.left(7) : latestRev.left(8), latestDate);

    QLabel* updateLabel = new QLabel(updateText, this);
    layout->addWidget(updateLabel);

    // Setup bottom layout with action buttons
    autoUpdateCheckBox = new QCheckBox(tr("Check for Updates at Startup"), this);
    layout->addWidget(autoUpdateCheckBox);

    QHBoxLayout* updatePromptLayout = new QHBoxLayout();
    QLabel* updatePromptLabel = new QLabel(tr("Do you want to update?"), this);
    updatePromptLayout->addWidget(updatePromptLabel);

    yesButton = new QPushButton(tr("Update"), this);
    noButton = new QPushButton(tr("No"), this);
    yesButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    noButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

    QSpacerItem* spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    updatePromptLayout->addItem(spacer);
    updatePromptLayout->addWidget(yesButton);
    updatePromptLayout->addWidget(noButton);

    layout->addLayout(updatePromptLayout);

    // Don't show changelog button if:
    // The current version is a pre-release and the version to be downloaded is a release.
    bool current_isWIP = currentRev.endsWith("WIP", Qt::CaseInsensitive);
    bool latest_isWIP = latestRev.endsWith("WIP", Qt::CaseInsensitive);
    if (current_isWIP && !latest_isWIP) {
    } else {
        QTextBrowser* textField = new QTextBrowser(this);
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
                    if (!textField->isVisible()) {
                        requestChangelog(currentRev, latestRev, downloadUrl, latestDate,
                                         currentDate);
                        textField->setVisible(true);
                        toggleButton->setText(tr("Hide Changelog"));
                        adjustSize();
                        textField->setFixedWidth(textField->width() + 20);
                    } else {
                        textField->setVisible(false);
                        toggleButton->setText(tr("Show Changelog"));
                        adjustSize();
                    }
                });

        if (m_gui_settings->GetValue(gui::gen_showChangeLog).toBool()) {
            requestChangelog(currentRev, latestRev, downloadUrl, latestDate, currentDate);
            textField->setVisible(true);
            toggleButton->setText(tr("Hide Changelog"));
            adjustSize();
            textField->setFixedWidth(textField->width() + 20);
        }
    }

    connect(yesButton, &QPushButton::clicked, this, [this, downloadUrl]() {
        yesButton->setEnabled(false);
        noButton->setEnabled(false);
        DownloadUpdate(downloadUrl);
    });

    connect(noButton, &QPushButton::clicked, this, [this]() { close(); });

    autoUpdateCheckBox->setChecked(m_gui_settings->GetValue(gui::gen_checkForUpdates).toBool());
#if (QT_VERSION < QT_VERSION_CHECK(6, 7, 0))
    connect(autoUpdateCheckBox, &QCheckBox::stateChanged, this, [this](int state) {
#else
    connect(autoUpdateCheckBox, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
#endif
        const auto user_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
        m_gui_settings->SetValue(gui::gen_checkForUpdates, (state == Qt::Checked));
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
                QTextBrowser* textField = findChild<QTextBrowser*>();
                if (textField) {
                    QRegularExpression re("\\(\\#(\\d+)\\)");
                    QString newChanges;
                    int lastIndex = 0;
                    QRegularExpressionMatchIterator i = re.globalMatch(changes);
                    while (i.hasNext()) {
                        QRegularExpressionMatch match = i.next();
                        newChanges += changes.mid(lastIndex, match.capturedStart() - lastIndex);
                        QString num = match.captured(1);
                        newChanges +=
                            QString(
                                "(<a "
                                "href=\"https://github.com/shadps4-emu/shadPS4/pull/%1\">#%1</a>)")
                                .arg(num);
                        lastIndex = match.capturedEnd();
                    }

                    newChanges += changes.mid(lastIndex);
                    changes = newChanges;

                    textField->setOpenExternalLinks(true);
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
#ifdef Q_OS_WIN
        QString tempDownloadPath =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
            "/Temp/temp_download_update";
#else
        QString tempDownloadPath = userPath + "/temp_download_update";
#endif
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

    QString rootPath;
    Common::FS::PathToQString(rootPath, std::filesystem::current_path());

    QString tempDirPath = userPath + "/temp_download_update";
    QString startingUpdate = tr("Starting Update...");

    QString binaryStartingUpdate;
    for (QChar c : startingUpdate) {
        binaryStartingUpdate.append(QString::number(c.unicode(), 2).rightJustified(16, '0'));
    }

    QString scriptContent;
    QString scriptFileName;
    QStringList arguments;
    QString processCommand;

#ifdef Q_OS_WIN
    // On windows, overwrite tempDirPath with AppData/Roaming/shadps4/Temp folder
    // due to PowerShell Expand-Archive not being able to handle correctly
    // paths in square brackets (ie: ./[shadps4])
    tempDirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
                  "/Temp/temp_download_update";

    // Windows Batch Script
    scriptFileName = tempDirPath + "/update.ps1";
    scriptContent = QStringLiteral(
        "Set-ExecutionPolicy Bypass -Scope Process -Force\n"
        "$binaryStartingUpdate = '%1'\n"
        "$chars = @()\n"
        "for ($i = 0; $i -lt $binaryStartingUpdate.Length; $i += 16) {\n"
        "    $chars += [char]([convert]::ToInt32($binaryStartingUpdate.Substring($i, 16), 2))\n"
        "}\n"
        "$startingUpdate = -join $chars\n"
        "Write-Output $startingUpdate\n"
        "Expand-Archive -Path '%2\\temp_download_update.zip' -DestinationPath '%2' -Force\n"
        "Start-Sleep -Seconds 3\n"
        "Copy-Item -Recurse -Force '%2\\*' '%3\\'\n"
        "Start-Sleep -Seconds 2\n"
        "Remove-Item -Force -LiteralPath '%3\\update.ps1'\n"
        "Remove-Item -Force -LiteralPath '%3\\temp_download_update.zip'\n"
        "Remove-Item -Recurse -Force '%2'\n"
        "Start-Process -FilePath '%3\\shadps4.exe' "
        "-WorkingDirectory ([WildcardPattern]::Escape('%3'))\n");
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
        "    if pgrep -f \"Shadps4-qt.AppImage\" > /dev/null; then\n"
        "        pkill -f \"Shadps4-qt.AppImage\"\n"
        "        sleep 2\n"
        "    fi\n"
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
        scriptFile.write("\xEF\xBB\xBF");
#ifdef Q_OS_WIN
        out << scriptContent.arg(binaryStartingUpdate).arg(tempDirPath).arg(rootPath);
#endif
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
        out << scriptContent.arg(startingUpdate).arg(tempDirPath).arg(rootPath);
#endif
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
