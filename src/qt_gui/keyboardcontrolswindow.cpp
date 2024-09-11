// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "./ui_keyboardcontrolswindow.h"
#include "common/config.h"
#include "keyboardcontrolswindow.h"

#include <QMessageBox>
#include <QTimer>

#include <iostream>

namespace {
static constexpr auto keyBindingsSettingsKey = "ShadPS4_Keyboard_Settings_KEY";
static constexpr auto inputErrorTimerTimeout = 2000;
} // namespace

void showError(const QString& message) {
    QMessageBox::critical(nullptr, "Error", message, QMessageBox::Ok);
}

void showWarning(const QString& message) {
    QMessageBox::warning(nullptr, "Warning", message, QMessageBox::Ok);
}

void showInfo(const QString& message) {
    QMessageBox::information(nullptr, "Info", message, QMessageBox::Ok);
}

KeyboardControlsWindow::KeyboardControlsWindow(QWidget* parent)
    : QDialog(parent), ui(new Ui::KeyboardControlsWindow) {
    ui->setupUi(this);

    m_keysMap = Config::getKeyboardBindingMap();

    for (auto& pair : m_keysMap) {
        m_reverseKeysMap.emplace(pair.second, pair.first);
    }

    m_listOfKeySequenceEdits = {ui->StartKeySequenceEdit,        ui->SelectKeySequenceEdit,
                                ui->LAnalogDownkeySequenceEdit,  ui->LAnalogLeftkeySequenceEdit,
                                ui->LAnalogUpkeySequenceEdit,    ui->LAnalogRightkeySequenceEdit,
                                ui->PSkeySequenceEdit,           ui->RAnalogDownkeySequenceEdit,
                                ui->RAnalogLeftkeySequenceEdit,  ui->RAnalogUpkeySequenceEdit,
                                ui->RAnalogRightkeySequenceEdit, ui->DPadLeftkeySequenceEdit,
                                ui->DPadRightkeySequenceEdit,    ui->DPadUpkeySequenceEdit,
                                ui->DPadDownkeySequenceEdit,     ui->L2keySequenceEdit,
                                ui->L1keySequenceEdit,           ui->CrossKeySequenceEdit,
                                ui->R2KeySequenceEdit,           ui->CircleKeySequenceEdit,
                                ui->R1KeySequenceEdit,           ui->SquareKeySequenceEdit,
                                ui->TriangleKeySequenceEdit};

    for (auto edit : m_listOfKeySequenceEdits) {
        edit->setStyleSheet("QLineEdit { qproperty-alignment: AlignCenter; }");
        QObject::connect(edit, &QKeySequenceEdit::editingFinished, this,
                         &KeyboardControlsWindow::onEditingFinished);
    }

    ui->StartKeySequenceEdit->setKeySequence(
        convertSDLKeyToQt(m_reverseKeysMap[KeysMapping::Start_Key]));
    ui->SelectKeySequenceEdit->setKeySequence(
        convertSDLKeyToQt(m_reverseKeysMap[KeysMapping::Select_Key]));
    ui->LAnalogDownkeySequenceEdit->setKeySequence(
        convertSDLKeyToQt(m_reverseKeysMap[KeysMapping::LAnalogDown_Key]));
    ui->LAnalogLeftkeySequenceEdit->setKeySequence(
        convertSDLKeyToQt(m_reverseKeysMap[KeysMapping::LAnalogLeft_Key]));
    ui->LAnalogUpkeySequenceEdit->setKeySequence(
        convertSDLKeyToQt(m_reverseKeysMap[KeysMapping::LAnalogUp_Key]));
    ui->LAnalogRightkeySequenceEdit->setKeySequence(
        convertSDLKeyToQt(m_reverseKeysMap[KeysMapping::LAnalogRight_Key]));
    ui->PSkeySequenceEdit->setKeySequence(convertSDLKeyToQt(m_reverseKeysMap[KeysMapping::PS_Key]));
    ui->RAnalogDownkeySequenceEdit->setKeySequence(
        convertSDLKeyToQt(m_reverseKeysMap[KeysMapping::RAnalogDown_Key]));
    ui->RAnalogLeftkeySequenceEdit->setKeySequence(
        convertSDLKeyToQt(m_reverseKeysMap[KeysMapping::RAnalogLeft_Key]));
    ui->RAnalogUpkeySequenceEdit->setKeySequence(
        convertSDLKeyToQt(m_reverseKeysMap[KeysMapping::RAnalogUp_Key]));
    ui->RAnalogRightkeySequenceEdit->setKeySequence(
        convertSDLKeyToQt(m_reverseKeysMap[KeysMapping::RAnalogRight_Key]));
    ui->DPadLeftkeySequenceEdit->setKeySequence(
        convertSDLKeyToQt(m_reverseKeysMap[KeysMapping::DPadLeft_Key]));
    ui->DPadRightkeySequenceEdit->setKeySequence(
        convertSDLKeyToQt(m_reverseKeysMap[KeysMapping::DPadRight_Key]));
    ui->DPadUpkeySequenceEdit->setKeySequence(
        convertSDLKeyToQt(m_reverseKeysMap[KeysMapping::DPadUp_Key]));
    ui->DPadDownkeySequenceEdit->setKeySequence(
        convertSDLKeyToQt(m_reverseKeysMap[KeysMapping::DPadDown_Key]));
    ui->L2keySequenceEdit->setKeySequence(convertSDLKeyToQt(m_reverseKeysMap[KeysMapping::L2_Key]));
    ui->L1keySequenceEdit->setKeySequence(convertSDLKeyToQt(m_reverseKeysMap[KeysMapping::L1_Key]));
    ui->CrossKeySequenceEdit->setKeySequence(
        convertSDLKeyToQt(m_reverseKeysMap[KeysMapping::Cross_Key]));
    ui->R2KeySequenceEdit->setKeySequence(convertSDLKeyToQt(m_reverseKeysMap[KeysMapping::R2_Key]));
    ui->CircleKeySequenceEdit->setKeySequence(
        convertSDLKeyToQt(m_reverseKeysMap[KeysMapping::Circle_Key]));
    ui->R1KeySequenceEdit->setKeySequence(convertSDLKeyToQt(m_reverseKeysMap[KeysMapping::R1_Key]));
    ui->SquareKeySequenceEdit->setKeySequence(
        convertSDLKeyToQt(m_reverseKeysMap[KeysMapping::Square_Key]));
    ui->TriangleKeySequenceEdit->setKeySequence(
        convertSDLKeyToQt(m_reverseKeysMap[KeysMapping::Triangle_Key]));

    QObject::connect(ui->applyButton, &QPushButton::clicked,
                     [this]() { validateAndSaveKeyBindings(); });

    QObject::connect(ui->cancelButton, &QPushButton::clicked, [this]() { this->close(); });
}

