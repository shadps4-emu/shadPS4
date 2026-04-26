// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/logging/log.h"
#include "core/libraries/ajm/ajm.h"
#include "core/libraries/app_content/app_content.h"
#include "core/libraries/audio/audioin.h"
#include "core/libraries/audio/audioout.h"
#include "core/libraries/audio3d/audio3d.h"
#include "core/libraries/audio3d/audio3d_openal.h"
#include "core/libraries/avplayer/avplayer.h"
#include "core/libraries/companion//companion_httpd.h"
#include "core/libraries/companion//companion_util.h"
#include "core/libraries/disc_map/disc_map.h"
#include "core/libraries/fiber/fiber.h"
#include "core/libraries/font/font.h"
#include "core/libraries/font/fontft.h"
#include "core/libraries/game_live_streaming/gamelivestreaming.h"
#include "core/libraries/gnmdriver/gnmdriver.h"
#include "core/libraries/hmd/hmd.h"
#include "core/libraries/hmd/hmd_setup_dialog.h"
#include "core/libraries/ime/error_dialog.h"
#include "core/libraries/ime/ime.h"
#include "core/libraries/ime/ime_dialog.h"
#include "core/libraries/jpeg/jpegenc.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/libc_internal/libc_internal.h"
#include "core/libraries/libpng/pngdec.h"
#include "core/libraries/libpng/pngenc.h"
#include "core/libraries/mouse/mouse.h"
#include "core/libraries/move/move.h"
#include "core/libraries/network/http.h"
#include "core/libraries/network/http2.h"
#include "core/libraries/network/net.h"
#include "core/libraries/network/netctl.h"
#include "core/libraries/network/ssl2.h"
#include "core/libraries/ngs2/ngs2.h"
#include "core/libraries/np/np_auth.h"
#include "core/libraries/np/np_commerce.h"
#include "core/libraries/np/np_common.h"
#include "core/libraries/np/np_manager.h"
#include "core/libraries/np/np_matching2.h"
#include "core/libraries/np/np_partner.h"
#include "core/libraries/np/np_party.h"
#include "core/libraries/np/np_profile_dialog/np_profile_dialog.h"
#include "core/libraries/np/np_score.h"
#include "core/libraries/np/np_sns_facebook_dialog.h"
#include "core/libraries/np/np_trophy.h"
#include "core/libraries/np/np_tus.h"
#include "core/libraries/np/np_web_api.h"
#include "core/libraries/np/np_web_api2.h"
#include "core/libraries/pad/pad.h"
#include "core/libraries/playgo/playgo.h"
#include "core/libraries/playgo/playgo_dialog.h"
#include "core/libraries/random/random.h"
#include "core/libraries/razor_cpu/razor_cpu.h"
#include "core/libraries/remote_play/remoteplay.h"
#include "core/libraries/rudp/rudp.h"
#include "core/libraries/save_data/dialog/savedatadialog.h"
#include "core/libraries/save_data/savedata.h"
#include "core/libraries/screenshot/screenshot.h"
#include "core/libraries/share_play/shareplay.h"
#include "core/libraries/signin_dialog/signindialog.h"
#include "core/libraries/sysmodule/sysmodule.h"
#include "core/libraries/system/msgdialog.h"
#include "core/libraries/system/posix.h"
#include "core/libraries/system/systemservice.h"
#include "core/libraries/ulobjmgr/ulobjmgr.h"
#include "core/libraries/usbd/usbd.h"
#include "core/libraries/videodec/videodec.h"
#include "core/libraries/videodec/videodec2.h"
#include "core/libraries/videoout/video_out.h"
#include "core/libraries/voice/voice.h"
#include "core/libraries/vr_tracker/vr_tracker.h"
#include "core/libraries/web_browser_dialog/webbrowserdialog.h"
#include "core/libraries/zlib/zlib_sce.h"

namespace Libraries {

struct Context {
    Context() {
        LOG_INFO(Lib_Kernel, "Initializing HLE libraries");
    }
};

struct HleLayer : public Context {
    HleLayer(Core::Loader::SymbolsResolver* sym);

