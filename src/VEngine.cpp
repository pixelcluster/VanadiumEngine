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
