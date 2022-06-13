#include <Engine.hpp>

#include <DataGeneratorModule.hpp>
#include <framegraph_nodes/PlanetRenderNode.hpp>
#include <framegraph_nodes/ScatteringLUTNode.hpp>

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