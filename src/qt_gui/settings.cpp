// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "settings.h"

Settings::Settings(QObject* parent) : QObject(parent), m_settings_dir(ComputeSettingsDir()) {}

Settings::~Settings() {
    if (m_settings) {
        m_settings->sync();
    }
}

QString Settings::GetSettingsDir() const {
    return m_settings_dir.absolutePath();
}

QString Settings::ComputeSettingsDir() {
    return ""; // TODO currently we configure same dir , make it configurable
}

void Settings::RemoveValue(const QString& key, const QString& name) const {
    if (m_settings) {
        m_settings->beginGroup(key);
        m_settings->remove(name);
        m_settings->endGroup();
    }
}

void Settings::RemoveValue(const GuiSave& entry) const {
    RemoveValue(entry.key, entry.name);
}

QVariant Settings::GetValue(const QString& key, const QString& name, const QVariant& def) const {
    return m_settings ? m_settings->value(key + "/" + name, def) : def;
}

QVariant Settings::GetValue(const GuiSave& entry) const {
    return GetValue(entry.key, entry.name, entry.def);
}

QVariant Settings::List2Var(const q_pair_list& list) {
    QByteArray ba;
    QDataStream stream(&ba, QIODevice::WriteOnly);
    stream << list;
    return QVariant(ba);
}

q_pair_list Settings::Var2List(const QVariant& var) {
    q_pair_list list;
    QByteArray ba = var.toByteArray();
    QDataStream stream(&ba, QIODevice::ReadOnly);
    stream >> list;
    return list;
}

void Settings::SetValue(const GuiSave& entry, const QVariant& value) const {
    if (m_settings) {
        m_settings->beginGroup(entry.key);
        m_settings->setValue(entry.name, value);
        m_settings->endGroup();
    }
}

void Settings::SetValue(const QString& key, const QVariant& value) const {
    if (m_settings) {
        m_settings->setValue(key, value);
    }
}

void Settings::SetValue(const QString& key, const QString& name, const QVariant& value) const {
    if (m_settings) {
        m_settings->beginGroup(key);
        m_settings->setValue(name, value);
        m_settings->endGroup();
    }
}
