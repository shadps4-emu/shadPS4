// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <string_view>

#include "assert.h"
#include "singleton.h"
#include "types.h"

namespace Core {
class Emulator;
}

namespace Common {

class ElfInfo {
    friend class Core::Emulator;

    bool initialized = false;

    std::string game_serial{};
    std::string title{};
    std::string app_ver{};
    u32 firmware_ver = 0;
    u32 raw_firmware_ver = 0;

public:
    static constexpr u32 FW_15 = 0x1500000;
    static constexpr u32 FW_16 = 0x1600000;
    static constexpr u32 FW_17 = 0x1700000;
    static constexpr u32 FW_20 = 0x2000000;
    static constexpr u32 FW_25 = 0x2500000;
    static constexpr u32 FW_30 = 0x3000000;
    static constexpr u32 FW_35 = 0x3500000;
    static constexpr u32 FW_40 = 0x4000000;
    static constexpr u32 FW_45 = 0x4500000;
    static constexpr u32 FW_50 = 0x5000000;
    static constexpr u32 FW_80 = 0x8000000;

    static ElfInfo& Instance() {
        return *Singleton<ElfInfo>::Instance();
    }

    [[nodiscard]] std::string_view GameSerial() const {
        ASSERT(initialized);
        return Instance().game_serial;
    }

    [[nodiscard]] std::string_view Title() const {
        ASSERT(initialized);
        return title;
    }

    [[nodiscard]] std::string_view AppVer() const {
        ASSERT(initialized);
        return app_ver;
    }

    [[nodiscard]] u32 FirmwareVer() const {
        ASSERT(initialized);
        return firmware_ver;
    }

    [[nodiscard]] u32 RawFirmwareVer() const {
        ASSERT(initialized);
        return raw_firmware_ver;
    }
};

} // namespace Common
