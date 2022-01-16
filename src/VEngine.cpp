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
