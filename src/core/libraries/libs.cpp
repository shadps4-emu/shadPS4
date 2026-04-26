// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/ajm/ajm.h"
#include "core/libraries/app_content/app_content.h"
#include "core/libraries/audio/audioin.h"
#include "core/libraries/audio/audioout.h"
#include "core/libraries/audio3d/audio3d.h"
#include "core/libraries/audio3d/audio3d_openal.h"
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
#include "core/libraries/system/commondialog.h"
#include "core/libraries/system/msgdialog.h"
#include "core/libraries/system/posix.h"
#include "core/libraries/system/systemservice.h"
#include "core/libraries/system/userservice.h"
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

namespace Libraries {

// clang-format off
HleLayer::HleLayer(Core::Loader::SymbolsResolver* sym)
    : m_kernel(sym),
      m_libc_internal(sym),
      m_gnm_driver(sym),
      m_video_out(sym, *m_gnm_driver.presenter),
      m_user_service(sym),
      m_system_service(sym),
      m_common_dialog(sym),
      m_msg_dialog(sym),
      m_audio_out(sym),
      m_http(sym),
      m_http2(sym),
      m_net(sym),
      m_net_ctl(sym),
      m_save_data(sym),
      m_save_data_dialog(sym),
      m_ssl2(sym),
      m_sys_module(sym),
      m_posix(sym),
      m_audio_in(sym),
      m_np_commerce(sym),
      m_np_common(sym),
      m_np_matching(sym),
      m_np_matching2(sym),
      m_np_score(sym),
      m_np_trophy(sym),
      m_np_web_api(sym),
      m_np_web_api2(sym),
      m_np_profile_dialog(sym),
      m_np_sns_facebook_dialog(sym),
      m_np_auth(sym),
      m_np_party(sym),
      m_np_partner(sym),
      m_np_tus(sym),
      m_screenshot(sym),
      m_app_content(sym),
      m_pngdec(sym),
      m_play_go(sym),
      m_play_go_dialog(sym),
      m_random(sym),
      m_usbd(sym),
      m_pad(sym),
      m_ajm(sym),
      m_error_dialog(sym),
      m_ime_dialog(sym),
      m_avplayer(sym),
      m_videodec(sym),
      m_videodec2(sym),
      m_audio3d_openal(EmulatorSettings.GetAudioBackend() == AudioBackend::OpenAL
            ? std::make_unique<Audio3dOpenAL::Library>(sym)
            : nullptr),
      m_audio3d(EmulatorSettings.GetAudioBackend() == AudioBackend::SDL
            ? std::make_unique<Audio3d::Library>(sym)
            : nullptr),
      m_ime(sym),
      m_game_live_streaming(sym),
      m_share_play(sym),
      m_remote_play(sym),
      m_razor_cpu(sym),
      m_move(sym),
      m_fiber(sym),
      m_mouse(sym),
      m_web_browser_dialog(sym),
      m_zlib(sym),
      m_hmd(sym),
      m_hmd_setup_dialog(sym),
      m_disc_map(sym),
      m_ul_obj_mgr(sym),
      m_signin_dialog(sym),
      m_camera(sym),
      m_companion_httpd(sym),
      m_companion_util(sym),
      m_voice(sym),
      m_rudp(sym),
      m_vr_tracker(sym) {}
// clang-format on

void HleLayer::load(const std::string_view& module_name, Core::Loader::SymbolsResolver* sym) {
    if (module_name == "libSceNgs2.sprx") {
        m_ngs2_engine = std::make_unique<Ngs2::Library>(sym);
    } else if (module_name == "libSceUlt.sprx") {
        // TODO m_ult_engine = std::make_unique<Ult::Library>(sym);
    } else if (module_name == "libSceRtc.sprx") {
        m_rtc_engine = std::make_unique<Rtc::Library>(sym);
    } else if (module_name == "libSceJpegDec.sprx") {
        // TODO m_jpeg_dec_engine = std::make_unique<JpegDec::Library>(sym);
    } else if (module_name == "libSceJpegEnc.sprx") {
        m_jpeg_enc_engine = std::make_unique<JpegEnc::Library>(sym);
    } else if (module_name == "libScePngEnc.sprx") {
        m_png_enc_engine = std::make_unique<PngEnc::Library>(sym);
    } else if (module_name == "libSceJson.sprx") {
        // TODO m_json_engine = std::make_unique<Json::Library>(sym);
    } else if (module_name == "libSceJson2.sprx") {
        // TODO m_json2_engine = std::make_unique<Json2::Library>(sym);
    } else if (module_name == "libSceLibcInternal.sprx") {
        // m_libc_internal = std::make_unique<LibcInternal::Library>(sym);
        m_libc_internal = LibcInternal::Library(sym);
    } else if (module_name == "libSceCesCs.sprx") {
        // TODO m_ces_cs_engine = std::make_unique<CesCs::Library>(sym);
    } else if (module_name == "libSceAudiodec.sprx") {
        // TODO m_audio_dec_engine = std::make_unique<Audiodec::Library>(sym);
    } else if (module_name == "libSceFont.sprx") {
        m_font_engine = std::make_unique<Font::Library>(sym);
    } else if (module_name == "libSceFontFt.sprx") {
        m_font_ft_engine = std::make_unique<FontFt::Library>(sym);
    } else if (module_name == "libSceFreeTypeOt.sprx") {
        // TODO m_freetype_ot_engine = std::make_unique<FreeTypeOt::Library>(sym);
    }
}

} // namespace Libraries
