// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QPainter>
#include <QStyledItemDelegate>

class GameListGridDelegate : public QStyledItemDelegate {
public:
    GameListGridDelegate(const QSize& imageSize, const qreal& margin_factor,
                         const qreal& margin_ratio, QObject* parent = nullptr);

    void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setItemSize(const QSize& size);

private:
    QSize m_size;
    qreal m_margin_factor;
    qreal m_text_factor;
};
