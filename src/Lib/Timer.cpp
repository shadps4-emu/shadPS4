#include "Timer.h"

Lib::Timer::Timer()
{
}

void Lib::Timer::Start()
{
}

void Lib::Timer::Pause()
{
}

void Lib::Timer::Resume()
{
}

bool Lib::Timer::IsPaused() const
{
	return false;
}

double Lib::Timer::GetTimeMsec() const
{
	return 0.0;
}

double Lib::Timer::GetTimeSec() const
{
	return 0.0;
}

u64 Lib::Timer::GetTicks() const
{
	return u64();
}

u64 Lib::Timer::GetFrequency() const
{
	return u64();
}