KeyboardControlsWindow::~KeyboardControlsWindow() {
    delete ui;
}

const std::map<u32, KeysMapping>& KeyboardControlsWindow::getKeysMapping() const {
    return m_keysMap;
}

void KeyboardControlsWindow::validateAndSaveKeyBindings() {
    int nOfUnconfiguredButtons = 0;
    for (auto& keyEdit : m_listOfKeySequenceEdits) {
        auto keySequence = keyEdit->keySequence();
        // If key sequence is empty (i.e. there is no key assigned to it) we highlight it in red
        if (keySequence.isEmpty()) {
            keyEdit->setStyleSheet("background-color: red; qproperty-alignment: AlignCenter;");
            QTimer::singleShot(inputErrorTimerTimeout, keyEdit, [keyEdit]() {
                keyEdit->setStyleSheet("qproperty-alignment: AlignCenter;"); // Reset to default
            });

            ++nOfUnconfiguredButtons;
        }
    }

    if (nOfUnconfiguredButtons > 0) {
        showError("Some of the buttons were not configured");
        return;
    }

    m_keysMap.clear();
    m_reverseKeysMap.clear();

    m_keysMap.emplace(convertQtKeyToSDL(ui->LAnalogDownkeySequenceEdit->keySequence()[0].key()),
                      KeysMapping::LAnalogDown_Key);
    m_keysMap.emplace(convertQtKeyToSDL(ui->LAnalogLeftkeySequenceEdit->keySequence()[0].key()),
                      KeysMapping::LAnalogLeft_Key);
    m_keysMap.emplace(convertQtKeyToSDL(ui->LAnalogUpkeySequenceEdit->keySequence()[0].key()),
                      KeysMapping::LAnalogUp_Key);
    m_keysMap.emplace(convertQtKeyToSDL(ui->LAnalogRightkeySequenceEdit->keySequence()[0].key()),
                      KeysMapping::LAnalogRight_Key);

    m_keysMap.emplace(convertQtKeyToSDL(ui->PSkeySequenceEdit->keySequence()[0].key()),
                      KeysMapping::PS_Key);
    m_keysMap.emplace(convertQtKeyToSDL(ui->StartKeySequenceEdit->keySequence()[0].key()),
                      KeysMapping::Start_Key);
    m_keysMap.emplace(convertQtKeyToSDL(ui->SelectKeySequenceEdit->keySequence()[0].key()),
                      KeysMapping::Select_Key);

    m_keysMap.emplace(convertQtKeyToSDL(ui->RAnalogDownkeySequenceEdit->keySequence()[0].key()),
                      KeysMapping::RAnalogDown_Key);
    m_keysMap.emplace(convertQtKeyToSDL(ui->RAnalogLeftkeySequenceEdit->keySequence()[0].key()),
                      KeysMapping::RAnalogLeft_Key);
    m_keysMap.emplace(convertQtKeyToSDL(ui->RAnalogUpkeySequenceEdit->keySequence()[0].key()),
                      KeysMapping::RAnalogUp_Key);
    m_keysMap.emplace(convertQtKeyToSDL(ui->RAnalogRightkeySequenceEdit->keySequence()[0].key()),
                      KeysMapping::RAnalogRight_Key);

    m_keysMap.emplace(convertQtKeyToSDL(ui->DPadLeftkeySequenceEdit->keySequence()[0].key()),
                      KeysMapping::DPadLeft_Key);
    m_keysMap.emplace(convertQtKeyToSDL(ui->DPadRightkeySequenceEdit->keySequence()[0].key()),
                      KeysMapping::DPadRight_Key);
    m_keysMap.emplace(convertQtKeyToSDL(ui->DPadUpkeySequenceEdit->keySequence()[0].key()),
                      KeysMapping::DPadUp_Key);
    m_keysMap.emplace(convertQtKeyToSDL(ui->DPadDownkeySequenceEdit->keySequence()[0].key()),
                      KeysMapping::DPadDown_Key);

    m_keysMap.emplace(convertQtKeyToSDL(ui->L1keySequenceEdit->keySequence()[0].key()),
                      KeysMapping::L1_Key);
    m_keysMap.emplace(convertQtKeyToSDL(ui->L2keySequenceEdit->keySequence()[0].key()),
                      KeysMapping::L2_Key);
    m_keysMap.emplace(convertQtKeyToSDL(ui->R1KeySequenceEdit->keySequence()[0].key()),
                      KeysMapping::R1_Key);
    m_keysMap.emplace(convertQtKeyToSDL(ui->R2KeySequenceEdit->keySequence()[0].key()),
                      KeysMapping::R2_Key);

    m_keysMap.emplace(convertQtKeyToSDL(ui->CrossKeySequenceEdit->keySequence()[0].key()),
                      KeysMapping::Cross_Key);
    m_keysMap.emplace(convertQtKeyToSDL(ui->CircleKeySequenceEdit->keySequence()[0].key()),
                      KeysMapping::Circle_Key);
    m_keysMap.emplace(convertQtKeyToSDL(ui->SquareKeySequenceEdit->keySequence()[0].key()),
                      KeysMapping::Square_Key);
    m_keysMap.emplace(convertQtKeyToSDL(ui->TriangleKeySequenceEdit->keySequence()[0].key()),
                      KeysMapping::Triangle_Key);

    for (auto& pair : m_keysMap) {
        m_reverseKeysMap.emplace(pair.second, pair.first);
    }

    // Saving into settings (for permanent storage)
    Config::setKeyboardBindingMap(m_keysMap);

    this->close();
}

