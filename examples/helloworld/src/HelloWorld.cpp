#include <modules/gpu/VGPUModule.hpp>
#include <modules/window/VGLFWWindowModule.hpp>
#include <modules/VertexBufferUpdateModule.hpp>

#include <framegraph_nodes/SwapchainClearNode.hpp>
#include <framegraph_nodes/HelloTriangleNode.hpp>

int main() {
	VEngine engine;
	VWindowModule* windowModule = engine.createModule<VGLFWWindowModule>(640, 480, "Vanadium Hello World");
	VGPUModule* gpuModule = engine.createModule<VGPUModule>("Vanadium Hello World", 0, windowModule);
	VertexBufferUpdateModule* bufferModule = engine.createModule<VertexBufferUpdateModule>(gpuModule, windowModule);

	gpuModule->framegraphContext().appendNode<SwapchainClearNode>();
	gpuModule->framegraphContext().appendNode<HelloTriangleNode>(bufferModule->vertexBufferHandle());

	engine.activateModule(windowModule);
	engine.activateModule(gpuModule);
	engine.activateModule(bufferModule);

	engine.addModuleDependency(windowModule, bufferModule);
	engine.addModuleDependency(windowModule, gpuModule);
	engine.addModuleDependency(bufferModule, gpuModule);
	engine.run();
}