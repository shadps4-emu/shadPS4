// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "log_presets_dialog.h"

#include <algorithm>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {
constexpr const char* kPresetsGroup = "logger_presets";
constexpr const char* kPresetsKey = "entries";

// Use a single line per preset encoded as: comment + '\t' + filter
inline QString MakeEntry(const QString& comment, const QString& filter) {
    return comment + "\t" + filter;
}

inline void SplitEntry(const QString& entry, QString& comment, QString& filter) {
    const int idx = entry.indexOf('\t');
    if (idx < 0) {
        comment = entry;
        filter = QString();
    } else {
        comment = entry.left(idx);
        filter = entry.mid(idx + 1);
    }
}
} // namespace

LogPresetsDialog::LogPresetsDialog(std::shared_ptr<gui_settings> gui_settings, QWidget* parent)
    : QDialog(parent), m_gui_settings(std::move(gui_settings)) {
    setWindowTitle(tr("Log Filter Presets"));
    resize(640, 360);

    auto* root = new QVBoxLayout(this);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(2);
    QStringList headers;
    headers << tr("Comment") << tr("Filter");
    m_table->setHorizontalHeaderLabels(headers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeMode::Interactive);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeMode::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
    m_table->setEditTriggers(QAbstractItemView::EditTrigger::DoubleClicked |
                             QAbstractItemView::EditTrigger::SelectedClicked |
                             QAbstractItemView::EditTrigger::EditKeyPressed);

    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int row, int /*col*/) {
        if (row >= 0) {
            const auto* item = m_table->item(row, 1);
            if (item) {
                emit PresetChosen(item->text());
                accept();
            }
        }
    });

    root->addWidget(m_table);

    auto* buttons_layout = new QHBoxLayout();
    m_add_btn = new QPushButton(tr("+"), this);
    m_remove_btn = new QPushButton(tr("-"), this);
    m_load_btn = new QPushButton(tr("Load"), this);
    m_close_btn = new QPushButton(tr("Close"), this);

    m_add_btn->setToolTip(tr("Add a new preset after the selected row"));
    m_remove_btn->setToolTip(tr("Remove selected presets"));
    m_load_btn->setToolTip(tr("Load the selected preset"));

    buttons_layout->addWidget(m_add_btn);
    buttons_layout->addWidget(m_remove_btn);
    buttons_layout->addStretch();
    buttons_layout->addWidget(m_load_btn);
    buttons_layout->addWidget(m_close_btn);
    root->addLayout(buttons_layout);

    connect(m_add_btn, &QPushButton::clicked, this, [this]() { AddAfterSelection(); });
    connect(m_remove_btn, &QPushButton::clicked, this, [this]() { RemoveSelected(); });
    connect(m_load_btn, &QPushButton::clicked, this, [this]() { LoadSelected(); });
    connect(m_close_btn, &QPushButton::clicked, this, [this]() { reject(); });

    LoadFromSettings();
}

void LogPresetsDialog::accept() {
    SaveToSettings();
    QDialog::accept();
}

void LogPresetsDialog::reject() {
    SaveToSettings();
    QDialog::reject();
}

void LogPresetsDialog::LoadFromSettings() {
    if (!m_gui_settings)
        return;
    const auto var = m_gui_settings->GetValue(kPresetsGroup, kPresetsKey, QVariant());
    QList<QString> list;
    if (var.isValid()) {
        list = m_gui_settings->Var2List(var);
    }
    PopulateFromList(list);
}

void LogPresetsDialog::SaveToSettings() {
    if (!m_gui_settings)
        return;
    const auto list = SerializeTable();
    m_gui_settings->SetValue(kPresetsGroup, kPresetsKey, m_gui_settings->List2Var(list));
}

QList<QString> LogPresetsDialog::SerializeTable() const {
    QList<QString> list;
    const int rows = m_table->rowCount();
    for (int r = 0; r < rows; ++r) {
        const auto* comment = m_table->item(r, 0);
        const auto* filter = m_table->item(r, 1);
        const QString c = comment ? comment->text() : QString();
        const QString f = filter ? filter->text() : QString();
        if (!c.isEmpty() || !f.isEmpty()) {
            list.push_back(MakeEntry(c, f));
        }
    }
    return list;
}

void LogPresetsDialog::PopulateFromList(const QList<QString>& list) {
    m_table->setRowCount(0);
    for (const auto& entry : list) {
        QString c, f;
        SplitEntry(entry, c, f);
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(c));
        m_table->setItem(row, 1, new QTableWidgetItem(f));
    }
}

void LogPresetsDialog::AddAfterSelection() {
    int insert_row = m_table->rowCount();
    const auto selected = m_table->selectionModel()->selectedRows();
    if (!selected.isEmpty()) {
        // place after the last selected row
        insert_row = selected.last().row() + 1;
    }
    m_table->insertRow(insert_row);
    m_table->setItem(insert_row, 0, new QTableWidgetItem(""));
    m_table->setItem(insert_row, 1, new QTableWidgetItem(""));
    m_table->setCurrentCell(insert_row, 0);
    m_table->editItem(m_table->item(insert_row, 0));
}

void LogPresetsDialog::RemoveSelected() {
    auto selected = m_table->selectionModel()->selectedRows();
    if (selected.isEmpty())
        return;
    // Remove from bottom to top to keep indices valid
    std::sort(selected.begin(), selected.end(),
              [](const QModelIndex& a, const QModelIndex& b) { return a.row() > b.row(); });
    for (const auto& idx : selected) {
        m_table->removeRow(idx.row());
    }
}

void LogPresetsDialog::LoadSelected() {
    const auto selected = m_table->selectionModel()->selectedRows();
    if (selected.isEmpty())
        return;
    const int row = selected.first().row();
    const auto* item = m_table->item(row, 1);
    if (item) {
        emit PresetChosen(item->text());
        accept();
    }
}
