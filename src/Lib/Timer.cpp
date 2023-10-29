#include "Timer.h"

#ifdef _WIN64
#include <windows.h>
#endif

Lib::Timer::Timer() {
#ifdef _WIN64
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    m_Frequency = f.QuadPart;
#else
#error Unimplemented Timer constructor
#endif
}

void Lib::Timer::Start() {
#ifdef _WIN64
    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    m_StartTime = c.QuadPart;
#else
#error Unimplemented Timer::Start()
#endif
    m_is_timer_paused = false;
}

void Lib::Timer::Pause() {
#ifdef _WIN64
    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    m_PauseTime = c.QuadPart;
#else
#error Unimplemented Timer::Pause()
#endif
    m_is_timer_paused = true;
}

void Lib::Timer::Resume() {
    u64 current_time = 0;
#ifdef _WIN64
    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    current_time = c.QuadPart;
#else
#error Unimplemented Timer::Resume()
#endif
    m_StartTime += current_time - m_PauseTime;
    m_is_timer_paused = false;
}

bool Lib::Timer::IsPaused() const { return m_is_timer_paused; }

double Lib::Timer::GetTimeMsec() const {
    if (m_is_timer_paused) {
        return 1000.0 * (static_cast<double>(m_PauseTime - m_StartTime)) / static_cast<double>(m_Frequency);
    }

    u64 current_time = 0;
#ifdef _WIN64
    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    current_time = c.QuadPart;
#else
#error Unimplemented Timer::GetTimeMsec()
#endif
    return 1000.0 * (static_cast<double>(current_time - m_StartTime)) / static_cast<double>(m_Frequency);
}

double Lib::Timer::GetTimeSec() const {
    if (m_is_timer_paused) {
        return (static_cast<double>(m_PauseTime - m_StartTime)) / static_cast<double>(m_Frequency);
    }

    u64 current_time = 0;
#ifdef _WIN64
    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    current_time = c.QuadPart;
#else
#error Unimplemented Timer::GetTimeSec()
#endif
    return (static_cast<double>(current_time - m_StartTime)) / static_cast<double>(m_Frequency);
}

u64 Lib::Timer::GetTicks() const {
    if (m_is_timer_paused) {
        return (m_PauseTime - m_StartTime);
    }

    u64 current_time = 0;
#ifdef _WIN64
    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    current_time = c.QuadPart;
#else
#error Unimplemented Timer::GetTicks()
#endif
    return (current_time - m_StartTime);
}

u64 Lib::Timer::GetFrequency() const { return m_Frequency; }

