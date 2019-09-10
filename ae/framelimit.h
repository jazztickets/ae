/******************************************************************************
* Copyright (c) 2019 Alan Witkowski
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would be
*    appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
*    misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*******************************************************************************/
#pragma once

// Libraries
#include <SDL_timer.h>

namespace ae {

class _FrameLimit {

	public:

		// Constructor
		_FrameLimit(double FrameRate, bool Vsync) : FrameRate(FrameRate), Vsync(Vsync) { Reset(); }

		// Set the frame timer = now()
		void Reset() {
			Timer = SDL_GetPerformanceCounter();
		}

		// Limit frame rate
		void Update() {
			if(Vsync)
				return;

			// Get frame time
			double LastFrameTime = (SDL_GetPerformanceCounter() - Timer) / (double)SDL_GetPerformanceFrequency();

			// Limit frame rate
			if(FrameRate > 0.0) {
				double ExtraTime = 1.0 / FrameRate - LastFrameTime;
				if(ExtraTime > 0.0) {
					SDL_Delay((Uint32)(ExtraTime * 1000));
				}
			}

			// Reset timer after delay
			Reset();
		}

		// Attributes
		double FrameRate;
		bool Vsync;

	private:

		// Time
		Uint64 Timer;
};

}
