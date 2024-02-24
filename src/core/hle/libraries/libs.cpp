// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/PS4/HLE/Graphics/video_out.h"
#include "core/hle/libraries/libc/libc.h"
#include "core/hle/libraries/libkernel/libkernel.h"
#include "core/hle/libraries/libpad/pad.h"
#include "core/hle/libraries/libs.h"
#include "core/hle/libraries/libscegnmdriver/libscegnmdriver.h"
#include "core/hle/libraries/libsystemservice/system_service.h"
#include "core/hle/libraries/libuserservice/libuserservice.h"

namespace Core::Libraries {

void InitHLELibs(Loader::SymbolsResolver* sym) {
    LibKernel::LibKernel_Register(sym);
    HLE::Libs::Graphics::VideoOut::videoOutRegisterLib(sym);
    LibSceGnmDriver::LibSceGnmDriver_Register(sym);
    LibUserService::userServiceSymbolsRegister(sym);
    LibPad::padSymbolsRegister(sym);
    LibSystemService::systemServiceSymbolsRegister(sym);
    LibC::libcSymbolsRegister(sym);
}

} // namespace Core::Libraries
