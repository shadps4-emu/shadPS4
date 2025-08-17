// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fstream>
#include <QKeyEvent>
#include <QtConcurrent>
#include <SDL3/SDL.h>

#include "common/config.h"
#include "common/logging/log.h"
#include "common/path_util.h"
#include "hotkeys.h"
#include "input/input_handler.h"
#include "ui_hotkeys.h"

hotkeys::hotkeys(bool isGameRunning, QWidget* parent)
    : QDialog(parent), GameRunning(isGameRunning), ui(new Ui::hotkeys) {

    ui->setupUi(this);
    installEventFilter(this);

    if (!GameRunning) {
        SDL_InitSubSystem(SDL_INIT_GAMEPAD);
        SDL_InitSubSystem(SDL_INIT_EVENTS);
    } else {
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    }

    LoadHotkeys();
    CheckGamePad();

    ButtonsList = {
        ui->fpsButtonPad,
        ui->quitButtonPad,
        ui->fullscreenButtonPad,
        ui->pauseButtonPad,
    };

    ui->buttonBox->button(QDialogButtonBox::Save)->setText(tr("Save"));
    ui->buttonBox->button(QDialogButtonBox::Apply)->setText(tr("Apply"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));

    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, [this](QAbstractButton* button) {
        if (button == ui->buttonBox->button(QDialogButtonBox::Save)) {
            SaveHotkeys(true);
        } else if (button == ui->buttonBox->button(QDialogButtonBox::Apply)) {
            SaveHotkeys(false);
        } else if (button == ui->buttonBox->button(QDialogButtonBox::Cancel)) {
            QWidget::close();
        }
    });

    for (auto& button : ButtonsList) {
        connect(button, &QPushButton::clicked, this,
                [this, &button]() { StartTimer(button, true); });
    }

    connect(this, &hotkeys::PushGamepadEvent, this, [this]() { CheckMapping(MappingButton); });

    SdlEventWrapper::Wrapper::wrapperActive = true;
    QObject::connect(SdlEventWrapper::Wrapper::GetInstance(), &SdlEventWrapper::Wrapper::SDLEvent,
                     this, &hotkeys::processSDLEvents);

    if (!GameRunning) {
        Polling = QtConcurrent::run(&hotkeys::pollSDLEvents, this);
    }
}

void hotkeys::DisableMappingButtons() {
    for (const auto& i : ButtonsList) {
        i->setEnabled(false);
    }
}

void hotkeys::EnableMappingButtons() {
    for (const auto& i : ButtonsList) {
        i->setEnabled(true);
    }
}

void hotkeys::SaveHotkeys(bool CloseOnSave) {
    const auto hotkey_file = Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "hotkeys.ini";
    if (!std::filesystem::exists(hotkey_file)) {
        Input::createHotkeyFile(hotkey_file);
    }

    QString controllerFullscreenString, controllerPauseString, controllerFpsString,
        controllerQuitString = "";
    std::ifstream file(hotkey_file);
    int lineCount = 0;
    std::string line = "";
    std::vector<std::string> lines;

    while (std::getline(file, line)) {
        lineCount++;

        std::size_t equal_pos = line.find('=');
        if (equal_pos == std::string::npos) {
            lines.push_back(line);
            continue;
        }

        if (line.contains("controllerFullscreen")) {
            line = "controllerFullscreen = " + ui->fullscreenButtonPad->text().toStdString();
        } else if (line.contains("controllerQuit")) {
            line = "controllerQuit = " + ui->quitButtonPad->text().toStdString();
        } else if (line.contains("controllerFps")) {
            line = "controllerFps = " + ui->fpsButtonPad->text().toStdString();
        } else if (line.contains("controllerPause")) {
            line = "controllerPause = " + ui->pauseButtonPad->text().toStdString();
        }

        lines.push_back(line);
    }

    file.close();

    std::ofstream output_file(hotkey_file);
    for (auto const& line : lines) {
        output_file << line << '\n';
    }
    output_file.close();

    Input::LoadHotkeyInputs();

    if (CloseOnSave)
        QWidget::close();
}

