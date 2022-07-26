/* VanadiumEngine, a Vulkan rendering toolkit
 * Copyright (C) 2022 Friedrich Vock
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once

#include <Slotmap.hpp>

namespace vanadium::timers {
	using TimerHandle = SlotmapHandle;

	using TimerTriggerCallback = void (*)(void* userData);
	using TimerDestroyCallback = void (*)(void* userData);

	struct PeriodicTimer {
		float duration;
		float progress;
		TimerTriggerCallback callback;
		TimerDestroyCallback destroyCallback;
		void* userData;
	};

	inline void emptyTimerDestroyCallback(void* userData) {}

	class TimerManager {
	  public:
		TimerManager() = default;
		TimerManager(const TimerManager&) = delete;
		TimerManager& operator=(const TimerManager&) = delete;
		TimerManager(TimerManager&&) = delete;
		TimerManager& operator=(TimerManager&&) = delete;
		~TimerManager();

		TimerHandle addTimer(float duration, TimerTriggerCallback triggerCallback, TimerDestroyCallback destroyCallback,
							 void* userData);

		void removeTimer(TimerHandle timer);

		void update(float deltaTime);

	  private:
		Slotmap<PeriodicTimer> m_periodicTimers;
	};

} // namespace vanadium::timers
