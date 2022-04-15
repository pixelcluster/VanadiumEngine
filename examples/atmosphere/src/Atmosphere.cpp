#include <Engine.hpp>

#include <DataGeneratorModule.hpp>
#include <framegraph_nodes/PlanetRenderNode.hpp>
#include <framegraph_nodes/ScatteringLUTNode.hpp>

void configureEngine(vanadium::EngineConfig& config) {
	config.setAppName("Vanadium Atmosphere");
	config.addCustomFramegraphNode<ScatteringLUTNode>();
	
	config.setUserPointer(config.addCustomFramegraphNode<PlanetRenderNode>());
}


void preFramegraphInit(vanadium::Engine& engine) {
	PlanetRenderNode* node = std::launder(reinterpret_cast<PlanetRenderNode*>(engine.userPointer()));
	DataGenerator* generator = new DataGenerator(engine.graphicsSubsystem(), engine.windowInterface(), node);
	engine.setUserPointer(generator);
}

void init(vanadium::Engine& engine) {}

bool onFrame(vanadium::Engine& engine) {
	DataGenerator* generator = std::launder(reinterpret_cast<DataGenerator*>(engine.userPointer()));
	generator->update(engine.graphicsSubsystem(), engine.windowInterface());
	return true;
}

void destroy(vanadium::Engine& engine) {
	DataGenerator* generator = std::launder(reinterpret_cast<DataGenerator*>(engine.userPointer()));
	generator->destroy(engine.graphicsSubsystem());
}