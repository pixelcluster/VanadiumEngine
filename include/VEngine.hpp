#pragma once

#include <string_view>
#include <cstddef>
#include <VModule.hpp>

template<typename Base, typename T>
concept DerivesFrom = requires(T* t) {
	static_cast<Base*>(t);
};

template<typename T, typename... Args>
concept ConstructibleWith = requires(Args... args) {
	T(args...);
};

class VEngine {
  public:
	VEngine(const std::string_view& appName, uint32_t appVersion);
	~VEngine();

	template <typename T, typename... Args>
	requires(DerivesFrom<VModule, T>&& ConstructibleWith<T, Args...>) T* createModule(Args... args);

	void activateModule(VModule* moduleToActivate);
	void activateModuleOnce(VModule* moduleToActivate);
	void deactivateModule(VModule* moduleToDeactivate);

  private:
	std::vector<VModule*> m_modules;
	std::vector<VModule*> m_activatedModules;
	std::vector<VModule*> m_modulesToDeactivate;
};

template <typename T, typename... Args>
requires(DerivesFrom<VModule, T>&& ConstructibleWith<T, Args...>) inline T* VEngine::createModule(Args... args) {
	m_modules.push_back(new T(args...));
	return m_modules.back();
}
