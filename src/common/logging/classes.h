// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>

namespace Common::Log {
// clang-format off
/// Llisting all log classes, if you add here, dont forget ALL_LOG_CLASSES
constexpr auto Log                              = "Log";
constexpr auto Common                           = "Common";
constexpr auto Common_Filesystem                = "Common.Filesystem";
constexpr auto Common_Memory                    = "Common.Memory";
constexpr auto Core                             = "Core";
constexpr auto Core_Linker                      = "Core.Linker";
constexpr auto Core_Devices                     = "Core.Devices";
constexpr auto Config                           = "Config";
constexpr auto Debug                            = "Debug";
constexpr auto Kernel                           = "Kernel";
constexpr auto Kernel_Pthread                   = "Kernel.Pthread";
constexpr auto Kernel_Vmm                       = "Kernel.Vmm";
constexpr auto Kernel_Fs                        = "Kernel.Fs";
constexpr auto Kernel_Event                     = "Kernel.Event";
constexpr auto Kernel_Sce                       = "Kernel.Sce";
constexpr auto Lib                              = "Lib";
constexpr auto Lib_LibC                         = "Lib.LibC";
constexpr auto Lib_LibcInternal                 = "Lib.LibcInternal";
constexpr auto Lib_Kernel                       = "Lib.Kernel";
constexpr auto Lib_Pad                          = "Lib.Pad";
constexpr auto Lib_SystemGesture                = "Lib.SystemGesture";
constexpr auto Lib_GnmDriver                    = "Lib.GnmDriver";
constexpr auto Lib_SystemService                = "Lib.SystemService";
constexpr auto Lib_UserService                  = "Lib.UserService";
constexpr auto Lib_VideoOut                     = "Lib.VideoOut";
constexpr auto Lib_CommonDlg                    = "Lib.CommonDlg";
constexpr auto Lib_MsgDlg                       = "Lib.MsgDlg";
constexpr auto Lib_AudioOut                     = "Lib.AudioOut";
constexpr auto Lib_AudioIn                      = "Lib.AudioIn";
constexpr auto Lib_Net                          = "Lib.Net";
constexpr auto Lib_NetCtl                       = "Lib.NetCtl";
constexpr auto Lib_SaveData                     = "Lib.SaveData";
constexpr auto Lib_SaveDataDialog               = "Lib.SaveDataDialog";
constexpr auto Lib_Http                         = "Lib.Http";
constexpr auto Lib_Http2                        = "Lib.Http2";
constexpr auto Lib_Ssl                          = "Lib.Ssl";
constexpr auto Lib_Ssl2                         = "Lib.Ssl2";
constexpr auto Lib_SysModule                    = "Lib.SysModule";
constexpr auto Lib_Move                         = "Lib.Move";
constexpr auto Lib_NpAuth                       = "Lib.NpAuth";
constexpr auto Lib_NpCommon                     = "Lib.NpCommon";
constexpr auto Lib_NpCommerce                   = "Lib.NpCommerce";
constexpr auto Lib_NpManager                    = "Lib.NpManager";
constexpr auto Lib_NpMatching2                  = "Lib.NpMatching2";
constexpr auto Lib_NpScore                      = "Lib.NpScore";
constexpr auto Lib_NpTrophy                     = "Lib.NpTrophy";
constexpr auto Lib_NpTus                        = "Lib.NpTus";
constexpr auto Lib_NpWebApi                     = "Lib.NpWebApi";
constexpr auto Lib_NpWebApi2                    = "Lib.NpWebApi2";
constexpr auto Lib_NpProfileDialog              = "Lib.NpProfileDialog";
constexpr auto Lib_NpSnsFacebookDialog          = "Lib.NpSnsFacebookDialog";
constexpr auto Lib_NpPartner                    = "Lib.NpPartner";
constexpr auto Lib_Screenshot                   = "Lib.Screenshot";
constexpr auto Lib_LibCInternal                 = "Lib.LibCInternal";
constexpr auto Lib_AppContent                   = "Lib.AppContent";
constexpr auto Lib_Rtc                          = "Lib.Rtc";
constexpr auto Lib_Rudp                         = "Lib.Rudp";
constexpr auto Lib_DiscMap                      = "Lib.DiscMap";
constexpr auto Lib_Png                          = "Lib.Png";
constexpr auto Lib_Jpeg                         = "Lib.Jpeg";
constexpr auto Lib_PlayGo                       = "Lib.PlayGo";
constexpr auto Lib_PlayGoDialog                 = "Lib.PlayGoDialog";
constexpr auto Lib_Random                       = "Lib.Random";
constexpr auto Lib_Usbd                         = "Lib.Usbd";
constexpr auto Lib_Ajm                          = "Lib.Ajm";
constexpr auto Lib_ErrorDialog                  = "Lib.ErrorDialog";
constexpr auto Lib_ImeDialog                    = "Lib.ImeDialog";
constexpr auto Lib_AvPlayer                     = "Lib.AvPlayer";
constexpr auto Lib_Ngs2                         = "Lib.Ngs2";
constexpr auto Lib_Audio3d                      = "Lib.Audio3d";
constexpr auto Lib_Ime                          = "Lib.Ime";
constexpr auto Lib_GameLiveStreaming            = "Lib.GameLiveStreaming";
constexpr auto Lib_Remoteplay                   = "Lib.Remoteplay";
constexpr auto Lib_SharePlay                    = "Lib.SharePlay";
constexpr auto Lib_Fiber                        = "Lib.Fiber";
constexpr auto Lib_Vdec2                        = "Lib.Vdec2";
constexpr auto Lib_Videodec                     = "Lib.Videodec";
constexpr auto Lib_RazorCpu                     = "Lib.RazorCpu";
constexpr auto Lib_Mouse                        = "Lib.Mouse";
constexpr auto Lib_WebBrowserDialog             = "Lib.WebBrowserDialog";
constexpr auto Lib_NpParty                      = "Lib.NpParty";
constexpr auto Lib_Zlib                         = "Lib.Zlib";
constexpr auto Lib_Hmd                          = "Lib.Hmd";
constexpr auto Lib_Font                         = "Lib.Font";
constexpr auto Lib_FontFt                       = "Lib.FontFt";
constexpr auto Lib_HmdSetupDialog               = "Lib.HmdSetupDialog";
constexpr auto Lib_SigninDialog                 = "Lib.SigninDialog";
constexpr auto Lib_Camera                       = "Lib.Camera";
constexpr auto Lib_CompanionHttpd               = "Lib.CompanionHttpd";
constexpr auto Lib_CompanionUtil                = "Lib.CompanionUtil";
constexpr auto Lib_Voice                        = "Lib.Voice";
constexpr auto Lib_VrTracker                    = "Lib.VrTracker";
constexpr auto Frontend                         = "Frontend";
constexpr auto Render                           = "Render";
constexpr auto Render_Vulkan                    = "Render.Vulkan";
constexpr auto Render_Recompiler                = "Render.Recompiler";
constexpr auto ImGui                            = "ImGui";
constexpr auto Input                            = "Input";
constexpr auto Tty                              = "Tty";
constexpr auto IPC                              = "IPC";
constexpr auto KeyManager                       = "KeyManager";
constexpr auto Loader                           = "Loader";
// clang-format on

constexpr std::array ALL_LOG_CLASSES{
    Log,
    Common,
    Common_Filesystem,
    Common_Memory,
    Core,
    Core_Linker,
    Core_Devices,
    Config,
    Debug,
    Kernel,
    Kernel_Pthread,
    Kernel_Vmm,
    Kernel_Fs,
    Kernel_Event,
    Kernel_Sce,
    Lib,
    Lib_LibC,
    Lib_LibcInternal,
    Lib_Kernel,
    Lib_Pad,
    Lib_SystemGesture,
    Lib_GnmDriver,
    Lib_SystemService,
    Lib_UserService,
    Lib_VideoOut,
    Lib_CommonDlg,
    Lib_MsgDlg,
    Lib_AudioOut,
    Lib_AudioIn,
    Lib_Net,
    Lib_NetCtl,
    Lib_SaveData,
    Lib_SaveDataDialog,
    Lib_Http,
    Lib_Http2,
    Lib_Ssl,
    Lib_Ssl2,
    Lib_SysModule,
    Lib_Move,
    Lib_NpAuth,
    Lib_NpCommon,
    Lib_NpCommerce,
    Lib_NpManager,
    Lib_NpMatching2,
    Lib_NpScore,
    Lib_NpTrophy,
    Lib_NpTus,
    Lib_NpWebApi,
    Lib_NpWebApi2,
    Lib_NpProfileDialog,
    Lib_NpSnsFacebookDialog,
    Lib_NpPartner,
    Lib_Screenshot,
    Lib_LibCInternal,
    Lib_AppContent,
    Lib_Rtc,
    Lib_Rudp,
    Lib_DiscMap,
    Lib_Png,
    Lib_Jpeg,
    Lib_PlayGo,
    Lib_PlayGoDialog,
    Lib_Random,
    Lib_Usbd,
    Lib_Ajm,
    Lib_ErrorDialog,
    Lib_ImeDialog,
    Lib_AvPlayer,
    Lib_Ngs2,
    Lib_Audio3d,
    Lib_Ime,
    Lib_GameLiveStreaming,
    Lib_Remoteplay,
    Lib_SharePlay,
    Lib_Fiber,
    Lib_Vdec2,
    Lib_Videodec,
    Lib_RazorCpu,
    Lib_Mouse,
    Lib_WebBrowserDialog,
    Lib_NpParty,
    Lib_Zlib,
    Lib_Hmd,
    Lib_Font,
    Lib_FontFt,
    Lib_HmdSetupDialog,
    Lib_SigninDialog,
    Lib_Camera,
    Lib_CompanionHttpd,
    Lib_CompanionUtil,
    Lib_Voice,
    Lib_VrTracker,
    Frontend,
    Render,
    Render_Vulkan,
    Render_Recompiler,
    ImGui,
    Input,
    Tty,
    IPC,
    KeyManager,
    Loader,
};
} // namespace Common::Log
