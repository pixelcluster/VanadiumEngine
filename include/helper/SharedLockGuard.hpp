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