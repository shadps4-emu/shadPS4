// SPDX-FileCopyrightText: Copyright 2024-2026 shadBloodborne Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/ajm/ajm.h"
#include "core/libraries/app_content/app_content.h"
#include "core/libraries/audio/audioin.h"
#include "core/libraries/audio/audioout.h"
#include "core/libraries/avplayer/avplayer.h"
#include "core/libraries/gnmdriver/gnmdriver.h"
#include "core/libraries/ime/error_dialog.h"
#include "core/libraries/ime/ime.h"
#include "core/libraries/ime/ime_dialog.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/libs.h"
#include "core/libraries/network/http.h"
#include "core/libraries/network/http2.h"
#include "core/libraries/network/net.h"
#include "core/libraries/network/netctl.h"
#include "core/libraries/network/ssl.h"
#include "core/libraries/network/ssl2.h"
#include "core/libraries/np/np_auth.h"
#include "core/libraries/np/np_commerce.h"
#include "core/libraries/np/np_common.h"
#include "core/libraries/np/np_manager.h"
#include "core/libraries/np/np_matching2.h"
#include "core/libraries/np/np_partner.h"
#include "core/libraries/np/np_party.h"
#include "core/libraries/np/np_profile_dialog.h"
#include "core/libraries/np/np_score.h"
#include "core/libraries/np/np_sns_facebook_dialog.h"
#include "core/libraries/np/np_trophy.h"
#include "core/libraries/np/np_tus.h"
#include "core/libraries/np/np_web_api.h"
#include "core/libraries/np/np_web_api2.h"
#include "core/libraries/pad/pad.h"
#include "core/libraries/playgo/playgo.h"
#include "core/libraries/playgo/playgo_dialog.h"
#include "core/libraries/save_data/dialog/savedatadialog.h"
#include "core/libraries/save_data/savedata.h"
#include "core/libraries/screenshot/screenshot.h"
#include "core/libraries/sysmodule/sysmodule.h"
#include "core/libraries/system/commondialog.h"
#include "core/libraries/system/msgdialog.h"
#include "core/libraries/system/posix.h"
#include "core/libraries/system/systemservice.h"
#include "core/libraries/system/userservice.h"
#include "core/libraries/system_gesture/system_gesture.h"
#include "core/libraries/videoout/video_out.h"
#include "core/libraries/voice/voice.h"

namespace Libraries {

void InitHLELibs(Core::Loader::SymbolsResolver* sym) {
    LOG_INFO(Lib_Kernel, "Initializing HLE libraries");
    Libraries::Kernel::RegisterLib(sym);
    Libraries::GnmDriver::RegisterLib(sym);
    Libraries::VideoOut::RegisterLib(sym);
    Libraries::UserService::RegisterLib(sym);
    Libraries::SystemService::RegisterLib(sym);
    Libraries::CommonDialog::RegisterLib(sym);
    Libraries::MsgDialog::RegisterLib(sym);
    Libraries::AudioOut::RegisterLib(sym);
    Libraries::Http::RegisterLib(sym);
    Libraries::Http2::RegisterLib(sym);
    Libraries::Net::RegisterLib(sym);
    Libraries::NetCtl::RegisterLib(sym);
    Libraries::SaveData::RegisterLib(sym);
    Libraries::SaveData::Dialog::RegisterLib(sym);
    Libraries::Ssl2::RegisterLib(sym);
    Libraries::SysModule::RegisterLib(sym);
    Libraries::Posix::RegisterLib(sym);
    Libraries::AudioIn::RegisterLib(sym);
    Libraries::Np::NpCommerce::RegisterLib(sym);
    Libraries::Np::NpCommon::RegisterLib(sym);
    Libraries::Np::NpManager::RegisterLib(sym);
    Libraries::Np::NpMatching2::RegisterLib(sym);
    Libraries::Np::NpScore::RegisterLib(sym);
    Libraries::Np::NpTrophy::RegisterLib(sym);
    Libraries::Np::NpWebApi::RegisterLib(sym);
    Libraries::Np::NpWebApi2::RegisterLib(sym);
    Libraries::Np::NpProfileDialog::RegisterLib(sym);
    Libraries::Np::NpSnsFacebookDialog::RegisterLib(sym);
    Libraries::Np::NpAuth::RegisterLib(sym);
    Libraries::Np::NpParty::RegisterLib(sym);
    Libraries::Np::NpPartner::RegisterLib(sym);
    Libraries::Np::NpTus::RegisterLib(sym);
    Libraries::ScreenShot::RegisterLib(sym);
    Libraries::AppContent::RegisterLib(sym);
    Libraries::PlayGo::RegisterLib(sym);
    Libraries::PlayGo::Dialog::RegisterLib(sym);
    Libraries::Pad::RegisterLib(sym);
    Libraries::SystemGesture::RegisterLib(sym);
    Libraries::Ajm::RegisterLib(sym);
    Libraries::ErrorDialog::RegisterLib(sym);
    Libraries::ImeDialog::RegisterLib(sym);
    Libraries::AvPlayer::RegisterLib(sym);
    Libraries::Ime::RegisterLib(sym);
    Libraries::Voice::RegisterLib(sym);

    // Loading libSceSsl is locked behind a title workaround that currently applies to nothing.
    // Libraries::Ssl::RegisterLib(sym);
}

} // namespace Libraries
