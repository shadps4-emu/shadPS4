// SPDX-FileCopyrightText: Copyright 2025-2026 shadLauncher4 Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "common/types.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

class KeyManager {
public:
    // ------------------- Nested keysets -------------------
    struct TrophyKeySet {
        std::vector<u8> ReleaseTrophyKey;
    };

    struct AllKeys {
        KeyManager::TrophyKeySet TrophyKeySet;
    };

    // ------------------- Construction -------------------
    KeyManager();
    ~KeyManager();

    // ------------------- Singleton -------------------
    static std::shared_ptr<KeyManager> GetInstance();
    static void SetInstance(std::shared_ptr<KeyManager> instance);

    // ------------------- File operations -------------------
    bool LoadFromFile();
    bool SaveToFile();

    // ------------------- Key operations -------------------
    void SetDefaultKeys();
    bool HasKeys() const;

    // ------------------- Getters / Setters -------------------
    const AllKeys& GetAllKeys() const {
        return m_keys;
    }
    void SetAllKeys(const AllKeys& keys) {
        m_keys = keys;
    }

    static std::vector<u8> HexStringToBytes(const std::string& hexStr);
    static std::string BytesToHexString(const std::vector<u8>& bytes);

private:
    void KeysToJson(json& j) const;
    void JsonToKeys(const json& j);

    AllKeys m_keys{};

    static std::shared_ptr<KeyManager> s_instance;
    static std::mutex s_mutex;
};

// ------------------- NLOHMANN macros -------------------
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(KeyManager::TrophyKeySet, ReleaseTrophyKey)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(KeyManager::AllKeys, TrophyKeySet)

namespace nlohmann {
template <>
struct adl_serializer<std::vector<u8>> {
    static void to_json(json& j, const std::vector<u8>& vec) {
        j = KeyManager::BytesToHexString(vec);
    }
    static void from_json(const json& j, std::vector<u8>& vec) {
        vec = KeyManager::HexStringToBytes(j.get<std::string>());
    }
};
} // namespace nlohmann
