// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/PS4/HLE/Graphics/video_out.h"
#include "core/hle/libraries/libc/libc.h"
#include "core/hle/libraries/libkernel/libkernel.h"
#include "core/hle/libraries/libpad/pad.h"
#include "core/hle/libraries/libs.h"
#include "core/hle/libraries/libscegnmdriver/libscegnmdriver.h"
#include "src/core/libraries/libscesystemservice.h"
#include "src/core/libraries/libsceuserservice.h"

namespace OldLibraries {

void InitHLELibs(Core::Loader::SymbolsResolver* sym) {
    Core::Libraries::LibKernel::LibKernel_Register(sym);
    HLE::Libs::Graphics::VideoOut::videoOutRegisterLib(sym);
    Core::Libraries::LibSceGnmDriver::LibSceGnmDriver_Register(sym);    
    Core::Libraries::LibPad::padSymbolsRegister(sym);
    Core::Libraries::LibC::libcSymbolsRegister(sym);

    Libraries::UserService::RegisterlibSceUserService(sym);
    Libraries::SystemService::RegisterlibSceSystemService(sym);
}

} // namespace Core::Libraries
