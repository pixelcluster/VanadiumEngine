#include <Engine.hpp>

namespace vanadium {
	Engine::Engine(const EngineConfig& config)
		: m_startupFlags(config.startupFlags()), m_windowInterface(config.settingsOverrides(), config.appName().data()),
		  m_graphicsSubsystem(config.appName(), config.pipelineLibraryFileName(), config.appVersion(),
							  m_windowInterface,
							  !(config.startupFlags() & static_cast<uint32_t>(EngineStartupFlag::Disable3D))) {}

	bool Engine::tickFrame() {
		if (m_lastRenderSuccessful)
			m_windowInterface.pollEvents();
		else
			m_windowInterface.waitEvents();

		m_lastRenderSuccessful = m_graphicsSubsystem.tickFrame();

		return !m_windowInterface.shouldClose();
	}
} // namespace vanadium