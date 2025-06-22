// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elf_viewer.h"

ElfViewer::ElfViewer(std::shared_ptr<gui_settings> gui_settings, QWidget* parent)
    : QTableWidget(parent), m_gui_settings(std::move(gui_settings)) {

    list = gui_settings::Var2List(m_gui_settings->GetValue(gui::gen_elfDirs));
    for (const auto& str : list) {
        dir_list.append(str);
    }

    CheckElfFolders();

    this->setShowGrid(false);
    this->setEditTriggers(QAbstractItemView::NoEditTriggers);
    this->setSelectionBehavior(QAbstractItemView::SelectRows);
    this->setSelectionMode(QAbstractItemView::SingleSelection);
    this->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    this->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    this->verticalScrollBar()->installEventFilter(this);
    this->verticalScrollBar()->setSingleStep(20);
    this->horizontalScrollBar()->setSingleStep(20);
    this->verticalHeader()->setVisible(false);
    this->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    this->horizontalHeader()->setHighlightSections(false);
    this->horizontalHeader()->setSortIndicatorShown(true);
    this->horizontalHeader()->setStretchLastSection(true);
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    this->setColumnCount(2);
    this->setColumnWidth(0, 250);
    this->setColumnWidth(1, 400);
    this->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    this->setStyleSheet("QTableWidget { background-color: #D3D3D3; }");
    OpenElfFiles();
    QStringList headers;
    headers << "Name"
            << "Path";
    this->setHorizontalHeaderLabels(headers);
    this->horizontalHeader()->setSortIndicatorShown(true);
    this->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
}

void ElfViewer::OpenElfFolder() {
    QString folderPath =
        QFileDialog::getExistingDirectory(this, tr("Open Folder"), QDir::homePath());
    if (!dir_list.contains(folderPath)) {
        dir_list.append(folderPath);
        QDir directory(folderPath);
        QFileInfoList fileInfoList = directory.entryInfoList(QDir::Files);
        for (const QFileInfo& fileInfo : fileInfoList) {
            QString file_ext = fileInfo.suffix();
            if (fileInfo.isFile() && (file_ext == "bin" || file_ext == "elf")) {
                m_elf_list.append(fileInfo.absoluteFilePath());
            }
        }
        std::ranges::sort(m_elf_list);
        OpenElfFiles();
        list.clear();
        for (auto dir : dir_list) {
            list.push_back(dir);
        }
        m_gui_settings->SetValue(gui::gen_elfDirs, gui_settings::List2Var(list));
    } else {
        // qDebug() << "Folder selection canceled.";
    }
}

void ElfViewer::CheckElfFolders() {
    m_elf_list.clear();
    for (const QString& dir : dir_list) {
        QDir directory(dir);
        QFileInfoList fileInfoList = directory.entryInfoList(QDir::Files);
        for (const QFileInfo& fileInfo : fileInfoList) {
            QString file_ext = fileInfo.suffix();
            if (fileInfo.isFile() && (file_ext == "bin" || file_ext == "elf")) {
                m_elf_list.append(fileInfo.absoluteFilePath());
            }
        }
    }
    std::sort(m_elf_list.begin(), m_elf_list.end());
}

void ElfViewer::OpenElfFiles() {
    this->clearContents();
    this->setRowCount(m_elf_list.size());
    for (int i = 0; auto elf : m_elf_list) {
        QTableWidgetItem* item = new QTableWidgetItem();
        QFileInfo fileInfo(m_elf_list[i]);
        QString fileName = fileInfo.baseName();
        SetTableItem(this, i, 0, fileName);
        item = new QTableWidgetItem();
        SetTableItem(this, i, 1, m_elf_list[i]);
        i++;
    }
    this->resizeColumnsToContents();
}