Qt::Key KeyboardControlsWindow::convertSDLKeyToQt(SDL_Keycode sdlKey) {
    switch (sdlKey) {
    case SDLK_A:
        return Qt::Key_A;
    case SDLK_B:
        return Qt::Key_B;
    case SDLK_C:
        return Qt::Key_C;
    case SDLK_D:
        return Qt::Key_D;
    case SDLK_E:
        return Qt::Key_E;
    case SDLK_F:
        return Qt::Key_F;
    case SDLK_G:
        return Qt::Key_G;
    case SDLK_H:
        return Qt::Key_H;
    case SDLK_I:
        return Qt::Key_I;
    case SDLK_J:
        return Qt::Key_J;
    case SDLK_K:
        return Qt::Key_K;
    case SDLK_L:
        return Qt::Key_L;
    case SDLK_M:
        return Qt::Key_M;
    case SDLK_N:
        return Qt::Key_N;
    case SDLK_O:
        return Qt::Key_O;
    case SDLK_P:
        return Qt::Key_P;
    case SDLK_Q:
        return Qt::Key_Q;
    case SDLK_R:
        return Qt::Key_R;
    case SDLK_S:
        return Qt::Key_S;
    case SDLK_T:
        return Qt::Key_T;
    case SDLK_U:
        return Qt::Key_U;
    case SDLK_V:
        return Qt::Key_V;
    case SDLK_W:
        return Qt::Key_W;
    case SDLK_X:
        return Qt::Key_X;
    case SDLK_Y:
        return Qt::Key_Y;
    case SDLK_Z:
        return Qt::Key_Z;
    case SDLK_0:
        return Qt::Key_0;
    case SDLK_1:
        return Qt::Key_1;
    case SDLK_2:
        return Qt::Key_2;
    case SDLK_3:
        return Qt::Key_3;
    case SDLK_4:
        return Qt::Key_4;
    case SDLK_5:
        return Qt::Key_5;
    case SDLK_6:
        return Qt::Key_6;
    case SDLK_7:
        return Qt::Key_7;
    case SDLK_8:
        return Qt::Key_8;
    case SDLK_9:
        return Qt::Key_9;
    case SDLK_SPACE:
        return Qt::Key_Space;
    case SDLK_RETURN:
        return Qt::Key_Return;
    case SDLK_ESCAPE:
        return Qt::Key_Escape;
    case SDLK_TAB:
        return Qt::Key_Tab;
    case SDLK_BACKSPACE:
        return Qt::Key_Backspace;
    case SDLK_DELETE:
        return Qt::Key_Delete;
    case SDLK_INSERT:
        return Qt::Key_Insert;
    case SDLK_HOME:
        return Qt::Key_Home;
    case SDLK_END:
        return Qt::Key_End;
    case SDLK_PAGEUP:
        return Qt::Key_PageUp;
    case SDLK_PAGEDOWN:
        return Qt::Key_PageDown;
    case SDLK_LEFT:
        return Qt::Key_Left;
    case SDLK_RIGHT:
        return Qt::Key_Right;
    case SDLK_UP:
        return Qt::Key_Up;
    case SDLK_DOWN:
        return Qt::Key_Down;
    case SDLK_CAPSLOCK:
        return Qt::Key_CapsLock;
    case SDLK_NUMLOCKCLEAR:
        return Qt::Key_NumLock;
    case SDLK_SCROLLLOCK:
        return Qt::Key_ScrollLock;
    case SDLK_F1:
        return Qt::Key_F1;
    case SDLK_F2:
        return Qt::Key_F2;
    case SDLK_F3:
        return Qt::Key_F3;
    case SDLK_F4:
        return Qt::Key_F4;
    case SDLK_F5:
        return Qt::Key_F5;
    case SDLK_F6:
        return Qt::Key_F6;
    case SDLK_F7:
        return Qt::Key_F7;
    case SDLK_F8:
        return Qt::Key_F8;
    case SDLK_F9:
        return Qt::Key_F9;
    case SDLK_F10:
        return Qt::Key_F10;
    case SDLK_F11:
        return Qt::Key_F11;
    case SDLK_F12:
        return Qt::Key_F12;
    case SDLK_LSHIFT:
        return Qt::Key_Shift;
    case SDLK_LCTRL:
        return Qt::Key_Control;
    case SDLK_LALT:
        return Qt::Key_Alt;
    case SDLK_LGUI:
        return Qt::Key_Meta;
    default:
        return Qt::Key_unknown;
    }
}

