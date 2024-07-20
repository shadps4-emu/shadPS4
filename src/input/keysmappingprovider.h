// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <SDL3/SDL_keycode.h>

#include <map>
#include <optional>

#include "input/keys_constants.h"

class KeysMappingProvider {
public:
    KeysMappingProvider(std::map<Uint32, KeysMapping> bindingsMap);

    std::optional<KeysMapping> mapKey(SDL_Keycode sdkKey);

private:
    std::map<Uint32, KeysMapping> m_bindingsMap;
};