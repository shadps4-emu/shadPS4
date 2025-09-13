// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDialog>
#include <QFuture>
#include <QTimer>
#include <SDL3/SDL_gamepad.h>

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
class Hotkeys;
}

class Hotkeys : public QDialog {
    Q_OBJECT

public:
    explicit Hotkeys(bool GameRunning, QWidget* parent = nullptr);
    ~Hotkeys();

private Q_SLOTS:
    void processSDLEvents(int Type, int Input, int Value);
    void StartTimer(QPushButton*& button, bool isPad);
    void SaveHotkeys(bool CloseOnSave);
    void SetDefault();

private:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void CheckMapping(QPushButton*& button);
    void DisableMappingButtons();
    void EnableMappingButtons();
    void LoadHotkeys();
    void pollSDLEvents();
    void CheckGamePad();
    void SetMapping(QString input);
    void Cleanup();

    bool GameRunning;
    bool EnablePadMapping = false;
    bool EnableKBMapping = false;
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
    QList<QPushButton*> PadButtonsList;
    QList<QPushButton*> KBButtonsList;
    QFuture<void> Polling;

    Ui::Hotkeys* ui;

    const std::vector<std::string> ControllerInputs = {
        "cross",        "circle",    "square",      "triangle",    "l1",
        "r1",           "l2",        "r2",          "l3",

        "r3",           "options",   "pad_up",

        "pad_down",

        "pad_left",     "pad_right", "axis_left_x", "axis_left_y", "axis_right_x",
        "axis_right_y", "back"};

protected:
    void closeEvent(QCloseEvent* event) override {
        Cleanup();
    }

    void accept() override {
        // Blank override to prevent quitting when save button pressed
    }
};