SDL_Keycode KeyboardControlsWindow::convertQtKeyToSDL(Qt::Key qtKey) {
    switch (qtKey) {
    case Qt::Key_A:
        return SDLK_A;
    case Qt::Key_B:
        return SDLK_B;
    case Qt::Key_C:
        return SDLK_C;
    case Qt::Key_D:
        return SDLK_D;
    case Qt::Key_E:
        return SDLK_E;
    case Qt::Key_F:
        return SDLK_F;
    case Qt::Key_G:
        return SDLK_G;
    case Qt::Key_H:
        return SDLK_H;
    case Qt::Key_I:
        return SDLK_I;
    case Qt::Key_J:
        return SDLK_J;
    case Qt::Key_K:
        return SDLK_K;
    case Qt::Key_L:
        return SDLK_L;
    case Qt::Key_M:
        return SDLK_M;
    case Qt::Key_N:
        return SDLK_N;
    case Qt::Key_O:
        return SDLK_O;
    case Qt::Key_P:
        return SDLK_P;
    case Qt::Key_Q:
        return SDLK_Q;
    case Qt::Key_R:
        return SDLK_R;
    case Qt::Key_S:
        return SDLK_S;
    case Qt::Key_T:
        return SDLK_T;
    case Qt::Key_U:
        return SDLK_U;
    case Qt::Key_V:
        return SDLK_V;
    case Qt::Key_W:
        return SDLK_W;
    case Qt::Key_X:
        return SDLK_X;
    case Qt::Key_Y:
        return SDLK_Y;
    case Qt::Key_Z:
        return SDLK_Z;
    case Qt::Key_0:
        return SDLK_0;
    case Qt::Key_1:
        return SDLK_1;
    case Qt::Key_2:
        return SDLK_2;
    case Qt::Key_3:
        return SDLK_3;
    case Qt::Key_4:
        return SDLK_4;
    case Qt::Key_5:
        return SDLK_5;
    case Qt::Key_6:
        return SDLK_6;
    case Qt::Key_7:
        return SDLK_7;
    case Qt::Key_8:
        return SDLK_8;
    case Qt::Key_9:
        return SDLK_9;
    case Qt::Key_Space:
        return SDLK_SPACE;
    case Qt::Key_Enter:
        return SDLK_RETURN;
    case Qt::Key_Return:
        return SDLK_RETURN;
    case Qt::Key_Escape:
        return SDLK_ESCAPE;
    case Qt::Key_Tab:
        return SDLK_TAB;
    case Qt::Key_Backspace:
        return SDLK_BACKSPACE;
    case Qt::Key_Delete:
        return SDLK_DELETE;
    case Qt::Key_Insert:
        return SDLK_INSERT;
    case Qt::Key_Home:
        return SDLK_HOME;
    case Qt::Key_End:
        return SDLK_END;
    case Qt::Key_PageUp:
        return SDLK_PAGEUP;
    case Qt::Key_PageDown:
        return SDLK_PAGEDOWN;
    case Qt::Key_Left:
        return SDLK_LEFT;
    case Qt::Key_Right:
        return SDLK_RIGHT;
    case Qt::Key_Up:
        return SDLK_UP;
    case Qt::Key_Down:
        return SDLK_DOWN;
    case Qt::Key_CapsLock:
        return SDLK_CAPSLOCK;
    case Qt::Key_NumLock:
        return SDLK_NUMLOCKCLEAR;
    case Qt::Key_ScrollLock:
        return SDLK_SCROLLLOCK;
    case Qt::Key_F1:
        return SDLK_F1;
    case Qt::Key_F2:
        return SDLK_F2;
    case Qt::Key_F3:
        return SDLK_F3;
    case Qt::Key_F4:
        return SDLK_F4;
    case Qt::Key_F5:
        return SDLK_F5;
    case Qt::Key_F6:
        return SDLK_F6;
    case Qt::Key_F7:
        return SDLK_F7;
    case Qt::Key_F8:
        return SDLK_F8;
    case Qt::Key_F9:
        return SDLK_F9;
    case Qt::Key_F10:
        return SDLK_F10;
    case Qt::Key_F11:
        return SDLK_F11;
    case Qt::Key_F12:
        return SDLK_F12;
    case Qt::Key_Shift:
        return SDLK_LSHIFT;
    case Qt::Key_Control:
        return SDLK_LCTRL;
    case Qt::Key_Alt:
        return SDLK_LALT;
    case Qt::Key_Meta:
        return SDLK_LGUI;
    default:
        return SDLK_UNKNOWN;
    }
}

