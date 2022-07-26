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
#include <shared_mutex>

namespace vanadium {
	class SharedLockGuard {
	  public:
		SharedLockGuard(std::shared_mutex& mutex) : m_mutex(mutex) { m_mutex.lock_shared(); }
		SharedLockGuard(std::shared_mutex& mutex, std::adopt_lock_t) : m_mutex(mutex) {}
		~SharedLockGuard() { m_mutex.unlock_shared(); }

	  private:
		std::shared_mutex& m_mutex;
	};
} // namespace vanadium
