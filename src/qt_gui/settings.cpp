// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <common/path_util.h>
#include "settings.h"

settings::settings(QObject* parent) : QObject(parent), m_settings_dir(ComputeSettingsDir()) {}

settings::~settings() {
    sync();
}

void settings::sync() {
    if (m_settings) {
        m_settings->sync();
    }
}

QString settings::GetSettingsDir() const {
    return m_settings_dir.absolutePath();
}

QString settings::ComputeSettingsDir() {
    const auto config_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
    return QString::fromStdString(config_dir.string() + "/");
}

void settings::RemoveValue(const QString& key, const QString& name, bool sync) const {
    if (m_settings) {
        m_settings->beginGroup(key);
        m_settings->remove(name);
        m_settings->endGroup();

        if (sync) {
            m_settings->sync();
        }
    }
}

void settings::RemoveValue(const gui_value& entry, bool sync) const {
    RemoveValue(entry.key, entry.name, sync);
}

QVariant settings::GetValue(const QString& key, const QString& name, const QVariant& def) const {
    return m_settings ? m_settings->value(key + "/" + name, def) : def;
}

QVariant settings::GetValue(const gui_value& entry) const {
    return GetValue(entry.key, entry.name, entry.def);
}

void settings::SetValue(const gui_value& entry, const QVariant& value, bool sync) const {
    SetValue(entry.key, entry.name, value, sync);
}

void settings::SetValue(const QString& key, const QVariant& value, bool sync) const {
    if (m_settings) {
        m_settings->setValue(key, value);

        if (sync) {
            m_settings->sync();
        }
    }
}

void settings::SetValue(const QString& key, const QString& name, const QVariant& value,
                        bool sync) const {
    if (m_settings) {
        m_settings->beginGroup(key);
        m_settings->setValue(name, value);
        m_settings->endGroup();

        if (sync) {
            m_settings->sync();
        }
    }
}
