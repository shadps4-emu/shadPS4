// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDialog>
#include "game_info.h"

// macros > declaring constants
// also, we were only using one counterpart
#ifdef _WIN32
#define LCTRL_KEY 29
#define LALT_KEY 56
#define LSHIFT_KEY 42
#else
#define LCTRL_KEY 37
#define LALT_KEY 64
#define LSHIFT_KEY 50
#endif

namespace Ui {
class KBMSettings;
}

class KBMSettings : public QDialog {
    Q_OBJECT
public:
    explicit KBMSettings(std::shared_ptr<GameInfoClass> game_info_get, bool GameRunning,
                         std::string GameRunningSerial, QWidget* parent = nullptr);
    ~KBMSettings();

signals:
    void PushKBMEvent();

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
    void Cleanup();

    std::string RunningGameSerial;
    QMap<int, QString> pressedKeys;
    bool GameRunning;
    bool EnableMapping = false;
    bool MappingCompleted = false;
    bool HelpWindowOpen = false;
    QString mapping;
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
