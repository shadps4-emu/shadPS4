// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fstream>
#include <QKeyEvent>
#include <QMessageBox>
#include <QtConcurrent/qtconcurrentrun.h>
#include <SDL3/SDL.h>

#include "common/config.h"
#include "common/logging/log.h"
#include "common/path_util.h"
#include "hotkeys.h"
#include "input/controller.h"
#include "input/input_handler.h"
#include "sdl_event_wrapper.h"
#include "ui_hotkeys.h"

Hotkeys::Hotkeys(bool isGameRunning, QWidget* parent)
    : QDialog(parent), GameRunning(isGameRunning), ui(new Ui::Hotkeys) {

    ui->setupUi(this);

    if (!GameRunning) {
        SDL_InitSubSystem(SDL_INIT_GAMEPAD);
        SDL_InitSubSystem(SDL_INIT_EVENTS);
    } else {
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    }

    LoadHotkeys();
    CheckGamePad();
    installEventFilter(this);

    PadButtonsList = {ui->fpsButtonPad, ui->quitButtonPad, ui->fullscreenButtonPad,
                      ui->pauseButtonPad, ui->reloadButtonPad};

    KBButtonsList = {ui->fpsButtonKB,         ui->quitButtonKB,   ui->fullscreenButtonKB,
                     ui->pauseButtonKB,       ui->reloadButtonKB, ui->renderdocButton,
                     ui->mouseJoystickButton, ui->mouseGyroButton};

    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, [this](QAbstractButton* button) {
        if (button == ui->buttonBox->button(QDialogButtonBox::Save)) {
            SaveHotkeys(true);
        } else if (button == ui->buttonBox->button(QDialogButtonBox::Apply)) {
            SaveHotkeys(false);
        } else if (button == ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)) {
            SetDefault();
        } else if (button == ui->buttonBox->button(QDialogButtonBox::Cancel)) {
            QWidget::close();
        }
    });

    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QWidget::close);

    ui->buttonBox->button(QDialogButtonBox::Save)->setText(tr("Save"));
    ui->buttonBox->button(QDialogButtonBox::Apply)->setText(tr("Apply"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)->setText(tr("Restore Defaults"));

    for (auto& button : PadButtonsList) {
        connect(button, &QPushButton::clicked, this,
                [this, &button]() { StartTimer(button, true); });
    }

    for (auto& button : KBButtonsList) {
        connect(button, &QPushButton::clicked, this,
                [this, &button]() { StartTimer(button, false); });
    }

    SdlEventWrapper::Wrapper::wrapperActive = true;
    QObject::connect(SdlEventWrapper::Wrapper::GetInstance(), &SdlEventWrapper::Wrapper::SDLEvent,
                     this, &Hotkeys::processSDLEvents);

    if (!GameRunning) {
        Polling = QtConcurrent::run(&Hotkeys::pollSDLEvents, this);
    }
}

void Hotkeys::DisableMappingButtons() {
    for (auto& i : PadButtonsList) {
        i->setEnabled(false);
    }

    for (auto& i : KBButtonsList) {
        i->setEnabled(false);
    }

    ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
    ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
}

void Hotkeys::EnableMappingButtons() {
    for (auto& i : PadButtonsList) {
        i->setEnabled(true);
    }

    for (auto& i : KBButtonsList) {
        i->setEnabled(true);
    }

    ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(true);
    ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
}

