// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "log_presets_dialog.h"

#include <algorithm>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QPushButton>
#include <QSet>
#include <QSignalBlocker>
#include <QStyleOptionViewItem>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {
constexpr const char* kPresetsGroup = "logger_presets";
constexpr const char* kPresetsKey = "entries";

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
    m_table->setColumnCount(3);
    m_table->horizontalHeader()->setSectionsClickable(true);
    m_table->horizontalHeader()->setHighlightSections(false);
    QStringList headers;
    headers << QString() << tr("Comment") << tr("Filter");
    m_table->setHorizontalHeaderLabels(headers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeMode::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeMode::Interactive);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeMode::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
    m_table->setEditTriggers(QAbstractItemView::EditTrigger::DoubleClicked |
                             QAbstractItemView::EditTrigger::SelectedClicked |
                             QAbstractItemView::EditTrigger::EditKeyPressed);

    connect(m_table, &QTableWidget::cellDoubleClicked, this,
            [this](int /*row*/, int /*col*/) { LoadSelected(); });

    connect(m_table, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* item) {
        if (m_updating_checks)
            return;
        if (item && item->column() == 0) {
            m_updating_checks = true;
            const int row = item->row();
            auto* sm = m_table->selectionModel();
            const auto idx = m_table->model()->index(row, 0);
            const bool check = (item->checkState() == Qt::Checked);
            sm->select(idx, (check ? QItemSelectionModel::Select : QItemSelectionModel::Deselect) |
                                QItemSelectionModel::Rows);
            m_updating_checks = false;
            UpdateHeaderCheckState();
            UpdateLoadButtonEnabled();
        }
    });

    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            [this](const QItemSelection& selected, const QItemSelection& deselected) {
                if (m_updating_checks)
                    return;
                m_updating_checks = true;
                QSet<int> selRows;
                for (const auto& idx : selected.indexes())
                    selRows.insert(idx.row());
                QSet<int> deselRows;
                for (const auto& idx : deselected.indexes())
                    deselRows.insert(idx.row());
                for (int r : selRows) {
                    auto* it = m_table->item(r, 0);
                    if (!it) {
                        it = new QTableWidgetItem();
                        it->setFlags((QTableWidgetItem().flags() | Qt::ItemIsUserCheckable) &
                                     ~Qt::ItemIsEditable);
                        m_table->setItem(r, 0, it);
                    }
                    it->setCheckState(Qt::Checked);
                }
                for (int r : deselRows) {
                    auto* it = m_table->item(r, 0);
                    if (it)
                        it->setCheckState(Qt::Unchecked);
                }
                m_updating_checks = false;
                UpdateHeaderCheckState();
                UpdateLoadButtonEnabled();
            });

    connect(m_table->horizontalHeader(), &QHeaderView::sectionClicked, this, [this](int section) {
        if (section != 0)
            return;
        const auto next = (m_header_checkbox && m_header_checkbox->checkState() == Qt::Checked)
                              ? Qt::Unchecked
                              : Qt::Checked;
        if (m_header_checkbox)
            m_header_checkbox->setCheckState(next);
        SetAllCheckStates(next);
        UpdateHeaderCheckState();
        UpdateLoadButtonEnabled();
    });

    m_header_checkbox = new QCheckBox(m_table->horizontalHeader());
    m_header_checkbox->setTristate(true);
    m_header_checkbox->setText(QString());
    m_header_checkbox->setFocusPolicy(Qt::NoFocus);
    PositionHeaderCheckbox();
    QObject::connect(m_table->horizontalHeader(), &QHeaderView::geometriesChanged, this,
                     [this]() { PositionHeaderCheckbox(); });
    QObject::connect(m_table->horizontalHeader(), &QHeaderView::sectionResized, this,
                     [this](int, int, int) { PositionHeaderCheckbox(); });
    QObject::connect(m_header_checkbox, &QCheckBox::clicked, this, [this](bool checked) {
        SetAllCheckStates(checked ? Qt::Checked : Qt::Unchecked);
        UpdateHeaderCheckState();
        UpdateLoadButtonEnabled();
    });

    root->addWidget(m_table);

    auto* buttons_layout = new QHBoxLayout();
    m_add_btn = new QPushButton("+", this);
    m_remove_btn = new QPushButton("-", this);
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

    m_add_btn->setAutoDefault(false);
    m_remove_btn->setAutoDefault(false);
    m_load_btn->setAutoDefault(false);
    m_close_btn->setAutoDefault(false);
    m_add_btn->setDefault(false);
    m_remove_btn->setDefault(false);
    m_load_btn->setDefault(false);
    m_close_btn->setDefault(false);

    connect(m_add_btn, &QPushButton::clicked, this, [this]() { AddAfterSelection(); });
    connect(m_remove_btn, &QPushButton::clicked, this, [this]() { RemoveSelected(); });
    connect(m_load_btn, &QPushButton::clicked, this, [this]() { LoadSelected(); });
    connect(m_close_btn, &QPushButton::clicked, this, [this]() { reject(); });

    LoadFromSettings();
    m_updating_checks = true;
    m_table->clearSelection();
    m_table->setCurrentItem(nullptr);
    m_updating_checks = false;
    m_table->setFocus(Qt::OtherFocusReason);
    UpdateLoadButtonEnabled();
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
        const auto* comment = m_table->item(r, 1);
        const auto* filter = m_table->item(r, 2);
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
        auto* chk = new QTableWidgetItem();
        chk->setFlags((chk->flags() | Qt::ItemIsUserCheckable) & ~Qt::ItemIsEditable);
        chk->setCheckState(Qt::Unchecked);
        m_table->setItem(row, 0, chk);

        m_table->setItem(row, 1, new QTableWidgetItem(c));
        m_table->setItem(row, 2, new QTableWidgetItem(f));
    }
    m_updating_checks = true;
    m_table->clearSelection();
    m_table->setCurrentItem(nullptr);
    m_updating_checks = false;
    UpdateHeaderCheckState();
    UpdateLoadButtonEnabled();
}

