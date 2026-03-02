// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/pad/pad.h"
#include "core/libraries/system/userservice.h"

#include <unordered_set>
#include <SDL3/SDL.h>

#pragma once

namespace Input {

static constexpr int VendorSony = 0x054C;
static constexpr int ProductMoveZCM1 = 0x03D5;
static constexpr int ProductMoveZCM2 = 0x0C5E;

struct OrbisFQuaternion {
    float x, y, z, w;
};

struct OrbisFVector3 {
    float x, y, z;
};

template <class T>
class RingBufferQueue {
public:
    RingBufferQueue(size_t size) : m_storage(size) {}

    void Push(T item) {
        const size_t index = (m_begin + m_size) % m_storage.size();
        m_storage[index] = std::move(item);
        if (m_size < m_storage.size()) {
            m_size += 1;
        } else {
            m_begin = (m_begin + 1) % m_storage.size();
        }
    }

    std::optional<T> Pop() {
        if (m_size == 0) {
            return {};
        }
        const size_t index = m_begin;
        m_begin = (m_begin + 1) % m_storage.size();
        m_size -= 1;
        return std::move(m_storage[index]);
    }

private:
    size_t m_begin = 0;
    size_t m_size = 0;
    std::vector<T> m_storage;
};

struct State {};

class MoveController {
    friend class MoveControllers;

public:
    MoveController();
    virtual ~MoveController() = default;

    // void ReadState(State* state, bool* isConnected, int* connectedCount);
    // int ReadStates(State* states, int states_num, bool* isConnected, int* connectedCount);

    // void Button(Libraries::Pad::OrbisPadButtonDataOffset button, bool isPressed);
    // void Axis(Input::Axis axis, int value, bool smooth = true);
    void Gyro(int id);
    void Acceleration(int id);
    void UpdateGyro(const float gyro[3]);
    void UpdateAcceleration(const float acceleration[3]);
    void UpdateAxisSmoothing();
    void SetLightBarRGB(u8 r, u8 g, u8 b);
    bool SetVibration(u8 smallMotor, u8 largeMotor);
    void SetTouchpadState(int touchIndex, bool touchDown, float x, float y);

    u8 GetTouchCount();
    void SetTouchCount(u8 touchCount);
    u8 GetSecondaryTouchCount();
    void SetSecondaryTouchCount(u8 touchCount);
    u8 GetPreviousTouchNum();
    void SetPreviousTouchNum(u8 touchNum);
    bool WasSecondaryTouchReset();
    void UnsetSecondaryTouchResetBool();

    void SetLastOrientation(OrbisFQuaternion& orientation);
    OrbisFQuaternion GetLastOrientation();
    std::chrono::steady_clock::time_point GetLastUpdate();
    void SetLastUpdate(std::chrono::steady_clock::time_point lastUpdate);
    static void CalculateOrientation(OrbisFVector3& acceleration, OrbisFVector3& angularVelocity,
                                     float deltaTime, OrbisFQuaternion& lastOrientation,
                                     OrbisFQuaternion& orientation);

    float gyro_poll_rate;
    float accel_poll_rate;
    float gyro_buf[3] = {0.0f, 0.0f, 0.0f}, accel_buf[3] = {0.0f, 9.81f, 0.0f};
    u32 user_id = Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID;
    SDL_Gamepad* m_sdl_gamepad = nullptr;

private:
    void PushState();

    bool m_connected = true;
    int m_connected_count = 1;
    u8 m_touch_count = 0;
    u8 m_secondary_touch_count = 0;
    u8 m_previous_touchnum = 0;
    bool m_was_secondary_reset = false;
    std::chrono::steady_clock::time_point m_last_update = {};
    OrbisFQuaternion m_orientation = {0.0f, 0.0f, 0.0f, 1.0f};

    u8 player_index = -1;
    State m_state;

    std::mutex m_states_queue_mutex;
    RingBufferQueue<State> m_states_queue;
};

class MoveControllers {
    std::array<MoveController*, 4> controllers;

public:
    MoveControllers()
        : controllers({new MoveController(), new MoveController(), new MoveController(),
                       new MoveController()}) {};
    virtual ~MoveControllers() = default;
    MoveController* operator[](const size_t& i) const {
        if (i > 3) {
            UNREACHABLE_MSG("Index {} is out of bounds for MoveControllers!", i);
        }
        return controllers[i];
    }
    static void TryOpenSDLControllers(MoveControllers& controllers);
    static u8 GetGamepadIndexFromJoystickId(SDL_JoystickID id);
};

} // namespace Input
