// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <QObject>
#include <SDL3/SDL_events.h>

namespace SdlEventWrapper {

class Wrapper : public QObject {
    Q_OBJECT

public:
    explicit Wrapper(QObject* parent = nullptr);
    ~Wrapper();
    bool ProcessEvent(SDL_Event* event);
    static Wrapper* GetInstance();
    static bool wrapperActive;
    static Wrapper* WrapperInstance;

signals:
    void SDLEvent(int Type, int Input, int Value);
    void audioDeviceChanged(bool isAdd);
};

} // namespace SdlEventWrapper
