// SPDX-FileCopyrightText: Copyright 2023 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Common::Log {

/// Specifies the severity or level of detail of the log message.
enum class Level : u8 {
    Trace, ///< Extremely detailed and repetitive debugging information that is likely to
    ///< pollute logs.
    Debug,   ///< Less detailed debugging information.
    Info,    ///< Status information from important points during execution.
    Warning, ///< Minor or potential problems found during execution of a task.
    Error,   ///< Major problems found during execution of a task that prevent it from being
    ///< completed.
    Critical, ///< Major problems during execution that threaten the stability of the entire
    ///< application.

    Count, ///< Total number of logging levels
};

/**
 * Specifies the sub-system that generated the log message.
 *
 * @note If you add a new entry here, also add a corresponding one to `ALL_LOG_CLASSES` in
 * filter.cpp.
 */
enum class Class : u8 {
    Log,                ///< Messages about the log system itself
    Common,             ///< Library routines
    Common_Filesystem,  ///< Filesystem interface library
    Common_Memory,      ///< Memory mapping and management functions
    Core,               ///< LLE emulation core
    Core_Linker,        ///< The module linker
    Config,             ///< Emulator configuration (including commandline)
    Debug,              ///< Debugging tools
    Kernel,             ///< The HLE implementation of the PS4 kernel.
    Kernel_Pthread,     ///< The pthread implementation of the kernel.
    Kernel_Fs,          ///< The filesystem implementation of the kernel.
    Kernel_Vmm,         ///< The virtual memory implementation of the kernel.
    Kernel_Event,       ///< The event management implementation of the kernel.
    Kernel_Sce,         ///< The sony specific interfaces provided by the kernel.
    Lib,                ///< HLE implementation of system library. Each major library
                        ///< should have its own subclass.
    Lib_LibC,           ///< The LibC implementation.
    Lib_Kernel,         ///< The LibKernel implementation.
    Lib_Pad,            ///< The LibScePad implementation.
    Lib_GnmDriver,      ///< The LibSceGnmDriver implementation.
    Lib_SystemService,  ///< The LibSceSystemService implementation.
    Lib_UserService,    ///< The LibSceUserService implementation.
    Lib_VideoOut,       ///< The LibSceVideoOut implementation.
    Lib_CommonDlg,      ///< The LibSceCommonDialog implementation.
    Lib_MsgDlg,         ///< The LibSceMsgDialog implementation.
    Lib_AudioOut,       ///< The LibSceAudioOut implementation.
    Lib_AudioIn,        ///< The LibSceAudioIn implementation.
    Lib_Net,            ///< The LibSceNet implementation.
    Lib_NetCtl,         ///< The LibSecNetCtl implementation.
    Lib_SaveData,       ///< The LibSceSaveData implementation.
    Lib_SaveDataDialog, ///< The LibSceSaveDataDialog implementation.
    Lib_Ssl,            ///< The LibSceSsl implementation.
    Lib_Http,           ///< The LibSceHttp implementation.
    Lib_SysModule,      ///< The LibSceSysModule implementation
    Lib_NpManager,      ///< The LibSceNpManager implementation
    Lib_NpScore,        ///< The LibSceNpScore implementation
    Lib_NpTrophy,       ///< The LibSceNpTrophy implementation
    Lib_Screenshot,     ///< The LibSceScreenshot implementation
    Lib_LibCInternal,   ///< The LibCInternal implementation.
    Lib_AppContent,     ///< The LibSceAppContent implementation.
    Lib_Rtc,            ///< The LibSceRtc implementation.
    Lib_DiscMap,        ///< The LibSceDiscMap implementation.
    Lib_Png,            ///< The LibScePng implementation.
    Lib_PlayGo,         ///< The LibScePlayGo implementation.
    Lib_Random,         ///< The libSceRandom implementation.
    Lib_Usbd,           ///< The LibSceUsbd implementation.
    Lib_Ajm,            ///< The LibSceAjm implementation.
    Lib_ErrorDialog,    ///< The LibSceErrorDialog implementation.
    Lib_ImeDialog,      ///< The LibSceImeDialog implementation.
    Lib_AvPlayer,       ///< The LibSceAvPlayer implementation.
    Lib_Ngs2,           ///< The LibSceNgs2 implementation.
    Frontend,           ///< Emulator UI
    Render,             ///< Video Core
    Render_Vulkan,      ///< Vulkan backend
    Render_Recompiler,  ///< Shader recompiler
    ImGui,              ///< ImGui
    Loader,             ///< ROM loader
    Input,              ///< Input emulation
    Tty,                ///< Debug output from emu
    Count               ///< Total number of logging classes
};

} // namespace Common::Log
