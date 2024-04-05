// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/PS4/GPU/gpu_memory.h"
#include "core/hle/libraries/libs.h"
#include "core/hle/libraries/libscegnmdriver/libscegnmdriver.h"
#include "emulator.h"

namespace Core::Libraries::LibSceGnmDriver {

int32_t PS4_SYSV_ABI sceGnmSubmitDone() {
    LOG_WARNING(Lib_GnmDriver, "(STUBBED) called");
    return 0;
}

void PS4_SYSV_ABI sceGnmFlushGarlic() {
    LOG_WARNING(Lib_GnmDriver, "(STUBBED) called");
    GPU::flushGarlic(Emu::getGraphicCtx());
}

s32 PS4_SYSV_ABI sceGnmDrawIndex(u32* cmd, u64 size, u32 index_count, const void* index_addr,
                                 u32 flags, u32 type) {
    LOG_INFO(Lib_GnmDriver,
             "(STUBBED) called cmd_buffer  = 0x{:x} size = {} index_count = {} index_addr = 0x{:x} "
             "flags = 0x{:x} type = {}",
             reinterpret_cast<uint64_t>(cmd), size, index_count,
             reinterpret_cast<uint64_t>(index_addr), flags, type);
    return 0;
}

int PS4_SYSV_ABI sceGnmSetVsShader(u32* cmd, u64 size, const u32* vs_regs, u32 shader_modifier) {
    LOG_INFO(Lib_GnmDriver,
             "(STUBBED) called cmd_buffer  = 0x{:x} size = {} shader_modifier = {} vs_reg0 = "
             "0x{:x} vs_reg1 = 0x{:x} vs_reg2 = 0x{:x} vs_reg3 = 0x{:x} vs_reg4 = 0x{:x} vs_reg5 = "
             "0x{:x} vs_reg6 = 0x{:x}",
             reinterpret_cast<uint64_t>(cmd), size, shader_modifier, vs_regs[0], vs_regs[1],
             vs_regs[2], vs_regs[3], vs_regs[4], vs_regs[5], vs_regs[6]);
    return 0;
}

int PS4_SYSV_ABI sceGnmUpdateVsShader(u32* cmd, u64 size, const u32* vs_regs, u32 shader_modifier) {
    LOG_INFO(Lib_GnmDriver,
             "(STUBBED) called cmd_buffer  = 0x{:x} size = {} shader_modifier = {} vs_reg0 = "
             "0x{:x} vs_reg1 = 0x{:x} vs_reg2 = 0x{:x} vs_reg3 = 0x{:x} vs_reg4 = 0x{:x} vs_reg5 = "
             "0x{:x} vs_reg6 = 0x{:x}",
             reinterpret_cast<uint64_t>(cmd), size, shader_modifier, vs_regs[0], vs_regs[1],
             vs_regs[2], vs_regs[3], vs_regs[4], vs_regs[5], vs_regs[6]);
    return 0;
}

u32 PS4_SYSV_ABI sceGnmDrawInitDefaultHardwareState350(u32* cmd, u64 size) {
    LOG_INFO(Lib_GnmDriver, "(STUBBED) called cmd_buffer  = 0x{:x} size = {}",
             reinterpret_cast<uint64_t>(cmd), size);
    return 0;
}

void LibSceGnmDriver_Register(Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("yvZ73uQUqrk", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSubmitDone);
    LIB_FUNCTION("iBt3Oe00Kvc", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmFlushGarlic);
    LIB_FUNCTION("HlTPoZ-oY7Y", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmDrawIndex);
    LIB_FUNCTION("gAhCn6UiU4Y", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSetVsShader);
    LIB_FUNCTION("V31V01UiScY", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmUpdateVsShader);
    LIB_FUNCTION("yb2cRhagD1I", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1,
                 sceGnmDrawInitDefaultHardwareState350);
}
}; // namespace Core::Libraries::LibSceGnmDriver
