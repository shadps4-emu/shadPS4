// SPDX-FileCopyrightText: Copyright 2025-2026 shadLauncher4 Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include "common/logging/log.h"
#include "key_manager.h"
#include "path_util.h"

std::shared_ptr<KeyManager> KeyManager::s_instance = nullptr;
std::mutex KeyManager::s_mutex;

// ------------------- Constructor & Singleton -------------------
KeyManager::KeyManager() {
    SetDefaultKeys();
}
KeyManager::~KeyManager() {
    SaveToFile();
}

std::shared_ptr<KeyManager> KeyManager::GetInstance() {
    std::lock_guard<std::mutex> lock(s_mutex);
    if (!s_instance)
        s_instance = std::make_shared<KeyManager>();
    return s_instance;
}

void KeyManager::SetInstance(std::shared_ptr<KeyManager> instance) {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_instance = instance;
}

// ------------------- Load / Save -------------------
bool KeyManager::LoadFromFile() {
    try {
        const auto userDir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
        const auto keysPath = userDir / "keys.json";

        if (!std::filesystem::exists(keysPath)) {
            SetDefaultKeys();
            SaveToFile();
            LOG_DEBUG(KeyManager, "Created default key file: {}", keysPath.string());
            return true;
        }

        std::ifstream file(keysPath);
        if (!file.is_open()) {
            LOG_ERROR(KeyManager, "Could not open key file: {}", keysPath.string());
            return false;
        }

        json j;
        file >> j;

        SetDefaultKeys(); // start from defaults

        if (j.contains("TrophyKeySet"))
            j.at("TrophyKeySet").get_to(m_keys.TrophyKeySet);

        LOG_DEBUG(KeyManager, "Successfully loaded keys from: {}", keysPath.string());
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR(KeyManager, "Error loading keys, using defaults: {}", e.what());
        SetDefaultKeys();
        return false;
    }
}

bool KeyManager::SaveToFile() {
    try {
        const auto userDir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
        const auto keysPath = userDir / "keys.json";

        json j;
        KeysToJson(j);

        std::ofstream file(keysPath);
        if (!file.is_open()) {
            LOG_ERROR(KeyManager, "Could not open key file for writing: {}", keysPath.string());
            return false;
        }

        file << std::setw(4) << j;
        file.flush();

        if (file.fail()) {
            LOG_ERROR(KeyManager, "Failed to write keys to: {}", keysPath.string());
            return false;
        }

        LOG_DEBUG(KeyManager, "Successfully saved keys to: {}", keysPath.string());
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR(KeyManager, "Error saving keys: {}", e.what());
        return false;
    }
}

// ------------------- JSON conversion -------------------
void KeyManager::KeysToJson(json& j) const {
    j = m_keys;
}
void KeyManager::JsonToKeys(const json& j) {
    json current = m_keys;           // serialize current defaults
    current.update(j);               // merge only fields present in file
    m_keys = current.get<AllKeys>(); // deserialize back
}

// ------------------- Defaults / Checks -------------------
void KeyManager::SetDefaultKeys() {
    m_keys = AllKeys{};
}

bool KeyManager::HasKeys() const {
    return !m_keys.TrophyKeySet.ReleaseTrophyKey.empty();
}

// ------------------- Hex conversion -------------------
std::vector<u8> KeyManager::HexStringToBytes(const std::string& hexStr) {
    std::vector<u8> bytes;
    if (hexStr.empty())
        return bytes;

    if (hexStr.size() % 2 != 0)
        throw std::runtime_error("Invalid hex string length");

    bytes.reserve(hexStr.size() / 2);

    auto hexCharToInt = [](char c) -> u8 {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
        throw std::runtime_error("Invalid hex character");
    };

    for (size_t i = 0; i < hexStr.size(); i += 2) {
        u8 high = hexCharToInt(hexStr[i]);
        u8 low = hexCharToInt(hexStr[i + 1]);
        bytes.push_back((high << 4) | low);
    }

    return bytes;
}

std::string KeyManager::BytesToHexString(const std::vector<u8>& bytes) {
    static const char hexDigits[] = "0123456789ABCDEF";
    std::string hexStr;
    hexStr.reserve(bytes.size() * 2);
    for (u8 b : bytes) {
        hexStr.push_back(hexDigits[(b >> 4) & 0xF]);
        hexStr.push_back(hexDigits[b & 0xF]);
    }
    return hexStr;
}