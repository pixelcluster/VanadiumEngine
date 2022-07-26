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
#include <VertexBufferUpdater.hpp>
#include <framegraph_nodes/HelloTriangleNode.hpp>
#include <framegraph_nodes/SwapchainClearNode.hpp>

void configureEngine(vanadium::EngineConfig& config) { config.setAppName("Vanadium Minimal UI"); }

void preFramegraphInit(vanadium::Engine& engine) {}

void init(vanadium::Engine& engine) {
	VertexBufferUpdater* updater = new VertexBufferUpdater;
	updater->init(engine.graphicsSubsystem());

	SwapchainClearNode* clearNode =
		engine.graphicsSubsystem().framegraphContext().insertNode<SwapchainClearNode>(nullptr);
	engine.graphicsSubsystem().framegraphContext().insertNode<HelloTriangleNode>(clearNode, updater);

	engine.setUserPointer(updater);
}

bool onFrame(vanadium::Engine& engine) {
	VertexBufferUpdater* updater = std::launder(reinterpret_cast<VertexBufferUpdater*>(engine.userPointer()));
	updater->update(engine.graphicsSubsystem(), engine.elapsedTime());
	return true;
}

void destroy(vanadium::Engine& engine) {}
