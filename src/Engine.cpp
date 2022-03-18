#include <Engine.hpp>

namespace vanadium {
	Engine::Engine(uint32_t engineStartupFlags)
		: m_startupFlags(engineStartupFlags), m_windowInterface(false, 640, 480, "Test"),
		  m_graphicsSubsystem("Test", 1, m_windowInterface, true) {}
} // namespace vanadium