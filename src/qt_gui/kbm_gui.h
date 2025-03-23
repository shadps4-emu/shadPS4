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

    bool eventFilter(QObject* obj, QEvent* event) override;
    void ButtonConnects();
    void SetUIValuestoMappings(std::string config_id);
    void GetGameTitle();
    void DisableMappingButtons();
    void EnableMappingButtons();
    void SetMapping(QString input);

    bool EnableMapping = false;
    bool MappingCompleted = false;
    bool HelpWindowOpen = false;
    QString mapping;
    QStringList mappinglist;
    QSet<QString> pressedKeys;
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

#ifdef _WIN32
    int lctrl = 29;
    int rctrl = 57373;
    int lalt = 56;
    int ralt = 57400;
    int lshift = 42;
    int rshift = 54;
#else
    int lctrl = 29;
    int rctrl = 97;
    int lalt = 56;
    int ralt = 100;
    int lshift = 42;
    int rshift = 54;
#endif

};

