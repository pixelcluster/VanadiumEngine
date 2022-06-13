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