void Hotkeys::SetDefault() {

    PadButtonsList = {ui->fpsButtonPad, ui->quitButtonPad, ui->fullscreenButtonPad,
                      ui->pauseButtonPad, ui->reloadButtonPad};

    KBButtonsList = {ui->fpsButtonKB,         ui->quitButtonKB,   ui->fullscreenButtonKB,
                     ui->pauseButtonKB,       ui->reloadButtonKB, ui->renderdocButton,
                     ui->mouseJoystickButton, ui->mouseGyroButton};

    ui->fpsButtonPad->setText("unmapped");
    ui->quitButtonPad->setText("unmapped");
    ui->fullscreenButtonPad->setText("unmapped");
    ui->pauseButtonPad->setText("unmapped");
    ui->reloadButtonPad->setText("unmapped");

    ui->fpsButtonKB->setText("f10");
    ui->quitButtonKB->setText("lctrl, lshift, end");
    ui->fullscreenButtonKB->setText("f11");
    ui->pauseButtonKB->setText("f9");
    ui->reloadButtonKB->setText("f8");

    ui->renderdocButton->setText("f12");
    ui->mouseJoystickButton->setText("f7");
    ui->mouseGyroButton->setText("f6");
}

void Hotkeys::SaveHotkeys(bool CloseOnSave) {
    std::vector<std::string> lines, inputs;

    auto add_mapping = [&](const QString& buttonText, const std::string& output_name) {
        if (buttonText.toStdString() != "unmapped") {
            lines.push_back(output_name + " = " + buttonText.toStdString());
            inputs.push_back(buttonText.toStdString());
        }
    };

    lines.push_back("# Anything put here will be loaded for all games,");
    lines.push_back("# alongside the game's config or default.ini depending on your preference.");
    lines.push_back("");

    add_mapping(ui->fullscreenButtonPad->text(), "hotkey_fullscreen");
    add_mapping(ui->fullscreenButtonKB->text(), "hotkey_fullscreen");
    lines.push_back("");

    add_mapping(ui->pauseButtonPad->text(), "hotkey_pause");
    add_mapping(ui->pauseButtonKB->text(), "hotkey_pause");
    lines.push_back("");

    add_mapping(ui->fpsButtonPad->text(), "hotkey_show_fps");
    add_mapping(ui->fpsButtonKB->text(), "hotkey_show_fps");
    lines.push_back("");

    add_mapping(ui->quitButtonPad->text(), "hotkey_quit");
    add_mapping(ui->quitButtonKB->text(), "hotkey_quit");
    lines.push_back("");

    add_mapping(ui->reloadButtonPad->text(), "hotkey_reload_inputs");
    add_mapping(ui->reloadButtonKB->text(), "hotkey_reload_inputs");
    lines.push_back("");

    add_mapping(ui->renderdocButton->text(), "hotkey_renderdoc_capture");
    add_mapping(ui->mouseJoystickButton->text(), "hotkey_toggle_mouse_to_joystick");
    add_mapping(ui->mouseGyroButton->text(), "hotkey_toggle_mouse_to_gyro");

    auto hotkey_file = Config::GetFoolproofInputConfigFile("global");
    std::fstream file(hotkey_file);
    int lineCount = 0;
    std::string line;
    while (std::getline(file, line)) {
        lineCount++;

        std::size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            if (!line.contains("Anything put here will be loaded for all games") &&
                !line.contains("alongside the game's config or default.ini depending on your"))
                lines.push_back(line);
            continue;
        }

        std::size_t equal_pos = line.find('=');
        if (equal_pos == std::string::npos) {
            lines.push_back(line);
            continue;
        }

        if (!line.contains("hotkey")) {
            lines.push_back(line);
        }
    }
    file.close();

    // Prevent duplicate inputs that break the input engine
    bool duplicateFound = false;
    QSet<QString> duplicateMappings;

    for (auto it = inputs.begin(); it != inputs.end(); ++it) {
        if (std::find(it + 1, inputs.end(), *it) != inputs.end()) {
            duplicateFound = true;
            duplicateMappings.insert(QString::fromStdString(*it));
        }
    }

    if (duplicateFound) {
        QStringList duplicatesList;
        for (const QString& mapping : duplicateMappings) {
            for (auto& button : PadButtonsList) {
                if (button->text() == mapping)
                    duplicatesList.append(button->objectName() + " - " + mapping);
            }

            for (auto& button : KBButtonsList) {
                if (button->text() == mapping)
                    duplicatesList.append(button->objectName() + " - " + mapping);
            }
        }
        QMessageBox::information(
            this, tr("Unable to Save"),
            // clang-format off
            QString(tr("Cannot bind any unique input more than once. Duplicate inputs mapped to the following buttons:\n\n%1").arg(duplicatesList.join("\n"))));
        // clang-format on
        return;
    }

    std::vector<std::string> save;
    bool CurrentLineEmpty = false, LastLineEmpty = false;
    for (auto const& line : lines) {
        LastLineEmpty = CurrentLineEmpty ? true : false;
        CurrentLineEmpty = line.empty() ? true : false;
        if (!CurrentLineEmpty || !LastLineEmpty)
            save.push_back(line);
    }

    std::ofstream output_file(hotkey_file);
    for (auto const& line : save) {
        output_file << line << '\n';
    }
    output_file.close();

    // this also parses global hotkeys
    if (GameRunning)
        Input::ParseInputConfig("default");

    if (CloseOnSave)
        QWidget::close();
}