void KeyboardControlsWindow::onEditingFinished() {
    auto sender = qobject_cast<QKeySequenceEdit*>(QObject::sender());
    auto new_keySequence = sender->keySequence();

    // If new key sequence is empty (i.e. there is no key assigned to it) - skip 'duplicate' checks
    // Two checks are needed for the sake of robustness (when we click on a widget but don't type
    // anything it might no longer be "empty")
    if (new_keySequence.isEmpty() || new_keySequence.toString().isEmpty()) {
        return;
    }

    // Check if sequance is not already used (i.e. making sure there are not duplicates)
    for (auto& keyEdit : m_listOfKeySequenceEdits) {
        if (keyEdit != sender && new_keySequence == keyEdit->keySequence()) {
            sender->clear();
            sender->setStyleSheet("background-color: red; qproperty-alignment: AlignCenter;");
            QTimer::singleShot(inputErrorTimerTimeout, sender, [sender]() {
                sender->setStyleSheet(
                    "QLineEdit { qproperty-alignment: AlignCenter; }"); // Reset to default
            });

            keyEdit->setStyleSheet("background-color: red; qproperty-alignment: AlignCenter;");
            QTimer::singleShot(inputErrorTimerTimeout, keyEdit, [keyEdit]() {
                keyEdit->setStyleSheet(
                    "QLineEdit { qproperty-alignment: AlignCenter; }"); // Reset to default
            });
        }
    }
}
