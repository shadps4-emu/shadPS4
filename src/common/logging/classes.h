// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>

namespace Common::Log::Class {
// clang-format off
/// Listing all log classes, if you add here, dont forget ALL_LOGGERS
constexpr auto Common = "Common";                                   ///< Library routines
constexpr auto Common_Filesystem = "Common.Filesystem";             ///< Filesystem interface library
constexpr auto Common_Memory = "Common.Memory";                     ///< Memory mapping and management functions
constexpr auto Config = "Config";                                   ///< Emulator configuration (including commandline)
constexpr auto Core = "Core";                                       ///< LLE emulation core
constexpr auto Core_Devices = "Core.Devices";                       ///< Devices emulation
constexpr auto Core_Linker = "Core.Linker";                         ///< The module linker
constexpr auto Debug = "Debug";                                     ///< Debugging tools
constexpr auto Frontend = "Frontend";                               ///< Emulator UI
constexpr auto IPC = "IPC";                                         ///< IPC
constexpr auto ImGui = "ImGui";                                     ///< ImGui
constexpr auto Input = "Input";                                     ///< Input emulation
constexpr auto Kernel = "Kernel";                                   ///< The HLE implementation of the PS4 kernel.
constexpr auto Kernel_Event = "Kernel.Event";                       ///< The event management implementation of the kernel.
constexpr auto Kernel_Fs = "Kernel.Fs";                             ///< The filesystem implementation of the kernel.
constexpr auto Kernel_Pthread = "Kernel.Pthread";                   ///< The pthread implementation of the kernel.
constexpr auto Kernel_Sce = "Kernel.Sce";                           ///< The Sony-specific interfaces provided by the kernel.
constexpr auto Kernel_Vmm = "Kernel.Vmm";                           ///< The virtual memory implementation of the kernel.
constexpr auto KeyManager = "KeyManager";                           ///< Key management system
constexpr auto Lib = "Lib";                                         ///< HLE implementation of system library. Each major library  should have its own subclass.
constexpr auto Lib_Ajm = "Lib.Ajm";                                 ///< The LibSceAjm implementation.
constexpr auto Lib_AppContent = "Lib.AppContent";                   ///< The LibSceAppContent implementation.
constexpr auto Lib_Audio3d = "Lib.Audio3d";                         ///< The LibSceAudio3d implementation.
constexpr auto Lib_AudioIn = "Lib.AudioIn";                         ///< The LibSceAudioIn implementation.
constexpr auto Lib_AudioOut = "Lib.AudioOut";                       ///< The LibSceAudioOut implementation.
constexpr auto Lib_AvPlayer = "Lib.AvPlayer";                       ///< The LibSceAvPlayer implementation.
constexpr auto Lib_Camera = "Lib.Camera";                           ///< The LibCamera implementation.
constexpr auto Lib_CommonDlg = "Lib.CommonDlg";                     ///< The LibSceCommonDialog implementation.
constexpr auto Lib_CompanionHttpd = "Lib.CompanionHttpd";           ///< The LibCompanionHttpd implementation.
constexpr auto Lib_CompanionUtil = "Lib.CompanionUtil";             ///< The LibCompanionUtil implementation.
constexpr auto Lib_DiscMap = "Lib.DiscMap";                         ///< The LibSceDiscMap implementation.
constexpr auto Lib_ErrorDialog = "Lib.ErrorDialog";                 ///< The LibSceErrorDialog implementation.
constexpr auto Lib_Fiber = "Lib.Fiber";                             ///< The LibSceFiber implementation.
constexpr auto Lib_Font = "Lib.Font";                               ///< The libSceFont implementation.
constexpr auto Lib_FontFt = "Lib.FontFt";                           ///< The libSceFontFt implementation.
constexpr auto Lib_GameLiveStreaming = "Lib.GameLiveStreaming";     ///< The LibSceGameLiveStreaming implementation
constexpr auto Lib_GnmDriver = "Lib.GnmDriver";                     ///< The LibSceGnmDriver implementation.
constexpr auto Lib_Hmd = "Lib.Hmd";                                 ///< The LibSceHmd implementation.
constexpr auto Lib_HmdSetupDialog = "Lib.HmdSetupDialog";           ///< The LibSceHmdSetupDialog implementation.
constexpr auto Lib_Http = "Lib.Http";                               ///< The LibSceHttp implementation.
constexpr auto Lib_Http2 = "Lib.Http2";                             ///< The LibSceHttp2 implementation.
constexpr auto Lib_Ime = "Lib.Ime";                                 ///< The LibSceIme implementation
constexpr auto Lib_ImeDialog = "Lib.ImeDialog";                     ///< The LibSceImeDialog implementation.
constexpr auto Lib_Jpeg = "Lib.Jpeg";                               ///< The LibSceJpeg implementation.
constexpr auto Lib_Kernel = "Lib.Kernel";                           ///< The LibKernel implementation.
constexpr auto Lib_LibcInternal = "Lib.LibcInternal";               ///< The LibcInternal implementation.
constexpr auto Lib_Mouse = "Lib.Mouse";                             ///< The LibSceMouse implementation
constexpr auto Lib_Move = "Lib.Move";                               ///< The LibSceMove implementation.
constexpr auto Lib_MsgDlg = "Lib.MsgDlg";                           ///< The LibSceMsgDialog implementation.
constexpr auto Lib_Net = "Lib.Net";                                 ///< The LibSceNet implementation.
constexpr auto Lib_NetCtl = "Lib.NetCtl";                           ///< The LibSceNetCtl implementation.
constexpr auto Lib_Ngs2 = "Lib.Ngs2";                               ///< The LibSceNgs2 implementation.
constexpr auto Lib_NpAuth = "Lib.NpAuth";                           ///< The LibSceNpAuth implementation
constexpr auto Lib_NpCommerce = "Lib.NpCommerce";                   ///< The LibSceNpCommerce implementation
constexpr auto Lib_NpCommon = "Lib.NpCommon";                       ///< The LibSceNpCommon implementation
constexpr auto Lib_NpManager = "Lib.NpManager";                     ///< The LibSceNpManager implementation
constexpr auto Lib_NpMatching2 = "Lib.NpMatching2";                 ///< The LibSceNpMatching2 implementation
constexpr auto Lib_NpPartner = "Lib.NpPartner";                     ///< The LibSceNpPartner implementation
constexpr auto Lib_NpParty = "Lib.NpParty";                         ///< The LibSceNpParty implementation
constexpr auto Lib_NpProfileDialog = "Lib.NpProfileDialog";         ///< The LibSceNpProfileDialog implementation
constexpr auto Lib_NpScore = "Lib.NpScore";                         ///< The LibSceNpScore implementation
constexpr auto Lib_NpSnsFacebookDialog = "Lib.NpSnsFacebookDialog"; ///< The LibSceNpSnsFacebookDialog implementation
constexpr auto Lib_NpTrophy = "Lib.NpTrophy";                       ///< The LibSceNpTrophy implementation
constexpr auto Lib_NpTus = "Lib.NpTus";                             ///< The LibSceNpTus implementation
constexpr auto Lib_NpWebApi = "Lib.NpWebApi";                       ///< The LibSceWebApi implementation
constexpr auto Lib_NpWebApi2 = "Lib.NpWebApi2";                     ///< The LibSceWebApi2 implementation
constexpr auto Lib_Pad = "Lib.Pad";                                 ///< The LibScePad implementation.
constexpr auto Lib_PlayGo = "Lib.PlayGo";                           ///< The LibScePlayGo implementation.
constexpr auto Lib_PlayGoDialog = "Lib.PlayGoDialog";               ///< The LibScePlayGoDialog implementation.
constexpr auto Lib_Png = "Lib.Png";                                 ///< The LibScePng implementation.
constexpr auto Lib_Random = "Lib.Random";                           ///< The LibSceRandom implementation.
constexpr auto Lib_RazorCpu = "Lib.RazorCpu";                       ///< The LibRazorCpu implementation.
constexpr auto Lib_Remoteplay = "Lib.Remoteplay";                   ///< The LibSceRemotePlay implementation
constexpr auto Lib_Rtc = "Lib.Rtc";                                 ///< The LibSceRtc implementation.
constexpr auto Lib_Rudp = "Lib.Rudp";                               ///< The LibSceRudp implementation.
constexpr auto Lib_SaveData = "Lib.SaveData";                       ///< The LibSceSaveData implementation.
constexpr auto Lib_SaveDataDialog = "Lib.SaveDataDialog";           ///< The LibSceSaveDataDialog implementation.
constexpr auto Lib_Screenshot = "Lib.Screenshot";                   ///< The LibSceScreenshot implementation
constexpr auto Lib_SharePlay = "Lib.SharePlay";                     ///< The LibSceSharePlay implemenation
constexpr auto Lib_SigninDialog = "Lib.SigninDialog";               ///< The LibSigninDialog implementation.
constexpr auto Lib_Ssl = "Lib.Ssl";                                 ///< The LibSceSsl implementation.
constexpr auto Lib_Ssl2 = "Lib.Ssl2";                               ///< The LibSceSsl2 implementation.
constexpr auto Lib_SysModule = "Lib.SysModule";                     ///< The LibSceSysModule implementation
constexpr auto Lib_SystemGesture = "Lib.SystemGesture";             ///< The LibSceSystemGesture implementation.
constexpr auto Lib_SystemService = "Lib.SystemService";             ///< The LibSceSystemService implementation.
constexpr auto Lib_Usbd = "Lib.Usbd";                               ///< The LibSceUsbd implementation.
constexpr auto Lib_UserService = "Lib.UserService";                 ///< The LibSceUserService implementation.
constexpr auto Lib_Vdec2 = "Lib.Vdec2";                             ///< The LibSceVideodec2 implementation.
constexpr auto Lib_VideoOut = "Lib.VideoOut";                       ///< The LibSceVideoOut implementation.
constexpr auto Lib_Videodec = "Lib.Videodec";                       ///< The LibSceVideodec implementation.
constexpr auto Lib_Voice = "Lib.Voice";                             ///< The LibSceVoice implementation.
constexpr auto Lib_VrTracker = "Lib.VrTracker";                     ///< The LibSceVrTracker implementation.
constexpr auto Lib_WebBrowserDialog = "Lib.WebBrowserDialog";       ///< The LibSceWebBrowserDialog implementation
constexpr auto Lib_Zlib = "Lib.Zlib";                               ///< The LibSceZlib implementation.
constexpr auto Loader = "Loader";                                   ///< ROM loader
constexpr auto Log = "Log";                                         ///< Messages about the log system itself
constexpr auto Render = "Render";                                   ///< Video Core
constexpr auto Render_Recompiler = "Render.Recompiler";             ///< Shader recompiler
constexpr auto Render_Vulkan = "Render.Vulkan";                     ///< Vulkan backend
constexpr auto Tty = "Tty";                                         ///< Debug output from emu
// clang-format on
} // namespace Common::Log::Class
