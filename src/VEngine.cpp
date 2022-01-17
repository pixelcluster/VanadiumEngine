#include <VEngine.hpp>
#include <VDebug.hpp>

void VEngine::activateModule(VModule* moduleToActivate) {
	if (std::find(m_activatedModules.begin(), m_activatedModules.end(), moduleToActivate) != m_activatedModules.end()) {
		//TODO: log activating a module twice
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
	if (std::find(m_modulesToDeactivate.begin(), m_modulesToDeactivate.end(), moduleToDeactivate) != m_modulesToDeactivate.end()) {
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
		//TODO: log destroying a module that wasn't created
	}
	else if (std::find(m_modulesToDestroy.begin(), m_modulesToDestroy.end(), moduleToDestroy) != m_modulesToDestroy.end()) {
		// TODO: log destroying a module twice
	} else {
		m_activatedModules.push_back(moduleToDestroy);
	}
}

void VEngine::addModuleDependency(VModule* from, VModule* to) {
	if (std::find(m_activatedModules.begin(), m_activatedModules.end(), from) == m_activatedModules.end() ||
		std::find(m_activatedModules.begin(), m_activatedModules.end(), to) == m_activatedModules.end()) {
		//TODO: log adding a dependency from/to a deactivated model
	}
	else {
		m_moduleDependencies.push_back({ from, to });
		m_scoreboardDirty = true;
	}
}

void VEngine::run() {
	while (!m_modules.empty() && !m_shouldExit) {
		if (m_scoreboardDirty)
			recalculateScoreboard();

		for (auto& moduleToRun : m_activatedModules) {
			moduleToRun->onExecute(*this);
		}

		for (auto& moduleToDeactivate : m_modulesToDeactivate) {
			auto iterator = std::find(m_activatedModules.begin(), m_activatedModules.end(), moduleToDeactivate);
			if (iterator == m_activatedModules.end()) {
				//TODO: log deactivation of non-activated module
			} else {
				m_activatedModules.erase(iterator);
			}
		}
	}
}

void VEngine::recalculateScoreboard() {
}

void VEngine::removeModuleDependencies(VModule* removeModule) {}
