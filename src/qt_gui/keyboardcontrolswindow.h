// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDialog>
#include <QKeySequenceEdit>
#include <QMainWindow>

#include <SDL3/SDL_keycode.h>
#include "input/keys_constants.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class KeyboardControlsWindow;
}
QT_END_NAMESPACE

class KeyboardControlsWindow : public QDialog {
    Q_OBJECT

public:
    KeyboardControlsWindow(QWidget* parent = nullptr);
    ~KeyboardControlsWindow();

    const std::map<u32, KeysMapping>& getKeysMapping() const;

private slots:
    void onEditingFinished();

private:
    void validateAndSaveKeyBindings();
    SDL_Keycode convertQtKeyToSDL(Qt::Key qtKey);
    Qt::Key convertSDLKeyToQt(SDL_Keycode qtKey);

    Ui::KeyboardControlsWindow* ui;
    QSet<QKeySequenceEdit*> m_listOfKeySequenceEdits;
    std::map<u32, KeysMapping> m_keysMap;
    std::map<KeysMapping, u32> m_reverseKeysMap;
};