void Hotkeys::LoadHotkeys() {
    auto hotkey_file = Config::GetFoolproofInputConfigFile("global");
    std::ifstream file(hotkey_file);
    int lineCount = 0;
    std::string line = "";

    while (std::getline(file, line)) {
        lineCount++;

        std::size_t equal_pos = line.find('=');
        if (equal_pos == std::string::npos)
            continue;

        std::string output_string = line.substr(0, equal_pos);
        std::string input_string = line.substr(equal_pos + 2);

        bool controllerInputDetected = false;
        for (const std::string& input : ControllerInputs) {
            // Needed to avoid detecting backspace while detecting back
            if (input_string.contains(input) && !input_string.contains("backspace")) {
                controllerInputDetected = true;
                break;
            }
        }

        if (output_string.contains("hotkey_fullscreen")) {
            controllerInputDetected
                ? ui->fullscreenButtonPad->setText(QString::fromStdString(input_string))
                : ui->fullscreenButtonKB->setText(QString::fromStdString(input_string));
        } else if (output_string.contains("hotkey_show_fps")) {
            controllerInputDetected
                ? ui->fpsButtonPad->setText(QString::fromStdString(input_string))
                : ui->fpsButtonKB->setText(QString::fromStdString(input_string));
        } else if (output_string.contains("hotkey_pause")) {
            controllerInputDetected
                ? ui->pauseButtonPad->setText(QString::fromStdString(input_string))
                : ui->pauseButtonKB->setText(QString::fromStdString(input_string));
        } else if (output_string.contains("hotkey_quit")) {
            controllerInputDetected
                ? ui->quitButtonPad->setText(QString::fromStdString(input_string))
                : ui->quitButtonKB->setText(QString::fromStdString(input_string));
        } else if (output_string.contains("hotkey_reload_inputs")) {
            controllerInputDetected
                ? ui->reloadButtonPad->setText(QString::fromStdString(input_string))
                : ui->reloadButtonKB->setText(QString::fromStdString(input_string));
        } else if (output_string.contains("hotkey_renderdoc_capture")) {
            ui->renderdocButton->setText(QString::fromStdString(input_string));
        } else if (output_string.contains("hotkey_toggle_mouse_to_joystick")) {
            ui->mouseJoystickButton->setText(QString::fromStdString(input_string));
        } else if (output_string.contains("hotkey_toggle_mouse_to_gyro")) {
            ui->mouseGyroButton->setText(QString::fromStdString(input_string));
        }
    }

    file.close();
}

