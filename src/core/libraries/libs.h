// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

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
#include "core/linker.h"
#include "core/loader/elf.h"
#include "core/loader/symbols_resolver.h"
#include "core/tls.h"
#include "font/font.h"
#include "font/fontft.h"
#include "jpeg/jpegenc.h"
#include "libpng/pngenc.h"

#define LIB_FUNCTION(nid, lib, libversion, mod, function)                                          \
    {                                                                                              \
        Core::Loader::SymbolResolver sr{};                                                         \
        sr.name = nid;                                                                             \
        sr.library = lib;                                                                          \
        sr.library_version = libversion;                                                           \
        sr.module = mod;                                                                           \
        sr.type = Core::Loader::SymbolType::Function;                                              \
        auto func = reinterpret_cast<u64>(HOST_CALL(function));                                    \
        sym->AddSymbol(sr, func);                                                                  \
    }

#define LIB_OBJ(nid, lib, libversion, mod, obj)                                                    \
    {                                                                                              \
        Core::Loader::SymbolResolver sr{};                                                         \
        sr.name = nid;                                                                             \
        sr.library = lib;                                                                          \
        sr.library_version = libversion;                                                           \
        sr.module = mod;                                                                           \
        sr.type = Core::Loader::SymbolType::Object;                                                \
        sym->AddSymbol(sr, reinterpret_cast<u64>(obj));                                            \
    }

namespace Libraries {

struct Context {
    Context() {
        LOG_INFO(Lib_Kernel, "Initializing HLE libraries");
    }
};

struct HleLayer : public Context {
    Kernel::Engine m_kernel;
    LibcInternal::Engine m_libc_internal;
    GnmDriver::Engine m_gnm_driver;
    VideoOut::Engine m_video_out;
    UserService::Engine m_user_service;
    SystemService::Engine m_system_service;
    CommonDialog::Engine m_common_dialog;
    MsgDialog::Engine m_msg_dialog;
    AudioOut::Engine m_audio_out;
    Http::Engine m_http;
    Http2::Engine m_http2;
    Net::Engine m_net;
    NetCtl::Engine m_net_ctl;
    SaveData::Engine m_save_data;
    SaveData::Dialog::Engine m_save_data_dialog;
    Ssl2::Engine m_ssl2;
    SysModule::Engine m_sys_module;
    Posix::Engine m_posix;
    AudioIn::Engine m_audio_in;
    Np::NpCommerce::Engine m_np_commerce;
    Np::NpCommon::Engine m_np_common;
    Np::NpManager::Engine m_np_matching;
    Np::NpMatching2::Engine m_np_matching2;
    Np::NpScore::Engine m_np_score;
    Np::NpTrophy::Engine m_np_trophy;
    Np::NpWebApi::Engine m_np_web_api;
    Np::NpWebApi2::Engine m_np_web_api2;
    Np::NpProfileDialog::Engine m_np_profile_dialog;
    Np::NpSnsFacebookDialog::Engine m_np_sns_facebook_dialog;
    Np::NpAuth::Engine m_np_auth;
    Np::NpParty::Engine m_np_party;
    Np::NpPartner::Engine m_np_partner;
    Np::NpTus::Engine m_np_tus;
    ScreenShot::Engine m_screenshot;
    AppContent::Engine m_app_content;
    PngDec::Engine m_pngdec;
    PlayGo::Engine m_play_go;
    PlayGo::Dialog::Engine m_play_go_dialog;
    Random::Engine m_random;
    Usbd::Engine m_usbd;
    Pad::Engine m_pad;
    SystemGesture::Engine m_system_gesture;
    Ajm::Engine m_ajm;
    ErrorDialog::Engine m_error_dialog;
    ImeDialog::Engine m_ime_dialog;
    AvPlayer::Engine m_avplayer;
    Videodec::Engine m_videodec;
    Videodec2::Engine m_videodec2;
    std::unique_ptr<Audio3dOpenAL::Engine> m_audio3d_openal;
    std::unique_ptr<Audio3d::Engine> m_audio3d;
    Ime::Engine m_ime;
    GameLiveStreaming::Engine m_game_live_streaming;
    SharePlay::Engine m_share_play;
    Remoteplay::Engine m_remote_play;
    RazorCpu::Engine m_razor_cpu;
    Move::Engine m_move;
    Fiber::Engine m_fiber;
    Mouse::Engine m_mouse;
    WebBrowserDialog::Engine m_web_browser_dialog;
    Zlib::Engine m_zlib;
    Hmd::Engine m_hmd;
    HmdSetupDialog::Engine m_hmd_setup_dialog;
    DiscMap::Engine m_disc_map;
    Ulobjmgr::Engine m_ul_obj_mgr;
    SigninDialog::Engine m_signin_dialog;
    Camera::Engine m_camera;
    CompanionHttpd::Engine m_companion_httpd;
    CompanionUtil::Engine m_companion_util;
    Voice::Engine m_voice;
    Rudp::Engine m_rudp;
    VrTracker::Engine m_vr_tracker;

    std::unique_ptr<Ngs2::Engine> m_ngs2_engine;
    std::unique_ptr<Rtc::Engine> m_rtc_engine;
    std::unique_ptr<JpegEnc::Engine> m_jpeg_enc_engine;
    std::unique_ptr<PngEnc::Engine> m_png_enc_engine;

    std::unique_ptr<Font::Engine> m_font_engine;
    std::unique_ptr<FontFt::Engine> m_font_ft_engine;

    // Loading libSceSsl is locked behind a title workaround that currently applies to nothing.
    // Ssl::Engine m_save_data_dialog;

    HleLayer(Core::Loader::SymbolsResolver* sym);

    void load(const std::string_view& module_name, Core::Loader::SymbolsResolver* sym);
};

} // namespace Libraries
