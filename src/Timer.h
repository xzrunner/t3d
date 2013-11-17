#pragma once

#include <Windows.h>

namespace t3d {

class Timer
{
public:
	Timer() : start_clock_count(0), count(0), time(0), fps(0) {}

	DWORD Get_Clock(void)
	{
		// this function returns the current tick count
		return(GetTickCount());
	}

	DWORD Start_Clock(void)
	{
		// this function starts the clock, that is, saves the current
		// count, use in conjunction with Wait_Clock()
		return(start_clock_count = Get_Clock());
	}

	DWORD Wait_Clock(DWORD count)
	{
		// this function is used to wait for a specific number of clicks
		// since the call to Start_Clock
		while((Get_Clock() - start_clock_count) < count);
		return(Get_Clock());
	}

	void Step() {
		static DWORD last = 0;
		DWORD curr = Get_Clock();
		
		int step = curr - last;
		if (step > 100) {
			last = curr;
			return;
		}
		time += step;
		++count;
		if (time > 1000)
		{
			fps = count * 1000.0f / time;
			time = count = 0;
		}

		last = curr;
	}
	int FPS() {
		return (int)fps;
	}

private:
	DWORD start_clock_count;     // used for timing

	int count;
	int time;
	float fps;

}; // Time

}