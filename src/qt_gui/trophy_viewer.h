// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QDockWidget>
#include <QFileInfoList>
#include <QGraphicsBlurEffect>
#include <QHeaderView>
#include <QLabel>
#include <QMainWindow>
#include <QPair>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QVector>
#include <QXmlStreamReader>

#include "common/types.h"
#include "core/file_format/trp.h"
#include "gui_settings.h"

struct TrophyGameInfo {
    QString name;
    QString trophyPath;
    QString gameTrpPath;
};

class TrophyViewer : public QMainWindow {
    Q_OBJECT
public:
    explicit TrophyViewer(
        std::shared_ptr<gui_settings> gui_settings, QString trophyPath, QString gameTrpPath,
        QString gameName = "",
        const QVector<TrophyGameInfo>& allTrophyGames = QVector<TrophyGameInfo>());

    void updateTrophyInfo();
    void updateTableFilters();
    void onDockClosed();
    void reopenLeftDock();

private slots:
    void onGameSelectionChanged(int index);

private:
    void PopulateTrophyWidget(QString title);
    void SetTableItem(QTableWidget* parent, int row, int column, QString str);
    bool userResizedWindow_ = false;
    bool programmaticResize_ = false;
    bool initialSizeApplied_ = false;

    QTabWidget* tabWidget = nullptr;
    QStringList headers;
    QString gameTrpPath_;
    QString currentGameName_;
    TRP trp;
    QLabel* trophyInfoLabel;
    QCheckBox* showEarnedCheck;
    QCheckBox* showNotEarnedCheck;
    QCheckBox* showHiddenCheck;
    QComboBox* gameSelectionComboBox;
    QPushButton* expandButton;
    QDockWidget* trophyInfoDock;
    QPushButton* reopenButton;
    QVector<TrophyGameInfo> allTrophyGames_;

    std::string GetTrpType(const QChar trp_) {
        switch (trp_.toLatin1()) {
        case 'B':
            return "bronze.png";
        case 'S':
            return "silver.png";
        case 'G':
            return "gold.png";
        case 'P':
            return "platinum.png";
        }
        return "Unknown";
    }
    std::shared_ptr<gui_settings> m_gui_settings;

protected:
    void resizeEvent(QResizeEvent* event) override;
};
