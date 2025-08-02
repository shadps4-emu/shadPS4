// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDialog>
#include <QFuture>
#include <SDL3/SDL_gamepad.h>

#include "sdl_event_wrapper.h"

namespace Ui {
class hotkeys;
}

class hotkeys : public QDialog {
    Q_OBJECT

public:
    explicit hotkeys(bool GameRunning, QWidget* parent = nullptr);
    ~hotkeys();

signals:
    void PushGamepadEvent();

private:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void CheckMapping(QPushButton*& button);
    void StartTimer(QPushButton*& button, bool isButton);
    void DisableMappingButtons();
    void EnableMappingButtons();
    void SaveHotkeys(bool CloseOnSave);
    void LoadHotkeys();
    void processSDLEvents(int Type, int Input, int Value);
    void pollSDLEvents();
    void CheckGamePad();
    void SetMapping(QString input);
    void Cleanup();

    bool GameRunning;
    bool EnableButtonMapping = false;
    bool MappingCompleted = false;
    bool L2Pressed = false;
    bool R2Pressed = false;
    int MappingTimer;
    int gamepad_count;
    QString mapping;
    QTimer* timer;
    QPushButton* MappingButton;
    SDL_Gamepad* h_gamepad = nullptr;
    SDL_JoystickID* h_gamepads;

    // use QMap instead of QSet to maintain order of inserted strings
    QMap<int, QString> pressedButtons;
    QList<QPushButton*> ButtonsList;
    QFuture<void> Polling;

    Ui::hotkeys* ui;

protected:
    void closeEvent(QCloseEvent* event) override {
        Cleanup();
    }
};
