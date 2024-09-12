// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <filesystem>
#include <fstream>
#include <iostream>
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
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>
#include <common/config.h>
#include <common/path_util.h>
#include <common/scm_rev.h>
#include "checkUpdate.h"
// #include "externals/minizip-ng/mz.h"
// #include "externals/minizip-ng/mz_strm.h"
// #include "externals/minizip-ng/mz_strm_buf.h"
// #include "externals/minizip-ng/mz_strm_mem.h"
// #include "externals/minizip-ng/mz_strm_os.h"
// #include "externals/minizip-ng/mz_zip.h"

using namespace Common::FS;
namespace fs = std::filesystem;

CheckUpdate::CheckUpdate(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Update Check");
    CheckForUpdates();
}

void CheckUpdate::CheckForUpdates() {
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest request(
        QUrl("https://api.github.com/repos/DanielSvoboda/teste/releases/latest"));
    QNetworkReply* reply = manager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::warning(this, tr("Error"),
                                 QString(tr("Network error: ") + "\n%1").arg(reply->errorString()));
            reply->deleteLater();
            return;
        }

        QByteArray response = reply->readAll();
        QJsonDocument jsonDoc(QJsonDocument::fromJson(response));
        QJsonObject jsonObj = jsonDoc.object();

        QString downloadUrl =
            jsonObj["assets"].toArray().first().toObject()["browser_download_url"].toString();
        QString latestVersion = QFileInfo(downloadUrl).baseName();
        QString latestRev = latestVersion.right(7);

        QString currentRev = QString::fromStdString(Common::g_scm_rev).left(7);

        if (latestRev == currentRev) {
            setupUI_NoUpdate();
        } else {
            updateDownloadUrl = downloadUrl;
            setupUI_UpdateAvailable(downloadUrl);
        }

        reply->deleteLater();
    });
}

void CheckUpdate::setupUI_NoUpdate() {
    QVBoxLayout* layout = new QVBoxLayout(this);

    QLabel* noUpdateLabel = new QLabel("Your version is already up to date!.", this);
    layout->addWidget(noUpdateLabel);

    autoUpdateCheckBox = new QCheckBox("Auto Update", this);
    layout->addWidget(autoUpdateCheckBox);

    autoUpdateCheckBox->setChecked(Config::autoUpdate());

    connect(autoUpdateCheckBox, &QCheckBox::stateChanged, this, [](int state) {
        const auto user_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
        Config::setAutoUpdate(state == Qt::Checked);
        Config::save(user_dir / "config.toml");
    });

    setLayout(layout);
}

void CheckUpdate::setupUI_UpdateAvailable(const QString& downloadUrl) {
    QVBoxLayout* layout = new QVBoxLayout(this);

    QLabel* updateLabel =
        new QLabel("An update is available!\nDo you want to download and install it?", this);
    updateLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(updateLabel);

    // Criar um layout horizontal para os botões
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    yesButton = new QPushButton("Yes", this);
    noButton = new QPushButton("No", this);

    // Adicionar os botões ao layout horizontal
    buttonLayout->addWidget(yesButton);
    buttonLayout->addWidget(noButton);

    // Adicionar o layout horizontal ao layout principal
    layout->addLayout(buttonLayout);

    connect(yesButton, &QPushButton::clicked, this,
            [this]() { DownloadAndInstallUpdate(updateDownloadUrl); });

    connect(noButton, &QPushButton::clicked, this, [this]() { close(); });

    autoUpdateCheckBox = new QCheckBox("Auto Update (Check at Startup)", this);
    layout->addWidget(autoUpdateCheckBox);

    autoUpdateCheckBox->setChecked(Config::autoUpdate());

    connect(autoUpdateCheckBox, &QCheckBox::stateChanged, this, [](int state) {
        const auto user_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
        Config::setAutoUpdate(state == Qt::Checked);
        Config::save(user_dir / "config.toml");
    });

    setLayout(layout);
}

void CheckUpdate::DownloadAndInstallUpdate(const QString& url) {
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest request(url);
    QNetworkReply* reply = manager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::warning(this, tr("Error"),
                                 QString(tr("Network error: ") + "\n%1").arg(reply->errorString()));
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
                                     tr("The update has been downloaded. Starting installation."));
            Unzip();
        } else {
            QMessageBox::warning(
                this, tr("Error"),
                QString(tr("Failed to save the update file at") + "\n%1").arg(downloadPath));
        }

        reply->deleteLater();
    });
}

