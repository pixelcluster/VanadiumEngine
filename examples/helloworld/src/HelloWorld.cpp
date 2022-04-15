#include <Engine.hpp>
#include <framegraph_nodes/SwapchainClearNode.hpp>
#include <framegraph_nodes/HelloTriangleNode.hpp>
#include <VertexBufferUpdater.hpp>

void configureEngine(vanadium::EngineConfig& config) {
	config.setAppName("Vanadium Minimal UI");
	VertexBufferUpdater* updater = new VertexBufferUpdater;
	config.setUserPointer(updater);
	config.addCustomFramegraphNode<SwapchainClearNode>();
	config.addCustomFramegraphNode<HelloTriangleNode>(updater);
}


void preFramegraphInit(vanadium::Engine& engine) {
	VertexBufferUpdater* updater = std::launder(reinterpret_cast<VertexBufferUpdater*>(engine.userPointer()));
	updater->init(engine.graphicsSubsystem());
}

void init(vanadium::Engine& engine) {}

bool onFrame(vanadium::Engine& engine) {
	VertexBufferUpdater* updater = std::launder(reinterpret_cast<VertexBufferUpdater*>(engine.userPointer()));
	updater->update(engine.graphicsSubsystem(), engine.elapsedTime());
	return true;
}

void destroy(vanadium::Engine& engine) {

}