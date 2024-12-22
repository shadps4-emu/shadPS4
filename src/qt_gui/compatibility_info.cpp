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

void CompatibilityInfoClass::UpdateCompatibilityDatabase(QWidget* parent) {
    if (LoadCompatibilityFile())
        return;

    QNetworkReply* reply = FetchPage(1);
    if (!WaitForReply(reply))
        return;

    QProgressDialog dialog(tr("Fetching compatibility data, please wait"), tr("Cancel"), 0, 0,
                           parent);
    dialog.setWindowTitle(tr("Loading..."));

    int remaining_pages = 0;
    if (reply->hasRawHeader("link")) {
        QRegularExpression last_page_re("(\\d+)(?=>; rel=\"last\")");
        QRegularExpressionMatch last_page_match =
            last_page_re.match(QString(reply->rawHeader("link")));
        if (last_page_match.hasMatch()) {
            remaining_pages = last_page_match.captured(0).toInt() - 1;
        }
    }

    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        QMessageBox::critical(parent, tr("Error"),
                              tr("Unable to update compatibility data! Try again later."));
        // Try loading compatibility_file.json again
        LoadCompatibilityFile();
        return;
    }

    ExtractCompatibilityInfo(reply->readAll());

    QVector<QNetworkReply*> replies(remaining_pages);
    QFutureWatcher<void> future_watcher;

    for (int i = 0; i < remaining_pages; i++) {
        replies[i] = FetchPage(i + 2);
    }

    future_watcher.setFuture(QtConcurrent::map(replies, WaitForReply));
    connect(&future_watcher, &QFutureWatcher<void>::finished, [&]() {
        for (int i = 0; i < remaining_pages; i++) {
            if (replies[i]->bytesAvailable()) {
                if (replies[i]->error() == QNetworkReply::NoError) {
                    ExtractCompatibilityInfo(replies[i]->readAll());
                }
                replies[i]->deleteLater();
            } else {
                // This means the request timed out
                return;
            }
        }

        QFile compatibility_file(m_compatibility_filename);

        if (!compatibility_file.open(QIODevice::WriteOnly | QIODevice::Truncate |
                                     QIODevice::Text)) {
            QMessageBox::critical(parent, tr("Error"),
                                  tr("Unable to open compatibility.json for writing."));
            return;
        }

        QJsonDocument json_doc;
        m_compatibility_database["version"] = COMPAT_DB_VERSION;

        json_doc.setObject(m_compatibility_database);
        compatibility_file.write(json_doc.toJson());
        compatibility_file.close();

        dialog.reset();
    });
    connect(&future_watcher, &QFutureWatcher<void>::canceled, [&]() {
        // Cleanup if user cancels pulling data
        for (int i = 0; i < remaining_pages; i++) {
            if (!replies[i]->bytesAvailable()) {
                replies[i]->deleteLater();
            } else if (!replies[i]->isFinished()) {
                replies[i]->abort();
            }
        }
    });
    connect(&dialog, &QProgressDialog::canceled, &future_watcher, &QFutureWatcher<void>::cancel);
    dialog.setRange(0, remaining_pages);
    connect(&future_watcher, &QFutureWatcher<void>::progressValueChanged, &dialog,
            &QProgressDialog::setValue);
    dialog.exec();
}

QNetworkReply* CompatibilityInfoClass::FetchPage(int page_num) {
    QUrl url = QUrl("https://api.github.com/repos/shadps4-emu/shadps4-game-compatibility/issues");
    QUrlQuery query;
    query.addQueryItem("per_page", QString("100"));
    query.addQueryItem(
        "tags", QString("status-ingame status-playable status-nothing status-boots status-menus"));
    query.addQueryItem("page", QString::number(page_num));
    url.setQuery(query);

    QNetworkRequest request(url);
    QNetworkReply* reply = m_network_manager->get(request);

    return reply;
}

bool CompatibilityInfoClass::WaitForReply(QNetworkReply* reply) {
    // Returns true if reply succeeded, false if reply timed out
    QTimer timer;
    timer.setSingleShot(true);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
    timer.start(5000);
    loop.exec();

    if (timer.isActive()) {
        timer.stop();
        return true;
    } else {
        disconnect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        reply->abort();
        return false;
    }
};

CompatibilityEntry CompatibilityInfoClass::GetCompatibilityInfo(const std::string& serial) {
    QString title_id = QString::fromStdString(serial);
    if (m_compatibility_database.contains(title_id)) {
        {
            QJsonObject compatibility_obj = m_compatibility_database[title_id].toObject();
            for (int os_int = 0; os_int != static_cast<int>(OSType::Last); os_int++) {
                QString os_string = OSTypeToString.at(static_cast<OSType>(os_int));
                if (compatibility_obj.contains(os_string)) {
                    QJsonObject compatibility_entry_obj = compatibility_obj[os_string].toObject();
                    CompatibilityEntry compatibility_entry{
                        LabelToCompatStatus.at(compatibility_entry_obj["status"].toString()),
                        compatibility_entry_obj["version"].toString(),
                        QDateTime::fromString(compatibility_entry_obj["last_tested"].toString(),
                                              Qt::ISODate),
                        compatibility_entry_obj["url"].toString(),
                        compatibility_entry_obj["issue_number"].toInt()};
                    return compatibility_entry;
                }
            }
        }
    }

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

    // Check database version
    int version_number;
    if (json_doc.object()["version"].isDouble()) {
        if (json_doc.object()["version"].toInt() < COMPAT_DB_VERSION)
            return false;
    } else
        return false;

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