void CheckUpdate::Unzip() {
    // QString userPath =
    //     QString::fromStdString(Common::FS::GetUserPath(Common::FS::PathType::UserDir).string());
    // QString tempDirPath = userPath + "/temp_download_update";
    // QString zipFilePath = tempDirPath + "/temp_download_update.zip";

    // if (!fs::exists(zipFilePath.toStdString())) {
    //     QMessageBox::warning(this, tr("Error"),
    //                          QString(tr("Arquivo zip não encontrado:") +
    //                          "\n%1").arg(zipFilePath));
    //     return;
    // }

    // void* zip_reader = NULL;
    // void* stream = NULL;

    // mz_stream_os_create();
    // if (mz_stream_os_open(stream, zipFilePath.toStdString().c_str(), MZ_OPEN_MODE_READ) != MZ_OK)
    // {

    //    QMessageBox::warning(this, tr("Error"),
    //                         QString(tr("Erro ao abrir o arquivo zip") +
    //                         "\n%1").arg(zipFilePath));
    //    mz_stream_os_delete(&stream);
    //    return;
    //}

    //// Criar o leitor do ZIP
    // mz_zip_create();
    // if (mz_zip_open(zip_reader, stream, MZ_OPEN_MODE_READ) != MZ_OK) {

    //    QMessageBox::warning(
    //        this, tr("Error"),
    //        QString(tr("Erro ao abrir o arquivo zip com mz_zip_open") + "\n%1").arg(zipFilePath));

    //    mz_zip_delete(&zip_reader);
    //    mz_stream_os_delete(&stream);
    //    return;
    //}

    //// Passa por (arquivo ou diretório) no arquivo zip
    // while (mz_zip_goto_next_entry(zip_reader) == MZ_OK) {
    //     mz_zip_file* file_info = nullptr;
    //     if (mz_zip_entry_get_info(zip_reader, &file_info) != MZ_OK) {
    //         QMessageBox::warning(
    //             this, tr("Error"),
    //             QString(tr("Erro ao obter informações da entrada no zip.") + "\n%1")
    //                 .arg(zipFilePath));
    //         continue;
    //     }

    //    QString caminho_arquivo = tempDirPath + "/" + file_info->filename;

    //    // Verifique se a entrada do zip é um diretório
    //    if (mz_zip_entry_is_dir(zip_reader) == MZ_OK) {
    //        fs::create_directories(caminho_arquivo.toStdString());
    //        QMessageBox::warning(this, tr("Error"),
    //                             QString(tr("Diretório criado:") + "\n%1").arg(caminho_arquivo));
    //    } else {
    //        // Certifique-se de que o diretório pai do arquivo exista
    //        fs::create_directories(fs::path(caminho_arquivo.toStdString()).parent_path());

    //        // Abra o arquivo de saída para gravação
    //        std::ofstream arquivo_saida(caminho_arquivo.toStdString(), std::ios::binary);
    //        if (!arquivo_saida) {

    //            QMessageBox::warning(
    //                this, tr("Error"),
    //                QString(tr("Erro ao abrir o arquivo de saída:") +
    //                "\n%1").arg(caminho_arquivo));

    //            mz_zip_entry_close(zip_reader);
    //            continue;
    //        }

    //        // Buffer temporário para leitura dos dados
    //        char buffer[4096];
    //        int32_t bytes_lidos;

    //        // Leia os dados do arquivo zip e escreva no arquivo de saída
    //        while ((bytes_lidos = mz_zip_entry_read(zip_reader, buffer, sizeof(buffer))) > 0) {
    //            arquivo_saida.write(buffer, bytes_lidos);
    //        }
    //        arquivo_saida.close();
    //        QMessageBox::warning(this, tr("Error"),
    //                             QString(tr("Arquivo extraído:") + "\n%1").arg(caminho_arquivo));
    //    }
    //    mz_zip_entry_close(zip_reader);
    //}

    //// Fechar o arquivo zip
    // mz_zip_close(zip_reader);
    // mz_zip_delete(&zip_reader);
    // mz_stream_os_close(stream);
    // mz_stream_os_delete(&stream);

    // QMessageBox::warning(this, tr("Error"),
    //                      QString(tr("Descompactação concluída em:") + "\n%1").arg(tempDirPath));
}

//// Criar e executar o arquivo batch para atualizar
// QFile batFile(tempDirPath + "/update.bat");
// if (batFile.open(QIODevice::WriteOnly)) {
//     QTextStream out(&batFile);
//     out << "@echo off\n";
//     out << "taskkill /IM shadps4.exe /F\n";
//     out << "move /Y \"" << tempDirPath << "\\*\" \"" << userPath << "\"\n";
//     out << "start \"\" \"" << userPath << "\\shadps4.exe\"\n";
//     batFile.close();

//    if (!QProcess::startDetached(batFile.fileName())) {
//        QMessageBox::warning(
//            this, tr("Error"),
//            tr("Failed to start the update batch file:\n%1").arg(batFile.fileName()));
//    }
//} else {
// QMessageBox::warning(
//    this, tr("Error"),
//    tr("Failed to create the update batch file:\n%1").arg(batFile.fileName()));
//}