void LogPresetsDialog::AddAfterSelection() {
    int insert_row = m_table->rowCount();
    const auto selected = m_table->selectionModel()->selectedRows();
    if (!selected.isEmpty()) {
        // place after the last selected row
        insert_row = selected.last().row() + 1;
    }
    m_table->insertRow(insert_row);
    auto* chk = new QTableWidgetItem();
    chk->setFlags((chk->flags() | Qt::ItemIsUserCheckable) & ~Qt::ItemIsEditable);
    chk->setCheckState(Qt::Unchecked);
    m_table->setItem(insert_row, 0, chk);
    m_table->setItem(insert_row, 1, new QTableWidgetItem(""));
    m_table->setItem(insert_row, 2, new QTableWidgetItem(""));
    UpdateHeaderCheckState();
    m_table->setCurrentCell(insert_row, 1);
    m_table->editItem(m_table->item(insert_row, 1));
    UpdateLoadButtonEnabled();
}

void LogPresetsDialog::RemoveSelected() {
    // Prefer checked rows; fall back to selected rows if none checked
    QList<int> to_remove = GetCheckedRows();
    if (to_remove.isEmpty()) {
        const auto selected = m_table->selectionModel()->selectedRows();
        for (const auto& idx : selected)
            to_remove.push_back(idx.row());
    }
    if (to_remove.isEmpty())
        return;
    std::sort(to_remove.begin(), to_remove.end(), [](int a, int b) { return a > b; });
    for (int row : to_remove)
        m_table->removeRow(row);
    UpdateHeaderCheckState();
    UpdateLoadButtonEnabled();
}

void LogPresetsDialog::LoadSelected() {
    QList<int> rows = GetCheckedRows();
    if (rows.isEmpty()) {
        const auto selected = m_table->selectionModel()->selectedRows();
        if (selected.isEmpty())
            return;
        rows.push_back(selected.first().row());
    }
    const int row = rows.first();
    const auto* item = m_table->item(row, 2);
    if (item) {
        emit PresetChosen(item->text());
        accept();
    }
}

