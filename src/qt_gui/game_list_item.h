// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>

#include <QObject>
#include <QTableWidgetItem>

using icon_callback_t = std::function<void(int)>;

class GameListItem : public QTableWidgetItem {
public:
    GameListItem() : QTableWidgetItem() {}
    GameListItem(const QString& text, int type = Type) : QTableWidgetItem(text, type) {}
    GameListItem(const QIcon& icon, const QString& text, int type = Type)
        : QTableWidgetItem(icon, text, type) {}

    ~GameListItem() {}

    void call_icon_func() const {
        if (m_icon_callback) {
            m_icon_callback(0);
        }
    }

    void set_icon_func(const icon_callback_t& func) {
        m_icon_callback = func;
        call_icon_func();
    }

private:
    icon_callback_t m_icon_callback = nullptr;
};
