// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/config.h"
#include "core/libraries/ajm/ajm.h"
#include "core/libraries/app_content/app_content.h"
#include "core/libraries/audio/audioin.h"
#include "core/libraries/audio/audioout.h"
#include "core/libraries/audio3d/audio3d.h"
#include "core/libraries/avplayer/avplayer.h"
#include "core/libraries/disc_map/disc_map.h"
#include "core/libraries/game_live_streaming/gamelivestreaming.h"
#include "core/libraries/gnmdriver/gnmdriver.h"
#include "core/libraries/hmd/hmd.h"
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
#include "core/libraries/np_common/np_common.h"
#include "core/libraries/np_manager/np_manager.h"
#include "core/libraries/np_party/np_party.h"
#include "core/libraries/np_score/np_score.h"
#include "core/libraries/np_trophy/np_trophy.h"
#include "core/libraries/np_web_api/np_web_api.h"
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
#include "core/libraries/system/commondialog.h"
#include "core/libraries/system/msgdialog.h"
#include "core/libraries/system/posix.h"
#include "core/libraries/system/sysmodule.h"
#include "core/libraries/system/systemservice.h"
#include "core/libraries/system/userservice.h"
#include "core/libraries/usbd/usbd.h"
#include "core/libraries/videodec/videodec.h"
#include "core/libraries/videodec/videodec2.h"
#include "core/libraries/videoout/video_out.h"
#include "core/libraries/web_browser_dialog/webbrowserdialog.h"
#include "core/libraries/zlib/zlib_sce.h"
#include "fiber/fiber.h"
#include "jpeg/jpegenc.h"

namespace Libraries {

void InitHLELibs(Core::Loader::SymbolsResolver* sym) {
    LOG_INFO(Lib_Kernel, "Initializing HLE libraries");
    Libraries::Kernel::RegisterKernel(sym);
    Libraries::GnmDriver::RegisterlibSceGnmDriver(sym);
    Libraries::VideoOut::RegisterLib(sym);
    Libraries::UserService::RegisterlibSceUserService(sym);
    Libraries::SystemService::RegisterlibSceSystemService(sym);
    Libraries::CommonDialog::RegisterlibSceCommonDialog(sym);
    Libraries::MsgDialog::RegisterlibSceMsgDialog(sym);
    Libraries::AudioOut::RegisterlibSceAudioOut(sym);
    Libraries::Http::RegisterlibSceHttp(sym);
    Libraries::Http2::RegisterlibSceHttp2(sym);
    Libraries::Net::RegisterlibSceNet(sym);
    Libraries::NetCtl::RegisterlibSceNetCtl(sym);
    Libraries::SaveData::RegisterlibSceSaveData(sym);
    Libraries::SaveData::Dialog::RegisterlibSceSaveDataDialog(sym);
    Libraries::Ssl::RegisterlibSceSsl(sym);
    Libraries::Ssl2::RegisterlibSceSsl2(sym);
    Libraries::SysModule::RegisterlibSceSysmodule(sym);
    Libraries::Posix::Registerlibsceposix(sym);
    Libraries::AudioIn::RegisterlibSceAudioIn(sym);
    Libraries::NpCommon::RegisterlibSceNpCommon(sym);
    Libraries::NpManager::RegisterlibSceNpManager(sym);
    Libraries::NpScore::RegisterlibSceNpScore(sym);
    Libraries::NpTrophy::RegisterlibSceNpTrophy(sym);
    Libraries::NpWebApi::RegisterlibSceNpWebApi(sym);
    Libraries::ScreenShot::RegisterlibSceScreenShot(sym);
    Libraries::AppContent::RegisterlibSceAppContent(sym);
    Libraries::PngDec::RegisterlibScePngDec(sym);
    Libraries::PlayGo::RegisterlibScePlayGo(sym);
    Libraries::PlayGo::Dialog::RegisterlibScePlayGoDialog(sym);
    Libraries::Random::RegisterlibSceRandom(sym);
    Libraries::Usbd::RegisterlibSceUsbd(sym);
    Libraries::Pad::RegisterlibScePad(sym);
    Libraries::Ajm::RegisterlibSceAjm(sym);
    Libraries::ErrorDialog::RegisterlibSceErrorDialog(sym);
    Libraries::ImeDialog::RegisterlibSceImeDialog(sym);
    Libraries::AvPlayer::RegisterlibSceAvPlayer(sym);
    Libraries::Vdec2::RegisterlibSceVdec2(sym);
    Libraries::Audio3d::RegisterlibSceAudio3d(sym);
    Libraries::Ime::RegisterlibSceIme(sym);
    Libraries::GameLiveStreaming::RegisterlibSceGameLiveStreaming(sym);
    Libraries::SharePlay::RegisterlibSceSharePlay(sym);
    Libraries::Remoteplay::RegisterlibSceRemoteplay(sym);
    Libraries::Videodec::RegisterlibSceVideodec(sym);
    Libraries::RazorCpu::RegisterlibSceRazorCpu(sym);
    Libraries::Move::RegisterlibSceMove(sym);
    Libraries::Fiber::RegisterlibSceFiber(sym);
    Libraries::JpegEnc::RegisterlibSceJpegEnc(sym);
    Libraries::Mouse::RegisterlibSceMouse(sym);
    Libraries::WebBrowserDialog::RegisterlibSceWebBrowserDialog(sym);
    Libraries::NpParty::RegisterlibSceNpParty(sym);
    Libraries::Zlib::RegisterlibSceZlib(sym);
    Libraries::Hmd::RegisterlibSceHmd(sym);
}

} // namespace Libraries
