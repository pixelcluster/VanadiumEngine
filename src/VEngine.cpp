#include <VDebug.hpp>
#include <VEngine.hpp>
#include <thread>

VEngine::~VEngine() {
	for (auto& parameters : m_threadParameters) {
		parameters.exitFlag = true;
		// threads will be in dormant state, have to wake them up so they will check the exit flag
		parameters.runFlag = true;
	}
	for (auto& thread : m_moduleThreads) {
		thread.join();
	}

	for (auto& moduleToDeactivate : m_activatedModules) {
		moduleToDeactivate->onDeactivate(*this);
	}
	for (auto& moduleToDestroy : m_modules) {
		moduleToDestroy->onDestroy(*this);

		delete moduleToDestroy;
	}
}

void VEngine::activateModule(VModule* moduleToActivate) {
	auto lock = std::lock_guard<std::mutex>(m_moduleAccessMutex);
	if (std::find(m_activatedModules.begin(), m_activatedModules.end(), moduleToActivate) != m_activatedModules.end()) {
		// TODO: log activating a module twice
	} else {
		moduleToActivate->onActivate(*this);
		m_activatedModules.push_back(moduleToActivate);
	}
}

void VEngine::deactivateModule(VModule* moduleToDeactivate) {
	auto lock = std::lock_guard<std::mutex>(m_moduleAccessMutex);
	auto activatedModuleIterator = std::find(m_activatedModules.begin(), m_activatedModules.end(), moduleToDeactivate);
	if (activatedModuleIterator != m_activatedModules.end()) {
		removeModuleDependencies(moduleToDeactivate);
		moduleToDeactivate->onDeactivate(*this);
		m_activatedModules.erase(activatedModuleIterator);
		m_scoreboardDirty = true;
	}
}

void VEngine::destroyModule(VModule* moduleToDestroy) {
	deactivateModule(moduleToDestroy);
	auto lock = std::lock_guard<std::mutex>(m_moduleAccessMutex);

	if (std::find(m_modules.begin(), m_modules.end(), moduleToDestroy) == m_modules.end()) {
		// TODO: log destroying a module that wasn't created
	} else if (std::find(m_modulesToDestroy.begin(), m_modulesToDestroy.end(), moduleToDestroy) !=
			   m_modulesToDestroy.end()) {
		// TODO: log destroying a module twice
	} else {
		m_modulesToDestroy.push_back(moduleToDestroy);
	}
}

void VEngine::addModuleDependency(VModule* from, VModule* to) {
	auto lock = std::lock_guard<std::mutex>(m_moduleAccessMutex);
	if (std::find(m_activatedModules.begin(), m_activatedModules.end(), from) == m_activatedModules.end() ||
		std::find(m_activatedModules.begin(), m_activatedModules.end(), to) == m_activatedModules.end()) {
		// TODO: log adding a dependency from/to a deactivated model
	} else {
		m_moduleDependencies.push_back({ from, to });
		m_scoreboardDirty = true;
	}
}

void VEngine::run() {
	while (!m_modules.empty() && !m_shouldExit) {
		if (m_scoreboardDirty)
			recreateModuleThreads();

		for (auto& parameters : m_threadParameters) {
			parameters.runFlag = true;
		}

		for (auto& info : m_threadExecutionInfos[0]) {
			executeModule(info, *this);
		}

		for (auto& parameters : m_threadParameters) {
			parameters.runFlag.value.wait(true);
		}

		for (auto& moduleToDestroy : m_modulesToDestroy) {
			//remove input variables referencing this module
			for (auto& createdModule : m_modules) {
				for (size_t i = 0; i < createdModule->inputVariables().size(); ++i) {
					if (createdModule->inputVariables()[i].outputModule == moduleToDestroy) {
						createdModule->removeInputVariable(i);
						--i;
					}
				}
			}

			moduleToDestroy->onDestroy(*this);

			auto moduleIterator = std::find(m_modules.begin(), m_modules.end(), moduleToDestroy);
			if (moduleIterator != m_modules.end()) {
				m_modules.erase(moduleIterator);
			}

			delete moduleToDestroy;
		}
		m_modulesToDestroy.clear();
	}
}

void VEngine::setExitFlag() { m_shouldExit = true; }

void VEngine::recreateModuleThreads() {
	vEngineScoreboardingFunction(m_activatedModules, m_moduleDependencies, m_threadExecutionInfos);

	for (auto& parameters : m_threadParameters) {
		parameters.exitFlag = true;
		// threads will be in dormant state, have to wake them up so they will check the exit flag
		parameters.runFlag = true;
	}

	m_moduleThreads.clear();
	m_threadParameters.clear();

	m_moduleThreads.reserve(m_threadExecutionInfos.size() - 1);
	m_threadParameters.reserve(m_threadExecutionInfos.size() - 1);

	for (size_t i = 1; i < m_threadExecutionInfos.size(); ++i) {
		m_threadParameters.push_back({ .executionInfo = &m_threadExecutionInfos[i], .engine = this, .runFlag = false, .exitFlag = false });
		m_moduleThreads.push_back(std::jthread(moduleThreadFunction, &m_threadParameters.back()));
	}
}

void VEngine::removeModuleDependencies(VModule* removeModule) {
	for (size_t i = 0; i < m_moduleDependencies.size(); ++i) {
		if (m_moduleDependencies[i].from == removeModule || m_moduleDependencies[i].to == removeModule) {
			m_moduleDependencies.erase(m_moduleDependencies.begin() + i);
			--i;
		}
	}
}