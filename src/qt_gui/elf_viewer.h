// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QFileDialog>

#include "core/loader/elf.h"
#include "game_list_frame.h"

class ElfViewer : public QTableWidget {
    Q_OBJECT
public:
    explicit ElfViewer(QWidget* parent = nullptr);
    QStringList m_elf_list;

private:
    void CheckElfFolders();
    void OpenElfFiles();

    Core::Loader::Elf m_elf_file;
    QStringList dir_list;
    QStringList elf_headers_list;
    std::vector<std::string> dir_list_std;

    void SetTableItem(QTableWidget* game_list, int row, int column, QString itemStr) {
        QTableWidgetItem* item = new QTableWidgetItem();
        QWidget* widget = new QWidget(this);
        QVBoxLayout* layout = new QVBoxLayout(widget);
        QLabel* label = new QLabel(itemStr, widget);

        label->setStyleSheet("color: white; font-size: 15px; font-weight: bold;");

        // Create shadow effect
        QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
        shadowEffect->setBlurRadius(5);               // Set the blur radius of the shadow
        shadowEffect->setColor(QColor(0, 0, 0, 160)); // Set the color and opacity of the shadow
        shadowEffect->setOffset(2, 2);                // Set the offset of the shadow

        label->setGraphicsEffect(shadowEffect); // Apply shadow effect to the QLabel

        layout->addWidget(label);
        if (column != 8 && column != 1)
            layout->setAlignment(Qt::AlignCenter);
        widget->setLayout(layout);
        game_list->setItem(row, column, item);
        game_list->setCellWidget(row, column, widget);
    }

public slots:
    void OpenElfFolder();
};