    void load(const std::string_view& module_name, Core::Loader::SymbolsResolver* sym);

    Kernel::Library m_kernel;
    LibcInternal::Library m_libc_internal;
    GnmDriver::Library m_gnm_driver;
    VideoOut::Library m_video_out;
    UserService::Library m_user_service;
    SystemService::Library m_system_service;
    CommonDialog::Library m_common_dialog;
    MsgDialog::Library m_msg_dialog;
    AudioOut::Library m_audio_out;
    Http::Library m_http;
    Http2::Library m_http2;
    Net::Library m_net;
    NetCtl::Library m_net_ctl;
    SaveData::Library m_save_data;
    SaveData::Dialog::Library m_save_data_dialog;
    Ssl2::Library m_ssl2;
    SysModule::Library m_sys_module;
    Posix::Library m_posix;
    AudioIn::Library m_audio_in;
    Np::NpCommerce::Library m_np_commerce;
    Np::NpCommon::Library m_np_common;
    Np::NpManager::Library m_np_matching;
    Np::NpMatching2::Library m_np_matching2;
    Np::NpScore::Library m_np_score;
    Np::NpTrophy::Library m_np_trophy;
    Np::NpWebApi::Library m_np_web_api;
    Np::NpWebApi2::Library m_np_web_api2;
    Np::NpProfileDialog::Library m_np_profile_dialog;
    Np::NpSnsFacebookDialog::Library m_np_sns_facebook_dialog;
    Np::NpAuth::Library m_np_auth;
    Np::NpParty::Library m_np_party;
    Np::NpPartner::Library m_np_partner;
    Np::NpTus::Library m_np_tus;
    ScreenShot::Library m_screenshot;
    AppContent::Library m_app_content;
    PngDec::Library m_pngdec;
    PlayGo::Library m_play_go;
    PlayGo::Dialog::Library m_play_go_dialog;
    Random::Library m_random;
    Usbd::Library m_usbd;
    Pad::Library m_pad;
    Ajm::Library m_ajm;
    ErrorDialog::Library m_error_dialog;
    ImeDialog::Library m_ime_dialog;
    AvPlayer::Library m_avplayer;
    Videodec::Library m_videodec;
    Videodec2::Library m_videodec2;
    std::unique_ptr<Audio3dOpenAL::Library> m_audio3d_openal;
    std::unique_ptr<Audio3d::Library> m_audio3d;
    Ime::Library m_ime;
    GameLiveStreaming::Library m_game_live_streaming;
    SharePlay::Library m_share_play;
    Remoteplay::Library m_remote_play;
    RazorCpu::Library m_razor_cpu;
    Move::Library m_move;
    Fiber::Library m_fiber;
    Mouse::Library m_mouse;
    WebBrowserDialog::Library m_web_browser_dialog;
    Zlib::Library m_zlib;
    Hmd::Library m_hmd;
    HmdSetupDialog::Library m_hmd_setup_dialog;
    DiscMap::Library m_disc_map;
    Ulobjmgr::Library m_ul_obj_mgr;
    SigninDialog::Library m_signin_dialog;
    Camera::Library m_camera;
    CompanionHttpd::Library m_companion_httpd;
    CompanionUtil::Library m_companion_util;
    Voice::Library m_voice;
    Rudp::Library m_rudp;
    VrTracker::Library m_vr_tracker;

    std::unique_ptr<Ngs2::Library> m_ngs2_engine;
    std::unique_ptr<Rtc::Library> m_rtc_engine;
    std::unique_ptr<JpegEnc::Library> m_jpeg_enc_engine;
    std::unique_ptr<PngEnc::Library> m_png_enc_engine;

    std::unique_ptr<Font::Library> m_font_engine;
    std::unique_ptr<FontFt::Library> m_font_ft_engine;

    // Loading libSceSsl is locked behind a title workaround that currently applies to nothing.
    // Ssl::Library m_save_data_dialog;
};

} // namespace Libraries
