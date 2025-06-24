// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDialog>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gamepad.h>
#include "game_info.h"
#include "sdl_event_wrapper.h"

namespace Ui {
class ControlSettings;
}

class ControlSettings : public QDialog {
    Q_OBJECT
public:
    explicit ControlSettings(std::shared_ptr<GameInfoClass> game_info_get, bool GameRunning,
                             QWidget* parent = nullptr);
    ~ControlSettings();

signals:
    void PushGamepadEvent();
    void AxisChanged();

private Q_SLOTS:
    void SaveControllerConfig(bool CloseOnSave);
    void SetDefault();
    void UpdateLightbarColor();
    void CheckMapping(QPushButton*& button);
    void StartTimer(QPushButton*& button, bool isButton);
    void ConnectAxisInputs(QPushButton*& button);

private:
    std::unique_ptr<Ui::ControlSettings> ui;
    std::shared_ptr<GameInfoClass> m_game_info;

    bool eventFilter(QObject* obj, QEvent* event) override;
    void AddBoxItems();
    void SetUIValuestoMappings();
    void GetGameTitle();
    void CheckGamePad();
    void processSDLEvents(int Type, int Input, int Value);
    void pollSDLEvents();
    void SetMapping(QString input);
    void DisableMappingButtons();
    void EnableMappingButtons();

    QList<QPushButton*> ButtonsList;
    QList<QPushButton*> AxisList;
    QSet<QString> pressedButtons;

    bool GameRunning;
    bool EnableButtonMapping = false;
    bool EnableAxisMapping = false;
    bool MappingCompleted = false;
    QString mapping;
    int MappingTimer;
    QTimer* timer;
    QPushButton* MappingButton;
    SDL_Gamepad* gamepad = nullptr;
    SdlEventWrapper::Wrapper* RemapWrapper;

    const std::vector<std::string> ControllerInputs = {
        "cross",        "circle",    "square",      "triangle",    "l1",
        "r1",           "l2",        "r2",          "l3",

        "r3",           "options",   "pad_up",

        "pad_down",

        "pad_left",     "pad_right", "axis_left_x", "axis_left_y", "axis_right_x",
        "axis_right_y", "back"};

protected:
    void closeEvent(QCloseEvent* event) override {
        SDL_Event quitLoop{};
        quitLoop.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&quitLoop);
    }
};
