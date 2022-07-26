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

#include <DataGeneratorModule.hpp>
#include <framegraph_nodes/PlanetRenderNode.hpp>
#include <framegraph_nodes/ScatteringLUTNode.hpp>
#include <graphics/GraphicsSubsystem.hpp>

void configureEngine(vanadium::EngineConfig& config) { config.setAppName("Vanadium Atmosphere"); }

void init(vanadium::Engine& engine) {
	ScatteringLUTNode* lutNode = engine.graphicsSubsystem().framegraphContext().insertNode<ScatteringLUTNode>(nullptr);
	PlanetRenderNode* node = engine.graphicsSubsystem().framegraphContext().insertNode<PlanetRenderNode>(lutNode);

	DataGenerator* generator = new DataGenerator(engine.graphicsSubsystem(), engine.windowInterface(), node);
	engine.setUserPointer(generator);
}

bool onFrame(vanadium::Engine& engine) {
	DataGenerator* generator = std::launder(reinterpret_cast<DataGenerator*>(engine.userPointer()));
	generator->update(engine.graphicsSubsystem(), engine.windowInterface());
	return true;
}

void destroy(vanadium::Engine& engine) {
	DataGenerator* generator = std::launder(reinterpret_cast<DataGenerator*>(engine.userPointer()));
	generator->destroy(engine.graphicsSubsystem());
}
