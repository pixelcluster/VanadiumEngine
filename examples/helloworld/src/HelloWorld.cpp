#include <modules/gpu/VGPUModule.hpp>
#include <modules/window/VGLFWWindowModule.hpp>

#include <framegraph_nodes/SwapchainClearNode.hpp>
#include <framegraph_nodes/HelloTriangleNode.hpp>

int main() {
	VEngine engine;
	VWindowModule* windowModule = engine.createModule<VGLFWWindowModule>(640, 480, "Vanadium Hello World");
	VGPUModule* gpuModule = engine.createModule<VGPUModule>("Vanadium Hello World", 0, windowModule);

	gpuModule->framegraphContext().appendNode<SwapchainClearNode>();
	gpuModule->framegraphContext().appendNode<HelloTriangleNode>();

	engine.activateModule(windowModule);
	engine.activateModule(gpuModule);

	engine.addModuleDependency(windowModule, gpuModule);

	engine.run();
}