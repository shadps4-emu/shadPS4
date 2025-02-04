// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDialog>
#include "game_info.h"

namespace Ui {
class ControlSettings;
}

class ControlSettings : public QDialog {
    Q_OBJECT
public:
    explicit ControlSettings(std::shared_ptr<GameInfoClass> game_info_get,
                             QWidget* parent = nullptr);
    ~ControlSettings();

private Q_SLOTS:
    void SaveControllerConfig(bool CloseOnSave);
    void SetDefault();

private:
    std::unique_ptr<Ui::ControlSettings> ui;
    std::shared_ptr<GameInfoClass> m_game_info;

    void AddBoxItems();
    void SetUIValuestoMappings();
    void GetGameTitle();

    const std::vector<std::string> ControllerInputs = {
        "cross",        "circle",    "square",      "triangle",    "l1",
        "r1",           "l2",        "r2",          "l3",

        "r3",           "options",   "pad_up",

        "pad_down",

        "pad_left",     "pad_right", "axis_left_x", "axis_left_y", "axis_right_x",
        "axis_right_y", "back"};

    const QStringList ButtonOutputs = {"cross",    "circle",    "square",   "triangle", "l1",
                                       "r1",       "l2",        "r2",       "l3",

                                       "r3",       "options",   "pad_up",

                                       "pad_down",

                                       "pad_left", "pad_right", "touchpad", "unmapped"};

    const QStringList StickOutputs = {"axis_left_x", "axis_left_y", "axis_right_x", "axis_right_y",
                                      "unmapped"};
};
