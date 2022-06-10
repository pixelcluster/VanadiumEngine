#include <cmath>
#include <timer/TimerManager.hpp>

namespace vanadium::timers {
	TimerManager::~TimerManager() {
		for (auto& timer : m_periodicTimers) {
			timer.destroyCallback(timer.userData);
		}
	}

	TimerHandle TimerManager::addTimer(float duration, TimerTriggerCallback triggerCallback,
									   TimerDestroyCallback destroyCallback, void* userData) {
		return m_periodicTimers.addElement({ .duration = duration,
											 .progress = 0.0f,
											 .callback = triggerCallback,
											 .destroyCallback = destroyCallback,
											 .userData = userData });
	}

	void TimerManager::removeTimer(TimerHandle timer) { m_periodicTimers.removeElement(timer); }

	void TimerManager::update(float deltaTime) {
		for (auto& timer : m_periodicTimers) {
			timer.progress += deltaTime;
			if (timer.progress > timer.duration) {
				timer.progress = fmodf(timer.progress, timer.duration);
				timer.callback(timer.userData);
			}
		}
	}
} // namespace vanadium::timers
