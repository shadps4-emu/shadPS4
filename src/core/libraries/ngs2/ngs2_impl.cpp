// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ngs2_error.h"
#include "ngs2_impl.h"

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/kernel.h"

using namespace Libraries::Kernel;

namespace Libraries::Ngs2 {

s32 Ngs2::HandleReportInvalid(OrbisNgs2Handle* handle, u32 handle_type) const {
    uintptr_t hAddress = reinterpret_cast<uintptr_t>(handle);
    switch (handle_type) {
    case 1:
        LOG_ERROR(Lib_Ngs2, "Invalid system handle {}", hAddress);
        return ORBIS_NGS2_ERROR_INVALID_SYSTEM_HANDLE;
    case 2:
        LOG_ERROR(Lib_Ngs2, "Invalid rack handle {}", hAddress);
        return ORBIS_NGS2_ERROR_INVALID_RACK_HANDLE;
    case 4:
        LOG_ERROR(Lib_Ngs2, "Invalid voice handle {}", hAddress);
        return ORBIS_NGS2_ERROR_INVALID_VOICE_HANDLE;
    case 8:
        LOG_ERROR(Lib_Ngs2, "Invalid report handle {}", hAddress);
        return ORBIS_NGS2_ERROR_INVALID_REPORT_HANDLE;
    default:
        LOG_ERROR(Lib_Ngs2, "Invalid handle {}", hAddress);
        return ORBIS_NGS2_ERROR_INVALID_HANDLE;
    }
}

} // namespace Libraries::Ngs2
