// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QString>
#include <QVariant>

struct GuiSave {
    QString key;
    QString name;
    QVariant def;

    GuiSave() {
        key = "";
        name = "";
        def = QVariant();
    }

    GuiSave(const QString& k, const QString& n, const QVariant& d) {
        key = k;
        name = n;
        def = d;
    }

    bool operator==(const GuiSave& rhs) const noexcept {
        return key == rhs.key && name == rhs.name && def == rhs.def;
    }
};
