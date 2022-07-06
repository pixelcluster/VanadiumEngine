#pragma once

#include <util/Slotmap.hpp>

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
