#pragma once

#include "common/types.h"

namespace Lib {
	class Timer final
	{
	public:
		Timer();
		~Timer() = default;

		void Start();
		void Pause();
		void Resume();
		bool IsPaused() const;

		double GetTimeMsec() const;// return time in milliseconds		
		double GetTimeSec() const;// return time in seconds		
		u64 GetTicks() const;// return time in ticks
		u64 GetFrequency() const;// return ticks frequency
        [[nodiscard]] static u64 getQueryPerformanceCounter();
	public:
		Timer(const Timer&) = delete;
		Timer& operator=(const Timer&) = delete;
		Timer(Timer&&) = delete;
		Timer& operator=(Timer&&) = delete;

	private:
		bool m_is_timer_paused = true;
		u64 m_Frequency = 0;
		u64 m_StartTime = 0;
		u64 m_PauseTime = 0;
	};
}