void hotkeys::LoadHotkeys() {
    const auto hotkey_file = Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "hotkeys.ini";
    if (!std::filesystem::exists(hotkey_file)) {
        Input::createHotkeyFile(hotkey_file);
    }

    QString controllerFullscreenString, controllerPauseString, controllerFpsString,
        controllerQuitString = "";
    std::ifstream file(hotkey_file);
    int lineCount = 0;
    std::string line = "";

    while (std::getline(file, line)) {
        lineCount++;

        std::size_t equal_pos = line.find('=');
        if (equal_pos == std::string::npos)
            continue;

        if (line.contains("controllerFullscreen")) {
            controllerFullscreenString = QString::fromStdString(line.substr(equal_pos + 2));
        } else if (line.contains("controllerQuit")) {
            controllerQuitString = QString::fromStdString(line.substr(equal_pos + 2));
        } else if (line.contains("controllerFps")) {
            controllerFpsString = QString::fromStdString(line.substr(equal_pos + 2));
        } else if (line.contains("controllerPause")) {
            controllerPauseString = QString::fromStdString(line.substr(equal_pos + 2));
        }
    }

    file.close();

    ui->fpsButtonPad->setText(controllerFpsString);
    ui->quitButtonPad->setText(controllerQuitString);
    ui->fullscreenButtonPad->setText(controllerFullscreenString);
    ui->pauseButtonPad->setText(controllerPauseString);
}

void hotkeys::CheckGamePad() {
    if (h_gamepad) {
        SDL_CloseGamepad(h_gamepad);
        h_gamepad = nullptr;
    }

    h_gamepads = SDL_GetGamepads(&gamepad_count);

    if (!h_gamepads) {
        LOG_ERROR(Input, "Cannot get gamepad list: {}", SDL_GetError());
    }

    int defaultIndex = GamepadSelect::GetIndexfromGUID(h_gamepads, gamepad_count,
                                                       Config::getDefaultControllerID());
    int activeIndex = GamepadSelect::GetIndexfromGUID(h_gamepads, gamepad_count,
                                                      GamepadSelect::GetSelectedGamepad());

    if (!GameRunning) {
        if (activeIndex != -1) {
            h_gamepad = SDL_OpenGamepad(h_gamepads[activeIndex]);
        } else if (defaultIndex != -1) {
            h_gamepad = SDL_OpenGamepad(h_gamepads[defaultIndex]);
        } else {
            LOG_INFO(Input, "Got {} gamepads. Opening the first one.", gamepad_count);
            h_gamepad = SDL_OpenGamepad(h_gamepads[0]);
        }

        if (!h_gamepad) {
            LOG_ERROR(Input, "Failed to open gamepad: {}", SDL_GetError());
        }
    }
}

void hotkeys::StartTimer(QPushButton*& button, bool isButton) {
    MappingTimer = 3;
    EnableButtonMapping = true;
    MappingCompleted = false;
    L2Pressed = false;
    R2Pressed = false;
    mapping = button->text();
    DisableMappingButtons();

    button->setText(tr("Press a button") + " [" + QString::number(MappingTimer) + "]");

    timer = new QTimer(this);
    MappingButton = button;
    timer->start(1000);
    connect(timer, &QTimer::timeout, this, [this]() { CheckMapping(MappingButton); });
}

void hotkeys::CheckMapping(QPushButton*& button) {
    MappingTimer -= 1;
    button->setText(tr("Press a button") + " [" + QString::number(MappingTimer) + "]");

    if (pressedButtons.size() > 0) {
        QStringList keyStrings;

        for (const QString& buttonAction : pressedButtons) {
            keyStrings << buttonAction;
        }

        QString combo = keyStrings.join(",");
        SetMapping(combo);
        MappingButton->setText(combo);
        pressedButtons.clear();
    }

    if (MappingCompleted || MappingTimer <= 0) {
        button->setText(mapping);
        EnableButtonMapping = false;
        EnableMappingButtons();
        timer->stop();
    }
}

void hotkeys::SetMapping(QString input) {
    mapping = input;
    MappingCompleted = true;
}

