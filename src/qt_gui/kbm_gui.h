// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDialog>
#include "game_info.h"

namespace Ui {
class KBMSettings;
}

class KBMSettings : public QDialog {
    Q_OBJECT
public:
    explicit KBMSettings(std::shared_ptr<GameInfoClass> game_info_get, QWidget* parent = nullptr);
    ~KBMSettings();

private Q_SLOTS:
    void SaveKBMConfig(bool CloseOnSave);
    void SetDefault();
    void CheckMapping(QPushButton*& button);
    void StartTimer(QPushButton*& button);
    void onHelpClicked();

private:
    std::unique_ptr<Ui::KBMSettings> ui;
    std::shared_ptr<GameInfoClass> m_game_info;

#ifdef _WIN32
    const int lctrl = 29;
    const int rctrl = 57373;
    const int lalt = 56;
    const int ralt = 57400;
    const int lshift = 42;
    const int rshift = 54;
#else
    const int lctrl = 37;
    const int rctrl = 105;
    const int lalt = 64;
    const int ralt = 108;
    const int lshift = 50;
    const int rshift = 62;
#endif

    bool eventFilter(QObject* obj, QEvent* event) override;
    void ButtonConnects();
    void SetUIValuestoMappings(std::string config_id);
    void GetGameTitle();
    void DisableMappingButtons();
    void EnableMappingButtons();
    void SetMapping(QString input);

    QSet<QString> pressedKeys;
    bool EnableMapping = false;
    bool MappingCompleted = false;
    bool HelpWindowOpen = false;
    QString mapping;
    QString modifier;
    int MappingTimer;
    QTimer* timer;
    QPushButton* MappingButton;
    QList<QPushButton*> ButtonsList;
    std::string config_id;
    const std::vector<std::string> ControllerInputs = {
        "cross",        "circle",    "square",      "triangle",    "l1",
        "r1",           "l2",        "r2",          "l3",

        "r3",           "options",   "pad_up",

        "pad_down",

        "pad_left",     "pad_right", "axis_left_x", "axis_left_y", "axis_right_x",
        "axis_right_y", "back"};
};
