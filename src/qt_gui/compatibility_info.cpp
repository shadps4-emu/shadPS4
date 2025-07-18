// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QFileInfo>
#include <QMessageBox>
#include <QProgressDialog>
#include <QTimer>

#include "common/path_util.h"
#include "compatibility_info.h"

CompatibilityInfoClass::CompatibilityInfoClass()
    : m_network_manager(new QNetworkAccessManager(this)) {
    QStringList file_paths;
    std::filesystem::path compatibility_file_path =
        Common::FS::GetUserPath(Common::FS::PathType::MetaDataDir) / "compatibility_data.json";
    Common::FS::PathToQString(m_compatibility_filename, compatibility_file_path);
};
CompatibilityInfoClass::~CompatibilityInfoClass() = default;

void CompatibilityInfoClass::UpdateCompatibilityDatabase(QWidget* parent, bool forced) {
    if (!forced && LoadCompatibilityFile())
        return;

    QUrl url("https://github.com/shadps4-compatibility/shadps4-game-compatibility/releases/latest/"
             "download/compatibility_data.json");
    QNetworkRequest request(url);
    QNetworkReply* reply = m_network_manager->get(request);

    QProgressDialog dialog(tr("Fetching compatibility data, please wait"), tr("Cancel"), 0, 100,
                           parent);
    dialog.setWindowTitle(tr("Loading..."));
    dialog.setWindowModality(Qt::WindowModal);
    dialog.setMinimumDuration(0);
    dialog.setValue(0);

    connect(reply, &QNetworkReply::downloadProgress,
            [&dialog](qint64 bytesReceived, qint64 bytesTotal) {
                if (bytesTotal > 0) {
                    dialog.setMaximum(bytesTotal);
                    dialog.setValue(bytesReceived);
                }
            });

    connect(&dialog, &QProgressDialog::canceled, reply, &QNetworkReply::abort);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        QMessageBox::critical(parent, tr("Error"),
                              tr("Unable to update compatibility data! Try again later."));
        // Try loading compatibility_file.json again
        if (!forced)
            LoadCompatibilityFile();
        return;
    }

    QFile compatibility_file(m_compatibility_filename);
    if (!compatibility_file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QMessageBox::critical(parent, tr("Error"),
                              tr("Unable to open compatibility_data.json for writing."));
        reply->deleteLater();
        return;
    }

    // Writes the received data to the file.
    QByteArray json_data = reply->readAll();
    compatibility_file.write(json_data);
    compatibility_file.close();
    reply->deleteLater();

    LoadCompatibilityFile();
}

CompatibilityEntry CompatibilityInfoClass::GetCompatibilityInfo(const std::string& serial) {
    QString title_id = QString::fromStdString(serial);
    if (m_compatibility_database.contains(title_id)) {
        QJsonObject compatibility_obj = m_compatibility_database[title_id].toObject();

        // Set current_os automatically
        QString current_os;
#ifdef Q_OS_WIN
        current_os = "os-windows";
#elif defined(Q_OS_MAC)
        current_os = "os-macOS";
#elif defined(Q_OS_LINUX)
        current_os = "os-linux";
#else
        current_os = "os-unknown";
#endif
        // Check if the game is compatible with the current operating system
        if (compatibility_obj.contains(current_os)) {
            QJsonObject compatibility_entry_obj = compatibility_obj[current_os].toObject();
            CompatibilityEntry compatibility_entry{
                LabelToCompatStatus.at(compatibility_entry_obj["status"].toString()),
                compatibility_entry_obj["version"].toString(),
                QDateTime::fromString(compatibility_entry_obj["last_tested"].toString(),
                                      Qt::ISODate),
                compatibility_entry_obj["url"].toString(),
                compatibility_entry_obj["issue_number"].toString()};
            return compatibility_entry;
        } else {
            // If there is no entry for the current operating system, return "Unknown"
            return CompatibilityEntry{CompatibilityStatus::Unknown, "",
                                      QDateTime::currentDateTime(), "", 0};
        }
    }

    // If title not found, return "Unknown"
    return CompatibilityEntry{CompatibilityStatus::Unknown, "", QDateTime::currentDateTime(), "",
                              0};
}

bool CompatibilityInfoClass::LoadCompatibilityFile() {
    // Returns true if compatibility is loaded succescfully
    QFileInfo check_file(m_compatibility_filename);
    const auto modified_delta = QDateTime::currentDateTime() - check_file.lastModified();
    if (!check_file.exists() || !check_file.isFile() ||
        std::chrono::duration_cast<std::chrono::minutes>(modified_delta).count() > 60) {
        return false;
    }

    QFile compatibility_file(m_compatibility_filename);
    if (!compatibility_file.open(QIODevice::ReadOnly)) {
        compatibility_file.close();
        return false;
    }
    QByteArray json_data = compatibility_file.readAll();
    compatibility_file.close();

    QJsonDocument json_doc = QJsonDocument::fromJson(json_data);
    if (json_doc.isEmpty() || json_doc.isNull()) {
        return false;
    }

    m_compatibility_database = json_doc.object();
    return true;
}

void CompatibilityInfoClass::ExtractCompatibilityInfo(QByteArray response) {
    QJsonDocument json_doc(QJsonDocument::fromJson(response));

    if (json_doc.isNull()) {
        return;
    }

    QJsonArray json_arr;

    json_arr = json_doc.array();

    for (const auto& issue_ref : std::as_const(json_arr)) {
        QJsonObject issue_obj = issue_ref.toObject();
        QString title_id;
        QRegularExpression title_id_regex("CUSA[0-9]{5}");
        QRegularExpressionMatch title_id_match =
            title_id_regex.match(issue_obj["title"].toString());
        QString current_os = "os-unknown";
        QString compatibility_status = "status-unknown";
        if (issue_obj.contains("labels") && title_id_match.hasMatch()) {
            title_id = title_id_match.captured(0);
            const QJsonArray& label_array = issue_obj["labels"].toArray();
            for (const auto& elem : label_array) {
                QString label = elem.toObject()["name"].toString();
                if (LabelToOSType.contains(label)) {
                    current_os = label;
                    continue;
                }
                if (LabelToCompatStatus.contains(label)) {
                    compatibility_status = label;
                    continue;
                }
            }

            // QJson does not support editing nested objects directly..

            QJsonObject compatibility_obj = m_compatibility_database[title_id].toObject();

            QJsonObject compatibility_data{
                {{"status", compatibility_status},
                 {"last_tested", issue_obj["updated_at"]},
                 {"version", issue_obj["milestone"].isNull()
                                 ? "unknown"
                                 : issue_obj["milestone"].toObject()["title"].toString()},
                 {"url", issue_obj["html_url"]},
                 {"issue_number", issue_obj["number"]}}};

            compatibility_obj[current_os] = compatibility_data;

            m_compatibility_database[title_id] = compatibility_obj;
        }
    }

    return;
}

const QString CompatibilityInfoClass::GetCompatStatusString(const CompatibilityStatus status) {
    switch (status) {
    case CompatibilityStatus::Unknown:
        return tr("Unknown");
    case CompatibilityStatus::Nothing:
        return tr("Nothing");
    case CompatibilityStatus::Boots:
        return tr("Boots");
    case CompatibilityStatus::Menus:
        return tr("Menus");
    case CompatibilityStatus::Ingame:
        return tr("Ingame");
    case CompatibilityStatus::Playable:
        return tr("Playable");
    default:
        return tr("Unknown");
    }
}
