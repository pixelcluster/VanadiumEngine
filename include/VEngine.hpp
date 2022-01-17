#pragma once

#include <VModule.hpp>
#include <cstddef>
#include <semaphore>
#include <string_view>

template <typename Base, typename T>
concept DerivesFrom = requires(T* t) {
	static_cast<Base*>(t);
};

template <typename T, typename... Args>
concept ConstructibleWith = requires(Args... args) {
	T(args...);
};

struct VModuleDependency {
	VModule* from;
	VModule* to;
};

struct VModuleExecutionInfo {
	std::vector<std::atomic<bool>*> waitAtomics;
	VModule* executedModule;
	std::optional<std::atomic<bool>> signalAtomic;
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

	// Both from and to must be activated modules
	void addModuleDependency(VModule* from, VModule* to);

	void run();

  private:
	void recalculateScoreboard();

	void removeModuleDependencies(VModule* removeModule);

	std::vector<VModule*> m_modules;
	std::vector<VModule*> m_activatedModules;
	std::vector<VModule*> m_modulesToDeactivate;
	std::vector<VModuleDependency> m_moduleDependencies;

	std::vector<std::vector<VModuleExecutionInfo>> m_threadExecutionInfos;
	bool m_scoreboardDirty;

	bool m_shouldExit = false;
};

template <typename T, typename... Args>
requires(DerivesFrom<VModule, T>&& ConstructibleWith<T, Args...>) inline T* VEngine::createModule(Args... args) {
	m_modules.push_back(new T(args...));
	return m_modules.back();
}
