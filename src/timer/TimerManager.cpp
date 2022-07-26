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
