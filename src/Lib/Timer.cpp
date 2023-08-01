#include "Timer.h"
#include <windows.h>

Lib::Timer::Timer() {
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    m_Frequency = f.QuadPart;
}

void Lib::Timer::Start() {
    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    m_StartTime = c.QuadPart;
    m_is_timer_paused = false;
}

void Lib::Timer::Pause() {
    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    m_PauseTime = c.QuadPart;
    m_is_timer_paused = true;
}

void Lib::Timer::Resume() {
    u64 current_time = 0;

    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    current_time = c.QuadPart;

    m_StartTime += current_time - m_PauseTime;
    m_is_timer_paused = false;
}

bool Lib::Timer::IsPaused() const { return m_is_timer_paused; }

double Lib::Timer::GetTimeMsec() const {
    if (m_is_timer_paused) {
        return 1000.0 * (static_cast<double>(m_PauseTime - m_StartTime)) / static_cast<double>(m_Frequency);
    }

    u64 current_time = 0;

    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    current_time = c.QuadPart;

    return 1000.0 * (static_cast<double>(current_time - m_StartTime)) / static_cast<double>(m_Frequency);
}

double Lib::Timer::GetTimeSec() const {
    if (m_is_timer_paused) {
        return (static_cast<double>(m_PauseTime - m_StartTime)) / static_cast<double>(m_Frequency);
    }

    u64 current_time = 0;

    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    current_time = c.QuadPart;

    return (static_cast<double>(current_time - m_StartTime)) / static_cast<double>(m_Frequency);
}

u64 Lib::Timer::GetTicks() const {
    if (m_is_timer_paused) {
        return (m_PauseTime - m_StartTime);
    }

    u64 current_time = 0;

    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    current_time = c.QuadPart;

    return (current_time - m_StartTime);
}

u64 Lib::Timer::GetFrequency() const { return m_Frequency; }

