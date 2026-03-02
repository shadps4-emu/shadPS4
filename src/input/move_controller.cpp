// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "move_controller.h"

#include "common/logging/log.h"
#include "core/emulator_settings.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/move/move.h"
#include "core/libraries/move/move_error.h"

namespace Input {

static bool is_first_check = true;

void MoveControllers::TryOpenSDLControllers(MoveControllers& controllers) {
    using namespace Libraries::UserService;
    int controller_count;
    SDL_JoystickID* new_joysticks = SDL_GetGamepads(&controller_count);

    std::unordered_set<SDL_JoystickID> assigned_ids;
    std::array<bool, 4> slot_taken{false, false, false, false};

    for (int i = 0; i < 4; i++) {
        SDL_Gamepad* pad = controllers[i]->m_sdl_gamepad;
        if (pad) {
            SDL_JoystickID id = SDL_GetGamepadID(pad);
            bool still_connected = false;
            for (int j = 0; j < controller_count; j++) {
                SDL_GUID guid = SDL_GetJoystickGUID(SDL_GetJoystickFromID(new_joysticks[j]));
                Uint16 vendor = 0, product = 0;
                SDL_GetJoystickGUIDInfo(guid, &vendor, &product, nullptr, nullptr);
                if (vendor != VendorSony || product != ProductMoveZCM1 ||
                    product != ProductMoveZCM2) {
                    continue; // not a Move controller
                }
                if (new_joysticks[j] == id) {
                    still_connected = true;
                    assigned_ids.insert(id);
                    slot_taken[i] = true;
                    break;
                }
            }
            if (!still_connected) {
                AddUserServiceEvent({OrbisUserServiceEventType::Logout, i + 1});
                SDL_CloseGamepad(pad);
                controllers[i]->m_sdl_gamepad = nullptr;
                controllers[i]->user_id = -1;
                slot_taken[i] = false;
            } else {
                controllers[i]->player_index = i;
            }
        }
    }

    for (int j = 0; j < controller_count; j++) {
        SDL_JoystickID id = new_joysticks[j];
        if (assigned_ids.contains(id))
            continue;

        SDL_Gamepad* pad = SDL_OpenGamepad(id);
        if (!pad)
            continue;

        for (int i = 0; i < 4; i++) {
            if (!slot_taken[i]) {
                auto* c = controllers[i];
                c->m_sdl_gamepad = pad;
                LOG_INFO(Input, "Gamepad registered for slot {}! Handle: {}", i,
                         SDL_GetGamepadID(pad));
                c->user_id = i + 1;
                slot_taken[i] = true;
                c->player_index = i;
                AddUserServiceEvent({OrbisUserServiceEventType::Login, i + 1});
                if (SDL_SetGamepadSensorEnabled(c->m_sdl_gamepad, SDL_SENSOR_GYRO, true)) {
                    c->gyro_poll_rate =
                        SDL_GetGamepadSensorDataRate(c->m_sdl_gamepad, SDL_SENSOR_GYRO);
                    LOG_INFO(Input, "Gyro initialized, poll rate: {}", c->gyro_poll_rate);
                } else {
                    LOG_ERROR(Input, "Failed to initialize gyro controls for gamepad {}",
                              c->user_id);
                }
                if (SDL_SetGamepadSensorEnabled(c->m_sdl_gamepad, SDL_SENSOR_ACCEL, true)) {
                    c->accel_poll_rate =
                        SDL_GetGamepadSensorDataRate(c->m_sdl_gamepad, SDL_SENSOR_ACCEL);
                    LOG_INFO(Input, "Accel initialized, poll rate: {}", c->accel_poll_rate);
                } else {
                    LOG_ERROR(Input, "Failed to initialize accel controls for gamepad {}",
                              c->user_id);
                }
                break;
            }
        }
    }
    if (is_first_check) [[unlikely]] {
        is_first_check = false;
        if (controller_count == 0) {
            controllers[0]->user_id = 1;
            AddUserServiceEvent({OrbisUserServiceEventType::Login, 1});
        }
    }
    SDL_free(new_joysticks);
}

} // namespace Input
