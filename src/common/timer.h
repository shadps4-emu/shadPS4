#pragma once

#include "common/types.h"

namespace Common {

class Timer final {
public:
    Timer();
    ~Timer() = default;

    void Start();
    void Pause();
    void Resume();

    bool IsPaused() const {
        return m_is_timer_paused;
    }

    u64 GetFrequency() const {
        return m_Frequency;
    }

    double GetTimeMsec() const;
    double GetTimeSec() const;
    u64 GetTicks() const;

    [[nodiscard]] static u64 getQueryPerformanceCounter();

public:
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;
    Timer(Timer&&) = delete;
    Timer& operator=(Timer&&) = delete;

private:
    bool m_is_timer_paused = true;
    u64 m_Frequency{};
    u64 m_StartTime{};
    u64 m_PauseTime{};
};

} // namespace Common
