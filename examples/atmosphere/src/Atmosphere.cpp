#include <VEngine.hpp>
#include <modules/window/VGLFWWindowModule.hpp>
#include <modules/gpu/VGPUModule.hpp>

#include <framegraph_nodes/PlanetRenderNode.hpp>
#include <DataGeneratorModule.hpp>

int main() {
	VEngine engine;
	VWindowModule* windowModule = engine.createModule<VGLFWWindowModule>(1280, 720, "Vanadium Hello World");
	VGPUModule* gpuModule = engine.createModule<VGPUModule>("Vanadium Hello World", 0, windowModule);
	DataGeneratorModule* bufferModule = engine.createModule<DataGeneratorModule>(gpuModule);

	gpuModule->framegraphContext().appendNode<PlanetRenderNode>(bufferModule->vertexBufferHandle(), totalPointCount);

	engine.activateModule(windowModule);
	engine.activateModule(gpuModule);
	engine.activateModule(bufferModule);

	engine.addModuleDependency(windowModule, bufferModule);
	engine.addModuleDependency(windowModule, gpuModule);
	engine.addModuleDependency(bufferModule, gpuModule);
	engine.run();
}