#pragma once

#include <thread>
#include <helper/VModuleThread.hpp>
#include <helper/VScoreboarding.hpp>
#include <cstddef>
#include <semaphore>
#include <string_view>

template <typename T, typename... Args>
concept ConstructibleWith = requires(Args... args) {
	T(args...);
};

using VScoreboardingFunction = void (*)(const std::vector<VModule*>& activatedModules,
										const std::vector<VModuleDependency>& moduleDependencies,
										std::vector<std::vector<VModuleExecutionInfo>>& threadExecutionInfos);

constexpr VScoreboardingFunction vEngineScoreboardingFunction = singleThreadScoreboard;

class VEngine {
  public:
	VEngine() {}
	VEngine(const VEngine&) = delete;
	VEngine& operator=(const VEngine&) = delete;
	VEngine(VEngine&&) = default;
	VEngine& operator=(VEngine&&) = default;
	~VEngine();

	template <DerivesFrom<VModule> T, typename... Args>
	requires(ConstructibleWith<T, Args...>) T* createModule(Args... args);

	void activateModule(VModule* moduleToActivate);
	void activateModuleOnce(VModule* moduleToActivate);
	void deactivateModule(VModule* moduleToDeactivate);

	void destroyModule(VModule* moduleToDestroy);

	template <DerivesFrom<VModuleGroup> T, typename... Args>
	requires(ConstructibleWith<T, Args...>) T* createModuleGroup(Args... args);

	void addModuleToGroup(VModuleGroup* group, VModule* moduleToAdd);
	void removeModuleFromGroup(VModuleGroup* group, VModule* moduleToRemove);
	
	void destroyModuleGroup(VModuleGroup* groupToDestroy);

	// Both from and to must be activated modules
	void addModuleDependency(VModule* from, VModule* to);

	void run();

	void setExitFlag();

	const std::vector<VModule*> modules() const { return m_modules; }

  private:
	void recreateModuleThreads();

	void removeModuleDependencies(VModule* removeModule);

	std::vector<VModule*> m_modules;
	std::vector<VModule*> m_activatedModules;
	std::vector<VModule*> m_modulesToDeactivate;
	std::vector<VModule*> m_modulesToDestroy;
	std::vector<VModuleDependency> m_moduleDependencies;

	std::vector<VModuleThreadParameters> m_threadParameters;
	std::vector<std::jthread> m_moduleThreads;

	std::vector<std::vector<VModuleExecutionInfo>> m_threadExecutionInfos;
	bool m_scoreboardDirty;

	std::atomic<bool> m_shouldExit = false;

	std::vector<VModuleGroup*> m_moduleGroups;
	std::vector<VModuleGroup*> m_moduleGroupsToDestroy;
};

template <DerivesFrom<VModule> T, typename... Args>
requires(ConstructibleWith<T, Args...>) inline T* VEngine::createModule(Args... args) {
	m_modules.push_back(new T(args...));
	m_modules.back()->onCreate(*this);
	return m_modules.back();
}

template <DerivesFrom<VModuleGroup> T, typename... Args>
requires(ConstructibleWith<T, Args...>) inline T* VEngine::createModuleGroup(Args... args) {
	m_moduleGroups.push_back(new T(args...));
	m_moduleGroups.back()->onCreate(*this);
	return m_moduleGroups.back();
}