void Hotkeys::CheckGamePad() {
    if (h_gamepad) {
        SDL_CloseGamepad(h_gamepad);
        h_gamepad = nullptr;
    }

    h_gamepads = SDL_GetGamepads(&gamepad_count);

    if (!h_gamepads) {
        LOG_ERROR(Input, "Cannot get gamepad list: {}", SDL_GetError());
        return;
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

void Hotkeys::StartTimer(QPushButton*& button, bool isButton) {
    MappingTimer = 3;
    MappingCompleted = false;
    mapping = button->text();
    DisableMappingButtons();

    isButton ? EnablePadMapping = true : EnableKBMapping = true;
    button->setText(tr("Waiting for inputs") + " [" + QString::number(MappingTimer) + "]");

    timer = new QTimer(this);
    MappingButton = button;
    timer->start(1000);
    connect(timer, &QTimer::timeout, this, [this]() { CheckMapping(MappingButton); });
}

void Hotkeys::CheckMapping(QPushButton*& button) {
    MappingTimer -= 1;
    button->setText(tr("Waiting for inputs") + " [" + QString::number(MappingTimer) + "]");

    if (pressedButtons.size() > 0) {
        QStringList keyStrings;

        for (QString& buttonAction : pressedButtons) {
            keyStrings << buttonAction;
        }

        QString combo = keyStrings.join(", ");
        SetMapping(combo);
        MappingButton->setText(combo);
        pressedButtons.clear();
    }

    if (MappingCompleted || MappingTimer <= 0) {
        button->setText(mapping);
        EnablePadMapping = false;
        EnableKBMapping = false;
        L2Pressed = false;
        R2Pressed = false;
        EnableMappingButtons();
        timer->stop();
    }
}

void Hotkeys::SetMapping(QString input) {
    mapping = input;
    MappingCompleted = true;
}

// use QT events instead of SDL for KB since SDL doesn't allow background KB events
bool Hotkeys::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            if (EnableKBMapping || EnablePadMapping) {
                SetMapping("unmapped");
                CheckMapping(MappingButton);
            }
            return true;
        } else {
            if (EnableKBMapping) {

                if (pressedButtons.size() >= 3) {
                    return true;
                }

                switch (keyEvent->key()) {
                // modifiers
                case Qt::Key_Shift:
                    if (keyEvent->nativeScanCode() == LSHIFT_KEY) {
                        pressedButtons.insert(1, "lshift");
                    } else {
                        pressedButtons.insert(2, "rshift");
                    }
                    break;
                case Qt::Key_Alt:
                    if (keyEvent->nativeScanCode() == LALT_KEY) {
                        pressedButtons.insert(3, "lalt");
                    } else {
                        pressedButtons.insert(4, "ralt");
                    }
                    break;
                case Qt::Key_Control:
                    if (keyEvent->nativeScanCode() == LCTRL_KEY) {
                        pressedButtons.insert(5, "lctrl");
                    } else {
                        pressedButtons.insert(6, "rctrl");
                    }
                    break;
                case Qt::Key_Meta:
#ifdef _WIN32
                    pressedButtons.insert(7, "lwin");
#else
                    pressedButtons.insert(7, "lmeta");
#endif
                    break;

                    // f keys

                case Qt::Key_F1:
                    pressedButtons.insert(8, "f1");
                    break;
                case Qt::Key_F2:
                    pressedButtons.insert(9, "f2");
                    break;
                case Qt::Key_F3:
                    pressedButtons.insert(10, "f3");
                    break;
                case Qt::Key_F4:
                    pressedButtons.insert(11, "f4");
                    break;
                case Qt::Key_F5:
                    pressedButtons.insert(12, "f5");
                    break;
                case Qt::Key_F6:
                    pressedButtons.insert(13, "f6");
                    break;
                case Qt::Key_F7:
                    pressedButtons.insert(14, "f7");
                    break;
                case Qt::Key_F8:
                    pressedButtons.insert(15, "f8");
                    break;
                case Qt::Key_F9:
                    pressedButtons.insert(16, "f9");
                    break;
                case Qt::Key_F10:
                    pressedButtons.insert(17, "f10");
                    break;
                case Qt::Key_F11:
                    pressedButtons.insert(18, "f11");
                    break;
                case Qt::Key_F12:
                    pressedButtons.insert(19, "f12");
                    break;

                // alphanumeric
                case Qt::Key_A:
                    pressedButtons.insert(20, "a");
                    break;
                case Qt::Key_B:
                    pressedButtons.insert(21, "b");
                    break;
                case Qt::Key_C:
                    pressedButtons.insert(22, "c");
                    break;
                case Qt::Key_D:
                    pressedButtons.insert(23, "d");
                    break;
                case Qt::Key_E:
                    pressedButtons.insert(24, "e");
                    break;
                case Qt::Key_F:
                    pressedButtons.insert(25, "f");
                    break;
                case Qt::Key_G:
                    pressedButtons.insert(26, "g");
                    break;
                case Qt::Key_H:
                    pressedButtons.insert(27, "h");
                    break;
                case Qt::Key_I:
                    pressedButtons.insert(28, "i");
                    break;
                case Qt::Key_J:
                    pressedButtons.insert(29, "j");
                    break;
                case Qt::Key_K:
                    pressedButtons.insert(30, "k");
                    break;
                case Qt::Key_L:
                    pressedButtons.insert(31, "l");
                    break;
                case Qt::Key_M:
                    pressedButtons.insert(32, "m");
                    break;
                case Qt::Key_N:
                    pressedButtons.insert(33, "n");
                    break;
                case Qt::Key_O:
                    pressedButtons.insert(34, "o");
                    break;
                case Qt::Key_P:
                    pressedButtons.insert(35, "p");
                    break;
                case Qt::Key_Q:
                    pressedButtons.insert(36, "q");
                    break;
                case Qt::Key_R:
                    pressedButtons.insert(37, "r");
                    break;
                case Qt::Key_S:
                    pressedButtons.insert(38, "s");
                    break;
                case Qt::Key_T:
                    pressedButtons.insert(39, "t");
                    break;
                case Qt::Key_U:
                    pressedButtons.insert(40, "u");
                    break;
                case Qt::Key_V:
                    pressedButtons.insert(41, "v");
                    break;
                case Qt::Key_W:
                    pressedButtons.insert(42, "w");
                    break;
                case Qt::Key_X:
                    pressedButtons.insert(43, "x");
                    break;
                case Qt::Key_Y:
                    pressedButtons.insert(44, "y");
                    break;
                case Qt::Key_Z:
                    pressedButtons.insert(45, "z");
                    break;
                case Qt::Key_0:
                    QApplication::keyboardModifiers() & Qt::KeypadModifier
                        ? pressedButtons.insert(46, "kp0")
                        : pressedButtons.insert(56, "0");
                    break;
                case Qt::Key_1:
                    QApplication::keyboardModifiers() & Qt::KeypadModifier
                        ? pressedButtons.insert(47, "kp1")
                        : pressedButtons.insert(57, "1");
                    break;
                case Qt::Key_2:
                    QApplication::keyboardModifiers() & Qt::KeypadModifier
                        ? pressedButtons.insert(48, "kp2")
                        : pressedButtons.insert(58, "2");
                    break;
                case Qt::Key_3:
                    QApplication::keyboardModifiers() & Qt::KeypadModifier
                        ? pressedButtons.insert(49, "kp3")
                        : pressedButtons.insert(59, "3");
                    break;
                case Qt::Key_4:
                    QApplication::keyboardModifiers() & Qt::KeypadModifier
                        ? pressedButtons.insert(50, "kp4")
                        : pressedButtons.insert(60, "4");
                    break;
                case Qt::Key_5:
                    QApplication::keyboardModifiers() & Qt::KeypadModifier
                        ? pressedButtons.insert(51, "kp5")
                        : pressedButtons.insert(61, "5");
                    break;
                case Qt::Key_6:
                    QApplication::keyboardModifiers() & Qt::KeypadModifier
                        ? pressedButtons.insert(52, "kp6")
                        : pressedButtons.insert(62, "6");
                    break;
                case Qt::Key_7:
                    QApplication::keyboardModifiers() & Qt::KeypadModifier
                        ? pressedButtons.insert(53, "kp7")
                        : pressedButtons.insert(63, "7");
                    break;
                case Qt::Key_8:
                    QApplication::keyboardModifiers() & Qt::KeypadModifier
                        ? pressedButtons.insert(54, "kp8")
                        : pressedButtons.insert(64, "8");
                    break;
                case Qt::Key_9:
                    QApplication::keyboardModifiers() & Qt::KeypadModifier
                        ? pressedButtons.insert(55, "kp9")
                        : pressedButtons.insert(65, "9");
                    break;

                    // symbols
                case Qt::Key_Asterisk:
                    QApplication::keyboardModifiers() & Qt::KeypadModifier
                        ? pressedButtons.insert(66, "kpasterisk")
                        : pressedButtons.insert(74, "asterisk");
                    break;
                case Qt::Key_Minus:
                    QApplication::keyboardModifiers() & Qt::KeypadModifier
                        ? pressedButtons.insert(67, "kpminus")
                        : pressedButtons.insert(75, "minus");
                    break;
                case Qt::Key_Equal:
                    QApplication::keyboardModifiers() & Qt::KeypadModifier
                        ? pressedButtons.insert(68, "kpequals")
                        : pressedButtons.insert(76, "equals");
                    break;
                case Qt::Key_Plus:
                    QApplication::keyboardModifiers() & Qt::KeypadModifier
                        ? pressedButtons.insert(69, "kpplus")
                        : pressedButtons.insert(77, "plus");
                    break;
                case Qt::Key_Slash:
                    QApplication::keyboardModifiers() & Qt::KeypadModifier
                        ? pressedButtons.insert(70, "kpslash")
                        : pressedButtons.insert(78, "slash");
                    break;
                case Qt::Key_Comma:
                    QApplication::keyboardModifiers() & Qt::KeypadModifier
                        ? pressedButtons.insert(71, "kpcomma")
                        : pressedButtons.insert(79, "comma");
                    break;
                case Qt::Key_Period:
                    QApplication::keyboardModifiers() & Qt::KeypadModifier
                        ? pressedButtons.insert(72, "kpperiod")
                        : pressedButtons.insert(80, "period");
                    break;
                case Qt::Key_Enter:
                    QApplication::keyboardModifiers() & Qt::KeypadModifier
                        ? pressedButtons.insert(73, "kpenter")
                        : pressedButtons.insert(81, "enter");
                    break;
                case Qt::Key_QuoteLeft:
                    pressedButtons.insert(82, "grave");
                    break;
                case Qt::Key_AsciiTilde:
                    pressedButtons.insert(83, "tilde");
                    break;
                case Qt::Key_Exclam:
                    pressedButtons.insert(84, "exclamation");
                    break;
                case Qt::Key_At:
                    pressedButtons.insert(85, "at");
                    break;
                case Qt::Key_NumberSign:
                    pressedButtons.insert(86, "hash");
                    break;
                case Qt::Key_Dollar:
                    pressedButtons.insert(87, "dollar");
                    break;
                case Qt::Key_Percent:
                    pressedButtons.insert(88, "percent");
                    break;
                case Qt::Key_AsciiCircum:
                    pressedButtons.insert(89, "caret");
                    break;
                case Qt::Key_Ampersand:
                    pressedButtons.insert(90, "ampersand");
                    break;
                case Qt::Key_ParenLeft:
                    pressedButtons.insert(91, "lparen");
                    break;
                case Qt::Key_ParenRight:
                    pressedButtons.insert(92, "rparen");
                    break;
                case Qt::Key_BracketLeft:
                    pressedButtons.insert(93, "lbracket");
                    break;
                case Qt::Key_BracketRight:
                    pressedButtons.insert(94, "rbracket");
                    break;
                case Qt::Key_BraceLeft:
                    pressedButtons.insert(95, "lbrace");
                    break;
                case Qt::Key_BraceRight:
                    pressedButtons.insert(96, "rbrace");
                    break;
                case Qt::Key_Underscore:
                    pressedButtons.insert(97, "underscore");
                    break;
                case Qt::Key_Backslash:
                    pressedButtons.insert(98, "backslash");
                    break;
                case Qt::Key_Bar:
                    pressedButtons.insert(99, "pipe");
                    break;
                case Qt::Key_Semicolon:
                    pressedButtons.insert(100, "semicolon");
                    break;
                case Qt::Key_Colon:
                    pressedButtons.insert(101, "colon");
                    break;
                case Qt::Key_Apostrophe:
                    pressedButtons.insert(102, "apostrophe");
                    break;
                case Qt::Key_QuoteDbl:
                    pressedButtons.insert(103, "quote");
                    break;
                case Qt::Key_Less:
                    pressedButtons.insert(104, "less");
                    break;
                case Qt::Key_Greater:
                    pressedButtons.insert(105, "greater");
                    break;
                case Qt::Key_Question:
                    pressedButtons.insert(106, "question");
                    break;

                // special keys
                case Qt::Key_Print:
                    pressedButtons.insert(107, "printscreen");
                    break;
                case Qt::Key_ScrollLock:
                    pressedButtons.insert(108, "scrolllock");
                    break;
                case Qt::Key_Pause:
                    pressedButtons.insert(109, "pausebreak");
                    break;
                case Qt::Key_Backspace:
                    pressedButtons.insert(110, "backspace");
                    break;
                case Qt::Key_Insert:
                    pressedButtons.insert(111, "insert");
                    break;
                case Qt::Key_Delete:
                    pressedButtons.insert(112, "delete");
                    break;
                case Qt::Key_Home:
                    pressedButtons.insert(113, "home");
                    break;
                case Qt::Key_End:
                    pressedButtons.insert(114, "end");
                    break;
                case Qt::Key_PageUp:
                    pressedButtons.insert(115, "pgup");
                    break;
                case Qt::Key_PageDown:
                    pressedButtons.insert(116, "pgdown");
                    break;
                case Qt::Key_Tab:
                    pressedButtons.insert(117, "tab");
                    break;
                case Qt::Key_CapsLock:
                    pressedButtons.insert(118, "capslock");
                    break;
                case Qt::Key_Return:
                    pressedButtons.insert(119, "enter");
                    break;
                case Qt::Key_Space:
                    pressedButtons.insert(120, "space");
                    break;
                case Qt::Key_Up:
                    pressedButtons.insert(121, "up");
                    break;
                case Qt::Key_Down:
                    pressedButtons.insert(122, "down");
                    break;
                case Qt::Key_Left:
                    pressedButtons.insert(123, "left");
                    break;
                case Qt::Key_Right:
                    pressedButtons.insert(124, "right");
                    break;
                default:
                    break;
                }
                return true;
            }
        }
    }

    if (event->type() == QEvent::KeyPress && EnableKBMapping) {
        CheckMapping(MappingButton);
        return true;
    }

    return QDialog::eventFilter(obj, event);
}

void Hotkeys::processSDLEvents(int Type, int Input, int Value) {
    if (EnablePadMapping) {

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
                        CheckMapping(MappingButton);
                }
                break;
            case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
                if (Value > 16000) {
                    pressedButtons.insert(2, "r2");
                    R2Pressed = true;
                } else if (Value < 5000) {
                    if (R2Pressed && !L2Pressed)
                        CheckMapping(MappingButton);
                }
                break;
            default:
                break;
            }
        }

        if (Type == SDL_EVENT_GAMEPAD_BUTTON_UP)
            CheckMapping(MappingButton);
    }

    if (Type == SDL_EVENT_GAMEPAD_ADDED || SDL_EVENT_GAMEPAD_REMOVED) {
        CheckGamePad();
    }
}

void Hotkeys::pollSDLEvents() {
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

void Hotkeys::Cleanup() {
    SdlEventWrapper::Wrapper::wrapperActive = false;
    if (h_gamepad) {
        SDL_CloseGamepad(h_gamepad);
        h_gamepad = nullptr;
    }

    if (h_gamepads)
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

Hotkeys::~Hotkeys() {}
