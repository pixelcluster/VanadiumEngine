#include <VDebug.hpp>
#include <VEngine.hpp>
#include <thread>

VEngine::~VEngine() {
	for (auto& moduleToDeactivate : m_activatedModules) {
		moduleToDeactivate->onDeactivate(*this);
	}
	for (auto& moduleToDestroy : m_modules) {
		if (moduleToDestroy->m_moduleGroup)
			moduleToDestroy->m_moduleGroup->onModuleRemove(*this, moduleToDestroy);

		moduleToDestroy->onDestroy(*this);

		delete moduleToDestroy;
	}
	for (auto& group : m_moduleGroups) {
		group->onDestroy(*this);
		delete group;
	}
}

void VEngine::activateModule(VModule* moduleToActivate) {
	if (std::find(m_activatedModules.begin(), m_activatedModules.end(), moduleToActivate) != m_activatedModules.end()) {
		// TODO: log activating a module twice
	} else {
		m_activatedModules.push_back(moduleToActivate);
	}
}

void VEngine::activateModuleOnce(VModule* moduleToActivate) {
	if (std::find(m_activatedModules.begin(), m_activatedModules.end(), moduleToActivate) != m_activatedModules.end()) {
		// TODO: log activating a module twice
	} else {
		m_activatedModules.push_back(moduleToActivate);
	}
}

void VEngine::deactivateModule(VModule* moduleToDeactivate) {
	if (std::find(m_modulesToDeactivate.begin(), m_modulesToDeactivate.end(), moduleToDeactivate) !=
		m_modulesToDeactivate.end()) {
		// TODO: log activating a module twice
	} else {
		m_modulesToDeactivate.push_back(moduleToDeactivate);
	}
}

void VEngine::destroyModule(VModule* moduleToDestroy) {
	if (std::find(m_activatedModules.begin(), m_activatedModules.end(), moduleToDestroy) != m_activatedModules.end()) {
		m_modulesToDeactivate.push_back(moduleToDestroy);
	}

	if (std::find(m_modules.begin(), m_modules.end(), moduleToDestroy) == m_modules.end()) {
		// TODO: log destroying a module that wasn't created
	} else if (std::find(m_modulesToDestroy.begin(), m_modulesToDestroy.end(), moduleToDestroy) !=
			   m_modulesToDestroy.end()) {
		// TODO: log destroying a module twice
	} else {
		m_modulesToDestroy.push_back(moduleToDestroy);
	}
}

void VEngine::addModuleToGroup(VModuleGroup* group, VModule* moduleToAdd) {
	group->m_modules.push_back(moduleToAdd);
	group->onModuleAdd(*this, moduleToAdd);
}

void VEngine::removeModuleFromGroup(VModuleGroup* group, VModule* moduleToRemove) {
	auto groupMemberIterator = std::find(group->m_modules.begin(), group->m_modules.end(), moduleToRemove);
	if (groupMemberIterator == group->m_modules.end()) {
		//TODO: log removing a module that isn't a member of the group
	}
	else {
		group->onModuleRemove(*this, moduleToRemove);
		group->m_modules.erase(groupMemberIterator);
	}
}

void VEngine::destroyModuleGroup(VModuleGroup* groupToDestroy) {
	auto groupIterator = std::find(m_moduleGroups.begin(), m_moduleGroups.end(), groupToDestroy);
	auto groupToDestroyIterator = std::find(m_moduleGroupsToDestroy.begin(), m_moduleGroupsToDestroy.end(), groupToDestroy);
	if (groupIterator == m_moduleGroups.end() || groupToDestroyIterator != m_moduleGroupsToDestroy.end()) {
		//TODO: log destroy module that was not created
	} else {
		m_moduleGroupsToDestroy.push_back(groupToDestroy);
	}
}

void VEngine::addModuleDependency(VModule* from, VModule* to) {
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

		for (auto& moduleToDeactivate : m_modulesToDeactivate) {
			removeModuleDependencies(moduleToDeactivate);
			auto iterator = std::find(m_activatedModules.begin(), m_activatedModules.end(), moduleToDeactivate);
			if (iterator == m_activatedModules.end()) {
				// TODO: log deactivation of non-activated module
			} else {
				m_activatedModules.erase(iterator);
			}
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

			if (moduleToDestroy->m_moduleGroup)
				moduleToDestroy->m_moduleGroup->onModuleRemove(*this, moduleToDestroy);

			auto moduleIterator = std::find(m_modules.begin(), m_modules.end(), moduleToDestroy);
			if (moduleIterator != m_modules.end()) {
				m_modules.erase(moduleIterator);
			}

			delete moduleToDestroy;
		}
		for (auto& group : m_moduleGroupsToDestroy) {
			auto groupIterator = std::find(m_moduleGroups.begin(), m_moduleGroups.end(), group);
			for (auto& removedModule : group->m_modules) {
				group->onModuleRemove(*this, removedModule);
			}
			m_moduleGroups.erase(groupIterator);
		}
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
