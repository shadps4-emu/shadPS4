// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDir>
#include <QSettings>
#include <QString>
#include <QVariant>

struct gui_value {
    QString key;
    QString name;
    QVariant def;

    gui_value() {}

    gui_value(const QString& k, const QString& n, const QVariant& d) : key(k), name(n), def(d) {}

    bool operator==(const gui_value& rhs) const noexcept {
        return key == rhs.key && name == rhs.name && def == rhs.def;
    }
};

class settings : public QObject {
    Q_OBJECT

public:
    explicit settings(QObject* parent = nullptr);
    ~settings();

    void sync();

    QString GetSettingsDir() const;

    QVariant GetValue(const QString& key, const QString& name, const QVariant& def) const;
    QVariant GetValue(const gui_value& entry) const;
    static QVariant List2Var(const QList<QString>& list);
    static QList<QString> Var2List(const QVariant& var);

public Q_SLOTS:
    /** Remove entry */
    void RemoveValue(const QString& key, const QString& name, bool sync = true) const;
    void RemoveValue(const gui_value& entry, bool sync = true) const;

    /** Write value to entry */
    void SetValue(const gui_value& entry, const QVariant& value, bool sync = true) const;
    void SetValue(const QString& key, const QVariant& value, bool sync = true) const;
    void SetValue(const QString& key, const QString& name, const QVariant& value,
                  bool sync = true) const;

protected:
    static QString ComputeSettingsDir();

    std::unique_ptr<QSettings> m_settings;
    QDir m_settings_dir;
};