// use QT events instead of SDL to override default event closing the window with escape
bool hotkeys::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::KeyPress && EnableButtonMapping) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            SetMapping("unmapped");
            PushGamepadEvent();
            return true;
        }
    }
    return QDialog::eventFilter(obj, event);
}

void hotkeys::processSDLEvents(int Type, int Input, int Value) {
    if (EnableButtonMapping) {

        if (pressedButtons.size() >= 3) {
            return;
        }

        if (Type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
            switch (Input) {
            case SDL_GAMEPAD_BUTTON_SOUTH:
                pressedButtons.insert(5, "cross");
                break;
            case SDL_GAMEPAD_BUTTON_EAST:
                pressedButtons.insert(6, "circle");
                break;
            case SDL_GAMEPAD_BUTTON_NORTH:
                pressedButtons.insert(7, "triangle");
                break;
            case SDL_GAMEPAD_BUTTON_WEST:
                pressedButtons.insert(8, "square");
                break;
            case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
                pressedButtons.insert(3, "l1");
                break;
            case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
                pressedButtons.insert(4, "r1");
                break;
            case SDL_GAMEPAD_BUTTON_LEFT_STICK:
                pressedButtons.insert(9, "l3");
                break;
            case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
                pressedButtons.insert(10, "r3");
                break;
            case SDL_GAMEPAD_BUTTON_DPAD_UP:
                pressedButtons.insert(13, "pad_up");
                break;
            case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
                pressedButtons.insert(14, "pad_down");
                break;
            case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
                pressedButtons.insert(15, "pad_left");
                break;
            case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
                pressedButtons.insert(16, "pad_right");
                break;
            case SDL_GAMEPAD_BUTTON_BACK:
                pressedButtons.insert(11, "back");
                break;
            case SDL_GAMEPAD_BUTTON_START:
                pressedButtons.insert(12, "options");
                break;
            default:
                break;
            }
        }

        if (Type == SDL_EVENT_GAMEPAD_AXIS_MOTION) {
            // SDL trigger axis values range from 0 to 32000, set mapping on half movement
            // Set zone for trigger release signal arbitrarily at 5000
            switch (Input) {
            case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
                if (Value > 16000) {
                    pressedButtons.insert(1, "l2");
                    L2Pressed = true;
                } else if (Value < 5000) {
                    if (L2Pressed && !R2Pressed)
                        emit PushGamepadEvent();
                }
                break;
            case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
                if (Value > 16000) {
                    pressedButtons.insert(2, "r2");
                    R2Pressed = true;
                } else if (Value < 5000) {
                    if (R2Pressed && !L2Pressed)
                        emit PushGamepadEvent();
                }
                break;
            default:
                break;
            }
        }

        if (Type == SDL_EVENT_GAMEPAD_BUTTON_UP)
            emit PushGamepadEvent();
    }

    if (Type == SDL_EVENT_GAMEPAD_ADDED || SDL_EVENT_GAMEPAD_REMOVED) {
        CheckGamePad();
    }
}

void hotkeys::pollSDLEvents() {
    SDL_Event event;
    while (SdlEventWrapper::Wrapper::wrapperActive) {

        if (!SDL_WaitEvent(&event)) {
            return;
        }

        if (event.type == SDL_EVENT_QUIT) {
            return;
        }

        SdlEventWrapper::Wrapper::GetInstance()->Wrapper::ProcessEvent(&event);
    }
}

void hotkeys::Cleanup() {
    SdlEventWrapper::Wrapper::wrapperActive = false;
    if (h_gamepad) {
        SDL_CloseGamepad(h_gamepad);
        h_gamepad = nullptr;
    }

    SDL_free(h_gamepads);

    if (!GameRunning) {
        SDL_Event quitLoop{};
        quitLoop.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&quitLoop);
        Polling.waitForFinished();

        SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
        SDL_QuitSubSystem(SDL_INIT_EVENTS);
        SDL_Quit();
    } else {
        if (!Config::getBackgroundControllerInput()) {
            SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "0");
        }
    }
}

hotkeys::~hotkeys() {}
