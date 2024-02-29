// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QTableWidgetItem>

#include "game_list_item.h"

class CustomTableWidgetItem : public GameListItem {
private:
    int m_sort_role = Qt::DisplayRole;

public:
    using QTableWidgetItem::setData;

    CustomTableWidgetItem() = default;
    CustomTableWidgetItem(const std::string& text, int sort_role = Qt::DisplayRole,
                          const QVariant& sort_value = 0);
    CustomTableWidgetItem(const QString& text, int sort_role = Qt::DisplayRole,
                          const QVariant& sort_value = 0);

    bool operator<(const QTableWidgetItem& other) const override;

    void setData(int role, const QVariant& value, bool assign_sort_role);
};
