// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QApplication>
#include <QDir>
#include <QFileInfoList>
#include <QGraphicsBlurEffect>
#include <QHeaderView>
#include <QLabel>
#include <QMainWindow>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QXmlStreamReader>

#include "common/types.h"
#include "core/file_format/trp.h"

class TrophyViewer : public QMainWindow {
    Q_OBJECT
public:
    explicit TrophyViewer(QString trophyPath, QString gameTrpPath);

private:
    void PopulateTrophyWidget(QString title);
    void SetTableItem(QTableWidget* parent, int row, int column, QString str);

    QTabWidget* tabWidget = nullptr;
    QStringList headers;
    QString gameTrpPath_;
    TRP trp;

    QString GetTrpType(const QChar trp_) {
        switch (trp_.toLatin1()) {
        case 'B':
            return "Bronze";
        case 'S':
            return "Silver";
        case 'G':
            return "Gold";
        case 'P':
            return "Platinum";
        }
        return "Unknown";
    }
};