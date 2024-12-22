// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <QtNetwork>

#include "common/config.h"
#include "core/file_format/psf.h"

static constexpr int COMPAT_DB_VERSION = 1;

enum class CompatibilityStatus {
    Unknown,
    Nothing,
    Boots,
    Menus,
    Ingame,
    Playable,
};

// Prioritize different compatibility reports based on user's platform
enum class OSType {
#ifdef Q_OS_WIN
    Win32 = 0,
    Unknown,
    Linux,
    macOS,
#elif defined(Q_OS_LINUX)
    Linux = 0,
    Unknown,
    Win32,
    macOS,
#elif defined(Q_OS_MAC)
    macOS = 0,
    Unknown,
    Linux,
    Win32,
#endif
    // Fake enum to allow for iteration
    Last
};

struct CompatibilityEntry {
    CompatibilityStatus status;
    QString version;
    QDateTime last_tested;
    QString url;
    int issue_number;
};

class CompatibilityInfoClass : public QObject {
    Q_OBJECT
public:
    // Please think of a better alternative
    inline static const std::unordered_map<QString, CompatibilityStatus> LabelToCompatStatus = {
        {QStringLiteral("status-unknown"), CompatibilityStatus::Unknown},
        {QStringLiteral("status-nothing"), CompatibilityStatus::Nothing},
        {QStringLiteral("status-boots"), CompatibilityStatus::Boots},
        {QStringLiteral("status-menus"), CompatibilityStatus::Menus},
        {QStringLiteral("status-ingame"), CompatibilityStatus::Ingame},
        {QStringLiteral("status-playable"), CompatibilityStatus::Playable}};
    inline static const std::unordered_map<QString, OSType> LabelToOSType = {
        {QStringLiteral("os-linux"), OSType::Linux},
        {QStringLiteral("os-macOS"), OSType::macOS},
        {QStringLiteral("os-windows"), OSType::Win32},
    };

    inline static const std::unordered_map<CompatibilityStatus, QString> CompatStatusToString = {
        {CompatibilityStatus::Unknown, QStringLiteral("Unknown")},
        {CompatibilityStatus::Nothing, QStringLiteral("Nothing")},
        {CompatibilityStatus::Boots, QStringLiteral("Boots")},
        {CompatibilityStatus::Menus, QStringLiteral("Menus")},
        {CompatibilityStatus::Ingame, QStringLiteral("Ingame")},
        {CompatibilityStatus::Playable, QStringLiteral("Playable")}};
    inline static const std::unordered_map<OSType, QString> OSTypeToString = {
        {OSType::Linux, QStringLiteral("os-linux")},
        {OSType::macOS, QStringLiteral("os-macOS")},
        {OSType::Win32, QStringLiteral("os-windows")},
        {OSType::Unknown, QStringLiteral("os-unknown")}};

    CompatibilityInfoClass();
    ~CompatibilityInfoClass();
    void UpdateCompatibilityDatabase(QWidget* parent = nullptr);
    bool LoadCompatibilityFile();
    CompatibilityEntry GetCompatibilityInfo(const std::string& serial);
    void ExtractCompatibilityInfo(QByteArray response);
    static bool WaitForReply(QNetworkReply* reply);
    QNetworkReply* FetchPage(int page_num);

private:
    QNetworkAccessManager* m_network_manager;
    QString m_compatibility_filename;
    QJsonObject m_compatibility_database;
};