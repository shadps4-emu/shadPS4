// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "random.h"
#include "random_error.h"

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"

namespace Libraries::Random {

s32 PS4_SYSV_ABI sceRandomGetRandomNumber(u8* buf, std::size_t size) {
    LOG_TRACE(Lib_Random, "called");
    if (size > SCE_RANDOM_MAX_SIZE) {
        return SCE_RANDOM_ERROR_INVALID;
    }

    for (auto i = 0; i < size; ++i) {
        buf[i] = std::rand() & 0xFF;
    }
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("PI7jIZj4pcE", "libSceRandom", 1, "libSceRandom", sceRandomGetRandomNumber);
};

} // namespace Libraries::Random
