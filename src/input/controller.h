// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <memory>
#include <mutex>
#include <vector>

#include <SDL3/SDL_gamepad.h>

#include "common/types.h"
#include "core/libraries/pad/pad.h"

namespace Input {

enum class Axis {
    LeftX = 0,
    LeftY = 1,
    RightX = 2,
    RightY = 3,
    TriggerLeft = 4,
    TriggerRight = 5,

    AxisMax
};

struct TouchpadEntry {
    u8 ID = 0;
    bool state{};
    u16 x{};
    u16 y{};
};

class State {
public:
    void OnButton(Libraries::Pad::OrbisPadButtonDataOffset, bool);
    void OnAxis(Axis, int);
    void OnTouchpad(int touchIndex, bool isDown, float x, float y);
    void OnGyro(const float[3]);
    void OnAccel(const float[3]);

    Libraries::Pad::OrbisPadButtonDataOffset buttonsState{};
    u64 time = 0;
    int axes[static_cast<int>(Axis::AxisMax)] = {128, 128, 128, 128, 0, 0};
    TouchpadEntry touchpad[2] = {{false, 0, 0}, {false, 0, 0}};
    Libraries::Pad::OrbisFVector3 acceleration = {0.0f, 0.0f, 0.0f};
    Libraries::Pad::OrbisFVector3 angularVelocity = {0.0f, 0.0f, 0.0f};
    Libraries::Pad::OrbisFQuaternion orientation = {0.0f, 0.0f, 0.0f, 1.0f};
};

class Engine {
public:
    virtual ~Engine() = default;
    virtual void Init() = 0;
    virtual void SetLightBarRGB(u8 r, u8 g, u8 b) = 0;
    virtual void SetVibration(u8 smallMotor, u8 largeMotor) = 0;
    virtual State ReadState() = 0;
    virtual float GetAccelPollRate() const = 0;
    virtual float GetGyroPollRate() const = 0;
    SDL_Gamepad* m_gamepad;
};

inline int GetAxis(int min, int max, int value) {
    return std::clamp((255 * (value - min)) / (max - min), 0, 255);
}

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

class GameController {
public:
    GameController();
    virtual ~GameController() = default;

    void ReadState(State* state, bool* isConnected, int* connectedCount);
    int ReadStates(State* states, int states_num, bool* isConnected, int* connectedCount);

    void Button(int id, Libraries::Pad::OrbisPadButtonDataOffset button, bool isPressed);
    void Axis(int id, Input::Axis axis, int value);
    void Gyro(int id, const float gyro[3]);
    void Acceleration(int id, const float acceleration[3]);
    void SetLightBarRGB(u8 r, u8 g, u8 b);
    void SetVibration(u8 smallMotor, u8 largeMotor);
    void SetTouchpadState(int touchIndex, bool touchDown, float x, float y);
    void SetEngine(std::unique_ptr<Engine>);
    Engine* GetEngine();
    u32 Poll();

    u8 GetTouchCount();
    void SetTouchCount(u8 touchCount);
    u8 GetSecondaryTouchCount();
    void SetSecondaryTouchCount(u8 touchCount);
    u8 GetPreviousTouchNum();
    void SetPreviousTouchNum(u8 touchNum);
    bool WasSecondaryTouchReset();
    void UnsetSecondaryTouchResetBool();

    void SetLastOrientation(Libraries::Pad::OrbisFQuaternion& orientation);
    Libraries::Pad::OrbisFQuaternion GetLastOrientation();
    std::chrono::steady_clock::time_point GetLastUpdate();
    void SetLastUpdate(std::chrono::steady_clock::time_point lastUpdate);
    static void CalculateOrientation(Libraries::Pad::OrbisFVector3& acceleration,
                                     Libraries::Pad::OrbisFVector3& angularVelocity,
                                     float deltaTime,
                                     Libraries::Pad::OrbisFQuaternion& lastOrientation,
                                     Libraries::Pad::OrbisFQuaternion& orientation);

private:
    void PushState();

    bool m_connected = true;
    int m_connected_count = 1;
    u8 m_touch_count = 0;
    u8 m_secondary_touch_count = 0;
    u8 m_previous_touchnum = 0;
    bool m_was_secondary_reset = false;
    std::chrono::steady_clock::time_point m_last_update = {};
    Libraries::Pad::OrbisFQuaternion m_orientation = {0.0f, 0.0f, 0.0f, 1.0f};

    State m_state;

    std::mutex m_states_queue_mutex;
    RingBufferQueue<State> m_states_queue;

    std::unique_ptr<Engine> m_engine = nullptr;
};

} // namespace Input

namespace GamepadSelect {

int GetIndexfromGUID(SDL_JoystickID* gamepadIDs, int gamepadCount, std::string GUID);
std::string GetGUIDString(SDL_JoystickID* gamepadIDs, int index);
std::string GetSelectedGamepad();
void SetSelectedGamepad(std::string GUID);

} // namespace GamepadSelect