void LogPresetsDialog::UpdateHeaderCheckState() {
    const int rows = m_table->rowCount();
    if (rows == 0) {
        if (m_header_checkbox)
            m_header_checkbox->setCheckState(Qt::Unchecked);
        return;
    }
    int checked = 0;
    for (int r = 0; r < rows; ++r) {
        auto* it = m_table->item(r, 0);
        if (it && it->checkState() == Qt::Checked)
            ++checked;
    }
    const auto state = (checked == 0)      ? Qt::Unchecked
                       : (checked == rows) ? Qt::Checked
                                           : Qt::PartiallyChecked;
    if (m_header_checkbox)
        m_header_checkbox->setCheckState(state);
}

void LogPresetsDialog::SetAllCheckStates(Qt::CheckState state) {
    const QSignalBlocker blocker(m_table);
    m_updating_checks = true;
    const int rows = m_table->rowCount();
    auto* sm = m_table->selectionModel();
    for (int r = 0; r < rows; ++r) {
        auto* it = m_table->item(r, 0);
        if (!it) {
            it = new QTableWidgetItem();
            it->setFlags((QTableWidgetItem().flags() | Qt::ItemIsUserCheckable) &
                         ~Qt::ItemIsEditable);
            m_table->setItem(r, 0, it);
        }
        it->setCheckState(state);
        const auto idx = m_table->model()->index(r, 0);
        sm->select(idx, ((state == Qt::Checked) ? QItemSelectionModel::Select
                                                : QItemSelectionModel::Deselect) |
                            QItemSelectionModel::Rows);
    }
    m_updating_checks = false;
}

QList<int> LogPresetsDialog::GetCheckedRows() const {
    QList<int> rows;
    const int count = m_table->rowCount();
    for (int r = 0; r < count; ++r) {
        const auto* it = m_table->item(r, 0);
        if (it && it->checkState() == Qt::Checked)
            rows.push_back(r);
    }
    return rows;
}

void LogPresetsDialog::UpdateLoadButtonEnabled() {
    if (!m_table || !m_table->selectionModel())
        return;
    const int count = m_table->selectionModel()->selectedRows().size();

    if (m_load_btn) {
        const bool showLoad = (count == 1);
        m_load_btn->setVisible(showLoad);
        m_load_btn->setEnabled(showLoad);
    }

    if (m_remove_btn) {
        const bool showRemove = (count >= 1);
        m_remove_btn->setVisible(showRemove);
        m_remove_btn->setEnabled(showRemove);
    }
}

void LogPresetsDialog::PositionHeaderCheckbox() {
    if (!m_header_checkbox)
        return;
    auto* hdr = m_table->horizontalHeader();
    const int section = 0;
    const int x = hdr->sectionViewportPosition(section);
    const int w = hdr->sectionSize(section);
    const int h = hdr->height();
    const QSize sz = m_header_checkbox->sizeHint();

    // Try to align horizontally with cell checkbox indicator for this column
    int left_in_section = 0;
    if (m_table->model()->rowCount() > 0) {
        QStyleOptionViewItem opt;
        opt.initFrom(m_table);
        opt.features = QStyleOptionViewItem::HasCheckIndicator;
        // Use a representative cell rect width (section width) and a typical row height
        const int cell_h = m_table->rowHeight(0) > 0 ? m_table->rowHeight(0) : sz.height();
        opt.rect = QRect(0, 0, w, cell_h);
        const QRect ind =
            m_table->style()->subElementRect(QStyle::SE_ItemViewItemCheckIndicator, &opt, m_table);
        left_in_section = ind.left();
        // Guard against styles that return empty rects
        if (ind.isEmpty()) {
            left_in_section =
                m_table->style()->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, m_table);
        }
    } else {
        left_in_section =
            m_table->style()->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, m_table);
    }

    int cx = x + left_in_section;
    // Center vertically in header
    const int cy = (h - sz.height()) / 2;
    // Ensure checkbox fully visible within the section bounds
    if (cx + sz.width() > x + w)
        cx = x + std::max(0, w - sz.width());
    m_header_checkbox->setGeometry(QRect(QPoint(cx, cy), sz));
    m_header_checkbox->show();
}
