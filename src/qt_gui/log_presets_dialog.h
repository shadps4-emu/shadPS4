// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDialog>
#include <QPointer>
class QTableWidget;
class QPushButton;
class QTableWidgetItem;
class QCheckBox;

#include "gui_settings.h"

class LogPresetsDialog : public QDialog {
    Q_OBJECT

public:
    explicit LogPresetsDialog(std::shared_ptr<gui_settings> gui_settings,
                              QWidget* parent = nullptr);
    ~LogPresetsDialog() override = default;

signals:
    void PresetChosen(const QString& filter);

protected:
    void accept() override;
    void reject() override;

private:
    void LoadFromSettings();
    void SaveToSettings();
    void AddAfterSelection();
    void RemoveSelected();
    void LoadSelected();
    void UpdateHeaderCheckState();
    void SetAllCheckStates(Qt::CheckState state);
    QList<int> GetCheckedRows() const;
    void UpdateLoadButtonEnabled();
    void PositionHeaderCheckbox();

    QList<QString> SerializeTable() const;
    void PopulateFromList(const QList<QString>& list);

private:
    std::shared_ptr<gui_settings> m_gui_settings;
    QTableWidget* m_table = nullptr;
    QCheckBox* m_header_checkbox = nullptr;
    QPushButton* m_add_btn = nullptr;
    QPushButton* m_remove_btn = nullptr;
    QPushButton* m_load_btn = nullptr;
    QPushButton* m_close_btn = nullptr;
    bool m_updating_checks = false;
};
