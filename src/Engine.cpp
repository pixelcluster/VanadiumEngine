/* VanadiumEngine, a Vulkan rendering toolkit
 * Copyright (C) 2022 Friedrich Vock
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <Engine.hpp>
#include <graphics/GraphicsSubsystem.hpp>
#include <ui/UISubsystem.hpp>
#include <windowing/WindowInterface.hpp>

namespace vanadium {
	Engine::Engine(const EngineConfig& config)
		: m_startupFlags(config.startupFlags()),
		  m_windowInterface(new windowing::WindowInterface(config.settingsOverrides(), config.appName().data())),
		  m_graphicsSubsystem(new graphics::GraphicsSubsystem(config.appName(), config.pipelineLibraryFileName(),
															  config.appVersion(), *m_windowInterface)),
		  m_uiSubsystem(new ui::UISubsystem(m_windowInterface, m_graphicsSubsystem->context(),
											config.fontLibraryFileName(), config.uiBackgroundColor())) {
		m_userPointer = config.userPointer();

		uint32_t width;
		uint32_t height;
		m_windowInterface->windowSize(width, height);
		m_uiSubsystem->setWindowSize(width, height);
		m_uiSubsystem->addRendererNode(m_graphicsSubsystem->framegraphContext());
	}

	Engine::~Engine() {
		// Since any subsystem might register child nodes that depend on subsystem structures, the framegraph must be
		// destroyed first
		m_graphicsSubsystem->destroyFramegraph();
	}

	bool Engine::tickFrame() {
		if (m_lastRenderSuccessful)
			m_windowInterface->pollEvents();
		else
			m_windowInterface->waitEvents();

		m_timerManager.update(m_windowInterface->deltaTime());

		m_lastRenderSuccessful = m_graphicsSubsystem->tickFrame();

		return !m_windowInterface->shouldClose();
	}

	float Engine::deltaTime() const { return m_windowInterface->deltaTime(); }
	float Engine::elapsedTime() const { return m_windowInterface->elapsedTime(); }
} // namespace vanadium
