// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/config.h"
#include "core/libraries/ajm/ajm.h"
#include "core/libraries/app_content/app_content.h"
#include "core/libraries/audio/audioin.h"
#include "core/libraries/audio/audioout.h"
#include "core/libraries/audio3d/audio3d.h"
#include "core/libraries/avplayer/avplayer.h"
#include "core/libraries/camera/camera.h"
#include "core/libraries/companion/companion_httpd.h"
#include "core/libraries/companion/companion_util.h"
#include "core/libraries/disc_map/disc_map.h"
#include "core/libraries/game_live_streaming/gamelivestreaming.h"
#include "core/libraries/gnmdriver/gnmdriver.h"
#include "core/libraries/hmd/hmd.h"
#include "core/libraries/hmd/hmd_setup_dialog.h"
#include "core/libraries/ime/error_dialog.h"
#include "core/libraries/ime/ime.h"
#include "core/libraries/ime/ime_dialog.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/libc_internal/libc_internal.h"
#include "core/libraries/libpng/pngdec.h"
#include "core/libraries/libs.h"
#include "core/libraries/mouse/mouse.h"
#include "core/libraries/move/move.h"
#include "core/libraries/network/http.h"
#include "core/libraries/network/http2.h"
#include "core/libraries/network/net.h"
#include "core/libraries/network/netctl.h"
#include "core/libraries/network/ssl.h"
#include "core/libraries/network/ssl2.h"
#include "core/libraries/np/np_auth.h"
#include "core/libraries/np/np_common.h"
#include "core/libraries/np/np_manager.h"
#include "core/libraries/np/np_party.h"
#include "core/libraries/np/np_profile_dialog.h"
#include "core/libraries/np/np_score.h"
#include "core/libraries/np/np_trophy.h"
#include "core/libraries/np/np_web_api.h"
#include "core/libraries/pad/pad.h"
#include "core/libraries/playgo/playgo.h"
#include "core/libraries/playgo/playgo_dialog.h"
#include "core/libraries/random/random.h"
#include "core/libraries/razor_cpu/razor_cpu.h"
#include "core/libraries/remote_play/remoteplay.h"
#include "core/libraries/rtc/rtc.h"
#include "core/libraries/save_data/dialog/savedatadialog.h"
#include "core/libraries/save_data/savedata.h"
#include "core/libraries/screenshot/screenshot.h"
#include "core/libraries/share_play/shareplay.h"
#include "core/libraries/signin_dialog/signindialog.h"
#include "core/libraries/system/commondialog.h"
#include "core/libraries/system/msgdialog.h"
#include "core/libraries/system/posix.h"
#include "core/libraries/system/sysmodule.h"
#include "core/libraries/system/systemservice.h"
#include "core/libraries/system/userservice.h"
#include "core/libraries/system_gesture/system_gesture.h"
#include "core/libraries/ulobjmgr/ulobjmgr.h"
#include "core/libraries/usbd/usbd.h"
#include "core/libraries/videodec/videodec.h"
#include "core/libraries/videodec/videodec2.h"
#include "core/libraries/videoout/video_out.h"
#include "core/libraries/voice/voice.h"
#include "core/libraries/vr_tracker/vr_tracker.h"
#include "core/libraries/web_browser_dialog/webbrowserdialog.h"
#include "core/libraries/zlib/zlib_sce.h"
#include "fiber/fiber.h"
#include "jpeg/jpegenc.h"

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
    Libraries::Ssl::RegisterLib(sym);
    Libraries::Ssl2::RegisterLib(sym);
    Libraries::SysModule::RegisterLib(sym);
    Libraries::Posix::RegisterLib(sym);
    Libraries::AudioIn::RegisterLib(sym);
    Libraries::Np::NpCommon::RegisterLib(sym);
    Libraries::Np::NpManager::RegisterLib(sym);
    Libraries::Np::NpScore::RegisterLib(sym);
    Libraries::Np::NpTrophy::RegisterLib(sym);
    Libraries::Np::NpWebApi::RegisterLib(sym);
    Libraries::Np::NpProfileDialog::RegisterLib(sym);
    Libraries::Np::NpAuth::RegisterLib(sym);
    Libraries::Np::NpParty::RegisterLib(sym);
    Libraries::ScreenShot::RegisterLib(sym);
    Libraries::AppContent::RegisterLib(sym);
    Libraries::PngDec::RegisterLib(sym);
    Libraries::PlayGo::RegisterLib(sym);
    Libraries::PlayGo::Dialog::RegisterLib(sym);
    Libraries::Random::RegisterLib(sym);
    Libraries::Usbd::RegisterLib(sym);
    Libraries::Pad::RegisterLib(sym);
    Libraries::SystemGesture::RegisterLib(sym);
    Libraries::Ajm::RegisterLib(sym);
    Libraries::ErrorDialog::RegisterLib(sym);
    Libraries::ImeDialog::RegisterLib(sym);
    Libraries::AvPlayer::RegisterLib(sym);
    Libraries::Vdec2::RegisterLib(sym);
    Libraries::Audio3d::RegisterLib(sym);
    Libraries::Ime::RegisterLib(sym);
    Libraries::GameLiveStreaming::RegisterLib(sym);
    Libraries::SharePlay::RegisterLib(sym);
    Libraries::Remoteplay::RegisterLib(sym);
    Libraries::Videodec::RegisterLib(sym);
    Libraries::RazorCpu::RegisterLib(sym);
    Libraries::Move::RegisterLib(sym);
    Libraries::Fiber::RegisterLib(sym);
    Libraries::JpegEnc::RegisterLib(sym);
    Libraries::Mouse::RegisterLib(sym);
    Libraries::WebBrowserDialog::RegisterLib(sym);
    Libraries::Zlib::RegisterLib(sym);
    Libraries::Hmd::RegisterLib(sym);
    Libraries::HmdSetupDialog::RegisterLib(sym);
    Libraries::DiscMap::RegisterLib(sym);
    Libraries::Ulobjmgr::RegisterLib(sym);
    Libraries::SigninDialog::RegisterLib(sym);
    Libraries::Camera::RegisterLib(sym);
    Libraries::CompanionHttpd::RegisterLib(sym);
    Libraries::CompanionUtil::RegisterLib(sym);
    Libraries::Voice::RegisterLib(sym);
    Libraries::Rtc::RegisterLib(sym);
    Libraries::VrTracker::RegisterLib(sym);
}

} // namespace Libraries
