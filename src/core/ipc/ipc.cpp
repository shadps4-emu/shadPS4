//  SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include "ipc.h"

#include <iostream>
#include <string>

#include <SDL3/SDL.h>

#include "common/config.h"
#include "common/memory_patcher.h"
#include "common/thread.h"
#include "common/types.h"
#include "core/debug_state.h"
#include "core/debugger.h"
#include "core/libraries/audio/audioout.h"
#include "input/input_handler.h"
#include "sdl_window.h"
#include "video_core/renderer_vulkan/vk_presenter.h"

extern std::unique_ptr<Vulkan::Presenter> presenter;

/**
 * Protocol summary:
 * - IPC is enabled by setting the SHADPS4_ENABLE_IPC environment variable to "true"
 * - Input will be stdin & output stderr
 * - Strings are sent as UTF8
 * - Each communication line is terminated by a newline character ('\n')
 * - Each command parameter will be separated by a newline character ('\n'),
 *   variadic commands will start sending the number of parameters after the cmd word.
 *   Any ('\n') in the parameter must be escaped by a backslash ('\\n')
 * - Numbers can be sent with any base. Must prefix the number with '0x' for hex,
 *   '0b' for binary, or '0' for octal. Decimal numbers
 *   will be sent without any prefix.
 * - Output will be started by (';')
 * - The IPC server(this) will send a block started by
 *   #IPC_ENABLED
 *   and ended by
 *   #IPC_END
 *   In between, it will send the current capabilities and commands before the emulator start
 * - The IPC client(e.g., launcher) will send RUN then START to continue the execution
 **/

/**
 * Command list:
 * - CAPABILITIES:
 *   - ENABLE_MEMORY_PATCH: enables PATCH_MEMORY command
 *   - ENABLE_EMU_CONTROL: enables PAUSE, RESUME, STOP, TOGGLE_FULLSCREEN commands
 * - INPUT CMD:
 *   - RUN: start the emulator execution
 *   - START: start the game execution
 *   - PATCH_MEMORY(
 *       modName: str, offset: str, value: str,
 *       target: str, size: str, isOffset: number, littleEndian: number,
 *       patchMask: number, maskOffset: number
 *     ): add a memory patch, check @ref MemoryPatcher::PatchMemory for details
 *   - PAUSE: pause the game execution
 *   - RESUME: resume the game execution
 *   - STOP: stop and quit the emulator
 *   - TOGGLE_FULLSCREEN: enable / disable fullscreen
 * - OUTPUT CMD:
 *   - RESTART(argn: number, argv: ...string): Request restart of the emulator, must call STOP
 **/

void IPC::Init() {
    const char* enabledEnv = std::getenv("SHADPS4_ENABLE_IPC");
    enabled = enabledEnv && strcmp(enabledEnv, "true") == 0;
    if (!enabled) {
        return;
    }

    input_thread = std::jthread([this] {
        Common::SetCurrentThreadName("IPC Read thread");
        this->InputLoop();
    });

    std::cerr << ";#IPC_ENABLED\n";
    std::cerr << ";ENABLE_MEMORY_PATCH\n";
    std::cerr << ";ENABLE_EMU_CONTROL\n";
    std::cerr << ";#IPC_END\n";
    std::cerr.flush();

    const auto ok = run_semaphore.try_acquire_for(std::chrono::seconds(5));
    if (!ok) {
        std::cerr << "IPC: Failed to acquire run semaphore, closing process.\n";
        exit(1);
    }
}

void IPC::SendRestart(const std::vector<std::string>& args) {
    std::cerr << ";RESTART\n";
    std::cerr << ";" << args.size() << "\n";
    for (const auto& arg : args) {
        std::cerr << ";" << arg << "\n";
    }
    std::cerr.flush();
}

void IPC::InputLoop() {
    auto next_str = [&] -> const std::string& {
        static std::string line_buffer;
        do {
            std::getline(std::cin, line_buffer, '\n');
        } while (!line_buffer.empty() && line_buffer.back() == '\\');
        return line_buffer;
    };
    auto next_u64 = [&] -> u64 {
        auto& str = next_str();
        return std::stoull(str, nullptr, 0);
    };

    while (true) {
        auto& cmd = next_str();
        if (cmd.empty()) {
            continue;
        }
        if (cmd == "RUN") {
            run_semaphore.release();
        } else if (cmd == "START") {
            start_semaphore.release();
        } else if (cmd == "PATCH_MEMORY") {
            MemoryPatcher::patchInfo entry;
            entry.gameSerial = "*";
            entry.modNameStr = next_str();
            entry.offsetStr = next_str();
            entry.valueStr = next_str();
            entry.targetStr = next_str();
            entry.sizeStr = next_str();
            entry.isOffset = next_u64() != 0;
            entry.littleEndian = next_u64() != 0;
            entry.patchMask = static_cast<MemoryPatcher::PatchMask>(next_u64());
            entry.maskOffset = static_cast<int>(next_u64());
            MemoryPatcher::AddPatchToQueue(entry);
        } else if (cmd == "PAUSE") {
            DebugState.PauseGuestThreads();
        } else if (cmd == "RESUME") {
            DebugState.ResumeGuestThreads();
        } else if (cmd == "STOP") {
            SDL_Event event;
            SDL_memset(&event, 0, sizeof(event));
            event.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&event);
        } else if (cmd == "TOGGLE_FULLSCREEN") {
            SDL_Event event;
            SDL_memset(&event, 0, sizeof(event));
            event.type = SDL_EVENT_TOGGLE_FULLSCREEN;
            SDL_PushEvent(&event);
        } else if (cmd == "ADJUST_VOLUME") {
            int value = static_cast<int>(next_u64());
            bool is_game_specific = next_u64() != 0;
            Config::setVolumeSlider(value, is_game_specific);
            Libraries::AudioOut::AdjustVol();
        } else if (cmd == "SET_FSR") {
            bool use_fsr = next_u64() != 0;
            if (presenter) {
                presenter->GetFsrSettingsRef().enable = use_fsr;
            }
        } else if (cmd == "SET_RCAS") {
            bool use_rcas = next_u64() != 0;
            if (presenter) {
                presenter->GetFsrSettingsRef().use_rcas = use_rcas;
            }
        } else if (cmd == "SET_RCAS_ATTENUATION") {
            int value = static_cast<int>(next_u64());
            if (presenter) {
                presenter->GetFsrSettingsRef().rcas_attenuation =
                    static_cast<float>(value / 1000.0f);
            }
        } else if (cmd == "RELOAD_INPUTS") {
            std::string config = next_str();
            Input::ParseInputConfig(config);
        } else if (cmd == "SET_ACTIVE_CONTROLLER") {
            std::string active_controller = next_str();
            GamepadSelect::SetSelectedGamepad(active_controller);
            SDL_Event checkGamepad;
            SDL_memset(&checkGamepad, 0, sizeof(checkGamepad));
            checkGamepad.type = SDL_EVENT_CHANGE_CONTROLLER;
            SDL_PushEvent(&checkGamepad);
        } else {
            std::cerr << ";UNKNOWN CMD: " << cmd << std::endl;
        }
    }
}
