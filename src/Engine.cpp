#include <Engine.hpp>

#include <hb.h>

namespace vanadium {
	Engine::Engine(const EngineConfig& config)
		: m_startupFlags(config.startupFlags()), m_windowInterface(config.settingsOverrides(), config.appName().data()),
		  m_graphicsSubsystem(config.appName(), config.pipelineLibraryFileName(), config.appVersion(),
							  m_windowInterface),
		  m_uiSubsystem(&m_windowInterface, m_graphicsSubsystem.context(), config.fontLibraryFileName(),
						config.startupFlags() & static_cast<uint32_t>(EngineStartupFlag::UIOnly), Vector4(1.0f)) {
		m_userPointer = config.userPointer();
		for (auto& node : config.customFramegraphNodes()) {
			node->create(&m_graphicsSubsystem.framegraphContext());
			m_graphicsSubsystem.framegraphContext().appendExistingNode(node);
		}
		
		uint32_t width;
		uint32_t height;
		m_windowInterface.windowSize(width, height);
		m_uiSubsystem.setWindowSize(width, height);
	}

	Engine::~Engine() {
		// Since any subsystem might register child nodes that depend on subsystem structures, the framegraph must be
		// destroyed first
		m_graphicsSubsystem.destroyFramegraph();
	}

	void Engine::initFramegraph() {
		m_uiSubsystem.addRendererNode(m_graphicsSubsystem.framegraphContext());
		m_graphicsSubsystem.setupFramegraphResources();
	}

	bool Engine::tickFrame() {
		if (m_lastRenderSuccessful)
			m_windowInterface.pollEvents();
		else
			m_windowInterface.waitEvents();

		m_lastRenderSuccessful = m_graphicsSubsystem.tickFrame();

		return !m_windowInterface.shouldClose();
	}
} // namespace vanadium