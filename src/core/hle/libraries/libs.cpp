// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <common/config.h>
#include "core/PS4/HLE/Graphics/video_out.h"
#include "core/hle/libraries/libc/libc.h"
#include "core/hle/libraries/libkernel/libkernel.h"
#include "core/hle/libraries/libpad/pad.h"
#include "core/hle/libraries/libs.h"
#include "core/hle/libraries/libscegnmdriver/libscegnmdriver.h"
#include "src/core/libraries/libkernel.h"
#include "src/core/libraries/libsceaudioin.h"
#include "src/core/libraries/libsceaudioout.h"
#include "src/core/libraries/libscecommondialog.h"
#include "src/core/libraries/libscehttp.h"
#include "src/core/libraries/libscemsgdialog.h"
#include "src/core/libraries/libscenet.h"
#include "src/core/libraries/libscenetctl.h"
#include "src/core/libraries/libsceposix.h"
#include "src/core/libraries/libscesavedata.h"
#include "src/core/libraries/libscessl.h"
#include "src/core/libraries/libscesysmodule.h"
#include "src/core/libraries/libscesystemservice.h"
#include "src/core/libraries/libsceuserservice.h"

namespace OldLibraries {

void InitHLELibs(Core::Loader::SymbolsResolver* sym) {
    Core::Libraries::LibKernel::LibKernel_Register(sym);
    HLE::Libs::Graphics::VideoOut::videoOutRegisterLib(sym);
    Core::Libraries::LibSceGnmDriver::LibSceGnmDriver_Register(sym);
    OldLibraries::LibPad::padSymbolsRegister(sym);
    if (!Config::isLleLibc()) {
        Core::Libraries::LibC::libcSymbolsRegister(sym);
    }

    // new libraries folder from autogen
    Libraries::UserService::RegisterlibSceUserService(sym);
    Libraries::SystemService::RegisterlibSceSystemService(sym);
    Libraries::CommonDialog::RegisterlibSceCommonDialog(sym);
    Libraries::MsgDialog::RegisterlibSceMsgDialog(sym);
    Libraries::AudioOut::RegisterlibSceAudioOut(sym);
    Libraries::Http::RegisterlibSceHttp(sym);
    Libraries::Net::RegisterlibSceNet(sym);
    Libraries::NetCtl::RegisterlibSceNetCtl(sym);
    Libraries::SaveData::RegisterlibSceSaveData(sym);
    Libraries::Ssl::RegisterlibSceSsl(sym);
    Libraries::SysModule::RegisterlibSceSysmodule(sym);
    Libraries::Kernel::Registerlibkernel(sym);
    Libraries::Posix::Registerlibsceposix(sym);
    Libraries::AudioIn::RegisterlibSceAudioIn(sym);
}

} // namespace OldLibraries
