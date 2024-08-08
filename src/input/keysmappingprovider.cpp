// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "input/keysmappingprovider.h"

KeysMappingProvider::KeysMappingProvider(std::map<Uint32, KeysMapping> bindingsMap)
    : m_bindingsMap{bindingsMap} {}

std::optional<KeysMapping> KeysMappingProvider::mapKey(SDL_Keycode sdkKey) {
    auto foundIt = m_bindingsMap.find(sdkKey);
    if (foundIt != m_bindingsMap.end()) {
        return foundIt->second;
    } else {
        return {};
    }
}
